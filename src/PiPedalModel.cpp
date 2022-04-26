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
#include "DeviceIdFile.hpp"
#include <sched.h>
#include "PiPedalModel.hpp"
#include "JackHost.hpp"
#include "Lv2Log.hpp"
#include <set>
#include "PiPedalConfiguration.hpp"
#include "AdminClient.hpp"
#include "SplitEffect.hpp"
#include "CpuGovernor.hpp"

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
    std::lock_guard<std::recursive_mutex> guard(mutex);

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
    try {
        adminClient.UnmonitorGovernor();
    } catch (...) // noexcept here!
    {

    }

    try {
        CurrentPreset currentPreset;
        currentPreset.modified_ = this->hasPresetChanged;
        currentPreset.preset_ = this->pedalBoard;
        storage.SaveCurrentPreset(currentPreset);
    } catch (...)
    {
        
    }
    
    try {
        if (jackHost)
        {
            jackHost->Close();
        }
    } catch (...)
    {

    }
}

#include <fstream>


void PiPedalModel::LoadLv2PluginInfo(const PiPedalConfiguration&configuration)
{

    storage.SetConfigRoot(configuration.GetDocRoot());
    storage.SetDataRoot(configuration.GetLocalStoragePath());
    storage.Initialize();

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

    // Copy all presets out of Lilv data to json files
    // so that we can close lilv while we're actually
    // running.
    for (const auto&plugin: lv2Host.GetPlugins())
    {
        if (plugin->has_factory_presets())
        {
            if (!storage.HasPluginPresets(plugin->uri()))
            {
                PluginPresets pluginPresets = lv2Host.GetFactoryPluginPresets(plugin->uri());
                storage.SavePluginPresets(plugin->uri(),pluginPresets);
            }
        }
    }
}

void PiPedalModel::Load(const PiPedalConfiguration &configuration)
{
    this->webRoot = configuration.GetWebRoot();
    this->webPort = (uint16_t)configuration.GetSocketServerPort();
    this->jackServerSettings.ReadJackConfiguration();


    adminClient.MonitorGovernor(storage.GetGovernorSettings());

    // lv2Host.Load(configuration.GetLv2Path().c_str());

    this->pedalBoard = storage.GetCurrentPreset(); // the current *saved* preset.

    // the current edited preset, saved only across orderly shutdowns.
    CurrentPreset currentPreset;
    if (storage.RestoreCurrentPreset(&currentPreset))
    {
        this->pedalBoard = currentPreset.preset_;
        this->hasPresetChanged = currentPreset.modified_;
    }
    UpdateDefaults(&this->pedalBoard);

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

    struct sched_param scheduler_params;
    scheduler_params.sched_priority = 10;
    memset(&scheduler_params,0,sizeof(sched_param));
    sched_setscheduler(0,SCHED_RR,&scheduler_params);

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
    std::lock_guard<std::recursive_mutex> guard(mutex);
    this->subscribers.push_back(pSubscriber);
}
void PiPedalModel::RemoveNotificationSubsription(IPiPedalModelSubscriber *pSubscriber)
{
    {
        std::lock_guard<std::recursive_mutex> guard(mutex);

        for (auto it = this->subscribers.begin(); it != this->subscribers.end(); ++it)
        {
            if ((*it) == pSubscriber)
            {
                this->subscribers.erase(it);
                break;
            }
        }
        int64_t clientId = pSubscriber->GetClientId();

        this->DeleteMidiListeners(clientId);
        this->DeleteAtomOutputListeners(clientId);

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

void PiPedalModel::PreviewControl(int64_t clientId, int64_t pedalItemId, const std::string &symbol, float value)
{

    jackHost->SetControlValue(pedalItemId, symbol, value);
}

void PiPedalModel::SetControl(int64_t clientId, int64_t pedalItemId, const std::string &symbol, float value)
{
    {
        std::lock_guard<std::recursive_mutex> guard(mutex);
        PreviewControl(clientId, pedalItemId, symbol, value);
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

            this->SetPresetChanged(clientId, true);
        }
    }
}

void PiPedalModel::FireJackConfigurationChanged(const JackConfiguration &jackConfiguration)
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

void PiPedalModel::FireBanksChanged(int64_t clientId)
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

void PiPedalModel::FirePedalBoardChanged(int64_t clientId)
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
        UpdateRealtimeVuSubscriptions();
        UpdateRealtimeMonitorPortSubscriptions();
    }
}
void PiPedalModel::SetPedalBoard(int64_t clientId, PedalBoard &pedalBoard)
{
    {
        std::lock_guard<std::recursive_mutex> guard(mutex);
        this->pedalBoard = pedalBoard;
        UpdateDefaults(&this->pedalBoard);

        this->FirePedalBoardChanged(clientId);
        this->SetPresetChanged(clientId, true);
    }
}

