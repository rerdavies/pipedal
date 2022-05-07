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

#pragma once
#include <mutex>
#include "Lv2Host.hpp"
#include "GovernorSettings.hpp"
#include "PedalBoard.hpp"
#include "Storage.hpp"
#include "Banks.hpp"
#include "JackConfiguration.hpp"
#include "JackHost.hpp"
#include "VuUpdate.hpp"
#include <functional>
#include <filesystem>
#include "Banks.hpp"
#include "PiPedalConfiguration.hpp"
#include "JackServerSettings.hpp"
#include "WifiConfigSettings.hpp"
#include "WifiDirectConfigSettings.hpp"
#include "AdminClient.hpp"
#include "AvahiService.hpp"
#include <thread>





namespace pipedal {




class IPiPedalModelSubscriber {
public: 
    virtual int64_t GetClientId() = 0;
    virtual void OnItemEnabledChanged(int64_t clientId, int64_t pedalItemId, bool enabled) = 0;
    virtual void OnControlChanged(int64_t clientId, int64_t pedalItemId, const std::string &symbol, float value) = 0;
    virtual void OnPedalBoardChanged(int64_t clientId, const PedalBoard &pedalBoard) = 0;
    virtual void OnPresetsChanged(int64_t clientId, const PresetIndex&presets) = 0;
    virtual void OnPluginPresetsChanged(const std::string&pluginUri) = 0;
    virtual void OnChannelSelectionChanged(int64_t clientId, const JackChannelSelection&channelSelection) = 0;
    virtual void OnVuMeterUpdate(const std::vector<VuUpdate>& updates) = 0;
    virtual void OnBankIndexChanged(const BankIndex& bankIndex) = 0;
    virtual void OnJackServerSettingsChanged(const JackServerSettings& jackServerSettings) = 0;
    virtual void OnJackConfigurationChanged(const JackConfiguration& jackServerConfiguration) = 0;
    virtual void OnLoadPluginPreset(int64_t instanceId,const std::vector<ControlValue>&controlValues) = 0;
    virtual void OnMidiValueChanged(int64_t instanceId, const std::string&symbol, float value) = 0;
    virtual void OnNotifyMidiListener(int64_t clientHandle, bool isNote, uint8_t noteOrControl) = 0;
    virtual void OnNotifyAtomOutput(int64_t clientModel,uint64_t instanceId,const std::string&atomType, const std::string&atomJson) = 0;
    virtual void OnWifiConfigSettingsChanged(const WifiConfigSettings&wifiConfigSettings) = 0;
    virtual void OnWifiDirectConfigSettingsChanged(const WifiDirectConfigSettings&wifiDirectConfigSettings) = 0;
    virtual void OnGovernorSettingsChanged(const std::string &governor) = 0;
    virtual void OnFavoritesChanged(const std::map<std::string,bool> &favorites) = 0;
    virtual void OnShowStatusMonitorChanged(bool show) = 0;
    virtual void Close() = 0;
};

class PiPedalModel: private IJackHostCallbacks {
private:
    std::unique_ptr<std::jthread> pingThread;

    AvahiService avahiService;
    uint16_t webPort;

    PiPedalAlsaDevices alsaDevices;
    std::recursive_mutex mutex;

    AdminClient adminClient;


    class MidiListener {
    public:
        int64_t clientId;
        int64_t clientHandle;
        bool listenForControlsOnly;
    };
    class AtomOutputListener {
    public:
        int64_t clientId;
        int64_t clientHandle;
        uint64_t instanceId;
    };
    void DeleteMidiListeners(int64_t clientId);
    void DeleteAtomOutputListeners(int64_t clientId);

    std::vector<MidiListener> midiEventListeners;
    std::vector<AtomOutputListener> atomOutputListeners;

    JackServerSettings jackServerSettings;
    Lv2Host lv2Host;
    PedalBoard pedalBoard;
    Storage storage;
    bool hasPresetChanged = false;

    std::unique_ptr<JackHost> jackHost;
    JackConfiguration jackConfiguration;
    std::shared_ptr<Lv2PedalBoard> lv2PedalBoard;
    std::filesystem::path webRoot;


    std::vector<IPiPedalModelSubscriber*> subscribers;
    void SetPresetChanged(int64_t clientId,bool value);
    void FirePresetsChanged(int64_t clientId);
    void FirePluginPresetsChanged(const std::string &pluginUri);
    void FirePedalBoardChanged(int64_t clientId);
    void FireChannelSelectionChanged(int64_t clientId);
    void FireBanksChanged(int64_t clientId);
    void FireJackConfigurationChanged(const JackConfiguration&jackConfiguration);

    void UpdateDefaults(PedalBoardItem*pedalBoardItem);
    void UpdateDefaults(PedalBoard*pedalBoard);

    
    class VuSubscription {
    public:
        int64_t subscriptionHandle;
        int64_t instanceid;
    };
    int64_t nextSubscriptionId = 1;
    std::vector<VuSubscription> activeVuSubscriptions;

    std::vector<MonitorPortSubscription> activeMonitorPortSubscriptions;

    void UpdateRealtimeVuSubscriptions();
    void UpdateRealtimeMonitorPortSubscriptions();
    void OnVuUpdate(const std::vector<VuUpdate>& updates);

    std::vector<RealtimeParameterRequest*> outstandingParameterRequests;

    IPiPedalModelSubscriber*GetNotificationSubscriber(int64_t clientId);

private: // IJackHostCallbacks
    virtual void OnNotifyVusSubscription(const std::vector<VuUpdate> & updates);
    virtual void OnNotifyMonitorPort(const MonitorPortUpdate &update);
    virtual void OnNotifyMidiValueChanged(int64_t instanceId, int portIndex, float value); 
    virtual void OnNotifyMidiListen(bool isNote, uint8_t noteOrControl);
    virtual void OnNotifyAtomOutput(uint64_t instanceId, const std::string&atomType,const std::string&atomJson);

public:
    PiPedalModel();
    virtual ~PiPedalModel();

