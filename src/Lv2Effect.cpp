// Copyright (c) 2022 Robin Davies
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files (the "Software"), to deal in
// the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
// the Software, and to permit persons to whom the Software is furnished to do so,
// subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
// FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
// IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include "pch.h"
#include "restrict.hpp"
#include "Lv2Effect.hpp"
#include "PiPedalException.hpp"
#include <lv2/lv2plug.in/ns/ext/worker/worker.h>
#include <lilv/lilv.h>
#include "lv2/atom/atom.h"
#include "lv2/atom/util.h"
// #include "lv2.h"
#include "lv2/log/log.h"
#include "lv2/log/logger.h"
#include "lv2/midi/midi.h"
#include "lv2/urid/urid.h"
#include "lv2/log/logger.h"
#include "lv2/uri-map/uri-map.h"
#include "lv2/atom/forge.h"
#include "lv2/state/state.h"
#include "lv2/worker/worker.h"
#include "lv2/patch/patch.h"
#include "lv2/parameters/parameters.h"
#include "lv2/units/units.h"
#include "lv2/atom/util.h"
#include "AudioHost.hpp"
#include <exception>
#include "RingBufferReader.hpp"
#include "Worker.hpp"

using namespace pipedal;
namespace fs = std::filesystem;

const float BYPASS_TIME_S = 0.1f;

static fs::path makeAbsolutePath(const std::filesystem::path &path, const std::filesystem::path &parentPath)
{
    if (path.is_absolute())
    {
        return path;
    }
    return parentPath / path;
}

inline void Lv2Effect::CheckStagingBufferSentries()
{
#ifndef NDEBUG
    for (size_t i = 0; i < inputStagingBuffers.size(); ++i)
    {
        if (inputStagingBuffers[i].at(stagingBufferSize) != 99.9f)
        {
            throw std::logic_error("Staging buffer sentry overwritten.");
        }

    }
    for (size_t i = 0; i < outputStagingBuffers.size(); ++i)
    {
        if (outputStagingBuffers[i].at(stagingBufferSize) != 99.9f)
        {
            throw std::logic_error("Staging buffer sentry overwritten.");
        }

    }
#endif
}