void PiPedalModel::SetPedalBoardItemEnable(int64_t clientId, int64_t pedalItemId, bool enabled)
{
    std::lock_guard<std::recursive_mutex> guard{mutex};
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
        this->SetPresetChanged(clientId, true);

        // Notify audo thread.
        this->jackHost->SetBypass(pedalItemId, enabled);
    }
}

void PiPedalModel::GetPresets(PresetIndex *pResult)
{
    std::lock_guard<std::recursive_mutex> guard(mutex);

    this->storage.GetPresetIndex(pResult);
    pResult->presetChanged(this->hasPresetChanged);
}

PedalBoard PiPedalModel::GetPreset(int64_t instanceId)
{
    std::lock_guard<std::recursive_mutex> guard(mutex);
    return this->storage.GetPreset(instanceId);
}
void PiPedalModel::GetBank(int64_t instanceId, BankFile *pResult)
{
    std::lock_guard<std::recursive_mutex> guard(mutex);
    this->storage.GetBankFile(instanceId, pResult);
}

void PiPedalModel::SetPresetChanged(int64_t clientId, bool value)
{
    if (value != this->hasPresetChanged)
    {
        hasPresetChanged = value;
        FirePresetsChanged(clientId);
    }
}

void PiPedalModel::FirePresetsChanged(int64_t clientId)
{
    std::lock_guard<std::recursive_mutex> guard{mutex};
    {
        // take a snapshot incase a client unsusbscribes in the notification handler (in which case the mutex won't protect us)
        IPiPedalModelSubscriber **t = new IPiPedalModelSubscriber *[this->subscribers.size()];
        for (size_t i = 0; i < subscribers.size(); ++i)
        {
            t[i] = this->subscribers[i];
        }
        size_t n = this->subscribers.size();

        PresetIndex presets;
        GetPresets(&presets);

        for (size_t i = 0; i < n; ++i)
        {
            t[i]->OnPresetsChanged(clientId, presets);
        }
        delete[] t;
    }
}
void PiPedalModel::FirePluginPresetsChanged(const std::string & pluginUri)
{
    std::lock_guard<std::recursive_mutex> guard{mutex};
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
            t[i]->OnPluginPresetsChanged(pluginUri);
        }
        delete[] t;
    }
}


void PiPedalModel::SaveCurrentPreset(int64_t clientId)
{
    std::lock_guard<std::recursive_mutex> guard{mutex};

    storage.SaveCurrentPreset(this->pedalBoard);
    this->SetPresetChanged(clientId, false);
}

uint64_t PiPedalModel::CopyPluginPreset(const std::string&pluginUri,uint64_t presetId)
{
    uint64_t result = storage.CopyPluginPreset(pluginUri,presetId);
    FirePluginPresetsChanged(pluginUri);
    return result;    
}

void PiPedalModel::UpdatePluginPresets(const PluginUiPresets&pluginPresets)
{
    storage.UpdatePluginPresets(pluginPresets);
    FirePluginPresetsChanged(pluginPresets.pluginUri_);
}
int64_t PiPedalModel::SavePluginPresetAs(int64_t instanceId, const std::string&name)
{
    auto *item = this->pedalBoard.GetItem(instanceId);
    if (!item)
    {
        throw PiPedalException("Plugin not found.");
    }
    std::map<std::string,float> values;
    for (const auto & controlValue: item->controlValues())
    {
        values[controlValue.key()] = controlValue.value();
    }
    uint64_t presetId = storage.SavePluginPreset(item->uri(),name,values);
    FirePluginPresetsChanged(item->uri());
    return presetId;
}

int64_t PiPedalModel::SaveCurrentPresetAs(int64_t clientId, const std::string &name, int64_t saveAfterInstanceId)
{
    std::lock_guard<std::recursive_mutex> guard{mutex};

    auto pedalboard = this->pedalBoard;
    pedalboard.name(name);
    int64_t result = storage.SaveCurrentPresetAs(pedalboard, name, saveAfterInstanceId);
    FirePresetsChanged(clientId);
    return result;
}

