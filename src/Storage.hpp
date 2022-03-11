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
#include "PedalBoard.hpp"
#include "Presets.hpp"
#include "PluginPreset.hpp"
#include "Banks.hpp"
#include "JackConfiguration.hpp"
#include "WifiConfigSettings.hpp"


namespace pipedal {


class CurrentPreset {
public:
    bool modified_ = false;
    PedalBoard preset_;

    DECLARE_JSON_MAP(CurrentPreset);
};

// controls user-defined storage. Implmentation hidden to allow to later migration to a database (perhaps)

class Storage {
private:
    std::filesystem::path dataRoot;
    BankIndex bankIndex;
    BankFile currentBank;
    PluginPresetIndex pluginPresetIndex;
    
private:
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
    std::string governorSettings = "performance";
public:
    Storage();
    void Initialize();
    void CreateBank(const std::string & name);

    void SetDataRoot(const char*path);

    std::vector<std::string> GetPedalBoards();

    const BankIndex & GetBanks() const { return bankIndex; }


    void LoadWifiConfigSettings();
    void LoadGovernorSettings();
    void LoadBank(int64_t instanceId);
    const PedalBoard& GetCurrentPreset();
    void SaveCurrentPreset(const PedalBoard&pedalBoard);
    int64_t SaveCurrentPresetAs(const PedalBoard&pedalBoard, const std::string&namne,int64_t saveAfterInstanceId = -1);
    int64_t GetCurrentPresetId() const;
    void GetPresetIndex(PresetIndex*pResult);
    void SetPresetIndex(const PresetIndex &presetIndex);
    PedalBoard GetPreset(int64_t instanceId) const;
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


    void SetJackChannelSelection(const JackChannelSelection&channelSelection);
    const JackChannelSelection&GetJackChannelSelection(const JackConfiguration &jackConfiguration);

    void SetWifiConfigSettings(const WifiConfigSettings & wifiConfigSettings);
    WifiConfigSettings GetWifiConfigSettings();

    void SetGovernorSettings(const std::string & governor);
    std::string GetGovernorSettings() const;


    void SaveCurrentPreset(const CurrentPreset &currentPreset);
    bool RestoreCurrentPreset(CurrentPreset*pResult);

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

};


} // namespace pipedal