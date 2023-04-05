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
#include <filesystem>
#include <iostream>
#include "Pedalboard.hpp"
#include "Presets.hpp"
#include "PluginPreset.hpp"
#include "Banks.hpp"
#include "JackConfiguration.hpp"
#include "JackServerSettings.hpp"
#include "WifiConfigSettings.hpp"
#include "WifiDirectConfigSettings.hpp"
#include <map>


namespace pipedal {

class UiFileProperty;
class Lv2PluginInfo;

class CurrentPreset {
public:
    bool modified_ = false;
    Pedalboard preset_;

    DECLARE_JSON_MAP(CurrentPreset);
};


class UserSettings {
public:
    std::string governor_ = "performance";
    bool showStatusMonitor_ = true;
    DECLARE_JSON_MAP(UserSettings);
};


// controls user-defined storage. Implmentation hidden to allow to later migration to a database (perhaps)

class Storage {
private:
    std::filesystem::path dataRoot;
    std::filesystem::path configRoot;
    BankIndex bankIndex;
    BankFile currentBank;
    PluginPresetIndex pluginPresetIndex;
    
private:

    void MaybeCopyDefaultPresets();
    static std::string SafeEncodeName(const std::string& name);
    static std::string SafeDecodeName(const std::string& name);
    std::filesystem::path GetPresetsDirectory() const;
    std::filesystem::path GetPluginPresetsDirectory() const;
    std::filesystem::path GetIndexFileName() const;
    std::filesystem::path GetBankFileName(const std::string & name) const;
    std::filesystem::path GetChannelSelectionFileName();
    std::filesystem::path GetCurrentPresetPath() const;

    void LoadBankIndex();
    void SaveBankIndex();
    void ReIndex();
    void LoadCurrentBank();
    void SaveCurrentBank();

    void LoadChannelSelection();
    void SaveChannelSelection();
    void SaveBankFile(const std::string& name,const BankFile&bankFile);
    void LoadBankFile(const std::string &name,BankFile *pBank);
    std::string GetPresetCopyName(const std::string &name);
    bool isJackChannelSelectionValid = false;
    JackChannelSelection jackChannelSelection;
    WifiConfigSettings wifiConfigSettings;
    WifiDirectConfigSettings wifiDirectConfigSettings;

    UserSettings userSettings;
public:
    Storage();
    void Initialize();
    void CreateBank(const std::string & name);

    void SetDataRoot(const std::filesystem::path& path);
    void SetConfigRoot(const std::filesystem::path& path);

    std::filesystem::path GetPluginStorageDirectory() const;

    std::vector<std::string> GetPedalboards();

    const BankIndex & GetBanks() const { return bankIndex; }

    JackServerSettings GetJackServerSettings();
    void SetJackServerSettings(const pipedal::JackServerSettings&jackServerSettings);

    void LoadWifiConfigSettings();
    void LoadWifiDirectConfigSettings();
    void LoadUserSettings();
    void SaveUserSettings();
    void LoadBank(int64_t instanceId);
    int64_t GetBankByMidiBankNumber(uint8_t bankNumber);
    const Pedalboard& GetCurrentPreset();
    void SaveCurrentPreset(const Pedalboard&pedalboard);
    int64_t SaveCurrentPresetAs(const Pedalboard&pedalboard, const std::string&namne,int64_t saveAfterInstanceId = -1);
    int64_t GetCurrentPresetId() const;
    void GetPresetIndex(PresetIndex*pResult);
    void SetPresetIndex(const PresetIndex &presetIndex);
    Pedalboard GetPreset(int64_t instanceId) const;
    int64_t GetPresetByProgramNumber(uint8_t program) const;
    void GetBankFile(int64_t instanceId,BankFile*pResult) const;
    int64_t UploadPreset(const BankFile&bankFile, int64_t uploadAfter);
    int64_t UploadBank(BankFile&bankFile, int64_t uploadAfter);

    
    bool LoadPreset(int64_t presetId);
    int64_t DeletePreset(int64_t presetId);
    bool RenamePreset(int64_t presetId, const std::string&name);
    int64_t CopyPreset(int64_t fromId, int64_t toId = -1);

    void RenameBank(int64_t bankId, const std::string&newName);
    int64_t SaveBankAs(int64_t bankId, const std::string&newName);

    void MoveBank(int from, int to);
    int64_t DeleteBank(int64_t bankId);

    std::vector<std::string> GetFileList(const UiFileProperty&fileProperty);


    void SetJackChannelSelection(const JackChannelSelection&channelSelection);
    const JackChannelSelection&GetJackChannelSelection(const JackConfiguration &jackConfiguration);

    void SetWifiConfigSettings(const WifiConfigSettings & wifiConfigSettings);
    WifiConfigSettings GetWifiConfigSettings();

    void SetWifiDirectConfigSettings(const WifiDirectConfigSettings & wifiDirectConfigSettings);
    WifiDirectConfigSettings GetWifiDirectConfigSettings();


    void SetGovernorSettings(const std::string & governor);
    std::string GetGovernorSettings() const;


    void SaveCurrentPreset(const CurrentPreset &currentPreset);
    bool RestoreCurrentPreset(CurrentPreset*pResult);

    //std::string MapPropertyFileName(Lv2PluginInfo*pluginInfo, const std::string&path);

private:
    bool pluginPresetIndexChanged = false;
    void LoadPluginPresetIndex();
    void SavePluginPresetIndex();
    std::filesystem::path GetPluginPresetPath(const std::string &pluginUri) const;
public:
    bool HasPluginPresets(const std::string&pluginUri) const;
    void SavePluginPresets(const std::string&pluginUri, const PluginPresets&presets);
    PluginPresets GetPluginPresets(const std::string&pluginUri) const;
    PluginUiPresets GetPluginUiPresets(const std::string&pluginUri) const;
    std::vector<ControlValue> GetPluginPresetValues(const std::string&pluginUri, uint64_t instanceId);
    uint64_t SavePluginPreset(const std::string&pluginUri, const std::string&name, const std::map<std::string,float> & values);
    void UpdatePluginPresets(const PluginUiPresets &pluginPresets);
    uint64_t CopyPluginPreset(const std::string&pluginUri,uint64_t presetId);

    std::map<std::string,bool> GetFavorites() const;
    void SetFavorites(const std::map<std::string,bool>&favorites);

    void SetShowStatusMonitor(bool show);
    bool GetShowStatusMonitor() const;
    void SetSystemMidiBindings(const std::vector<MidiBinding>&bindings);
    std::vector<MidiBinding> GetSystemMidiBindings();

};


} // namespace pipedal