void  PiPedalModel::UploadPluginPresets(const PluginPresets&pluginPresets)
{
    if (pluginPresets.pluginUri_.length() == 0)
    {
        throw PiPedalException("Invalid plugin presets.");
    }
    std::lock_guard<std::recursive_mutex> guard{mutex};
    storage.SavePluginPresets(pluginPresets.pluginUri_,pluginPresets);
    FirePluginPresetsChanged(pluginPresets.pluginUri_);

}
int64_t PiPedalModel::UploadPreset(const BankFile &bankFile, int64_t uploadAfter)
{
    std::lock_guard<std::recursive_mutex> guard{mutex};

    int64_t newPreset = this->storage.UploadPreset(bankFile, uploadAfter);
    FirePresetsChanged(-1);
    return newPreset;
}
int64_t PiPedalModel::UploadBank(BankFile &bankFile, int64_t uploadAfter)
{
    std::lock_guard<std::recursive_mutex> guard{mutex};

    int64_t newPreset = this->storage.UploadBank(bankFile, uploadAfter);
    FireBanksChanged(-1);
    return newPreset;
}

void PiPedalModel::LoadPreset(int64_t clientId, int64_t instanceId)
{
    std::lock_guard<std::recursive_mutex> guard{mutex};

    if (storage.LoadPreset(instanceId))
    {
        this->pedalBoard = storage.GetCurrentPreset();
        UpdateDefaults(&this->pedalBoard);

        this->hasPresetChanged = false; // no fire.
        this->FirePedalBoardChanged(clientId);
        this->FirePresetsChanged(clientId); // fire now.
    }
}

int64_t PiPedalModel::CopyPreset(int64_t clientId, int64_t from, int64_t to)
{
    std::lock_guard<std::recursive_mutex> guard{mutex};

    int64_t result = storage.CopyPreset(from, to);
    if (result != -1)
    {
        this->FirePresetsChanged(clientId); // fire now.
    }
    else
    {
        throw PiPedalStateException("Copy failed.");
    }
    return result;
}
bool PiPedalModel::UpdatePresets(int64_t clientId, const PresetIndex &presets)
{
    std::lock_guard<std::recursive_mutex> guard{mutex};
    storage.SetPresetIndex(presets);
    FirePresetsChanged(clientId);
    return true;
}

void PiPedalModel::MoveBank(int64_t clientId, int from, int to)
{
    std::lock_guard<std::recursive_mutex> guard{mutex};
    storage.MoveBank(from, to);
    FireBanksChanged(clientId);
}
int64_t PiPedalModel::DeleteBank(int64_t clientId, int64_t instanceId)
{
    std::lock_guard<std::recursive_mutex> guard{mutex};
    int64_t selectedBank = this->storage.GetBanks().selectedBank();
    int64_t newSelection = storage.DeleteBank(instanceId);

    int64_t newSelectedBank = this->storage.GetBanks().selectedBank();

    this->FireBanksChanged(clientId); // fire now.

    if (newSelectedBank != selectedBank)
    {
        this->OpenBank(clientId, newSelectedBank);
    }
    return newSelection;
}

int64_t PiPedalModel::DeletePreset(int64_t clientId, int64_t instanceId)
{
    std::lock_guard<std::recursive_mutex> guard{mutex};
    int64_t oldSelection = storage.GetCurrentPresetId();
    int64_t newSelection = storage.DeletePreset(instanceId);
    this->FirePresetsChanged(clientId); // fire now.
    if (oldSelection != newSelection)
    {
        this->LoadPreset(
            -1, // can't use cached version.
            newSelection);
    }
    return newSelection;    
}
bool PiPedalModel::RenamePreset(int64_t clientId, int64_t instanceId, const std::string &name)
{
    std::lock_guard<std::recursive_mutex> guard(mutex);
    if (storage.RenamePreset(instanceId, name))
    {
        this->FirePresetsChanged(clientId);
        return true;
    }
    else
    {
        throw PiPedalStateException("Rename failed.");
    }
}