    uint16_t GetWebPort() const { return webPort; }
    void Close();

    void UpdateDnsSd();

    AdminClient&GetAdminClient() { return adminClient; }


    void LoadLv2PluginInfo(const PiPedalConfiguration&configuration);
    void Load(const PiPedalConfiguration&configuration);

    const Lv2Host& GetLv2Host() const { return lv2Host; }
    PedalBoard  GetCurrentPedalBoardCopy() 
    { 
        std::lock_guard<std::recursive_mutex> guard(mutex);
        return pedalBoard; // can return a referece because we'd lose  mutex protection
    }
    PluginUiPresets GetPluginUiPresets(const std::string& pluginUri);
    PluginPresets GetPluginPresets(const std::string&pluginUri);

    void LoadPluginPreset(int64_t pluginInstanceId,uint64_t presetInstanceId);

    void AddNotificationSubscription(IPiPedalModelSubscriber* pSubscriber);
    void RemoveNotificationSubsription(IPiPedalModelSubscriber* pSubscriber);

    void SetPedalBoardItemEnable(int64_t clientId, int64_t instanceId, bool enabled);
    void SetControl(int64_t clientId,int64_t pedalItemId, const std::string&symbol,float value);
    void PreviewControl(int64_t clientId,int64_t pedalItemId, const std::string&symbol,float value);
    void SetPedalBoard(int64_t clientId,PedalBoard &pedalBoard);

    void GetPresets(PresetIndex*pResult);
    PedalBoard GetPreset(int64_t instanceId);
    void GetBank(int64_t instanceId, BankFile*pBank);

    int64_t UploadBank(BankFile&bankFile, int64_t uploadAfter = -1);
    int64_t UploadPreset(const BankFile&bankFile, int64_t uploadAfter = -1);
    void  UploadPluginPresets(const PluginPresets&pluginPresets);
    void SaveCurrentPreset(int64_t clientId);
    int64_t SaveCurrentPresetAs(int64_t clientId, const std::string& name,int64_t saveAfterInstanceId = -1);
    int64_t SavePluginPresetAs(int64_t instanceId, const std::string&name);

    void LoadPreset(int64_t clientId, int64_t instanceId);
    bool UpdatePresets(int64_t clientId,const  PresetIndex&presets);
    void UpdatePluginPresets(const PluginUiPresets&pluginPresets);
    int64_t DeletePreset(int64_t clientId,int64_t instanceId);
    bool RenamePreset(int64_t clientId,int64_t instanceId,const std::string&name);
    int64_t CopyPreset(int64_t clientId, int64_t fromIndex, int64_t toIndex);
    uint64_t CopyPluginPreset(const std::string&pluginUri,uint64_t presetId);

    void MoveBank(int64_t clientId,int from, int to);
    int64_t DeleteBank(int64_t clientId, int64_t instanceId);


    int64_t DuplicatePreset(int64_t clientId, int64_t instanceId) { return CopyPreset(clientId,instanceId,-1); }

    JackConfiguration GetJackConfiguration();

    void SetJackChannelSelection(int64_t clientId,const JackChannelSelection &channelSelection);
    JackChannelSelection GetJackChannelSelection();

    void SetShowStatusMonitor(bool show);
    bool GetShowStatusMonitor();

    void SetWifiConfigSettings(const WifiConfigSettings&wifiConfigSettings);
    WifiConfigSettings GetWifiConfigSettings();
    
    void SetWifiDirectConfigSettings(const WifiDirectConfigSettings&wifiConfigSettings);
    WifiDirectConfigSettings GetWifiDirectConfigSettings();
    
    void SetGovernorSettings(const std::string& governor);
    GovernorSettings GetGovernorSettings();



    int64_t AddVuSubscription(int64_t instanceId);
    void RemoveVuSubscription(int64_t subscriptionHandle);


    int64_t MonitorPort(int64_t instanceId,const std::string &key,float updateInterval, PortMonitorCallback onUpdate);
    void UnmonitorPort(int64_t subscriptionHandle);

    void GetLv2Parameter(
        int64_t clientId,
        int64_t instanceId,
        const std::string uri,
        std::function<void (const std::string&jsonResjult)> onSuccess,
        std::function<void (const std::string& error)> onError
    );

    BankIndex GetBankIndex() const;
    void RenameBank(int64_t clientId,int64_t bankId, const std::string& newName);
    int64_t SaveBankAs(int64_t clientId,int64_t bankId, const std::string& newName);
    void OpenBank(int64_t clientId,int64_t bankId);

    JackHostStatus GetJackStatus() {
        return this->jackHost->getJackStatus();
    }
    JackServerSettings GetJackServerSettings();
    void SetJackServerSettings(const JackServerSettings& jackServerSettings);

    void ListenForMidiEvent(int64_t clientId, int64_t clientHandle, bool listenForControlsOnly);
    void CancelListenForMidiEvent(int64_t clientId, int64_t clientHandle);

    void ListenForAtomOutputs(int64_t clientId, int64_t clientHandle, uint64_t instanceId);
    void CancelListenForAtomOutputs(int64_t clientId, int64_t clientHandle);

    std::vector<AlsaDeviceInfo> GetAlsaDevices();
    const std::filesystem::path& GetWebRoot() const;

    std::map<std::string,bool> GetFavorites() const;
    void SetFavorites(const std::map<std::string,bool> &favorites);
};

} // namespace pipedal.