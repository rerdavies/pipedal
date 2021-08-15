#pragma once
#include <filesystem>
#include <iostream>
#include "PedalBoard.hpp"
#include "Presets.hpp"
#include "Banks.hpp"
#include "JackConfiguration.hpp"


namespace pipedal {


// controls user-defined storage. Implmentation hidden to allow to later migration to a database (perhaps)

class Storage {
private:
    std::filesystem::path dataRoot;
    BankIndex bankIndex;
    BankFile currentBank;

private:
    static std::string SafeEncodeName(const std::string& name);
    static std::string SafeDecodeName(const std::string& name);
    std::filesystem::path GetPresetsDirectory() const;
    std::filesystem::path GetIndexFileName();
    std::filesystem::path GetBankFileName(const std::string & name);
    std::filesystem::path GetChannelSelectionFileName();

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
public:
    Storage();
    void Initialize();
    void CreateBank(const std::string & name);

    void SetDataRoot(const char*path);

    std::vector<std::string> GetPedalBoards();

    const BankIndex & GetBanks() const { return bankIndex; }


    void LoadBank(int64_t instanceId);
    const PedalBoard& GetCurrentPreset();
    void saveCurrentPreset(const PedalBoard&pedalBoard);
    int64_t saveCurrentPresetAs(const PedalBoard&pedalBoard, const std::string&namne,int64_t saveAfterInstanceId = -1);

    void GetPresetIndex(PresetIndex*pResult);
    void SetPresetIndex(const PresetIndex &presetIndex);
    PedalBoard GetPreset(int64_t instanceId) const;
    int64_t UploadPreset(const BankFile&bankFile, int64_t uploadAfter);


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


};


} // namespace pipedal