GovernorSettings PiPedalModel::GetGovernorSettings()
{
    {
        std::lock_guard<std::recursive_mutex> guard(mutex);
        GovernorSettings result;
        result.governor_ = storage.GetGovernorSettings();
        result.governors_ = pipedal::GetAvailableGovernors();
        return result;
    }
}
void PiPedalModel::SetGovernorSettings(const std::string& governor)
{
    std::lock_guard<std::recursive_mutex> guard(mutex);
    adminClient.SetGovernorSettings(governor);

    this->storage.SetGovernorSettings(governor);
    
    IPiPedalModelSubscriber **t = new IPiPedalModelSubscriber *[this->subscribers.size()];
    {
        for (size_t i = 0; i < subscribers.size(); ++i)
        {
            t[i] = this->subscribers[i];
        }
        size_t n = this->subscribers.size();
        for (size_t i = 0; i < n; ++i)
        {
            t[i]->OnGovernorSettingsChanged(governor);
        }
        delete[] t;
    }

}
    
void PiPedalModel::SetWifiConfigSettings(const WifiConfigSettings &wifiConfigSettings)
{
    std::lock_guard<std::recursive_mutex> guard(mutex);

    adminClient.SetWifiConfig(wifiConfigSettings);

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

void PiPedalModel::UpdateDnsSd()
{
    avahiService.Unannounce();

    DeviceIdFile deviceIdFile;
    deviceIdFile.Load();
    avahiService.Announce(webPort,deviceIdFile.deviceName,deviceIdFile.uuid,"pipedal");

}
void PiPedalModel::SetWifiDirectConfigSettings(const WifiDirectConfigSettings &wifiDirectConfigSettings)
{
    std::lock_guard<std::recursive_mutex> guard(mutex);

    adminClient.SetWifiDirectConfig(wifiDirectConfigSettings);

    this->storage.SetWifiDirectConfigSettings(wifiDirectConfigSettings);

    {
        IPiPedalModelSubscriber **t = new IPiPedalModelSubscriber *[this->subscribers.size()];
        for (size_t i = 0; i < subscribers.size(); ++i)
        {
            t[i] = this->subscribers[i];
        }
        size_t n = this->subscribers.size();

        WifiDirectConfigSettings tWifiDirectConfigSettings = storage.GetWifiDirectConfigSettings(); // (the passwordless version)

        for (size_t i = 0; i < n; ++i)
        {
            t[i]->OnWifiDirectConfigSettingsChanged(tWifiDirectConfigSettings);
        }
        delete[] t;

        // update NSD-SD announement.
        UpdateDnsSd();

            

    }

}

WifiConfigSettings PiPedalModel::GetWifiConfigSettings()
{
    std::lock_guard<std::recursive_mutex> guard(mutex);
    return this->storage.GetWifiConfigSettings();
}
WifiDirectConfigSettings PiPedalModel::GetWifiDirectConfigSettings()
{
    std::lock_guard<std::recursive_mutex> guard(mutex);
    return this->storage.GetWifiDirectConfigSettings();
}

JackConfiguration PiPedalModel::GetJackConfiguration()
{
    std::lock_guard<std::recursive_mutex> guard(mutex); // copy atomically.
    return this->jackConfiguration;
}

void PiPedalModel::SetJackChannelSelection(int64_t clientId, const JackChannelSelection &channelSelection)
{
    std::lock_guard<std::recursive_mutex> guard(mutex); // copy atomically.
    this->storage.SetJackChannelSelection(channelSelection);
    this->FireChannelSelectionChanged(clientId);

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
        this->UpdateRealtimeVuSubscriptions();
    }
}

