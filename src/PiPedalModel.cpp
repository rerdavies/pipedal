// Copyright (c) 2021 Robin Davies
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

#include "PiPedalModel.hpp"
#include "JackHost.hpp"
#include "Lv2Log.hpp"
#include <set>
#include "PiPedalConfiguration.hpp"
#include "ShutdownClient.hpp"
#include "SplitEffect.hpp"

#ifndef NO_MLOCK
#include <sys/mman.h>
#endif /* NO_MLOCK */

using namespace pipedal;

template <typename T>
T &constMutex(const T &mutex)
{
    return const_cast<T &>(mutex);
}

std::string AtomToJson(int length, uint8_t *pData)
{
    return "";
}

PiPedalModel::PiPedalModel()
{
    this->pedalBoard = PedalBoard::MakeDefault();
    this->jackServerSettings.ReadJackConfiguration();

}

void PiPedalModel::Close()
{
    std::lock_guard lock(this->mutex);

    // take a snapshot incase a client unsusbscribes in the notification handler (in which case the mutex won't protect us)
    IPiPedalModelSubscriber **t = new IPiPedalModelSubscriber *[this->subscribers.size()];
    for (size_t i = 0; i < subscribers.size(); ++i)
    {
        t[i] = this->subscribers[i];
    }
    size_t n = this->subscribers.size();
    for (size_t i = 0; i < n; ++i)
    {
        t[i]->Close();
    }
    delete[] t;

    if (jackHost)
    {
        jackHost->Close();
    }
}

PiPedalModel::~PiPedalModel()
{
    CurrentPreset currentPreset;
    currentPreset.modified_ = this->hasPresetChanged;
    currentPreset.preset_ = this->pedalBoard;
    storage.SaveCurrentPreset(currentPreset);
    
    if (jackHost)
    {
        jackHost->Close();
    }
}

#include <fstream>


void PiPedalModel::LoadLv2PluginInfo(const PiPedalConfiguration&configuration)
{
    // Not all Lv2 directories have the lv2 base declarations. Load a full set of plugin classes generated on a default /usr/local/lib/lv2 directory.
    std::filesystem::path pluginClassesPath = configuration.GetDocRoot() / "plugin_classes.json";
    try
    {
        if (!std::filesystem::exists(pluginClassesPath))
            throw PiPedalException("File not found.");
        lv2Host.LoadPluginClassesFromJson(pluginClassesPath);
    }
    catch (const std::exception &e)
    {
        std::stringstream s;
        s << "Unable to load " << pluginClassesPath << ". " << e.what();
        throw PiPedalException(s.str().c_str());
    }

    lv2Host.Load(configuration.GetLv2Path().c_str());

}

void PiPedalModel::Load(const PiPedalConfiguration &configuration)
{

    this->jackServerSettings.ReadJackConfiguration();


    storage.SetDataRoot(configuration.GetLocalStoragePath().c_str());
    storage.Initialize();

    // lv2Host.Load(configuration.GetLv2Path().c_str());

    this->pedalBoard = storage.GetCurrentPreset(); // the current *saved* preset.

    // the current edited preset, saved only across orderly shutdowns.
    CurrentPreset currentPreset;
    if (storage.RestoreCurrentPreset(&currentPreset))
    {
        this->pedalBoard = currentPreset.preset_;
        this->hasPresetChanged = currentPreset.modified_;
    }
    updateDefaults(&this->pedalBoard);

    std::unique_ptr<JackHost> p{JackHost::CreateInstance(lv2Host.asIHost())};
    this->jackHost = std::move(p);

    this->jackHost->SetNotificationCallbacks(this);

    if (configuration.GetMLock())
    {
#ifndef NO_MLOCK
        int result = mlockall(MCL_CURRENT | MCL_FUTURE);
        if (result)
        {
            throw PiPedalStateException("mlockall failed. You can disable the call to mlockall  in 'config.json'.");
        }

#endif
    }

    this->jackConfiguration = jackHost->GetServerConfiguration();
    if (this->jackConfiguration.isValid())
    {
        JackChannelSelection selection = storage.GetJackChannelSelection(jackConfiguration);
        selection = selection.RemoveInvalidChannels(jackConfiguration);

        this->lv2Host.OnConfigurationChanged(jackConfiguration, selection);
        try
        {
            jackHost->Open(selection);

            std::shared_ptr<Lv2PedalBoard> lv2PedalBoard{this->lv2Host.CreateLv2PedalBoard(this->pedalBoard)};
            this->lv2PedalBoard = lv2PedalBoard;
            jackHost->SetPedalBoard(lv2PedalBoard);
        }
        catch (PiPedalException &e)
        {
            Lv2Log::error("Failed to load initial plugin. (%s)", e.what());
        }
    }
}