Lv2Effect::Lv2Effect(
    IHost *pHost_,
    const std::shared_ptr<Lv2PluginInfo> &info_,
    PedalboardItem &pedalboardItem)
    : pHost(pHost_), pInstance(nullptr), info(info_), urids(pHost), instanceId(pedalboardItem.instanceId())
{
    auto pWorld = pHost_->getWorld();

    size_t stagedBufferSize = GetStagedBufferSize();

    logFeature.Prepare(&pHost_->GetMapFeature(), info_->name() + ": ", this);

    optionsFeature.Prepare(pHost->GetMapFeature(), 44100, stagedBufferSize, pHost->GetAtomBufferSize());

    this->bypassStartingSamples = (uint32_t)(pHost->GetSampleRate() * BYPASS_TIME_S);

    this->bypass = pedalboardItem.isEnabled();

    this->workerThread = std::make_unique<HostWorkerThread>();
    if (info_->WantsWorkerThread())
    {
        workerThread->StartThread();
    }
    // stash a list of known file properties that we want to keep synced.
    if (info->piPedalUI())
    {
        for (auto fileProperty : info->piPedalUI()->fileProperties())
        {
            LV2_URID filePropertyUrid = pHost->GetLv2Urid(fileProperty->patchProperty().c_str());
            this->pathProperties.push_back(filePropertyUrid);

            this->pathPropertyWriters.push_back(PatchPropertyWriter(instanceId, filePropertyUrid));
        }
    }
    for (auto &pathProperty : pedalboardItem.pathProperties_)
    {
        SetPathPatchProperty(pathProperty.first, pathProperty.second);
    }

    // initialize the atom forge used on the realtime thread.
    LV2_URID_Map *map = this->pHost->GetLv2UridMap();
    lv2_atom_forge_init(&inputForgeRt, map);
    lv2_atom_forge_init(&outputForgeRt, map);
    lv2_atom_forge_init(&stagedInputForgeRt, map);

    const LilvPlugins *plugins = lilv_world_get_all_plugins(pWorld);

    // FIXME: could we not stash the pPlugin in the plugin info?
    auto uriNode = lilv_new_uri(pWorld, pedalboardItem.uri().c_str());
    const LilvPlugin *pPlugin = lilv_plugins_get_by_uri(plugins, uriNode);

    for (auto &port : info->ports())
    {
        if (port->is_bypass())
        {
            this->bypassControlIndex = port->index();
            break;
        }
    }
    lilv_node_free(uriNode);
    {
        AutoLilvNode bundleUri = lilv_plugin_get_bundle_uri(pPlugin);
        char *bundleUriString = lilv_file_uri_parse(bundleUri.AsUri().c_str(), nullptr);

        std::string storagePath = pHost_->GetPluginStoragePath();

        fileBrowserFilesFeature.Initialize(
            pHost_->GetMapFeature().GetMap(),
            logFeature.GetLog(),
            bundleUriString,
            storagePath);

        mapPathFeature.Prepare(&(pHost_->GetMapFeature()));
        mapPathFeature.SetPluginStoragePath(pHost_->GetPluginStoragePath());
        if (info->piPedalUI())
        {
            const auto &fileProperties = info_->piPedalUI()->fileProperties();
            for (const auto &fileProperty : fileProperties)
            {
                fs::path targetPath = fileProperty->directory() / std::filesystem::path(bundleUriString).parent_path().filename();
                mapPathFeature.AddResourceFileMapping({bundleUriString,
                                                       storagePath / targetPath,
                                                       fileProperty->fileTypes()});
            }
        }

        lilv_free(bundleUriString);
    }

    LV2_Feature *const *features = pHost_->GetLv2Features();

    for (auto p = features; *p != nullptr; ++p)
    {
        if (strcmp((*p)->URI, LV2_LOG__log) != 0)
        { // ommit the host's LOG feature.
            this->features.push_back(*p);
        }
    }
    this->features.push_back(logFeature.GetFeature());
    this->features.push_back(optionsFeature.GetFeature());

    this->features.push_back(mapPathFeature.GetMapPathFeature());
    this->features.push_back(mapPathFeature.GetMakePathFeature());
    this->features.push_back(mapPathFeature.GetFreePathFeature());

    this->features.push_back(this->fileBrowserFilesFeature.GetFeature());

    this->work_schedule_feature = nullptr;
    if (true) // info_->hasExtension(LV2_WORKER__interface))
    {
        LV2_Worker_Schedule *schedule = (LV2_Worker_Schedule *)malloc(sizeof(LV2_Worker_Schedule));
        schedule->handle = this;
        schedule->schedule_work = worker_schedule_fn;

        work_schedule_feature = (LV2_Feature *)malloc(sizeof(LV2_Feature));
        work_schedule_feature->URI = LV2_WORKER__schedule;
        work_schedule_feature->data = schedule;

        this->features.push_back(work_schedule_feature);
    }
    this->features.push_back(nullptr);

    const LV2_Feature **myFeatures = &this->features.at(0);

    LilvInstance *pInstance = nullptr;
    try
    {
        pInstance = lilv_plugin_instantiate(pPlugin, pHost->GetSampleRate(), myFeatures);
    }
    catch (const std::exception &e)
    {
        this->pInstance = nullptr;
        throw PiPedalException(SS("Plugin threw an exception: " << e.what() << " '" << info_->name() << "'"));
    }
    this->pInstance = pInstance;
    if (this->pInstance == nullptr)
    {
        throw PiPedalException(SS("Failed to create plugin \'" << info_->name() << "\'."));
    }

    const LV2_Worker_Interface *worker_interface =
        (const LV2_Worker_Interface *)lilv_instance_get_extension_data(pInstance,
                                                                       LV2_WORKER__interface);
    this->worker = std::make_unique<Worker>(workerThread, pInstance, worker_interface);

    const LV2_State_Interface *state_interface =
        (const LV2_State_Interface *)lilv_instance_get_extension_data(pInstance,
                                                                      LV2_STATE__interface);

    if (state_interface)
    {
        this->stateInterface = std::make_unique<StateInterface>(pHost, &(this->features.at(0)), pInstance, state_interface);
    }

    this->instanceId = pedalboardItem.instanceId();

    PreparePortIndices();

    // Copy default pedalboard settings.
    size_t maxPortIndex = 0;
    std::vector<std::shared_ptr<Lv2PortInfo>> &t = info->ports();
    for (std::shared_ptr<Lv2PortInfo> &port : info->ports())
    {
        if (port->is_control_port())
        {
            auto index = port->index();
            if (index + 1 > maxPortIndex)
            {
                maxPortIndex = index + 1;
            }
        }
    }
    if (maxPortIndex > info->ports().size())
    {
        throw std::runtime_error("Ports are not consecutive");
    }

    this->controlValues.resize(info->ports().size());
    for (auto i = pedalboardItem.controlValues().begin(); i != pedalboardItem.controlValues().end(); ++i)
    {
        auto &v = (*i);
        int index = GetControlIndex(v.key());
        if (index != -1)
        {
            this->controlValues.at(index) = v.value();
        }
    }

    ConnectControlPorts();

    if (!pedalboardItem.lilvPresetUri().empty())
    {
        AutoLilvNode presetNode = lilv_new_uri(pWorld, pedalboardItem.lilvPresetUri().c_str());
        lilv_world_load_resource(pWorld, presetNode);
        LilvState *pState = lilv_state_new_from_world(pWorld, pHost->GetMapFeature().GetMap(), presetNode);
        if (pState)
        {
            if (this->stateInterface)
            {
                this->stateInterface->RestoreState(pState);
            }
            lilv_state_free(pState);
        }
        // now that we've loaded the preset, clear the uri, and save new state
        // Why? because lilv doesn't provide facilities for reading state.
        pedalboardItem.lv2State(this->stateInterface->Save());
        RestoreState(pedalboardItem); // reload it with OUR map/unmap handling.
        pedalboardItem.lilvPresetUri("");
    }
    else
    {
        if (!RestoreState(pedalboardItem))
        {
            if (info->hasDefaultState())
            {
                // restore the default state.
                try
                {
                    // REsTORE from LV2_STATE__state default state.
                    AutoLilvNode pluginNode = lilv_new_uri(pWorld, info->uri().c_str());
                    LilvState *pState = lilv_state_new_from_world(pWorld, pHost->GetMapFeature().GetMap(), pluginNode);
                    if (pState)
                    {
                        if (this->stateInterface)
                        {
                            this->stateInterface->RestoreState(pState);
                        }
                        lilv_state_free(pState);
                    }
                    pedalboardItem.lv2State(this->stateInterface->Save());
                    RestoreState(pedalboardItem); // do it with OUR map/unmap file handling.
                }
                catch (const std::exception &e)
                {
                    Lv2Log::warning(SS("Failed to restore default state for " << info->name() << ": " << e.what()));
                }
            }
        }
    }
}
bool Lv2Effect::RestoreState(PedalboardItem &pedalboardItem)
{
    // Restore state if present.
    if (this->stateInterface)
    {
        try
        {
            if (pedalboardItem.lv2State().isValid_)
            {
                this->stateInterface->Restore(pedalboardItem.lv2State());
                return true;
            }
            return false;
        }
        catch (const std::exception &e)
        {
            std::string name = pedalboardItem.pluginName();
            Lv2Log::warning(SS(name << ": " << e.what()));
            return false;
        }
    }
    return true;
}

void Lv2Effect::ConnectControlPorts()
{
    // shared_ptr is not thread-safe.
    // Get naked pointers to use on the realtime thread.
    int controlArrayLength = 0;
    for (int i = 0; i < info->ports().size(); ++i)
    {
        if (info->ports().at(i)->index() >= controlArrayLength)
        {
            controlArrayLength = info->ports().at(i)->index() + 1;
        }
    }
    this->realtimePortInfo.resize(controlArrayLength);
    for (int i = 0; i < info->ports().size(); ++i)
    {
        const auto &port = info->ports().at(i);
        if (port->is_control_port())
        {
            int index = port->index();
            realtimePortInfo.at(index) = port.get();
            lilv_instance_connect_port(pInstance, i, &this->controlValues.at(index));
        }
    }
}
void Lv2Effect::PreparePortIndices()
{
    size_t nPorts = info->ports().size();
    isInputControlPort.resize(nPorts);
    this->defaultInputControlValues.resize(nPorts);
    this->isInputTriggerControlPort.resize(nPorts);

    for (int i = 0; i < info->ports().size(); ++i)
    {
        const auto &port = info->ports().at(i);

        int portIndex = port->index();
        if (port->is_audio_port())
        {
            if (port->is_input())
            {
                this->inputAudioPortIndices.push_back(portIndex);
            }
            else
            {
                this->outputAudioPortIndices.push_back(portIndex);
            }
        }
        else if (port->is_atom_port())
        {
            if (port->is_input())
            {
                if (port->supports_midi())
                {
                    this->inputMidiPortIndices.push_back(portIndex);
                }
                this->inputAtomPortIndices.push_back(portIndex);
            }
            else
            {
                this->outputAtomPortIndices.push_back(portIndex);
                if (port->supports_midi())
                {
                    this->outputMidiPortIndices.push_back(portIndex);
                }
            }
        }
        else if (port->is_control_port())
        {
            controlIndex[port->symbol()] = portIndex;
            if (port->is_input())
            {
                this->isInputControlPort.at(portIndex) = true;
                this->defaultInputControlValues.at(portIndex) = port->default_value();
                if (port->trigger_property())
                {
                    this->isInputTriggerControlPort.at(portIndex) = true;
                }
            }
        }
    }
    size_t maxInputControlPort = isInputControlPort.size();
    while (maxInputControlPort != 0 && !isInputControlPort.at(maxInputControlPort - 1))
    {
        --maxInputControlPort;
    }
    this->maxInputControlPort = maxInputControlPort;

    inputAudioBuffers.resize(inputAudioPortIndices.size());
    outputAudioBuffers.resize(outputAudioPortIndices.size());
    inputAtomBuffers.resize(inputAtomPortIndices.size());
    outputAtomBuffers.resize(outputAtomPortIndices.size());

    if (RequiresBufferStaging())
    {
        EnableBufferStaging(
            GetStagedBufferSize(),
            this->GetNumberOfInputAudioBuffers(),
            this->GetNumberOfOutputAudioBuffers());
    }
}