void PiPedalModel::FireChannelSelectionChanged(int64_t clientId)
{
    std::lock_guard<std::recursive_mutex> guard{mutex};
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

JackChannelSelection PiPedalModel::GetJackChannelSelection()
{
    std::lock_guard<std::recursive_mutex> guard(mutex);
    JackChannelSelection t = this->storage.GetJackChannelSelection(this->jackConfiguration);
    if (this->jackConfiguration.isValid())
    {
        t = t.RemoveInvalidChannels(this->jackConfiguration);
    }
    return t;
}

int64_t PiPedalModel::AddVuSubscription(int64_t instanceId)
{
    std::lock_guard<std::recursive_mutex> guard(mutex);
    int64_t subscriptionId = ++nextSubscriptionId;
    activeVuSubscriptions.push_back(VuSubscription{subscriptionId, instanceId});

    UpdateRealtimeVuSubscriptions();

    return subscriptionId;
}
void PiPedalModel::RemoveVuSubscription(int64_t subscriptionHandle)
{
    std::lock_guard<std::recursive_mutex> guard(mutex);
    for (auto i = activeVuSubscriptions.begin(); i != activeVuSubscriptions.end(); ++i)
    {
        if ((*i).subscriptionHandle == subscriptionHandle)
        {
            activeVuSubscriptions.erase(i);
            break;
        }
    }
    UpdateRealtimeVuSubscriptions();
}

void PiPedalModel::OnNotifyMidiValueChanged(int64_t instanceId, int portIndex, float value)
{
    std::lock_guard<std::recursive_mutex> guard(mutex);
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

                this->SetPresetChanged(-1, true);
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

                            this->SetPresetChanged(-1, true);
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
    std::lock_guard<std::recursive_mutex> guard(mutex);
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

void PiPedalModel::UpdateRealtimeVuSubscriptions()
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

void PiPedalModel::UpdateRealtimeMonitorPortSubscriptions()
{
    jackHost->SetMonitorPortSubscriptions(this->activeMonitorPortSubscriptions);
}

int64_t PiPedalModel::MonitorPort(int64_t instanceId, const std::string &key, float updateInterval, PortMonitorCallback onUpdate)
{
    std::lock_guard<std::recursive_mutex> guard(mutex);
    int64_t subscriptionId = ++nextSubscriptionId;
    activeMonitorPortSubscriptions.push_back(
        MonitorPortSubscription{subscriptionId, instanceId, key, updateInterval, onUpdate});

    UpdateRealtimeMonitorPortSubscriptions();

    return subscriptionId;
}
void PiPedalModel::UnmonitorPort(int64_t subscriptionHandle)
{
    std::lock_guard<std::recursive_mutex> guard(mutex);

    for (auto i = activeMonitorPortSubscriptions.begin(); i != activeMonitorPortSubscriptions.end(); ++i)
    {
        if ((*i).subscriptionHandle == subscriptionHandle)
        {
            activeMonitorPortSubscriptions.erase(i);
            UpdateRealtimeMonitorPortSubscriptions();
            break;
        }
    }
}


void PiPedalModel::OnNotifyMonitorPort(const MonitorPortUpdate &update)
{
    std::lock_guard<std::recursive_mutex> guard(mutex);

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

void PiPedalModel::GetLv2Parameter(
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
                std::lock_guard<std::recursive_mutex> guard(mutex);
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

    std::lock_guard<std::recursive_mutex> guard(mutex);
    outstandingParameterRequests.push_back(request);
    this->jackHost->getRealtimeParameter(request);
}

BankIndex PiPedalModel::GetBankIndex() const
{
    std::lock_guard<std::recursive_mutex> guard(const_cast<std::recursive_mutex&>(mutex));
    return storage.GetBanks();
}

void PiPedalModel::RenameBank(int64_t clientId, int64_t bankId, const std::string &newName)
{
    std::lock_guard<std::recursive_mutex> guard(mutex);
    storage.RenameBank(bankId, newName);
    FireBanksChanged(clientId);
}

int64_t PiPedalModel::SaveBankAs(int64_t clientId, int64_t bankId, const std::string &newName)
{
    std::lock_guard<std::recursive_mutex> guard(mutex);
    int64_t newId = storage.SaveBankAs(bankId, newName);
    FireBanksChanged(clientId);
    return newId;
}

void PiPedalModel::OpenBank(int64_t clientId, int64_t bankId)
{
    std::lock_guard<std::recursive_mutex> guard(mutex);

    storage.LoadBank(bankId);
    FireBanksChanged(clientId);

    FirePresetsChanged(clientId);

    this->pedalBoard = storage.GetCurrentPreset();
    UpdateDefaults(&this->pedalBoard);
    this->hasPresetChanged = false;
    this->FirePedalBoardChanged(clientId);
}

JackServerSettings PiPedalModel::GetJackServerSettings()
{
    std::lock_guard<std::recursive_mutex> guard(mutex);
    return this->jackServerSettings;
}

void PiPedalModel::SetJackServerSettings(const JackServerSettings &jackServerSettings)
{
    std::lock_guard<std::recursive_mutex> guard(mutex);

    if (!adminClient.CanUseShutdownClient())
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

    if (adminClient.CanUseShutdownClient())
    {

        // save the current (edited) preset now in case the service shutdown isn't clean.
        CurrentPreset currentPreset;
        currentPreset.modified_ = this->hasPresetChanged;
        currentPreset.preset_ = this->pedalBoard;
        storage.SaveCurrentPreset(currentPreset);
    
        this->jackConfiguration.SetIsRestarting(true);
        FireJackConfigurationChanged(this->jackConfiguration);
        this->jackHost->UpdateServerConfiguration(
            jackServerSettings,
            [this](bool success, const std::string &errorMessage)
            {
                std::lock_guard<std::recursive_mutex> guard(mutex);
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
                    FireJackConfigurationChanged(this->jackConfiguration);
                }
                else
                {
                    // we now do a complete restart of the services,
                    // so just sit tight and wait for the restart.
#ifdef JUNK
                    this->jackConfiguration.SetErrorStatus("");
                    FireJackConfigurationChanged(this->jackConfiguration);

                    // restart the pedalboard on a new instance.
                    std::shared_ptr<Lv2PedalBoard> lv2PedalBoard{this->lv2Host.CreateLv2PedalBoard(this->pedalBoard)};
                    this->lv2PedalBoard = lv2PedalBoard;

                    jackHost->SetPedalBoard(lv2PedalBoard);
                    UpdateRealtimeVuSubscriptions();
                    UpdateRealtimeMonitorPortSubscriptions();
#endif
                }
            });
    } 
}

void PiPedalModel::UpdateDefaults(PedalBoardItem *pedalBoardItem)
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
        UpdateDefaults(&(pedalBoardItem->topChain()[i]));
    }
    for (size_t i = 0; i < pedalBoardItem->bottomChain().size(); ++i)
    {
        UpdateDefaults(&(pedalBoardItem->bottomChain()[i]));
    }
}
void PiPedalModel::UpdateDefaults(PedalBoard *pedalBoard)
{
    for (size_t i = 0; i < pedalBoard->items().size(); ++i)
    {
        UpdateDefaults(&(pedalBoard->items()[i]));
    }
}

