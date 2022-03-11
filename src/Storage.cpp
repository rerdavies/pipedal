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
#include "Storage.hpp"
#include "PiPedalException.hpp"
#include <sstream>
#include "json.hpp"
#include <fstream>
#include "Lv2Log.hpp"
#include <map>

using namespace pipedal;

const char *BANK_EXTENSION = ".bank";
const char *BANKS_FILENAME = "index.banks";

#define WIFI_CONFIG_SETTINGS_FILENAME "wifiConfigSettings.json";
#define GOVERNOR_SETTINGS_FILENAME "governorSettings.json";

Storage::Storage()
{
    SetDataRoot("~/var/PiPedal");
}

inline bool isSafeCharacter(char c)
{
    return (c >= '0' && c <= '9') | (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '-';
}

static int fromhexStrict(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    // lowercase letters won't round-trip properly.
    return -1;
}

std::string Storage::SafeDecodeName(const std::string &name)
{
    std::stringstream s;
    for (int i = 0; i < name.length(); /**/)
    {
        char c = name[i++];
        if (c < ' ')
            throw PiPedalArgumentException("Unsafe file name.");

        if (c == '+')
        {
            s << ' ';
        }
        else if (c == '%')
        {
            if (i + 2 > name.length())
            {
                throw PiPedalArgumentException("Unsafe file name.");
            }
            int v1 = fromhexStrict(name[i]);
            int v2 = fromhexStrict(name[i + 1]);
            i += 2;
            if (v1 == -1 || v2 == -1)
                throw PiPedalArgumentException("Unsafe file name.");
            char c = (char)((v1 << 4) + v2);
            if (isSafeCharacter(c))
            {
                // name won't round-trip because the escape will not be re-encoded.
                throw PiPedalArgumentException("Unsafe file name.");
            }
            s << c;
        }
        else
        {
            if (isSafeCharacter(c))
            {
                s << c;
            }
            else
            {
                throw PiPedalArgumentException("Unsafe file name.");
            }
        }
    }
    return s.str();
}

static const char *hex = "0123456789ABCDEF";

std::string Storage::SafeEncodeName(const std::string &name)
{
    std::stringstream s;
    for (char c : name)
    {
        if (c < ' ')
            c = ' ';
        bool safe = isSafeCharacter(c);
        if (safe)
        {
            s << c;
        }
        else if (c == ' ')
        {
            s << '+';
        }
        else
        {
            s << '%' << hex[(c >> 4) & 0x0F] << hex[(c)&0x0F];
        }
    }
    return s.str();
}

std::filesystem::path ResolveHomePath(std::filesystem::path path)
{
    if (path.begin() == path.end())
        return path;
    // is the first element "~"?
    auto it = path.begin();
    auto el = (*it);
    if (el != "~")
        return path;
    const char *homeDirectory = nullptr;

    homeDirectory = getenv("HOME");
    if (homeDirectory == nullptr)
    {
        homeDirectory = getenv("USERPROFILE");
    }
    std::filesystem::path result = homeDirectory;

    bool first = true;
    for (auto i = path.begin(); i != path.end(); ++i)
    {
        if (!first)
        {
            result /= *i;
        }
        first = false;
    }
    return result;
}
void Storage::SetDataRoot(const char *path)
{
    this->dataRoot = ResolveHomePath(path);
}

void Storage::Initialize()
{
    try
    {
        std::filesystem::create_directories(this->GetPresetsDirectory());
        std::filesystem::create_directories(this->GetPluginPresetsDirectory());
    }
    catch (const std::exception &e)
    {
        throw PiPedalStateException("Can't create presets directory. (" + (std::string)this->GetPresetsDirectory() + ") (" + e.what() + ")");
    }
    LoadPluginPresetIndex();
    LoadBankIndex();
    LoadCurrentBank();
    try
    {
        LoadChannelSelection();
    }
    catch (const std::exception &)
    {
    }
    LoadWifiConfigSettings();
    LoadGovernorSettings();
}


void Storage::LoadBank(int64_t instanceId)
{
    auto indexEntry = this->bankIndex.getBankIndexEntry(instanceId);

    LoadBankFile(indexEntry.name(), &(this->currentBank));
    if (this->bankIndex.selectedBank() != instanceId)
    {
        this->bankIndex.selectedBank(instanceId);
        SaveBankIndex();
    }
    this->LoadPreset(this->currentBank.selectedPreset());
}

void Storage::LoadCurrentBank()
{
    LoadBank(bankIndex.selectedBank());
}

std::filesystem::path Storage::GetPresetsDirectory() const
{
    return this->dataRoot / "presets";
}
std::filesystem::path Storage::GetPluginPresetsDirectory() const
{
    return this->dataRoot / "plugin_presets";
}
std::filesystem::path Storage::GetCurrentPresetPath() const
{
    return this->dataRoot / "currentPreset.json";
}


std::filesystem::path Storage::GetChannelSelectionFileName()
{
    return this->dataRoot / "JackChannelSelection.json";
}
std::filesystem::path Storage::GetIndexFileName() const
{
    return this->GetPresetsDirectory() / BANKS_FILENAME;
}
std::filesystem::path Storage::GetBankFileName(const std::string &name) const
{
    std::string fileName = SafeEncodeName(name) + BANK_EXTENSION;
    return this->GetPresetsDirectory() / fileName;
}

void Storage::LoadBankIndex()
{
    try
    {
        auto path = GetIndexFileName();
        std::ifstream s;
        s.open(path);
        json_reader reader(s);
        reader.read(&bankIndex);
    }
    catch (const std::exception &)
    {
        bankIndex.clear();
        ReIndex();

        if (bankIndex.entries().size() == 0)
        {
            currentBank.clear();
            PedalBoard defaultPedalBoard = PedalBoard::MakeDefault();
            int64_t instanceId = currentBank.addPreset(defaultPedalBoard);
            currentBank.selectedPreset(instanceId);

            std::string name = "Default Bank";
            SaveBankFile(name, currentBank);
            int64_t selectedBank = bankIndex.addBank(-1,name);
            bankIndex.selectedBank(selectedBank);
        }
        SaveBankIndex();
    }
}

void Storage::LoadPluginPresetIndex()
{
    try
    {
        auto path = GetPluginPresetsDirectory() / "index.json";
        std::ifstream s;
        if (std::filesystem::exists(path))
        {
            s.open(path);
            json_reader reader(s);
            reader.read(&pluginPresetIndex);
        }
    }
    catch (const std::exception &)
    {
    }
}

void Storage::SavePluginPresetIndex()
{
    if (pluginPresetIndexChanged)
    {
        pluginPresetIndexChanged = false;
        std::ofstream os;
        auto path = GetPluginPresetsDirectory() / "index.json";
        os.open(path, std::ios_base::trunc);
        if (os.fail())
        {
            throw PiPedalException(SS("Can't write to " << path));
        }

        json_writer writer(os);
        writer.write(this->pluginPresetIndex);
    }
}

void Storage::SaveBankIndex()
{
    std::ofstream os;
    os.open(GetIndexFileName(), std::ios_base::trunc);
    json_writer writer(os);
    writer.write(this->bankIndex);
}

void Storage::ReIndex()
{
    for (const auto &dirEntry : std::filesystem::directory_iterator(GetPresetsDirectory()))
    {
        if (!dirEntry.is_directory())
        {
            auto path = dirEntry.path();
            if (path.extension() == BANK_EXTENSION)
            {
                std::string name = SafeDecodeName(path.stem());
                bankIndex.addBank(-1,name);
            }
        }
    }
    if (bankIndex.entries().size() == 0)
    {
        CreateBank("Default Bank");
    }
    bankIndex.selectedBank(bankIndex.entries()[0].instanceId());
}

void Storage::CreateBank(const std::string &name)
{
    BankFile bankFile;
    PedalBoard defaultPreset = PedalBoard::MakeDefault();
    defaultPreset.name(std::string("Default Preset"));

    bankFile.addPreset(defaultPreset);

    int64_t instanceId = bankFile.presets()[0]->instanceId();
    bankFile.selectedPreset(instanceId);
    SaveBankFile(name, bankFile);
    this->bankIndex.addBank(-1,name);
    this->SaveBankIndex();
}

void Storage::GetBankFile(int64_t instanceId,BankFile *pBank) const {
    auto indexEntry = this->bankIndex.getBankIndexEntry(instanceId);
    auto name = indexEntry.name();
    std::filesystem::path fileName = GetBankFileName(name);
    std::ifstream is(fileName);
    json_reader reader(is);
    reader.read(pBank);
    pBank->name(indexEntry.name());
}

void Storage::LoadBankFile(const std::string &name, BankFile *pBank)
{
    std::filesystem::path fileName = GetBankFileName(name);
    std::ifstream is(fileName);
    json_reader reader(is);
    reader.read(pBank);
    
}

void Storage::SaveBankFile(const std::string &name, const BankFile &bankFile)
{
    std::filesystem::path fileName = GetBankFileName(name);
    std::filesystem::path backupFile = ((std::string)fileName) + ".$$$";
    if (std::filesystem::exists(backupFile))
    {
        std::filesystem::remove(backupFile);
    }
    if (std::filesystem::exists(fileName))
    {
        std::filesystem::rename(fileName, backupFile);
    }
    try
    {
        std::ofstream s;
        s.open(fileName, std::ios_base::trunc);
        json_writer writer(s);
        writer.write(bankFile);
        if (std::filesystem::exists(backupFile))
        {
            std::filesystem::remove(backupFile);
        }
    }
    catch (const std::exception &e)
    {
        std::filesystem::remove(fileName);
        if (std::filesystem::exists(backupFile))
        {
            std::filesystem::rename(backupFile, fileName);
        }
        throw;
    }
}

void Storage::SaveCurrentBank()
{
    auto indexEntry = this->bankIndex.getBankIndexEntry(this->bankIndex.selectedBank());
    SaveBankFile(indexEntry.name(), this->currentBank);
}

const PedalBoard &Storage::GetCurrentPreset()
{
    auto &item = currentBank.getItem(currentBank.selectedPreset());
    return item.preset();
}

bool Storage::LoadPreset(int64_t instanceId)
{
    if (!currentBank.hasItem(instanceId))
        return false; 
    if (instanceId != currentBank.selectedPreset()) {
        currentBank.selectedPreset(instanceId);
        SaveCurrentBank();
    }
    return true;
}
void Storage::SaveCurrentPreset(const PedalBoard &pedalBoard)
{
    auto &item = currentBank.getItem(currentBank.selectedPreset());
    item.preset(pedalBoard);
    SaveCurrentBank();
}
int64_t Storage::SaveCurrentPresetAs(const PedalBoard &pedalBoard, const std::string &name, int64_t saveAfterInstanceId)
{
    PedalBoard newPedalBoard = pedalBoard;
    newPedalBoard.name(name);

    int64_t newInstanceId = currentBank.addPreset(newPedalBoard, saveAfterInstanceId);
    currentBank.selectedPreset(newInstanceId);
    SaveCurrentBank();
    return newInstanceId;
}

void Storage::SetPresetIndex(const PresetIndex &presets)
{
    // painful because we must move unique_ptrs.
    std::map<int64_t, std::unique_ptr<BankFileEntry> *> entries;
    for (int i = 0; i < this->currentBank.presets().size(); ++i)
    {
        auto & preset = this->currentBank.presets()[i];
        entries[preset->instanceId()] = &(this->currentBank.presets()[i]);
    }
    std::vector<std::unique_ptr<BankFileEntry>*> newPresets;
    for (size_t i = 0; i < presets.presets().size(); ++i)
    {
        std::unique_ptr<BankFileEntry>* p = entries[presets.presets()[i].instanceId()];
        if (p == nullptr)
        {
            throw PiPedalStateException("Presets do not match the currently loaded bank.");
        }
        newPresets.push_back(p);
    }
    // we made it this far. So now we know we can rebuild a new BankFile.
    BankFile bankFile;
    bankFile.presets().resize(newPresets.size());
    for (int i = 0; i < presets.presets().size(); ++i)
    {
        std::unique_ptr<BankFileEntry> *oldBankFilePreset = newPresets[i];

        bankFile.presets()[i] = std::move((*oldBankFilePreset));
    }
    bankFile.nextInstanceId(currentBank.nextInstanceId());
    bankFile.selectedPreset(currentBank.selectedPreset());

    this->currentBank = std::move(bankFile); // deleting any stray presets while we're at it.
    this->SaveCurrentBank();
}
void Storage::GetPresetIndex(PresetIndex *pResult)
{
    *pResult = PresetIndex();
    pResult->selectedInstanceId(this->currentBank.selectedPreset());
    for (auto &item : this->currentBank.presets())
    {
        PresetIndexEntry entry;
        entry.instanceId(item->instanceId());
        entry.name(item->preset().name());
        pResult->presets().push_back(entry);
    }
}

PedalBoard Storage::GetPreset(int64_t instanceId) const
{
    for (size_t i = 0; i < currentBank.presets().size(); ++i)
    {
        if (currentBank.presets()[i]->instanceId() == instanceId)
        {
            return currentBank.presets()[i]->preset();
        }
    }
    throw PiPedalException("Not found.");
}

int64_t Storage::DeletePreset(int64_t presetId)
{
    int64_t newSelection =  currentBank.deletePreset(presetId);
    SaveCurrentBank();
    return newSelection;
}

bool Storage::RenamePreset(int64_t presetId, const std::string &name)
{
    if (this->currentBank.renamePreset(presetId, name))
    {
        SaveCurrentBank();
        return true;
    }
    return false;
}

static int lastIndexOf(const char *sz, char c)
{
    size_t len = strlen(sz);
    for (size_t i = len - 1; i >= 0; --i)
    {
        if (sz[i] == c)
        {
            if (i > 0 && sz[i - 1] == ' ')
                --i;
            return i;
        }
    }
    return -1;
}
std::string Storage::GetPresetCopyName(const std::string &name)
{
    const char *str = name.c_str();
    std::string stem;
    if (name.length() != 0 && name[name.length() - 1] == ')')
    {
        const char *str = name.c_str();
        size_t pos = lastIndexOf(str, '(');
        if (pos == -1)
        {
            stem = name;
        }
        else
        {
            stem = name.substr(0, pos);
        }
    }
    else
    {
        stem = name;
    }
    int copyNumber = 1;
    while (true)
    {
        std::string candidate;
        if (copyNumber == 1)
        {
            candidate = stem + " (Copy)";
        }
        else
        {
            std::stringstream s;
            s << stem << " (Copy " << copyNumber << ")";
            candidate = s.str();
        }
        if (!this->currentBank.hasName(candidate))
        {
            return candidate;
        }
        ++copyNumber;
    }
}
int64_t Storage::CopyPreset(int64_t fromId, int64_t toId)
{
    auto &fromItem = this->currentBank.getItem(fromId);
    if (toId == -1)
    {
        PedalBoard newPedalBoard = fromItem.preset();
        std::string name = GetPresetCopyName(fromItem.preset().name());
        newPedalBoard.name(name);
        return this->currentBank.addPreset(newPedalBoard, fromId);
    }
    else
    {
        auto &toItem = this->currentBank.getItem(toId);
        toItem.preset(fromItem.preset());
        return toId;
    }
}

void Storage::SetJackChannelSelection(const JackChannelSelection &channelSelection)
{
    this->jackChannelSelection = channelSelection;
    this->isJackChannelSelectionValid = true;
    SaveChannelSelection();
}
const JackChannelSelection &Storage::GetJackChannelSelection(const JackConfiguration &jackConfiguration)
{
    if (!this->isJackChannelSelectionValid)
    {
        if (jackConfiguration.isValid())
        {
            auto defaultSelection = JackChannelSelection::MakeDefault(jackConfiguration);
            SetJackChannelSelection(defaultSelection);
            return this->jackChannelSelection;
        }
    }
    return jackChannelSelection;
}

void Storage::LoadChannelSelection()
{
    auto fileName = this->GetChannelSelectionFileName();
    if (std::filesystem::exists(fileName))
    {
        try
        {
            std::ifstream s(fileName);
            json_reader reader(s);
            reader.read(&this->jackChannelSelection);
            this->isJackChannelSelectionValid = true;
        }
        catch (const std::exception &e)
        {
            Lv2Log::error("I/O error reading %s: %s", fileName.c_str(), e.what());
            throw PiPedalStateException("Unexpected error reading Jack settings file.");
        }
    }
}
void Storage::SaveChannelSelection()
{
    auto fileName = this->GetChannelSelectionFileName();
    try
    {
        std::ofstream s(fileName);
        json_writer writer(s);
        writer.write(this->jackChannelSelection);
    }
    catch (const std::exception &e)
    {
        Lv2Log::error("I/O error writing %s: %s", fileName.c_str(), e.what());
        throw PiPedalStateException("Unexpected error writing Jack settings file.");
    }
}

void Storage::RenameBank(int64_t bankId, const std::string &newName)
{
    auto existingBank = this->bankIndex.getEntryByName(newName);
    if (existingBank != nullptr)
    {
        throw PiPedalStateException("A bank by that name already exists.");
    }
    auto &entry = this->bankIndex.getBankIndexEntry(bankId);
    std::filesystem::path oldPath = this->GetBankFileName(entry.name());
    std::filesystem::path newPath = this->GetBankFileName(newName);
    try
    {
        std::filesystem::rename(oldPath, newPath);
    }
    catch (std::exception &e)
    {
        std::stringstream s;
        s << "Unable to rename the bank. (" << e.what() << ")";
        throw PiPedalException(s.str());
    }
    entry.name(newName);
    SaveBankIndex();
}

int64_t Storage::SaveBankAs(int64_t bankId, const std::string &newName)
{
    auto existingBank = this->bankIndex.getEntryByName(newName);
    if (existingBank != nullptr)
    {
        throw PiPedalStateException("A bank by that name already exists.");
    }
    auto &entry = this->bankIndex.getBankIndexEntry(bankId);
    std::filesystem::path oldPath = this->GetBankFileName(entry.name());
    std::filesystem::path newPath = this->GetBankFileName(newName);
    try
    {
        std::filesystem::copy(oldPath,newPath);
    }
    catch (std::exception &e)
    {
        std::stringstream s;
        s << "Unable to save the bank. (" << e.what() << ")";
        throw PiPedalException(s.str());
    }
    int64_t newId =  this->bankIndex.addBank(bankId,newName);
    SaveBankIndex();
    return newId;
}

void Storage::MoveBank(int from, int to)
{
    this->bankIndex.move(from,to);
    this->SaveBankIndex();
}
int64_t Storage::DeleteBank(int64_t bankId)
{
    auto & entries = this->bankIndex.entries();

    for (size_t i = 0; i < entries.size(); ++i)
    {
        auto & entry = entries[i];

        if (entry.instanceId() == bankId) {
            std::filesystem::path fileName = this->GetBankFileName(entry.name());
            entries.erase(entries.begin()+i);

            int64_t newSelection;
            if (i < entries.size())
            {
                newSelection = entries[i].instanceId();
            } else if (entries.size() > 0)
            {
                newSelection = entries[entries.size()-1].instanceId();
            } else {
                // zero entries?
                // Create a default empty bank.
                BankIndexEntry newEntry;

                BankFile defaultBank;
                PedalBoard defaultPedalBoard = PedalBoard::MakeDefault();
                int64_t instanceId = defaultBank.addPreset(defaultPedalBoard);
                defaultBank.selectedPreset(instanceId);

                std::string name = "Default Bank";
                SaveBankFile(name, defaultBank);
                int64_t selectedBank = bankIndex.addBank(-1,name);
                newSelection = selectedBank;
            }
            if (this->bankIndex.selectedBank() == bankId)
            {
                this->bankIndex.selectedBank(newSelection);
            }
            this->SaveBankIndex();
            std::filesystem::remove(fileName);
            return newSelection;
        }
    }
    throw PiPedalStateException("Bank not found.");
}

int64_t Storage::GetCurrentPresetId() const
{
    return this->currentBank.selectedPreset();
}

int64_t  Storage::UploadPreset(const BankFile&bankFile,int64_t uploadAfter)
{
    int64_t lastPreset = this->currentBank.selectedPreset();
    if (uploadAfter != -1)
    {
        lastPreset = uploadAfter;
    }

    if (bankFile.presets().size() == 0) {
        throw PiPedalException("Invalid preset.");
    }
    for (size_t i = 0; i < bankFile.presets().size(); ++i)
    {
        PedalBoard preset = bankFile.presets()[i]->preset();

        int n = 2;
        std::string baseName = preset.name();
        while (this->currentBank.hasName(preset.name()))
        {
            std::stringstream s;
            s << baseName << "(" << n++ << ")";
            preset.name(s.str());
        }

        lastPreset = this->currentBank.addPreset(preset,lastPreset);
    }
    this->SaveCurrentBank();
    return lastPreset;
}
int64_t  Storage::UploadBank(BankFile&bankFile,int64_t uploadAfter)
{
    int64_t lastBank = this->bankIndex.selectedBank();
    if (uploadAfter != -1)
    {
        lastBank = uploadAfter;
    }

    if (bankFile.presets().size() == 0) {
        throw PiPedalException("Invalid bank.");
    }
    bankFile.updateNextIndex();

    int n = 2;
    std::string baseName = bankFile.name();
    while (this->bankIndex.hasName(bankFile.name()))
    {
        std::stringstream s;
        s << baseName << "(" << n++ << ")";
        bankFile.name(s.str());
    }
    std::filesystem::path path = this->GetBankFileName(bankFile.name());
    std::ofstream f(path);
    if (!f.is_open()) {
        throw PiPedalException("Can't write to bank file.");
    }
    json_writer writer(f);
    writer.write(bankFile);

    lastBank = this->bankIndex.addBank(lastBank,bankFile.name());
    this->SaveBankIndex();
    return lastBank;
}

void Storage::SetGovernorSettings(const std::string & governor)
{
        std::filesystem::path path = this->dataRoot / GOVERNOR_SETTINGS_FILENAME;
    {
        std::ofstream f(path);
        if (!f.is_open())
        {
            throw PiPedalException("Unable to write to " + ((std::string)path));
        }
        json_writer writer(f);
        writer.write(governor);
        this->governorSettings = governor;
    }
}
std::string Storage::GetGovernorSettings() const
{
    return this->governorSettings;
}


void Storage::SetWifiConfigSettings(const WifiConfigSettings & wifiConfigSettings)
{
    WifiConfigSettings copyToSave = wifiConfigSettings;
    copyToSave.rebootRequired_ = false;
    if (!copyToSave.enable_)
    {
        copyToSave.hasPassword_ = false;
    }
    copyToSave.password_ = "";

    std::filesystem::path path = this->dataRoot / WIFI_CONFIG_SETTINGS_FILENAME;
    {
        std::ofstream f(path);
        if (!f.is_open())
        {
            throw PiPedalException("Unable to write to " + ((std::string)path));
        }
        json_writer writer(f);
        writer.write(&copyToSave);
    }

    WifiConfigSettings copyToStore = wifiConfigSettings;
    if (copyToStore.enable_)
    {
        copyToStore.hasPassword_ = copyToStore.password_.length() != 0 || this->wifiConfigSettings.hasPassword_;
    } else {
        copyToStore.hasPassword_ = false;
    }
    copyToStore.password_ = "";
    copyToStore.rebootRequired_ = false;
    if (copyToStore.enable_ && !copyToStore.hasPassword_) 
    {
        copyToStore.hasPassword_ = this->wifiConfigSettings.hasPassword_;
    }
    this->wifiConfigSettings = copyToStore;
}
void Storage::LoadGovernorSettings()
{
    std::filesystem::path path = this->dataRoot / GOVERNOR_SETTINGS_FILENAME;

    try {
        if (std::filesystem::is_regular_file(path))
        {
            std::ifstream f(path);
            if (!f.is_open())
            {
                throw PiPedalException("Unable to write to " + ((std::string)path));
            }
            json_reader reader(f);
            std::string governorSettings;
            reader.read(&governorSettings);
            this->governorSettings = governorSettings;
        }
    } catch (const std::exception&)
    {
        this->governorSettings = "performance";
    }
    this->wifiConfigSettings.valid_ = true;

}
void Storage::LoadWifiConfigSettings()
{
    std::filesystem::path path = this->dataRoot / WIFI_CONFIG_SETTINGS_FILENAME;

    try {
        if (std::filesystem::is_regular_file(path))
        {
            std::ifstream f(path);
            if (!f.is_open())
            {
                throw PiPedalException("Unable to write to " + ((std::string)path));
            }
            json_reader reader(f);
            WifiConfigSettings wifiConfigSettings;
            reader.read(&wifiConfigSettings);
            this->wifiConfigSettings = wifiConfigSettings;
        }
    } catch (const std::exception&)
    {

    }
    this->wifiConfigSettings.valid_ = true;
}


WifiConfigSettings Storage::GetWifiConfigSettings()
{
    return this->wifiConfigSettings;
}


void Storage::SaveCurrentPreset(const CurrentPreset &currentPreset)
{
    try {
        std::filesystem::path path = GetCurrentPresetPath();

        std::ofstream f(path);
        json_writer writer(f);
        writer.write(currentPreset);
    } catch (std::exception&)
    {
        // called from destructor. Must be nothrow().
    }

}
bool Storage::RestoreCurrentPreset(CurrentPreset*pResult)
{
    std::filesystem::path path = GetCurrentPresetPath();
    if (std::filesystem::exists(path)) {
        try {
            std::ifstream f(path);
            json_reader reader(f);
            reader.read(pResult);
            std::filesystem::remove(path); // one-shot only, restore the state from the last *orderly* shutdown.
        } catch (const std::exception&)
        {
            return false;
        }
        return true;
    } else {
        return false;
    }

}
bool Storage::HasPluginPresets(const std::string &pluginUri) const
{
    for (const auto &entry: this->pluginPresetIndex.entries_)
    {
        if (entry.pluginUri_ == pluginUri)
        {
            return true;
        }
    }
    return false;
}

std::filesystem::path Storage::GetPluginPresetPath(const std::string &pluginUri) const
{
    for (const auto &entry: this->pluginPresetIndex.entries_)
    {
        if (entry.pluginUri_ == pluginUri)
        {
            return this->GetPluginPresetsDirectory() / entry.fileName_;
        }
    }
    throw PiPedalArgumentException("Plugin preset file not found.");

}
void Storage::SavePluginPresets(const std::string&pluginUri, const PluginPresets&presets)
{
    std::string name;
    std::filesystem::path path;
    bool presetAdded = false;
    if (!HasPluginPresets(pluginUri))
    {
        name = SS(pluginPresetIndex.nextInstanceId_ << ".json");
        path = GetPluginPresetsDirectory() / name;
        presetAdded = true;
    } else {
        path = GetPluginPresetPath(pluginUri);
    }
    auto tempPath = path.string()+".$$$";
    {
        std::ofstream os;
        os.open(tempPath, std::ios_base::trunc);
        if (os.fail())
        {
            throw PiPedalException(SS("Can't write to " << path));
        }
        json_writer writer(os);
        writer.write(presets);
    }
    if (std::filesystem::exists(path))
    {
        std::filesystem::remove(path);
    }
    std::filesystem::rename(tempPath,path);

    if (presetAdded)
    {
        pluginPresetIndex.entries_.push_back(
            PluginPresetIndexEntry(
                pluginUri,name
            )
        );
        pluginPresetIndex.nextInstanceId_++;
        this->pluginPresetIndexChanged = true;
        SavePluginPresetIndex();
    }
}

PluginPresets Storage::GetPluginPresets(const std::string&pluginUri) const
{
    PluginPresets result;
    if (!HasPluginPresets(pluginUri))
    {
        result.pluginUri_ = pluginUri;
        return result;
    }
    std::filesystem::path path = GetPluginPresetPath(pluginUri);
    std::ifstream s;
    s.open(path);
    if (s.fail())
    {
        return result;
    }
    json_reader reader(s);
    reader.read(&result);
    return result;
}
PluginUiPresets Storage::GetPluginUiPresets(const std::string&pluginUri) const
{
    PluginPresets presets = GetPluginPresets(pluginUri);
    PluginUiPresets result;
    result.pluginUri_ = presets.pluginUri_;
    for (size_t i = 0; i < presets.presets_.size(); ++i)
    {
        const auto& preset = presets.presets_[i];
        result.presets_.push_back(
            PluginUiPreset 
            {
                preset.label_,
                preset.instanceId_
            }
        );
    }
    return result;
}

std::vector<ControlValue> Storage::GetPluginPresetValues(const std::string&pluginUri, uint64_t instanceId)
{
    auto presets = GetPluginPresets(pluginUri);
    for (const auto & preset: presets.presets_)
    {
        if (preset.instanceId_ == instanceId)
        {
            std::vector<ControlValue> result;
            for (const auto &valuePair: preset.controlValues_)
            {
                result.push_back(ControlValue(valuePair.first.c_str(),valuePair.second));
            }
            return result;
        }
    }
    throw PiPedalException("Plugin preset not found.");
}

uint64_t Storage::SavePluginPreset(
    const std::string&pluginUri, 
    const std::string&name, 
    const std::map<std::string,float> & values)
{
    auto presets = GetPluginPresets(pluginUri);
    uint64_t result = -1;
    bool existing = false;
    for (size_t i = 0; i < presets.presets_.size(); ++i)
    {
        auto & preset = presets.presets_[i];
        if (preset.label_ == name)
        {
            preset.controlValues_ = values;
            existing = true;
            result = preset.instanceId_;
            break;
        }
    }
    if (!existing)
    {
        result = presets.nextInstanceId_++;
        presets.presets_.push_back(
            PluginPreset(
                result,
                name,
                values
            ));
    }
    this->SavePluginPresets(pluginUri,presets);
    return result;
}

void Storage::UpdatePluginPresets(const PluginUiPresets &pluginPresets)
{
    // handles deletions, renaming, and reordering only.
    // If you need to add a preset, you neet to call SavePluginPreset or DulicatePluginPreset instead.
    
    PluginPresets presets = this->GetPluginPresets(pluginPresets.pluginUri_);
    PluginPresets newPresets;
    newPresets.pluginUri_ = pluginPresets.pluginUri_;
    newPresets.nextInstanceId_ = presets.nextInstanceId_;

    for (const auto&preset: pluginPresets.presets_)
    {
        PluginPreset newPreset = presets.GetPreset(preset.instanceId_);
        newPreset.label_ = preset.label_;
        newPresets.presets_.push_back(std::move(newPreset));
    }
    SavePluginPresets(newPresets.pluginUri_,newPresets);


}

static std::string stripCopySuffix(const std::string &s)
{
    int pos = s.length()-1;
    if (pos >= 0 && s[pos] == ')')
    {
        --pos;
        while (pos >= 0 && s[pos] >= '0' && s[pos] <= '9')
        {
            --pos;
        }
        if  (pos >= 0 && s[pos] == '(')
        {
            --pos;
        }
        while (pos >= 0 && s[pos] == ' ')
        {
            --pos;
        }
        return s.substr(0,pos+1);
    }
    return s;
}
uint64_t Storage::CopyPluginPreset(const std::string&pluginUri,uint64_t presetId)
{
    PluginPresets presets = this->GetPluginPresets(pluginUri);
    size_t pos = presets.Find(presetId);
    if (pos == -1)
    {
        throw PiPedalException("Perset not found.");
    }
    PluginPreset t = presets.presets_[pos];
    t.instanceId_ = presets.nextInstanceId_++;
    std::string baseName = stripCopySuffix(t.label_);
    std::string name = baseName;
    if (presets.Find(name) != -1)
    {
        int nCopy = 1;
        while (true)
        {
            name = SS(baseName << " (" << nCopy << ")");
            if (presets.Find(name) == -1)
            {
                break;
            }
            ++nCopy;
        }
    }
    t.label_ = name;
    presets.presets_.insert(presets.presets_.begin()+pos+1,t);
    SavePluginPresets(presets.pluginUri_,presets);
    return t.instanceId_;



}


JSON_MAP_BEGIN(CurrentPreset)
    JSON_MAP_REFERENCE(CurrentPreset,modified)
    JSON_MAP_REFERENCE(CurrentPreset,preset)
JSON_MAP_END()