void Lv2Effect::PrepareNoInputEffect(int numberOfInputs, size_t maxBufferSize)
{
    if (outputAudioPortIndices.size() == 0)
    {
        // pass the input through unmodified.
        inputAudioBuffers.resize(std::max((size_t)numberOfInputs, inputAudioPortIndices.size()));
        outputAudioBuffers.resize(numberOfOutputs);
    }
    else if (inputAudioPortIndices.size() == 0)
    {
        inputAudioBuffers.resize(numberOfInputs);
        outputAudioBuffers.resize(std::max((size_t)numberOfInputs, outputAudioPortIndices.size()));

        // allocate a working buffer which we will mix with passed-through data.
        outputMixBuffers.resize(outputAudioPortIndices.size());
        for (size_t i = 0; i < outputMixBuffers.size(); ++i)
        {
            outputMixBuffers.at(i).resize(maxBufferSize);
        }
        // connect the plugin to the mix buffer instead of output buffer.
        for (size_t i = 0; i < outputAudioPortIndices.size(); ++i)
        {
            int pluginIndex = this->outputAudioPortIndices.at(i);
            lilv_instance_connect_port(this->pInstance, pluginIndex, outputMixBuffers.at(i).data());
        }
    }
}

void Lv2Effect::SetAudioInputBuffer(int index, float *buffer)
{
    this->inputAudioBuffers.at(index) = buffer;

    if (borrowedEffect)
    {
        // Already running on the realtime thread,
        // so don't update the audio ports until the effect gets placed on the realtime thread.
        return;
    }

    if (inputAudioPortIndices.size() == inputAudioBuffers.size())
    {
        if (stagingBufferSize != 0)
        {
            int pluginIndex = this->inputAudioPortIndices.at(index);
            if (index >= inputStagingBufferPointers.size())
            {
                throw std::runtime_error("Invalid input staging buffer index.");
            }
            lilv_instance_connect_port(this->pInstance, pluginIndex, inputStagingBufferPointers.at(index));
        }
        else
        {
            int pluginIndex = this->inputAudioPortIndices.at(index);
            lilv_instance_connect_port(this->pInstance, pluginIndex, buffer);
        }
    }
    else
    {
        throw std::runtime_error("Invalid input buffer index.");
        // // cases: 1->0, 1->1, 2->0, 2->1
        // if (index < inputAudioPortIndices.size())
        // {
        //     int pluginIndex = this->inputAudioPortIndices.at(index);
        //     lilv_instance_connect_port(this->pInstance, pluginIndex, buffer);
        // }
    }
}

void Lv2Effect::SetAudioInputBuffer(float *left)
{
    if (GetNumberOfInputAudioBuffers() > 1)
    {
        SetAudioInputBuffer(0, left);
        SetAudioInputBuffer(1, left);
    }
    else if (GetNumberOfInputAudioBuffers() != 0)
    {
        SetAudioInputBuffer(0, left);
    }
}

void Lv2Effect::SetAudioInputBuffers(float *left, float *right)
{
    if (GetNumberOfInputAudioBuffers() == 1)
    {
        SetAudioInputBuffer(0, left);
    }
    else if (GetNumberOfInputAudioBuffers() > 1)
    {
        SetAudioInputBuffer(0, left);
        SetAudioInputBuffer(1, right);
    }
}

void Lv2Effect::SetAudioOutputBuffer(int index, float *buffer)
{
    this->outputAudioBuffers.at(index) = buffer;

    if (borrowedEffect)
    {
        // Effect is already running on the realtime thread,
        // so don't update the audio ports until the updated pedalboard gets placed on the realtime thread.
        return;
    }

    if (this->inputAudioPortIndices.size() != 0) // i.e. we're not mixing a zero-input control
    {
        if (this->stagingBufferSize != 0)
        {
            if ((size_t)index < this->outputStagingBufferPointers.size())
            {
                int pluginIndex = this->outputAudioPortIndices.at(index);
                lilv_instance_connect_port(pInstance, pluginIndex, outputStagingBufferPointers.at(index));
            }
            else
            {
                throw std::runtime_error("outputStagingBufferPointers index out of range.");
            }
        }
        else
        {
            if ((size_t)index < this->outputAudioPortIndices.size())
            {
                int pluginIndex = this->outputAudioPortIndices.at(index);
                lilv_instance_connect_port(pInstance, pluginIndex, buffer);
            }
        }
    }
}

int Lv2Effect::GetControlIndex(const std::string &key) const
{
    auto i = controlIndex.find(key);
    if (i == controlIndex.end())
    {
        return -1;
    }
    return i->second;
}

Lv2Effect::~Lv2Effect()
{
    if (deleted)
    {
        try {
            throw std::runtime_error("Deleted twice!");
        } catch (const std::exception&e)
        {
            std::terminate();
        }
    }
    deleted = true;
    if (worker)
    {
        worker->Close();
        worker = nullptr; // delete the worker first!
    }
    if (activated)
    {
        Deactivate();
        activated = false;
    }
    if (pInstance)
    {
        lilv_instance_free(pInstance);
        pInstance = nullptr;
    }
    if (work_schedule_feature)
    {
        free(work_schedule_feature->data);
        work_schedule_feature->data = nullptr;
        free(work_schedule_feature);
        work_schedule_feature = nullptr;
    }
}