PluginPresets PiPedalModel::GetPluginPresets(const std::string& pluginUri)
{
    std::lock_guard<std::recursive_mutex> guard(mutex);

    return storage.GetPluginPresets(pluginUri);

}

PluginUiPresets PiPedalModel::GetPluginUiPresets(const std::string& pluginUri) 
{
    std::lock_guard<std::recursive_mutex> guard(mutex);

    return storage.GetPluginUiPresets(pluginUri);
}

void PiPedalModel::LoadPluginPreset(int64_t pluginInstanceId, uint64_t presetInstanceId)
{
    std::lock_guard<std::recursive_mutex> guard(mutex);

    PedalBoardItem *pedalBoardItem = this->pedalBoard.GetItem(pluginInstanceId);
    if (pedalBoardItem != nullptr)
    {
        std::vector<ControlValue> controlValues = storage.GetPluginPresetValues(pedalBoardItem->uri(), presetInstanceId);
        jackHost->SetPluginPreset(pluginInstanceId, controlValues);

        for (size_t i = 0; i < controlValues.size(); ++i)
        {
            const ControlValue &value = controlValues[i];
            this->pedalBoard.SetControlValue(pluginInstanceId, value.key(), value.value());
        }

        IPiPedalModelSubscriber **t = new IPiPedalModelSubscriber *[this->subscribers.size()];
        for (size_t i = 0; i < subscribers.size(); ++i)
        {
            t[i] = this->subscribers[i];
        }
        size_t n = this->subscribers.size();
        for (size_t i = 0; i < n; ++i)
        {
            t[i]->OnLoadPluginPreset(pluginInstanceId, controlValues);
        }
        delete[] t;
    }
}

void PiPedalModel::DeleteAtomOutputListeners(int64_t clientId)
{
    std::lock_guard<std::recursive_mutex> guard(mutex);
    for (size_t i = 0; i < atomOutputListeners.size(); ++i)
    {
        if (atomOutputListeners[i].clientId == clientId)
        {
            atomOutputListeners.erase(atomOutputListeners.begin() + i);
            --i;
        }
    }
    jackHost->SetListenForAtomOutput(atomOutputListeners.size() != 0);
}