IPiPedalModelSubscriber *PiPedalModel::GetNotificationSubscriber(int64_t clientId)
{
    for (size_t i = 0; i < subscribers.size(); ++i)
    {
        if (subscribers[i]->GetClientId() == clientId)
        {
            return subscribers[i];
        }
    }
    return nullptr;
}

void PiPedalModel::AddNotificationSubscription(IPiPedalModelSubscriber *pSubscriber)
{
    std::lock_guard lock(this->mutex);
    this->subscribers.push_back(pSubscriber);
}
void PiPedalModel::RemoveNotificationSubsription(IPiPedalModelSubscriber *pSubscriber)
{
    {
        std::lock_guard lock(this->mutex);

        for (auto it = this->subscribers.begin(); it != this->subscribers.end(); ++it)
        {
            if ((*it) == pSubscriber)
            {
                this->subscribers.erase(it);
                break;
            }
        }
        int64_t clientId = pSubscriber->GetClientId();

        this->deleteMidiListeners(clientId);

        for (int i = 0; i < this->outstandingParameterRequests.size(); ++i)
        {
            if (outstandingParameterRequests[i]->clientId == clientId)
            {
                outstandingParameterRequests.erase(outstandingParameterRequests.begin() + i);
                --i;
            }
        }
    }
}

void PiPedalModel::previewControl(int64_t clientId, int64_t pedalItemId, const std::string &symbol, float value)
{

    jackHost->SetControlValue(pedalItemId, symbol, value);
}

void PiPedalModel::setControl(int64_t clientId, int64_t pedalItemId, const std::string &symbol, float value)
{
    {
        std::lock_guard guard(mutex);
        previewControl(clientId, pedalItemId, symbol, value);
        this->pedalBoard.SetControlValue(pedalItemId, symbol, value);
        {

            // take a snapshot incase a client unsusbscribes in the notification handler (in which case the mutex won't protect us)
            IPiPedalModelSubscriber **t = new IPiPedalModelSubscriber *[this->subscribers.size()];
            for (size_t i = 0; i < subscribers.size(); ++i)
            {
                t[i] = this->subscribers[i];
            }
            size_t n = this->subscribers.size();
            for (size_t i = 0; i < n; ++i)
            {
                t[i]->OnControlChanged(clientId, pedalItemId, symbol, value);
            }
            delete[] t;

            this->setPresetChanged(clientId, true);
        }
    }
}

void PiPedalModel::fireJackConfigurationChanged(const JackConfiguration &jackConfiguration)
{
    // noify subscribers.
    IPiPedalModelSubscriber **t = new IPiPedalModelSubscriber *[this->subscribers.size()];
    for (size_t i = 0; i < subscribers.size(); ++i)
    {
        t[i] = this->subscribers[i];
    }
    size_t n = this->subscribers.size();
    for (size_t i = 0; i < n; ++i)
    {
        t[i]->OnJackConfigurationChanged(jackConfiguration);
    }
    delete[] t;
}

void PiPedalModel::fireBanksChanged(int64_t clientId)
{
    // noify subscribers.
    IPiPedalModelSubscriber **t = new IPiPedalModelSubscriber *[this->subscribers.size()];
    for (size_t i = 0; i < subscribers.size(); ++i)
    {
        t[i] = this->subscribers[i];
    }
    size_t n = this->subscribers.size();
    for (size_t i = 0; i < n; ++i)
    {
        t[i]->OnBankIndexChanged(this->storage.GetBanks());
    }
    delete[] t;
}