void Lv2Effect::Activate()
{
    if (this->activated)
    {
        return;
    }
    this->activated = true;
    this->AssignUnconnectedPorts();
    lilv_instance_activate(pInstance);
    if (this->bypassControlIndex == -1)
    {
        this->BypassDezipperSet(this->bypass ? 1.0f : 0.0f);
    }
    else
    {
        this->BypassDezipperSet(1.0f);
        this->controlValues.at(this->bypassControlIndex) = this->bypass ? 1.0f : 0.0f;
    }
}

void Lv2Effect::UpdateAudioPorts()
{
    // called on realtime thread to switch borrowed effects to the new buffer pointers.
    if (borrowedEffect)
    {
        if (stagingBufferSize != 0)
        {
            for (size_t i = 0; i < this->inputAudioPortIndices.size(); ++i)
            {
                int portIndex = this->inputAudioPortIndices.at(i);
                if (inputStagingBufferPointers.at(i) != nullptr)
                {
                    lilv_instance_connect_port(pInstance, portIndex, inputStagingBufferPointers.at(i));
                }
            }
            for (size_t i = 0; i < this->outputAudioPortIndices.size(); ++i)
            {
                int portIndex = this->outputAudioPortIndices.at(i);
                if (outputStagingBufferPointers.at(i) != nullptr)
                {
                    lilv_instance_connect_port(pInstance, portIndex, outputStagingBufferPointers.at(i));
                }
            }
            for (size_t i = 0; i < this->inputAtomPortIndices.size(); ++i)
            {
                if (i == 0)
                {
                    if (stagedInputAtomBufferPointer == nullptr)
                    {
                        throw std::runtime_error("Invalid astagedInputAtomBufferPointer");
                    }
                    lilv_instance_connect_port(pInstance, inputAtomPortIndices[i],stagedInputAtomBufferPointer);
                } else {
                    auto atomInputBuffer = this->GetAtomInputBuffer(i);
                    lilv_instance_connect_port(pInstance, inputAtomPortIndices[i],atomInputBuffer);
                }
            }
            for (size_t i = 0; i < this->outputAtomPortIndices.size(); ++i)
            {
                if (i == 0)
                {
                    if (stagedOutputAtomBufferPointer == nullptr)
                    {
                        throw std::runtime_error("Invalid astagedOutputAtomBufferPointer");
                    }
                    lilv_instance_connect_port(pInstance, outputAtomPortIndices[i],stagedOutputAtomBufferPointer);
                } else {
                    auto atomOutputBuffer = this->GetAtomOutputBuffer(i);
                    lilv_instance_connect_port(pInstance, outputAtomPortIndices[i],atomOutputBuffer);
                }
            }
            
        } else {
            for (size_t i = 0; i < this->inputAudioPortIndices.size(); ++i)
            {
                int portIndex = this->inputAudioPortIndices.at(i);
                if (GetAudioInputBuffer(i) != nullptr)
                {
                    lilv_instance_connect_port(pInstance, portIndex, GetAudioInputBuffer(i));
                }
            }
            for (size_t i = 0; i < this->outputAudioPortIndices.size(); ++i)
            {
                int portIndex = this->outputAudioPortIndices.at(i);
                if (GetAudioOutputBuffer(i) != nullptr)
                {
                    lilv_instance_connect_port(pInstance, portIndex, GetAudioOutputBuffer(i));
                }
            }
            for (size_t i = 0; i < this->inputAtomPortIndices.size(); ++i)
            {
                auto atomInputBuffer = this->GetAtomInputBuffer(i);
                lilv_instance_connect_port(pInstance, inputAtomPortIndices[i],atomInputBuffer);
            }
            for (size_t i = 0; i < this->outputAtomPortIndices.size(); ++i)
            {
                auto atomOutputBuffer = this->GetAtomOutputBuffer(i);
                lilv_instance_connect_port(pInstance, outputAtomPortIndices[i],atomOutputBuffer);
            }
            // for (size_t i = 0; i < this->inputMidiPortIndices.size(); ++i)
            // {
            //     auto midiInputBuffer = this->GetMidiInputBuffer(i);
            //     lilv_instance_connect_port(pInstance, inputMidiPortIndices[i],midiInputBuffer);
            // }
            // for (size_t i = 0; i < this->outputMidiPortIndices.size(); ++i)
            // {
            //     auto midiOutputBuffer = this->GetMidiOutputBuffer(i);
            //     lilv_instance_connect_port(pInstance, outputMidiPortIndices[i],midiOutputBuffer);
            // }

        }
    }
}

void Lv2Effect::AssignUnconnectedPorts()
{
    for (size_t i = 0; i < this->inputAudioPortIndices.size(); ++i)
    {
        if (GetAudioInputBuffer(i) == nullptr)
        {
            int pluginIndex = this->inputAudioPortIndices.at(i);

            float *buffer = bufferPool.AllocateBuffer<float>(pHost->GetMaxAudioBufferSize());
            lilv_instance_connect_port(pInstance, pluginIndex, buffer);
        }
    }

    if (this->inputAudioPortIndices.size() != 0) // i.e. not using a mix buffer.
    {
        for (size_t i = 0; i < this->outputAudioPortIndices.size(); ++i)
        {
            if (GetAudioOutputBuffer(i) == nullptr)
            {
                float *buffer = bufferPool.AllocateBuffer<float>(pHost->GetMaxAudioBufferSize());
                int pluginIndex = this->outputAudioPortIndices.at(i);
                lilv_instance_connect_port(pInstance, pluginIndex, buffer);
            }
        }
    }
    for (int i = 0; i < this->GetNumberOfInputAtomPorts(); ++i)
    {
        if (GetAtomInputBuffer(i) == nullptr)
        {
            int pluginIndex = this->inputAtomPortIndices.at(i);

            uint8_t *buffer = bufferPool.AllocateBuffer<uint8_t>(pHost->GetAtomBufferSize());
            if (stagedInputAtomBufferPointer && i == 0)
            {
                lilv_instance_connect_port(pInstance, pluginIndex, stagedInputAtomBufferPointer);
                ResetInputAtomBuffer((char *)(stagedInputAtomBufferPointer));
            }
            else
            {
                lilv_instance_connect_port(pInstance, pluginIndex, buffer);
            }
            ResetInputAtomBuffer((char *)buffer);
            this->inputAtomBuffers.at(i) = (char *)buffer;
        }
    }
    for (int i = 0; i < this->GetNumberOfOutputAtomPorts(); ++i)
    {
        if (GetAtomOutputBuffer(i) == nullptr)
        {
            int pluginIndex = this->outputAtomPortIndices.at(i);

            uint8_t *buffer = bufferPool.AllocateBuffer<uint8_t>(pHost->GetAtomBufferSize());
            ResetOutputAtomBuffer((char *)buffer);

            if (stagedOutputAtomBufferPointer && i == 0)
            {
                lilv_instance_connect_port(pInstance, pluginIndex, stagedOutputAtomBufferPointer);
            }
            else
            {
                lilv_instance_connect_port(pInstance, pluginIndex, buffer);
            }

            lilv_instance_connect_port(pInstance, pluginIndex, buffer);
            this->outputAtomBuffers.at(i) = (char *)buffer;
        }
    }
}
void Lv2Effect::Deactivate()
{
    if (!activated)
    {
        return;
    }
    activated = false;
    if (worker)
    {
        worker->Close();
    }
    lilv_instance_deactivate(pInstance);
}

