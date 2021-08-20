#pragma once
#include <mutex>
#include "Lv2Host.hpp"
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


namespace pipedal {




class IPiPedalModelSubscriber {
public: 
    virtual int64_t GetClientId() = 0;
    virtual void OnItemEnabledChanged(int64_t clientId, int64_t pedalItemId, bool enabled) = 0;
    virtual void OnControlChanged(int64_t clientId, int64_t pedalItemId, const std::string &symbol, float value) = 0;
    virtual void OnPedalBoardChanged(int64_t clientId, const PedalBoard &pedalBoard) = 0;
    virtual void OnPresetsChanged(int64_t clientId, const PresetIndex&presets) = 0;
    virtual void OnChannelSelectionChanged(int64_t clientId, const JackChannelSelection&channelSelection) = 0;
    virtual void OnVuMeterUpdate(const std::vector<VuUpdate>& updates) = 0;
    virtual void OnBankIndexChanged(const BankIndex& bankIndex) = 0;
    virtual void OnJackServerSettingsChanged(const JackServerSettings& jackServerSettings) = 0;
    virtual void OnJackConfigurationChanged(const JackConfiguration& jackServerConfiguration) = 0;
    virtual void OnLoadPluginPreset(int64_t instanceId,const std::vector<ControlValue>&controlValues) = 0;
    virtual void OnMidiValueChanged(int64_t instanceId, const std::string&symbol, float value) = 0;
    virtual void OnNotifyMidiListener(int64_t clientHandle, bool isNote, uint8_t noteOrControl) = 0;
    virtual void OnWifiConfigSettingsChanged(const WifiConfigSettings&wifiConfigSettings) = 0;
    virtual void Close() = 0;
};

class PiPedalModel: private IJackHostCallbacks {
private:

    class MidiListener {
    public:
        int64_t clientId;
        int64_t clientHandle;
        bool listenForControlsOnly;
    };
    void deleteMidiListeners(int64_t clientId);

    std::vector<MidiListener> midiEventListeners;

    JackServerSettings jackServerSettings;
    Lv2Host lv2Host;
    std::recursive_mutex mutex;
    PedalBoard pedalBoard;
    Storage storage;
    bool hasPresetChanged = false;

    std::unique_ptr<JackHost> jackHost;
    JackConfiguration jackConfiguration;
    std::shared_ptr<Lv2PedalBoard> lv2PedalBoard;


    std::vector<IPiPedalModelSubscriber*> subscribers;
    void setPresetChanged(int64_t clientId,bool value);
    void firePresetsChanged(int64_t clientId);
    void firePedalBoardChanged(int64_t clientId);
    void fireChannelSelectionChanged(int64_t clientId);
    void fireBanksChanged(int64_t clientId);
    void fireJackConfigurationChanged(const JackConfiguration&jackConfiguration);

    void updateDefaults(PedalBoardItem*pedalBoardItem);
    void updateDefaults(PedalBoard*pedalBoard);

    
    class VuSubscription {
    public:
        int64_t subscriptionHandle;
        int64_t instanceid;
    };
    int64_t nextSubscriptionId = 1;
    std::vector<VuSubscription> activeVuSubscriptions;

    std::vector<MonitorPortSubscription> activeMonitorPortSubscriptions;

    void updateRealtimeVuSubscriptions();
    void updateRealtimeMonitorPortSubscriptions();
    void OnVuUpdate(const std::vector<VuUpdate>& updates);

    std::vector<RealtimeParameterRequest*> outstandingParameterRequests;

    IPiPedalModelSubscriber*GetNotificationSubscriber(int64_t clientId);

private: // IJackHostCallbacks
    virtual void OnNotifyVusSubscription(const std::vector<VuUpdate> & updates);
    virtual void OnNotifyMonitorPort(const MonitorPortUpdate &update);
    virtual void OnNotifyMidiValueChanged(int64_t instanceId, int portIndex, float value); 
    virtual void OnNotifyMidiListen(bool isNote, uint8_t noteOrControl);


public:
    PiPedalModel();
    virtual ~PiPedalModel();

    void Close();
    void Load(const PiPedalConfiguration&configuration);

    const Lv2Host& getPlugins() const { return lv2Host; }
    PedalBoard  getCurrentPedalBoardCopy() { 
        std::lock_guard guard(mutex);
        return pedalBoard; 
    }
    std::vector<Lv2PluginPreset> GetPluginPresets(std::string pluginUri);
    void LoadPluginPreset(int64_t instanceId,const std::string &presetUri);

    void AddNotificationSubscription(IPiPedalModelSubscriber* pSubscriber);
    void RemoveNotificationSubsription(IPiPedalModelSubscriber* pSubscriber);

    void setPedalBoardItemEnable(int64_t clientId, int64_t instanceId, bool enabled);
    void setControl(int64_t clientId,int64_t pedalItemId, const std::string&symbol,float value);
    void previewControl(int64_t clientId,int64_t pedalItemId, const std::string&symbol,float value);
    void setPedalBoard(int64_t clientId,PedalBoard &pedalBoard);

    void getPresets(PresetIndex*pResult);
    PedalBoard getPreset(int64_t instanceId);
    int64_t uploadPreset(const BankFile&bankFile, int64_t uploadAfter = -1);
    void saveCurrentPreset(int64_t clientId);
    int64_t saveCurrentPresetAs(int64_t clientId, const std::string& name,int64_t saveAfterInstanceId = -1);

    void loadPreset(int64_t clientId, int64_t instanceId);
    bool updatePresets(int64_t clientId,const  PresetIndex&presets);
    int64_t deletePreset(int64_t clientId,int64_t instanceId);
    bool renamePreset(int64_t clientId,int64_t instanceId,const std::string&name);
    int64_t copyPreset(int64_t clientId, int64_t fromIndex, int64_t toIndex);

    void moveBank(int64_t clientId,int from, int to);
    int64_t deleteBank(int64_t clientId, int64_t instanceId);


    int64_t duplicatePreset(int64_t clientId, int64_t instanceId) { return copyPreset(clientId,instanceId,-1); }

    JackConfiguration getJackConfiguration();

    void setJackChannelSelection(int64_t clientId,const JackChannelSelection &channelSelection);
    JackChannelSelection getJackChannelSelection();

    void setWifiConfigSettings(const WifiConfigSettings&wifiConfigSettings);
    WifiConfigSettings getWifiConfigSettings();



    int64_t addVuSubscription(int64_t instanceId);
    void removeVuSubscription(int64_t subscriptionHandle);


    int64_t monitorPort(int64_t instanceId,const std::string &key,float updateInterval, PortMonitorCallback onUpdate);
    void unmonitorPort(int64_t subscriptionHandle);

    void getLv2Parameter(
        int64_t clientId,
        int64_t instanceId,
        const std::string uri,
        std::function<void (const std::string&jsonResjult)> onSuccess,
        std::function<void (const std::string& error)> onError
    );

    BankIndex getBankIndex() const;
    void renameBank(int64_t clientId,int64_t bankId, const std::string& newName);
    int64_t saveBankAs(int64_t clientId,int64_t bankId, const std::string& newName);
    void openBank(int64_t clientId,int64_t bankId);

    JackHostStatus getJackStatus() {
        return this->jackHost->getJackStatus();
    }
    JackServerSettings GetJackServerSettings();
    void SetJackServerSettings(const JackServerSettings& jackServerSettings);

    void listenForMidiEvent(int64_t clientId, int64_t clientHandle, bool listenForControlsOnly);
    void cancelListenForMidiEvent(int64_t clientId, int64_t clientHandled);

};
} // namespace pipedal.