void PiPedalModel::DeleteMidiListeners(int64_t clientId)
{
    std::lock_guard<std::recursive_mutex> guard(mutex);
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

void PiPedalModel::OnNotifyAtomOutput(uint64_t instanceId, const std::string&atomType,const std::string&atomJson)
{
    std::lock_guard<std::recursive_mutex> guard(mutex);

    for (int i = 0; i < atomOutputListeners.size(); ++i)
    {
        auto &listener = atomOutputListeners[i];
        if (listener.instanceId == instanceId)
        {
            auto subscriber = this->GetNotificationSubscriber(listener.clientId);
            if (subscriber)
            {
                subscriber->OnNotifyAtomOutput(listener.clientHandle,instanceId,atomType,atomJson);
            } else {
                atomOutputListeners.erase(atomOutputListeners.begin() + i);
                --i;
            }
        }
    }
    jackHost->SetListenForAtomOutput(atomOutputListeners.size() != 0);
    
}



void PiPedalModel::OnNotifyMidiListen(bool isNote, uint8_t noteOrControl)
{
    std::lock_guard<std::recursive_mutex> guard(mutex);

    for (int i = 0; i < midiEventListeners.size(); ++i)
    {
        auto &listener = midiEventListeners[i];
        if ((!isNote) || (!listener.listenForControlsOnly))
        {
            auto subscriber = this->GetNotificationSubscriber(listener.clientId);
            if (subscriber)
            {
                subscriber->OnNotifyMidiListener(listener.clientHandle, isNote, noteOrControl);
            } else {
                midiEventListeners.erase(midiEventListeners.begin() + i);
                --i;
            }
        }
    }
    jackHost->SetListenForMidiEvent(midiEventListeners.size() != 0);
}

void PiPedalModel::ListenForMidiEvent(int64_t clientId, int64_t clientHandle, bool listenForControlsOnly)
{
    std::lock_guard<std::recursive_mutex> guard(mutex);
    MidiListener listener{clientId, clientHandle, listenForControlsOnly};
    midiEventListeners.push_back(listener);
    jackHost->SetListenForMidiEvent(true);
}

void PiPedalModel::ListenForAtomOutputs(int64_t clientId, int64_t clientHandle, uint64_t instanceId)
{
    std::lock_guard<std::recursive_mutex> guard(mutex);
    AtomOutputListener listener{clientId, clientHandle, instanceId};
    atomOutputListeners.push_back(listener);
    jackHost->SetListenForAtomOutput(true);
}

void PiPedalModel::CancelListenForMidiEvent(int64_t clientId, int64_t clientHandle)
{
    std::lock_guard<std::recursive_mutex> guard(mutex);
    for (size_t i = 0; i < midiEventListeners.size(); ++i)
    {
        const auto& listener = midiEventListeners[i];
        if (listener.clientId == clientId && listener.clientHandle == clientHandle)
        {
            midiEventListeners.erase(midiEventListeners.begin() + i);
            break;
        }
    }
    if (midiEventListeners.size() == 0)
    {
        jackHost->SetListenForMidiEvent(false);
    }
}
void PiPedalModel::CancelListenForAtomOutputs(int64_t clientId, int64_t clientHandle)
{
    std::lock_guard<std::recursive_mutex> guard(mutex);
    for (size_t i = 0; i < atomOutputListeners.size(); ++i)
    {
        const auto&listener = atomOutputListeners[i];
        if (listener.clientId == clientId && listener.clientHandle == clientHandle)
        {
            atomOutputListeners.erase(atomOutputListeners.begin() + i);
            break;
        }
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

const std::filesystem::path& PiPedalModel::GetWebRoot() const
{
    return webRoot;
}

std::map<std::string,bool> PiPedalModel::GetFavorites() const
{
    std::lock_guard<std::recursive_mutex> guard(const_cast<std::recursive_mutex&>(mutex));
    
    return storage.GetFavorites();
}
void PiPedalModel::SetFavorites(const std::map<std::string,bool> &favorites)
{
    std::lock_guard<std::recursive_mutex> guard(mutex);
    storage.SetFavorites(favorites);

    // take a snapshot incase a client unsusbscribes in the notification handler (in which case the mutex won't protect us)
    std::vector<IPiPedalModelSubscriber*> t;
    t.reserve(this->subscribers.size());

    for (size_t i = 0; i < subscribers.size(); ++i)
    {
        t.push_back(this->subscribers[i]);
    }
    for (size_t i = 0; i < t.size(); ++i)
    {
        t[i]->OnFavoritesChanged(favorites);
    }
}