void PiPedalModel::firePedalBoardChanged(int64_t clientId)
{
    // noify subscribers.
    IPiPedalModelSubscriber **t = new IPiPedalModelSubscriber *[this->subscribers.size()];
    for (size_t i = 0; i < subscribers.size(); ++i)
    {
        t[i] = this->subscribers[i];
    }
    size_t n = this->subscribers.size();
    for (size_t i = 0; i < n; ++i)
    {
        t[i]->OnPedalBoardChanged(clientId, this->pedalBoard);
    }
    delete[] t;

    // notify the audio thread.
    if (jackHost->IsOpen())
    {
        std::shared_ptr<Lv2PedalBoard> lv2PedalBoard{this->lv2Host.CreateLv2PedalBoard(this->pedalBoard)};
        this->lv2PedalBoard = lv2PedalBoard;
        jackHost->SetPedalBoard(lv2PedalBoard);
        updateRealtimeVuSubscriptions();
        updateRealtimeMonitorPortSubscriptions();
    }
}
void PiPedalModel::setPedalBoard(int64_t clientId, PedalBoard &pedalBoard)
{
    {
        std::lock_guard guard(this->mutex);
        this->pedalBoard = pedalBoard;
        updateDefaults(&this->pedalBoard);

        this->firePedalBoardChanged(clientId);
        this->setPresetChanged(clientId, true);
    }
}

void PiPedalModel::setPedalBoardItemEnable(int64_t clientId, int64_t pedalItemId, bool enabled)
{
    std::lock_guard(this->mutex);
    {
        this->pedalBoard.SetItemEnabled(pedalItemId, enabled);

        // Notify clients.
        IPiPedalModelSubscriber **t = new IPiPedalModelSubscriber *[this->subscribers.size()];
        for (size_t i = 0; i < subscribers.size(); ++i)
        {
            t[i] = this->subscribers[i];
        }
        size_t n = this->subscribers.size();
        for (size_t i = 0; i < n; ++i)
        {
            t[i]->OnItemEnabledChanged(clientId, pedalItemId, enabled);
        }
        delete[] t;
        this->setPresetChanged(clientId, true);

        // Notify audo thread.
        this->jackHost->SetBypass(pedalItemId, enabled);
    }
}

void PiPedalModel::getPresets(PresetIndex *pResult)
{
    std::lock_guard guard(mutex);

    this->storage.GetPresetIndex(pResult);
    pResult->presetChanged(this->hasPresetChanged);
}

PedalBoard PiPedalModel::getPreset(int64_t instanceId)
{
    std::lock_guard guard(mutex);
    return this->storage.GetPreset(instanceId);
}
void PiPedalModel::getBank(int64_t instanceId, BankFile *pResult)
{
    std::lock_guard guard(mutex);
    this->storage.GetBankFile(instanceId, pResult);
}

void PiPedalModel::setPresetChanged(int64_t clientId, bool value)
{
    if (value != this->hasPresetChanged)
    {
        hasPresetChanged = value;
        firePresetsChanged(clientId);
    }
}

void PiPedalModel::firePresetsChanged(int64_t clientId)
{
    std::lock_guard(this->mutex);
    {
        // take a snapshot incase a client unsusbscribes in the notification handler (in which case the mutex won't protect us)
        IPiPedalModelSubscriber **t = new IPiPedalModelSubscriber *[this->subscribers.size()];
        for (size_t i = 0; i < subscribers.size(); ++i)
        {
            t[i] = this->subscribers[i];
        }
        size_t n = this->subscribers.size();

        PresetIndex presets;
        getPresets(&presets);

        for (size_t i = 0; i < n; ++i)
        {
            t[i]->OnPresetsChanged(clientId, presets);
        }
        delete[] t;
    }
}


void PiPedalModel::saveCurrentPreset(int64_t clientId)
{
    std::lock_guard(this->mutex);

    storage.saveCurrentPreset(this->pedalBoard);
    this->setPresetChanged(clientId, false);
}

int64_t PiPedalModel::saveCurrentPresetAs(int64_t clientId, const std::string &name, int64_t saveAfterInstanceId)
{
    std::lock_guard(this->mutex);

    int64_t result = storage.saveCurrentPresetAs(this->pedalBoard, name, saveAfterInstanceId);
    this->hasPresetChanged = false;
    firePresetsChanged(clientId);
    return result;
}

int64_t PiPedalModel::uploadPreset(const BankFile &bankFile, int64_t uploadAfter)
{
    std::lock_guard(this->mutex);

    int64_t newPreset = this->storage.UploadPreset(bankFile, uploadAfter);
    firePresetsChanged(-1);
    return newPreset;
}
int64_t PiPedalModel::uploadBank(BankFile &bankFile, int64_t uploadAfter)
{
    std::lock_guard(this->mutex);

    int64_t newPreset = this->storage.UploadBank(bankFile, uploadAfter);
    fireBanksChanged(-1);
    return newPreset;
}