static inline void CopyBuffer(float *restrict input, float *restrict output, uint32_t frames)
{
    for (uint32_t i = 0; i < frames; ++i)
    {
        output[i] = input[i];
    }
}

size_t Lv2Effect::stageToOutput(size_t outputIndex, size_t nFrames)
{
    size_t thisTime = nFrames - outputIndex;
    size_t stagedOutputAvailable = this->stagingBufferSize - this->stagingOutputIx;
    if (stagedOutputAvailable < thisTime)
    {
        thisTime = stagedOutputAvailable;
    }
    if (thisTime)
    {
        for (size_t ch = 0; ch < this->GetNumberOfOutputAudioBuffers(); ++ch)
        {
            float *restrict pIn = this->outputStagingBufferPointers.at(ch) + this->stagingOutputIx;
            float *restrict pOut = this->GetAudioOutputBuffer(ch) + outputIndex;
            for (size_t i = 0; i < thisTime; ++i)
            {
                pOut[i] = pIn[i];
            }
        }
        this->stagingOutputIx += thisTime;
    }
    return outputIndex + thisTime;
}

void Lv2Effect::copyAtomBufferEventSequence(LV2_Atom_Sequence *controlInput, LV2_Atom_Forge &outputForge)
{
    LV2_ATOM_SEQUENCE_FOREACH(controlInput, ev)
    {
        lv2_atom_forge_frame_time(&outputForge, ev->time.frames);
        lv2_atom_forge_raw(&outputForge, &(ev->body), ev->body.size); // literal copy of the atom body.
    }
}

size_t Lv2Effect::stageToInput(size_t inputSampleOffset, size_t samples)
{

    size_t thisTime = samples - inputSampleOffset;
    size_t inputAvailable = this->stagingBufferSize - this->stagingInputIx;
    if (thisTime > inputAvailable)
    {
        thisTime = inputAvailable;
    }
    // copy into staging buffers.
    for (size_t nInput = 0; nInput < this->inputAudioBuffers.size(); ++nInput)
    {
        float *restrict pInput = this->inputAudioBuffers[nInput] + inputSampleOffset;
        float *restrict pOutput = this->inputStagingBufferPointers.at(nInput) + this->stagingInputIx;
        for (size_t i = 0; i < thisTime; ++i)
        {
            pOutput[i] = pInput[i];
        }
    }
    this->stagingInputIx += thisTime;
    inputSampleOffset += thisTime;

    if (stagingInputIx == this->stagingBufferSize)
    {
        // close off the atom input frame.
        if (stagedInputAtomBufferPointer)
        {
            lv2_atom_forge_pop(&this->stagedInputForgeRt, &staged_input_frame);
        }
        if (stagedOutputAtomBufferPointer)
        {
            ResetOutputAtomBuffer((char *)stagedOutputAtomBufferPointer);
        }

        lilv_instance_run(pInstance, this->stagingBufferSize);

        if (worker)
        {

            worker->EmitResponses();
        }
        if (stagedOutputAtomBufferPointer)
        {
            copyAtomBufferEventSequence((LV2_Atom_Sequence *)stagedOutputAtomBufferPointer, this->outputForgeRt);
        }

        this->stagingInputIx = 0;
        this->stagingOutputIx = 0;
        this->resetStagedInputAtomBuffer();
    }
    return inputSampleOffset;
}

void Lv2Effect::resetStagedInputAtomBuffer()
{
    if (stagedInputAtomBufferPointer)
    {
        const uint32_t notify_capacity = pHost->GetAtomBufferSize();
        lv2_atom_forge_set_buffer(
            &(this->stagedInputForgeRt), (uint8_t *)(this->stagedInputAtomBufferPointer), notify_capacity);
        lv2_atom_forge_sequence_head(&this->inputForgeRt, &staged_input_frame, urids.units__frame);
    }
}
void Lv2Effect::RunWithBufferStaging(uint32_t samples, RealtimeRingBufferWriter *realtimeRingBufferWriter)
{
    // accumulte control input sequence until we can execute a run operation.
    if (this->inputAtomBuffers.size() != 0)
    {
        lv2_atom_forge_pop(&this->inputForgeRt, &input_frame);

        LV2_Atom_Sequence *controlInput = (LV2_Atom_Sequence *)GetAtomInputBuffer(0);
        copyAtomBufferEventSequence(controlInput, this->stagedInputForgeRt);
    }
    // Prepare ACTUAL control output port.
    if (this->stagedOutputAtomBufferPointer)
    {
        const uint32_t notify_capacity = pHost->GetAtomBufferSize();
        lv2_atom_forge_set_buffer(
            &(this->outputForgeRt), (uint8_t *)(this->inputAtomBuffers.at(0)), notify_capacity);
        lv2_atom_forge_sequence_head(&this->outputForgeRt, &output_frame, urids.units__frame);
    }

    uint32_t inputSampleOffset = 0;
    uint32_t outputSampleOffset = 0;

    while (true)
    {
        outputSampleOffset = stageToOutput(outputSampleOffset, samples);

        CheckStagingBufferSentries();

        if (inputSampleOffset == samples)
        {
            break;
        }
        inputSampleOffset = stageToInput(inputSampleOffset, samples);
    }
    // no staging data avaialble? Output zeros.
    if (outputSampleOffset != samples)
    {
        size_t thisTime = samples - outputSampleOffset;
        for (size_t ch = 0; ch < this->GetNumberOfOutputAudioBuffers(); ++ch)
        {
            float *pOut = this->GetAudioOutputBuffer(ch) + outputSampleOffset;
            for (size_t i = 0; i < thisTime; ++i)
            {
                pOut[i] = 0;
            }
        }
    }
    MixOutput(samples, realtimeRingBufferWriter);
}