void PiPedalModel::loadPreset(int64_t clientId, int64_t instanceId)
{
    std::lock_guard(this->mutex);

    if (storage.LoadPreset(instanceId))
    {
        this->pedalBoard = storage.GetCurrentPreset();
        updateDefaults(&this->pedalBoard);

        this->hasPresetChanged = false; // no fire.
        this->firePedalBoardChanged(clientId);
        this->firePresetsChanged(clientId); // fire now.
    }
}

int64_t PiPedalModel::copyPreset(int64_t clientId, int64_t from, int64_t to)
{
    std::lock_guard(this->mutex);

    int64_t result = storage.CopyPreset(from, to);
    if (result != -1)
    {
        this->firePresetsChanged(clientId); // fire now.
    }
    else
    {
        throw PiPedalStateException("Copy failed.");
    }
    return result;
}
bool PiPedalModel::updatePresets(int64_t clientId, const PresetIndex &presets)
{
    std::lock_guard(this->mutex);
    storage.SetPresetIndex(presets);
    firePresetsChanged(clientId);
    return true;
}

void PiPedalModel::moveBank(int64_t clientId, int from, int to)
{
    std::lock_guard(this->mutex);
    storage.MoveBank(from, to);
    fireBanksChanged(clientId);
}
int64_t PiPedalModel::deleteBank(int64_t clientId, int64_t instanceId)
{
    std::lock_guard(this->mutex);
    int64_t selectedBank = this->storage.GetBanks().selectedBank();
    int64_t newSelection = storage.DeleteBank(instanceId);

    int64_t newSelectedBank = this->storage.GetBanks().selectedBank();

    this->fireBanksChanged(clientId); // fire now.

    if (newSelectedBank != selectedBank)
    {
        this->openBank(clientId, newSelectedBank);
    }
    return newSelection;
}

int64_t PiPedalModel::deletePreset(int64_t clientId, int64_t instanceId)
{
    std::lock_guard(this->mutex);

    int64_t newSelection = storage.DeletePreset(instanceId);
    this->firePresetsChanged(clientId); // fire now.
    return newSelection;
}
bool PiPedalModel::renamePreset(int64_t clientId, int64_t instanceId, const std::string &name)
{
    std::lock_guard guard(mutex);
    if (storage.RenamePreset(instanceId, name))
    {
        this->firePresetsChanged(clientId);
        return true;
    }
    else
    {
        throw PiPedalStateException("Rename failed.");
    }
}

void PiPedalModel::setWifiConfigSettings(const WifiConfigSettings &wifiConfigSettings)
{
    std::lock_guard lock(this->mutex);

    ShutdownClient::SetWifiConfig(wifiConfigSettings);

    this->storage.SetWifiConfigSettings(wifiConfigSettings);

    {
        IPiPedalModelSubscriber **t = new IPiPedalModelSubscriber *[this->subscribers.size()];
        for (size_t i = 0; i < subscribers.size(); ++i)
        {
            t[i] = this->subscribers[i];
        }
        size_t n = this->subscribers.size();

        WifiConfigSettings tWifiConfigSettings = storage.GetWifiConfigSettings(); // (the passwordless version)

        for (size_t i = 0; i < n; ++i)
        {
            t[i]->OnWifiConfigSettingsChanged(tWifiConfigSettings);
        }
        delete[] t;
    }
}
WifiConfigSettings PiPedalModel::getWifiConfigSettings()
{
    std::lock_guard lock(this->mutex);
    return this->storage.GetWifiConfigSettings();
}

JackConfiguration PiPedalModel::getJackConfiguration()
{
    std::lock_guard lock(this->mutex); // copy atomically.
    return this->jackConfiguration;
}

void PiPedalModel::setJackChannelSelection(int64_t clientId, const JackChannelSelection &channelSelection)
{
    std::lock_guard lock(this->mutex); // copy atomically.
    this->storage.SetJackChannelSelection(channelSelection);
    this->fireChannelSelectionChanged(clientId);

    this->lv2Host.OnConfigurationChanged(jackConfiguration, channelSelection);
    if (this->jackHost->IsOpen())
    {
        // do a complete reload.

        this->jackHost->Close();
        this->jackHost->SetPedalBoard(nullptr);
        this->jackHost->Open(channelSelection);
        std::shared_ptr<Lv2PedalBoard> lv2PedalBoard{this->lv2Host.CreateLv2PedalBoard(this->pedalBoard)};
        this->lv2PedalBoard = lv2PedalBoard;
        jackHost->SetPedalBoard(lv2PedalBoard);
        this->updateRealtimeVuSubscriptions();
    }
}

void PiPedalModel::fireChannelSelectionChanged(int64_t clientId)
{
    std::lock_guard(this->mutex);
    {
        // take a snapshot incase a client unsusbscribes in the notification handler (in which case the mutex won't protect us)
        IPiPedalModelSubscriber **t = new IPiPedalModelSubscriber *[this->subscribers.size()];
        for (size_t i = 0; i < subscribers.size(); ++i)
        {
            t[i] = this->subscribers[i];
        }
        size_t n = this->subscribers.size();

        JackChannelSelection channelSelection = storage.GetJackChannelSelection(this->jackConfiguration);

        for (size_t i = 0; i < n; ++i)
        {
            t[i]->OnChannelSelectionChanged(clientId, channelSelection);
        }
        delete[] t;
    }
}

JackChannelSelection PiPedalModel::getJackChannelSelection()
{
    std::lock_guard guard(mutex);
    JackChannelSelection t = this->storage.GetJackChannelSelection(this->jackConfiguration);
    if (this->jackConfiguration.isValid())
    {
        t = t.RemoveInvalidChannels(this->jackConfiguration);
    }
    return t;
}

int64_t PiPedalModel::addVuSubscription(int64_t instanceId)
{
    std::lock_guard guard(mutex);
    int64_t subscriptionId = ++nextSubscriptionId;
    activeVuSubscriptions.push_back(VuSubscription{subscriptionId, instanceId});

    updateRealtimeVuSubscriptions();

    return subscriptionId;
}
void PiPedalModel::removeVuSubscription(int64_t subscriptionHandle)
{
    std::lock_guard guard(mutex);
    for (auto i = activeVuSubscriptions.begin(); i != activeVuSubscriptions.end(); ++i)
    {
        if ((*i).subscriptionHandle == subscriptionHandle)
        {
            activeVuSubscriptions.erase(i);
            break;
        }
    }
    updateRealtimeVuSubscriptions();
}

void PiPedalModel::OnNotifyMidiValueChanged(int64_t instanceId, int portIndex, float value)
{
    std::lock_guard guard(mutex);
    PedalBoardItem *item = this->pedalBoard.GetItem(instanceId);
    if (item)
    {
        const Lv2PluginInfo *pPluginInfo;
        if (item->uri() == SPLIT_PEDALBOARD_ITEM_URI)
        {
            pPluginInfo = GetSplitterPluginInfo();
        }
        else
        {
            auto pluginInfo = lv2Host.GetPluginInfo(item->uri());
            pPluginInfo = pluginInfo.get();
        }
        if (pPluginInfo)
        {
            if (portIndex == -1)
            {
                // bypass!
                this->pedalBoard.SetItemEnabled(instanceId, value != 0);
                // take a snapshot incase a client unsusbscribes in the notification handler (in which case the mutex won't protect us)
                IPiPedalModelSubscriber **t = new IPiPedalModelSubscriber *[this->subscribers.size()];
                for (size_t i = 0; i < subscribers.size(); ++i)
                {
                    t[i] = this->subscribers[i];
                }
                size_t n = this->subscribers.size();
                for (size_t i = 0; i < n; ++i)
                {
                    t[i]->OnItemEnabledChanged(-1, instanceId, value != 0);
                }
                delete[] t;

                this->setPresetChanged(-1, true);
                return;
            }
            else
            {
                for (int i = 0; i < pPluginInfo->ports().size(); ++i)
                {
                    auto &port = pPluginInfo->ports()[i];
                    if (port->index() == portIndex)
                    {
                        std::string symbol = port->symbol();

                        this->pedalBoard.SetControlValue(instanceId, symbol, value);
                        {

                            // take a snapshot incase a client unsusbscribes in the notification handler (in which case the mutex won't protect us)
                            IPiPedalModelSubscriber **t = new IPiPedalModelSubscriber *[this->subscribers.size()];
                            for (size_t i = 0; i < subscribers.size(); ++i)
                            {
                                t[i] = this->subscribers[i];
                            }
                            size_t n = this->subscribers.size();
                            for (size_t i = 0; i < n; ++i)
                            {
                                t[i]->OnMidiValueChanged(instanceId, symbol, value);
                            }
                            delete[] t;

                            this->setPresetChanged(-1, true);
                            return;
                        }
                    }
                }
            }
        }
    }
}
void PiPedalModel::OnNotifyVusSubscription(const std::vector<VuUpdate> &updates)
{
    std::lock_guard guard(mutex);
    for (size_t i = 0; i < updates.size(); ++i)
    {
        // take a snapshot incase a client unsusbscribes in the notification handler (in which case the mutex won't protect us)
        IPiPedalModelSubscriber **t = new IPiPedalModelSubscriber *[this->subscribers.size()];
        for (size_t i = 0; i < subscribers.size(); ++i)
        {
            t[i] = this->subscribers[i];
        }
        size_t n = this->subscribers.size();
        for (size_t i = 0; i < n; ++i)
        {
            t[i]->OnVuMeterUpdate(updates);
        }
        delete[] t;
    }
}