inline void Lv2Effect::MixOutput(uint32_t samples, RealtimeRingBufferWriter *realtimeRingBufferWriter)
{
    // for zero-input plugins, mix the plugin output with the input signal.
    if (this->inputAudioPortIndices.size() == 0)
    {

        // mix a zero input controls into the output buffer using a triangular mix curve.
        float pluginLevel = std::max(1.0f, this->zeroInputMix * 2);
        float inputLevel = std::max(1.0f, (1 - this->zeroInputMix) * 2);

        // case
        // 1 plugin output into 1 output.
        // 2 plugin outputs into 2 outputs.
        if (this->outputAudioBuffers.size() == this->outputMixBuffers.size())
        {
            for (size_t i = 0; i < this->outputMixBuffers.size(); ++i)
            {
                float *restrict input;
                if (i >= this->inputAudioBuffers.size())
                {
                    if (this->inputAudioBuffers.size() == 0)
                    {
                        break;
                    }
                    input = this->inputAudioBuffers.at(0);
                }
                else
                {
                    input = this->inputAudioBuffers.at(i);
                }
                float *restrict pluginOutput = this->outputMixBuffers.at(i).data();
                float *restrict finalOutput = this->outputAudioBuffers.at(i);

                for (uint32_t i = 0; i < samples; ++i)
                {
                    finalOutput[i] = input[i] * inputLevel + pluginOutput[i] * pluginLevel;
                }
            }
        }
        else if (this->outputAudioPortIndices.size() == 1 && this->outputAudioBuffers.size() == 2)
        {
            // 1 plugin output into 2 outputs.
            float *restrict pluginOutput = this->outputMixBuffers.at(0).data();
            for (size_t i = 0; i < this->outputMixBuffers.size(); ++i)
            {
                float *restrict input = this->inputAudioBuffers.at(i);
                float *restrict finalOutput = this->outputAudioBuffers.at(i);

                for (uint32_t i = 0; i < samples; ++i)
                {
                    finalOutput[i] = input[i] * inputLevel + pluginOutput[i] * pluginLevel;
                }
            }
        }
        else
        {
            // e.g. 2 plugin outputs into 1 output (should never happen)
            std::runtime_error("Internal error 0xEA48");
        }
    }

    // do soft bypass.
    if (this->bypassSamplesRemaining == 0)
    {
        if (this->currentBypass == 0)
        {
            // replace the contents of the output buffer(s) with the input buffer(s).
            if (this->outputAudioBuffers.size() == 1)
            {
                CopyBuffer(this->inputAudioBuffers.at(0), this->outputAudioBuffers.at(0), samples);
            }
            else
            {
                if (this->inputAudioBuffers.size() == 1)
                {
                    CopyBuffer(this->inputAudioBuffers.at(0), this->outputAudioBuffers.at(0), samples);
                    CopyBuffer(this->inputAudioBuffers.at(0), this->outputAudioBuffers.at(1), samples);
                }
                else
                {
                    CopyBuffer(this->inputAudioBuffers.at(0), this->outputAudioBuffers.at(0), samples);
                    CopyBuffer(this->inputAudioBuffers.at(1), this->outputAudioBuffers.at(1), samples);
                }
            }
        } // else leave the output alone.
    }
    else
    {
        double currentBypass = this->currentBypass;
        double currentBypassDx = this->currentBypassDx;
        int32_t bypassSamplesRemaining = (int)this->bypassSamplesRemaining;

        if (this->outputAudioBuffers.size() == 1)
        {
            float *restrict input = this->inputAudioBuffers.at(0);
            float *restrict output = this->outputAudioBuffers.at(0);
            for (uint32_t i = 0; i < samples; ++i)
            {
                output[i] = currentBypass * output[i] + (1 - currentBypass) * input[i];

                if (--bypassSamplesRemaining == 0)
                {
                    currentBypassDx = 0;
                    currentBypass = this->targetBypass;
                }
                currentBypass += currentBypassDx;
            }
        }
        else
        {
            float *restrict inputL;
            float *restrict inputR;
            if (this->inputAudioBuffers.size() == 1)
            {
                inputL = inputR = inputAudioBuffers.at(0);
            }
            else
            {
                inputL = inputAudioBuffers.at(0);
                inputR = inputAudioBuffers.at(1);
            }
            float *restrict outputL = outputAudioBuffers.at(0);
            float *restrict outputR = outputAudioBuffers.at(1);
            for (uint32_t i = 0; i < samples; ++i)
            {
                outputL[i] = currentBypass * outputL[i] + (1 - currentBypass) * inputL[i];
                outputR[i] = currentBypass * outputR[i] + (1 - currentBypass) * inputR[i];
                if (--bypassSamplesRemaining == 0)
                {
                    currentBypassDx = 0;
                    currentBypass = this->targetBypass;
                }
                currentBypass += currentBypassDx;
            }
        }
        if (bypassSamplesRemaining <= 0)
        {
            this->bypassSamplesRemaining = 0;
            this->currentBypass = this->targetBypass;
            this->currentBypassDx = 0;
        }
        else
        {
            this->currentBypass = currentBypass;
            this->currentBypassDx = currentBypassDx;
            this->bypassSamplesRemaining = bypassSamplesRemaining;
        }
    }
    RelayPatchSetMessages(this->instanceId, realtimeRingBufferWriter);
}

void Lv2Effect::Run(uint32_t samples, RealtimeRingBufferWriter *realtimeRingBufferWriter)
{
    // close off the atom input frame.
    if (this->inputAtomBuffers.size() != 0)
    {
        lv2_atom_forge_pop(&this->inputForgeRt, &input_frame);
    }
    lilv_instance_run(pInstance, samples);

    if (worker)
    {
        // relay worker response
        worker->EmitResponses();
    }

    MixOutput(samples, realtimeRingBufferWriter);
}

LV2_Worker_Status Lv2Effect::worker_schedule_fn(LV2_Worker_Schedule_Handle handle,
                                                uint32_t size,
                                                const void *data)
{
    Lv2Effect *this_ = (Lv2Effect *)handle;
    this_->worker->ScheduleWork(size, data);
    return LV2_WORKER_SUCCESS;
}

struct BufferHeader
{
    uint32_t size;
    uint32_t type;
};

void Lv2Effect::ResetInputAtomBuffer(char *data)
{
    BufferHeader *header = (BufferHeader *)data;
    header->size = sizeof(LV2_Atom_Sequence_Body);
    header->type = urids.atom__Sequence;
}
void Lv2Effect::ResetOutputAtomBuffer(char *data)
{
    BufferHeader *header = (BufferHeader *)data;
    header->size = pHost->GetAtomBufferSize() - 8;
    header->type = urids.atom__Chunk;
}
void Lv2Effect::BypassDezipperSet(float targetValue)
{
    this->targetBypass = targetValue;
    this->currentBypass = targetValue;
    this->currentBypassDx = 0;
    this->bypassSamplesRemaining = 0;
}

void Lv2Effect::BypassDezipperTo(float targetValue)
{
    this->targetBypass = targetValue;
    double dx = targetValue - this->currentBypass;
    if (dx != 0)
    {
        this->bypassSamplesRemaining = (int)(bypassStartingSamples * std::abs(dx));
        if (this->bypassStartingSamples == 0)
        {
            currentBypassDx = 0;
            this->currentBypass = targetBypass;
        }
        else
        {
            this->currentBypassDx = dx / this->bypassSamplesRemaining;
        }
    }
}

void Lv2Effect::ResetAtomBuffers()
{
    for (size_t i = 0; i < this->inputAtomBuffers.size(); ++i)
    {
        ResetInputAtomBuffer(this->inputAtomBuffers.at(i));
    }
    for (size_t i = 0; i < this->outputAtomBuffers.size(); ++i)
    {
        ResetOutputAtomBuffer(this->outputAtomBuffers.at(i));
    }
    if (inputAtomBuffers.size() != 0)
    {
        const uint32_t notify_capacity = pHost->GetAtomBufferSize();
        lv2_atom_forge_set_buffer(
            &(this->inputForgeRt), (uint8_t *)(this->inputAtomBuffers.at(0)), notify_capacity);

        // Start a sequence in the notify input port.

        lv2_atom_forge_sequence_head(&this->inputForgeRt, &input_frame, urids.units__frame);
    }
}

void Lv2Effect::RequestPatchProperty(LV2_URID uridUri)
{
    lv2_atom_forge_frame_time(&inputForgeRt, 0);

    LV2_Atom_Forge_Frame objectFrame;
    LV2_Atom_Forge_Ref set =
        lv2_atom_forge_object(&inputForgeRt, &objectFrame, 0, urids.patch__Get);

    lv2_atom_forge_key(&inputForgeRt, urids.patch__property);
    lv2_atom_forge_urid(&inputForgeRt, uridUri);
    lv2_atom_forge_pop(&inputForgeRt, &objectFrame);
}
void Lv2Effect::RequestAllPathPatchProperties()
{
    for (LV2_URID urid : this->pathProperties)
    {
        RequestPatchProperty(urid);
    }
}

void Lv2Effect::SetPatchProperty(LV2_URID uridUri, size_t size, LV2_Atom *value)
{
    lv2_atom_forge_frame_time(&inputForgeRt, 0);

    LV2_Atom_Forge_Frame objectFrame;
    LV2_Atom_Forge_Ref set =
        lv2_atom_forge_object(&inputForgeRt, &objectFrame, 0, urids.patch__Set);
    {
        lv2_atom_forge_key(&inputForgeRt, urids.patch__property);
        lv2_atom_forge_urid(&inputForgeRt, uridUri);
        lv2_atom_forge_key(&inputForgeRt, urids.patch__value);
        lv2_atom_forge_write(&inputForgeRt, value, size);
    }

    lv2_atom_forge_pop(&inputForgeRt, &objectFrame);
    this->requestStateChangedNotification = true;
}

void Lv2Effect::RelayPatchSetMessages(uint64_t instanceId, RealtimeRingBufferWriter *realtimeRingBufferWriter)
{
    LV2_Atom_Sequence *controlOutput = (LV2_Atom_Sequence *)GetAtomOutputBuffer();
    if (controlOutput == nullptr)
    {
        return;
    }

    bool maybeStateChanged = false;
    LV2_ATOM_SEQUENCE_FOREACH(controlOutput, ev)
    {

        // frame_offset = ev->time.frames;  // not really interested.

        if (lv2_atom_forge_is_object_type(&this->outputForgeRt, ev->body.type))
        {
            const LV2_Atom_Object *obj = (const LV2_Atom_Object *)&ev->body;
            if (obj->body.otype == urids.state__StateChanged)
            {
                requestStateChangedNotification = true;
            }
            else if (obj->body.otype == urids.patch__Set) // patch_Set is handled elsewhere.
            {
                maybeStateChanged = true;
                realtimeRingBufferWriter->AtomOutput(instanceId, obj->atom.size + sizeof(obj->atom), (uint8_t *)obj);
            }
        }
    }
    if (this->requestStateChangedNotification)
    {
        requestStateChangedNotification = false;
        realtimeRingBufferWriter->Lv2StateChanged(instanceId);
    }
    else if (maybeStateChanged)
    {
        realtimeRingBufferWriter->MaybeLv2StateChanged(instanceId);
    }
}

void Lv2Effect::GatherPathPatchProperties(IPatchWriterCallback *cbPatchWriter)
{
    if (pathPropertyWriters.size() != 0)
    {
        LV2_Atom_Sequence *controlInput = (LV2_Atom_Sequence *)GetAtomOutputBuffer();
        if (controlInput == nullptr)
        {
            return;
        }
        LV2_ATOM_SEQUENCE_FOREACH(controlInput, ev)
        {

            auto frame_offset = ev->time.frames; // not really interested.

            if (lv2_atom_forge_is_object_type(&this->outputForgeRt, ev->body.type))
            {
                const LV2_Atom_Object *obj = (const LV2_Atom_Object *)&ev->body;
                if (obj->body.otype == urids.patch__Set)
                {
                    // Get the property and value of the set message
                    const LV2_Atom *property = NULL;
                    const LV2_Atom *value = NULL;

                    lv2_atom_object_get(
                        obj,
                        urids.patch__property, &property,
                        urids.patch__value, &value,
                        0);

                    if (property && property->type == urids.atom__URID && value)
                    {
                        LV2_URID key = ((const LV2_Atom_URID *)property)->body;

                        for (PatchPropertyWriter &pathPropertyWriter : pathPropertyWriters)
                        {
                            if (key == pathPropertyWriter.patchPropertyUrid)
                            {
                                auto buffer = pathPropertyWriter.AquireWriteBuffer();
                                size_t atom_size = value->size + sizeof(LV2_Atom);
                                buffer->memory.resize(atom_size);
                                memcpy(buffer->memory.data(), value, atom_size);
                                break;
                            }
                        }
                    }
                }
            }
        }
        for (auto &writer : this->pathPropertyWriters)
        {
            writer.FlushWrites(cbPatchWriter);
        }
    }
}