void PiPedalModel::updateRealtimeVuSubscriptions()
{
    std::set<int64_t> addedInstances;

    for (int i = 0; i < activeVuSubscriptions.size(); ++i)
    {
        auto instanceId = activeVuSubscriptions[i].instanceid;
        if (pedalBoard.HasItem(instanceId))
        {
            addedInstances.insert(activeVuSubscriptions[i].instanceid);
        }
    }
    std::vector<int64_t> instanceids(addedInstances.begin(), addedInstances.end());
    jackHost->SetVuSubscriptions(instanceids);
}

void PiPedalModel::updateRealtimeMonitorPortSubscriptions()
{
    jackHost->SetMonitorPortSubscriptions(this->activeMonitorPortSubscriptions);
}

int64_t PiPedalModel::monitorPort(int64_t instanceId, const std::string &key, float updateInterval, PortMonitorCallback onUpdate)
{
    std::lock_guard guard(mutex);
    int64_t subscriptionId = ++nextSubscriptionId;
    activeMonitorPortSubscriptions.push_back(
        MonitorPortSubscription{subscriptionId, instanceId, key, updateInterval, onUpdate});

    updateRealtimeMonitorPortSubscriptions();

    return subscriptionId;
}
void PiPedalModel::unmonitorPort(int64_t subscriptionHandle)
{
    std::lock_guard guard(mutex);

    for (auto i = activeMonitorPortSubscriptions.begin(); i != activeMonitorPortSubscriptions.end(); ++i)
    {
        if ((*i).subscriptionHandle == subscriptionHandle)
        {
            activeMonitorPortSubscriptions.erase(i);
            updateRealtimeMonitorPortSubscriptions();
            break;
        }
    }
}
void PiPedalModel::OnNotifyMonitorPort(const MonitorPortUpdate &update)
{
    std::lock_guard guard(mutex);

    for (auto i = activeMonitorPortSubscriptions.begin(); i != activeMonitorPortSubscriptions.end(); ++i)
    {
        if ((*i).subscriptionHandle == update.subscriptionHandle)
        {

            // make the call ONLY if the subscription handle is still valid.
            (*update.callbackPtr)(update.subscriptionHandle, update.value);
            break;
        }
    }
}

void PiPedalModel::getLv2Parameter(
    int64_t clientId,
    int64_t instanceId,
    const std::string uri,
    std::function<void(const std::string &jsonResjult)> onSuccess,
    std::function<void(const std::string &error)> onError)
{
    std::function<void(RealtimeParameterRequest *)> onRequestComplete{
        [this](RealtimeParameterRequest *pParameter)
        {
            {
                std::lock_guard guard(this->mutex);
                bool cancelled = true;
                for (auto i = this->outstandingParameterRequests.begin();
                     i != this->outstandingParameterRequests.end(); ++i)
                {
                    if ((*i) == pParameter)
                    {
                        cancelled = false;
                        this->outstandingParameterRequests.erase(i);
                        break;
                    }
                }
                if (!cancelled)
                {
                    if (pParameter->errorMessage != nullptr)
                    {
                        pParameter->onError(pParameter->errorMessage);
                    }
                    else if (pParameter->responseLength == 0)
                    {
                        pParameter->onError("No response.");
                    }
                    else
                    {
                        pParameter->onSuccess(pParameter->jsonResponse);
                    }
                }
                delete pParameter;
            }
        }};

    LV2_URID urid = this->lv2Host.GetLv2Urid(uri.c_str());
    RealtimeParameterRequest *request = new RealtimeParameterRequest(
        onRequestComplete,
        clientId, instanceId, urid, onSuccess, onError);

    std::lock_guard guard(constMutex(mutex));
    outstandingParameterRequests.push_back(request);
    this->jackHost->getRealtimeParameter(request);
}

BankIndex PiPedalModel::getBankIndex() const
{
    std::lock_guard guard(constMutex(mutex));
    return storage.GetBanks();
}

void PiPedalModel::renameBank(int64_t clientId, int64_t bankId, const std::string &newName)
{
    std::lock_guard guard(mutex);
    storage.RenameBank(bankId, newName);
    fireBanksChanged(clientId);
}

int64_t PiPedalModel::saveBankAs(int64_t clientId, int64_t bankId, const std::string &newName)
{
    std::lock_guard guard(mutex);
    int64_t newId = storage.SaveBankAs(bankId, newName);
    fireBanksChanged(clientId);
    return newId;
}

void PiPedalModel::openBank(int64_t clientId, int64_t bankId)
{
    std::lock_guard guard(mutex);

    storage.LoadBank(bankId);
    fireBanksChanged(clientId);

    firePresetsChanged(clientId);

    this->pedalBoard = storage.GetCurrentPreset();
    updateDefaults(&this->pedalBoard);
    this->hasPresetChanged = false;
    this->firePedalBoardChanged(clientId);
}

JackServerSettings PiPedalModel::GetJackServerSettings()
{
    std::lock_guard guard(mutex);
    return this->jackServerSettings;
}

void PiPedalModel::SetJackServerSettings(const JackServerSettings &jackServerSettings)
{
    std::lock_guard guard(mutex);

    if (!ShutdownClient::CanUseShutdownClient())
    {
        throw PiPedalException("Can't change server settings when running a debug server.");
    }

    this->jackServerSettings = jackServerSettings;


    // take a snapshot incase a client unsusbscribes in the notification handler (in which case the mutex won't protect us)
    IPiPedalModelSubscriber **t = new IPiPedalModelSubscriber *[this->subscribers.size()];
    for (size_t i = 0; i < subscribers.size(); ++i)
    {
        t[i] = this->subscribers[i];
    }
    size_t n = this->subscribers.size();
    for (size_t i = 0; i < n; ++i)
    {
        t[i]->OnJackServerSettingsChanged(jackServerSettings);
    }
    delete[] t;

    if (ShutdownClient::CanUseShutdownClient())
    {

        // save the current (edited) preset now in case the service shutdown isn't clean.
        CurrentPreset currentPreset;
        currentPreset.modified_ = this->hasPresetChanged;
        currentPreset.preset_ = this->pedalBoard;
        storage.SaveCurrentPreset(currentPreset);
    
        this->jackConfiguration.SetIsRestarting(true);
        fireJackConfigurationChanged(this->jackConfiguration);
        this->jackHost->UpdateServerConfiguration(
            jackServerSettings,
            [this](bool success, const std::string &errorMessage)
            {
                std::lock_guard guard(mutex);
                if (!success)
                {
                    std::stringstream s;
                    s << "UpdateServerconfiguration failed: " << errorMessage;
                    Lv2Log::error(s.str().c_str());
                }
                // Update jack server status.
                if (!success)
                {
                    this->jackConfiguration.SetIsRestarting(false);
                    this->jackConfiguration.SetErrorStatus(errorMessage);
                    fireJackConfigurationChanged(this->jackConfiguration);
                }
                else
                {
                    // we now do a complete restart of the services,
                    // so just sit tight and wait for the restart.
#ifdef JUNK
                    this->jackConfiguration.SetErrorStatus("");
                    fireJackConfigurationChanged(this->jackConfiguration);

                    // restart the pedalboard on a new instance.
                    std::shared_ptr<Lv2PedalBoard> lv2PedalBoard{this->lv2Host.CreateLv2PedalBoard(this->pedalBoard)};
                    this->lv2PedalBoard = lv2PedalBoard;

                    jackHost->SetPedalBoard(lv2PedalBoard);
                    updateRealtimeVuSubscriptions();
                    updateRealtimeMonitorPortSubscriptions();
#endif
                }
            });
    } 
}