void Lv2Effect::GatherPatchProperties(RealtimePatchPropertyRequest *pRequest)
{
    if (pRequest->requestType == RealtimePatchPropertyRequest::RequestType::PatchGet)
    {
        LV2_Atom_Sequence *controlInput = (LV2_Atom_Sequence *)GetAtomOutputBuffer();
        if (controlInput == nullptr)
        {
            return;
        }
        LV2_ATOM_SEQUENCE_FOREACH(controlInput, ev)
        {

            auto frame_offset = ev->time.frames; // not really interested.

            if (lv2_atom_forge_is_object_type(&this->outputForgeRt, ev->body.type))
            {
                const LV2_Atom_Object *obj = (const LV2_Atom_Object *)&ev->body;
                if (obj->body.otype == urids.patch__Set)
                {
                    // Get the property and value of the set message
                    const LV2_Atom *property = NULL;
                    const LV2_Atom *value = NULL;

                    lv2_atom_object_get(
                        obj,
                        urids.patch__property, &property,
                        urids.patch__value, &value,
                        0);

                    if (property && property->type == urids.atom__URID && value)
                    {
                        LV2_URID key = ((const LV2_Atom_URID *)property)->body;
                        if (key == pRequest->uridUri)
                        {
                            int atom_size = value->size + sizeof(LV2_Atom);
                            pRequest->SetSize(atom_size);
                            memcpy(pRequest->GetBuffer(), value, atom_size);
                            break;
                        }
                    }
                }
            }
        }
    }
}

void Lv2Effect::SetLv2State(Lv2PluginState &state)
{
    if (state.isValid_)
    {
        return;
    }
    if (!this->stateInterface)
    {
        return;
    }
    try
    {

        this->stateInterface->Restore(state);
    }
    catch (const std::exception &e)
    {
        Lv2Log::error("Failed to restore LV2 state.");
    }
}
bool Lv2Effect::GetLv2State(Lv2PluginState *state)
{
    if (!this->stateInterface)
        return false;
    try
    {
        if (this->stateInterface == nullptr)
        {
            state->Erase();
            return false;
        }

        *state = this->stateInterface->Save();
        state->isValid_ = true;
        return true;
    }
    catch (const std::exception &e)
    {
        state->Erase();
        throw;
    }
}

void Lv2Effect::OnLogError(const char *message)
{
    // only errors get transmitted to the client.
    strncpy(this->errorMessage, message, sizeof(errorMessage));
    errorMessage[sizeof(errorMessage) - 1] = '\0';
    this->hasErrorMessage = true;
}

void Lv2Effect::OnLogWarning(const char *message)
{
    Lv2Log::warning(message);
}
void Lv2Effect::OnLogInfo(const char *message)
{
    Lv2Log::info(message);
}
void Lv2Effect::OnLogDebug(const char *message)
{
    Lv2Log::debug(message);
}

bool Lv2Effect::GetRequestStateChangedNotification() const { return requestStateChangedNotification; }
void Lv2Effect::SetRequestStateChangedNotification(bool value) { requestStateChangedNotification = value; }

uint64_t Lv2Effect::GetMaxInputControl() const { return maxInputControlPort; }

bool Lv2Effect::IsInputControl(uint64_t index) const
{
    if (index >= isInputControlPort.size())
        return false;
    return isInputControlPort.at(index);
}
float Lv2Effect::GetDefaultInputControlValue(uint64_t index) const
{
    return defaultInputControlValues.at(index);
}

std::string Lv2Effect::GetPathPatchProperty(const std::string &propertyUri)
{
    if (!this->mainThreadPathProperties.contains(propertyUri))
    {
        return "";
    }
    return this->mainThreadPathProperties[propertyUri];
}
void Lv2Effect::SetPathPatchProperty(const std::string &propertyUri, const std::string &jsonAtom)
{
    mainThreadPathProperties[propertyUri] = jsonAtom;
}

void Lv2Effect::EnableBufferStaging(size_t bufferSize, size_t nInputs, size_t nOutputs)
{

    stagingBufferSize = bufferSize;
    stagingOutputIx = bufferSize;
    stagingInputIx = 0;
    inputStagingBuffers.resize(nInputs);
    outputStagingBuffers.resize(nOutputs);
    inputStagingBufferPointers.resize(nInputs);
    outputStagingBufferPointers.resize(nOutputs);

    if (inputAtomBuffers.size() != 0)
    {
        stagedInputAtomBuffer.resize(pHost->GetAtomBufferSize());
        stagedInputAtomBufferPointer = stagedInputAtomBuffer.data();
        resetStagedInputAtomBuffer();
    }
    else
    {
        stagedInputAtomBufferPointer = nullptr;
    }
    stagedOutputAtomBufferPointer = nullptr;
    if (outputAtomBuffers.size() != 0)
    {
        stagedOutputAtomBuffer.resize(pHost->GetAtomBufferSize());
        stagedOutputAtomBufferPointer = stagedOutputAtomBuffer.data();
    }
    for (size_t i = 0; i < nInputs; ++i)
    {
        inputStagingBuffers.at(i).resize(bufferSize + 1);
        inputStagingBuffers[i][bufferSize] = 99.9f; // guard entry
        inputStagingBufferPointers.at(i) = inputStagingBuffers.at(i).data();
    }
    for (size_t i = 0; i < nOutputs; ++i)
    {
        outputStagingBuffers.at(i).resize(bufferSize + 1);
        outputStagingBuffers[i][bufferSize] = 99.9f; // guard entry
        outputStagingBufferPointers.at(i) = outputStagingBuffers.at(i).data();
    }
}

static size_t NextPowerOfTwo(size_t value)
{

    size_t i = 1;
    while (i < value && i < 65536UL)
    {
        i *= 2;
    }
    return i;
}

size_t Lv2Effect::GetStagedBufferSize() const
{
    size_t pluginBlockLength = pHost->GetMaxAudioBufferSize();
    if (info->minBlockLength() != -1 || info->maxBlockLength() != -1)
    {
        if (info->minBlockLength() != -1 && pluginBlockLength < info->minBlockLength())
        {
            pluginBlockLength = info->minBlockLength();
        }
        if (info->maxBlockLength() != -1 && pluginBlockLength > info->maxBlockLength())
        {
            pluginBlockLength = info->maxBlockLength();
        }
        if (info->powerOf2BlockLength())
        {
            pluginBlockLength = NextPowerOfTwo(pluginBlockLength);
        }
    }
    return pluginBlockLength;
}

bool Lv2Effect::RequiresBufferStaging() const
{
    return GetStagedBufferSize() != pHost->GetMaxAudioBufferSize();
}

float *Lv2Effect::GetAudioInputBuffer(int index) const
{
    if (index < 0 || index >= this->inputAudioBuffers.size())
        throw std::range_error("Lv2Effect::GetAudioInputBuffer");
    return this->inputAudioBuffers.at(index);
}

float *Lv2Effect::GetAudioOutputBuffer(int index) const
{
    if (index < 0 || index >= this->outputAudioBuffers.size())
    {
        throw std::range_error("Lv2Effect::GetAudioOutputBuffer");
    }
    return this->outputAudioBuffers.at(index);
}