void PiPedalModel::updateDefaults(PedalBoardItem *pedalBoardItem)
{
    std::shared_ptr<Lv2PluginInfo> t = lv2Host.GetPluginInfo(pedalBoardItem->uri());
    const Lv2PluginInfo *pPlugin = t.get();
    if (pPlugin == nullptr)
    {
        if (pedalBoardItem->uri() == SPLIT_PEDALBOARD_ITEM_URI)
        {
            pPlugin = GetSplitterPluginInfo();
        }
    }
    if (pPlugin != nullptr)
    {
        for (size_t i = 0; i < pPlugin->ports().size(); ++i)
        {
            auto port = pPlugin->ports()[i];
            if (port->is_control_port() && port->is_input())
            {
                ControlValue *pValue = pedalBoardItem->GetControlValue(port->symbol());
                if (pValue == nullptr)
                {
                    // Missing? Set it to default value.
                    pedalBoardItem->controlValues().push_back(
                        pipedal::ControlValue(port->symbol().c_str(), port->default_value()));
                }
            }
        }
    }
    for (size_t i = 0; i < pedalBoardItem->topChain().size(); ++i)
    {
        updateDefaults(&(pedalBoardItem->topChain()[i]));
    }
    for (size_t i = 0; i < pedalBoardItem->bottomChain().size(); ++i)
    {
        updateDefaults(&(pedalBoardItem->bottomChain()[i]));
    }
}
void PiPedalModel::updateDefaults(PedalBoard *pedalBoard)
{
    for (size_t i = 0; i < pedalBoard->items().size(); ++i)
    {
        updateDefaults(&(pedalBoard->items()[i]));
    }
}

std::vector<Lv2PluginPreset> PiPedalModel::GetPluginPresets(std::string pluginUri)
{
    std::lock_guard guard(mutex);

    return lv2Host.GetPluginPresets(pluginUri);
}

void PiPedalModel::LoadPluginPreset(int64_t instanceId, const std::string &presetUri)
{
    std::lock_guard guard(mutex);

    PedalBoardItem *pedalBoardItem = this->pedalBoard.GetItem(instanceId);
    if (pedalBoardItem != nullptr)
    {
        std::vector<ControlValue> controlValues = lv2Host.LoadPluginPreset(pedalBoardItem, presetUri);
        jackHost->SetPluginPreset(instanceId, controlValues);

        for (size_t i = 0; i < controlValues.size(); ++i)
        {
            const ControlValue &value = controlValues[i];
            this->pedalBoard.SetControlValue(instanceId, value.key(), value.value());
        }

        IPiPedalModelSubscriber **t = new IPiPedalModelSubscriber *[this->subscribers.size()];
        for (size_t i = 0; i < subscribers.size(); ++i)
        {
            t[i] = this->subscribers[i];
        }
        size_t n = this->subscribers.size();
        for (size_t i = 0; i < n; ++i)
        {
            t[i]->OnLoadPluginPreset(instanceId, controlValues);
        }
        delete[] t;
    }
}

void PiPedalModel::deleteMidiListeners(int64_t clientId)
{
    std::lock_guard guard(mutex);
    for (size_t i = 0; i < midiEventListeners.size(); ++i)
    {
        if (midiEventListeners[i].clientId == clientId)
        {
            midiEventListeners.erase(midiEventListeners.begin() + i);
            --i;
        }
    }
    jackHost->SetListenForMidiEvent(midiEventListeners.size() != 0);
}

void PiPedalModel::OnNotifyMidiListen(bool isNote, uint8_t noteOrControl)
{
    std::lock_guard guard(mutex);

    for (int i = 0; i < midiEventListeners.size(); ++i)
    {
        auto &listener = midiEventListeners[i];
        if ((!isNote) || (!listener.listenForControlsOnly))
        {
            auto subscriber = this->GetNotificationSubscriber(listener.clientId);
            if (subscriber)
            {
                subscriber->OnNotifyMidiListener(listener.clientHandle, isNote, noteOrControl);
            }
            midiEventListeners.erase(midiEventListeners.begin() + i);
            --i;
        }
    }
    jackHost->SetListenForMidiEvent(midiEventListeners.size() != 0);
}

void PiPedalModel::listenForMidiEvent(int64_t clientId, int64_t clientHandle, bool listenForControlsOnly)
{
    std::lock_guard guard(mutex);
    MidiListener listener{clientId, clientHandle, listenForControlsOnly};
    midiEventListeners.push_back(listener);
    jackHost->SetListenForMidiEvent(true);
}
void PiPedalModel::cancelListenForMidiEvent(int64_t clientId, int64_t clientHandled)
{
    std::lock_guard guard(mutex);
    for (size_t i = 0; i < midiEventListeners.size(); ++i)
    {
        midiEventListeners.erase(midiEventListeners.begin() + i);
    }
    if (midiEventListeners.size() == 0)
    {
        jackHost->SetListenForMidiEvent(false);
    }
}
std::vector<AlsaDeviceInfo> PiPedalModel::GetAlsaDevices()
{
    return this->alsaDevices.GetAlsaDevices();
}