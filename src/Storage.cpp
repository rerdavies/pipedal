// Copyright (c) 2022-2024 Robin Davies
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
#include "Locale.hpp"
#include "Storage.hpp"
#include "AudioConfig.hpp"
#include "PiPedalException.hpp"
#include <stdexcept>
#include <sstream>
#include "json.hpp"
#include <fstream>
#include "Lv2Log.hpp"
#include <map>
#include <sys/stat.h>
#include "PiPedalUI.hpp"
#include "PluginHost.hpp"
#include "ss.hpp"
#include "ofstream_synced.hpp"
#include "ModFileTypes.hpp"
#include <set>
#include <MimeTypes.hpp>
#include "util.hpp"
#include "AudioFiles.hpp"

using namespace pipedal;
namespace fs = std::filesystem;

const char *BANK_EXTENSION = ".bank";
const char *BANKS_FILENAME = "index.banks";

#define USER_SETTINGS_FILENAME "userSettings.json";

static bool hasSyntheticModRoot(const UiFileProperty &fileProperty)
{
    return (fileProperty.modDirectories().size() > 1 || (fileProperty.modDirectories().size() == 1 && fileProperty.useLegacyModDirectory()));
}

Storage::Storage()
{
    SetConfigRoot("~/var/Config");
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
            s << '%' << hex[(c >> 4) & 0x0F] << hex[(c) & 0x0F];
        }
    }
    return s.str();
}

std::filesystem::path ResolveHomePath(const std::filesystem::path &path)
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
    if (homeDirectory == nullptr)
    {
        return path;
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

void Storage::SetConfigRoot(const std::filesystem::path &path)
{
    this->configRoot = ResolveHomePath(path);
}
void Storage::SetDataRoot(const std::filesystem::path &path)
{
    this->dataRoot = ResolveHomePath(path);
}

const std::filesystem::path &Storage::GetConfigRoot()
{
    return this->configRoot;
}
const std::filesystem::path &Storage::GetDataRoot()
{
    return this->dataRoot;
}

static void CopyDirectory(const std::filesystem::path &source, const std::filesystem::path &destination)
{
    for (auto &directoryEntry : std::filesystem::directory_iterator(source))
    {
        if (directoryEntry.is_regular_file())
        {
            std::filesystem::path sourceFile = directoryEntry.path();
            std::filesystem::path destFile = destination / sourceFile.filename();

            std::filesystem::copy_file(sourceFile, destFile);
            std::ignore = chmod(destFile.c_str(), S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
        }
    }
}
void Storage::MaybeCopyDefaultPresets()
{
    auto presetsDirectory = this->GetPresetsDirectory();

    if (!std::filesystem::exists(presetsDirectory / "index.banks"))
    {
        CopyDirectory(this->configRoot / "default_presets" / "presets",
                      presetsDirectory);
    }

    // Obsolete: TooB effects now have correct preset declarations.
    // auto pluginDirectory = this->GetPluginPresetsDirectory();
    // if (!std::filesystem::exists(pluginDirectory / "index.json"))
    // {
    //     CopyDirectory(this->configRoot / "default_presets" / "plugin_presets",
    //                   pluginDirectory);
    // }
}
void Storage::Initialize()
{
    try
    {
        std::filesystem::create_directories(this->GetPresetsDirectory());
        std::filesystem::create_directories(this->GetPluginPresetsDirectory());

        MaybeCopyDefaultPresets();
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
    LoadWifiDirectConfigSettings();
    LoadUserSettings();
}

void Storage::LoadBank(int64_t instanceId)
{
    auto indexEntry = this->bankIndex.getBankIndexEntry(instanceId);

    try
    {
        LoadBankFile(indexEntry.name(), &(this->currentBank));
        if (this->bankIndex.selectedBank() != instanceId)
        {
            this->bankIndex.selectedBank(instanceId);
            SaveBankIndex();
        }
        this->LoadPreset(this->currentBank.selectedPreset());
    }
    catch (const std::exception &e)
    {
        throw std::logic_error(SS("Bank file corrupted. " << e.what() << "(" << GetBankFileName(indexEntry.name()) << ")"));
    }
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
std::filesystem::path Storage::GetPluginUploadDirectory() const
{
    return this->dataRoot / "audio_uploads";
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
            Pedalboard defaultPedalboard = Pedalboard::MakeDefault();
            int64_t instanceId = currentBank.addPreset(defaultPedalboard);
            currentBank.selectedPreset(instanceId);

            std::string name = "Default Bank";
            SaveBankFile(name, currentBank);
            int64_t selectedBank = bankIndex.addBank(-1, name);
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
        pipedal::ofstream_synced os;
        auto path = GetPluginPresetsDirectory() / "index.json";
        os.open(path, std::ios_base::trunc);
        if (os.fail())
        {
            throw PiPedalException(SS("Can't write to " << path));
        }

        json_writer writer(os, true);
        writer.write(this->pluginPresetIndex);
    }
}

void Storage::SaveBankIndex()
{
    pipedal::ofstream_synced os;
    os.open(GetIndexFileName(), std::ios_base::trunc);
    json_writer writer(os, true);
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
                bankIndex.addBank(-1, name);
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
    Pedalboard defaultPreset = Pedalboard::MakeDefault();
    defaultPreset.name(std::string("Default Preset"));

    bankFile.addPreset(defaultPreset);

    int64_t instanceId = bankFile.presets()[0]->instanceId();
    bankFile.selectedPreset(instanceId);
    SaveBankFile(name, bankFile);
    this->bankIndex.addBank(-1, name);
    this->SaveBankIndex();
}

void Storage::GetBankFile(int64_t instanceId, BankFile *pBank) const
{
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
        pipedal::ofstream_synced s;
        s.open(fileName, std::ios_base::trunc);
        json_writer writer(s, true);
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

const Pedalboard &Storage::GetCurrentPreset()
{
    auto &item = currentBank.getItem(currentBank.selectedPreset());
    return item.preset();
}

bool Storage::LoadPreset(int64_t instanceId)
{
    if (!currentBank.hasItem(instanceId))
        return false;
    if (instanceId != currentBank.selectedPreset())
    {
        currentBank.selectedPreset(instanceId);
        SaveCurrentBank();
    }
    return true;
}
void Storage::SaveCurrentPreset(const Pedalboard &pedalboard)
{
    auto &item = currentBank.getItem(currentBank.selectedPreset());
    item.preset(pedalboard);
    SaveCurrentBank();
}
int64_t Storage::SaveCurrentPresetAs(const Pedalboard &pedalboard, const std::string &name, int64_t saveAfterInstanceId)
{
    Pedalboard newPedalboard = pedalboard;
    newPedalboard.name(name);

    int64_t newInstanceId = currentBank.addPreset(newPedalboard, saveAfterInstanceId);
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
        auto &preset = this->currentBank.presets()[i];
        entries[preset->instanceId()] = &(this->currentBank.presets()[i]);
    }
    std::vector<std::unique_ptr<BankFileEntry> *> newPresets;
    for (size_t i = 0; i < presets.presets().size(); ++i)
    {
        std::unique_ptr<BankFileEntry> *p = entries[presets.presets()[i].instanceId()];
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
int64_t Storage::GetPresetByProgramNumber(uint8_t program) const
{
    if (program >= currentBank.presets().size())
    {
        if (currentBank.presets().size() == 0)
            return -1;
        program = (uint8_t)currentBank.presets().size() - 1;
    }
    return currentBank.presets()[program]->instanceId();
}

Pedalboard Storage::GetPreset(int64_t instanceId) const
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
    int64_t newSelection = currentBank.deletePreset(presetId);
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

int64_t Storage::CreateNewPreset()
{
    Pedalboard newPedalboard = Pedalboard::MakeDefault();
    std::string name = "New";
    if (currentBank.hasName(name))
    {
        int copyNumber = 2;
        while (true)
        {
            name = SS("New (" << copyNumber << ")");
            if (!currentBank.hasName(name))
            {
                break;
            }
            ++copyNumber;
        }
    }
    newPedalboard.name(name);
    return this->currentBank.addPreset(newPedalboard, -1);
}

int64_t Storage::CopyPreset(int64_t fromId, int64_t toId)
{
    auto &fromItem = this->currentBank.getItem(fromId);
    if (toId == -1)
    {
        Pedalboard newPedalboard = fromItem.preset();
        std::string name = GetPresetCopyName(fromItem.preset().name());
        newPedalboard.name(name);
        return this->currentBank.addPreset(newPedalboard, fromId);
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
JackChannelSelection Storage::GetJackChannelSelection(const JackConfiguration &jackConfiguration)
{
    if (!this->isJackChannelSelectionValid)
    {
        if (jackConfiguration.isValid())
        {
            auto defaultSelection = JackChannelSelection::MakeDefault(jackConfiguration);
            SetJackChannelSelection(defaultSelection);
        }
    }
    return jackChannelSelection.RemoveInvalidChannels(jackConfiguration);
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
        pipedal::ofstream_synced s(fileName);
        json_writer writer(s, false);
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
        std::filesystem::copy(oldPath, newPath);
    }
    catch (std::exception &e)
    {
        std::stringstream s;
        s << "Unable to save the bank. (" << e.what() << ")";
        throw PiPedalException(s.str());
    }
    int64_t newId = this->bankIndex.addBank(bankId, newName);
    SaveBankIndex();
    return newId;
}

void Storage::MoveBank(int from, int to)
{
    this->bankIndex.move(from, to);
    this->SaveBankIndex();
}

int64_t Storage::GetBankByMidiBankNumber(uint8_t bankNumber)
{
    auto &entries = this->bankIndex.entries();
    if (bankNumber >= entries.size())
    {
        if (entries.size() == 0)
            return -1;
        bankNumber = (uint8_t)(entries.size() - 1);
    }
    return entries[bankNumber].instanceId();
}
int64_t Storage::DeleteBank(int64_t bankId)
{
    auto &entries = this->bankIndex.entries();

    for (size_t i = 0; i < entries.size(); ++i)
    {
        auto &entry = entries[i];

        if (entry.instanceId() == bankId)
        {
            std::filesystem::path fileName = this->GetBankFileName(entry.name());
            entries.erase(entries.begin() + i);

            int64_t newSelection;
            if (i < entries.size())
            {
                newSelection = entries[i].instanceId();
            }
            else if (entries.size() > 0)
            {
                newSelection = entries[entries.size() - 1].instanceId();
            }
            else
            {
                // zero entries?
                // Create a default empty bank.
                BankIndexEntry newEntry;

                BankFile defaultBank;
                Pedalboard defaultPedalboard = Pedalboard::MakeDefault();
                int64_t instanceId = defaultBank.addPreset(defaultPedalboard);
                defaultBank.selectedPreset(instanceId);

                std::string name = "Default Bank";
                SaveBankFile(name, defaultBank);
                int64_t selectedBank = bankIndex.addBank(-1, name);
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

int64_t Storage::UploadPreset(const BankFile &bankFile, int64_t uploadAfter)
{
    int64_t lastPreset = this->currentBank.selectedPreset();
    if (uploadAfter != -1)
    {
        lastPreset = uploadAfter;
    }

    if (bankFile.presets().size() == 0)
    {
        throw PiPedalException("Invalid preset.");
    }
    for (size_t i = 0; i < bankFile.presets().size(); ++i)
    {
        Pedalboard preset = bankFile.presets()[i]->preset();

        int n = 2;
        std::string baseName = preset.name();
        while (this->currentBank.hasName(preset.name()))
        {
            std::stringstream s;
            s << baseName << "(" << n++ << ")";
            preset.name(s.str());
        }

        lastPreset = this->currentBank.addPreset(preset, lastPreset);
    }
    this->SaveCurrentBank();
    return lastPreset;
}
int64_t Storage::UploadBank(BankFile &bankFile, int64_t uploadAfter)
{
    int64_t lastBank = this->bankIndex.selectedBank();
    if (uploadAfter != -1)
    {
        lastBank = uploadAfter;
    }

    if (bankFile.presets().size() == 0)
    {
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
    pipedal::ofstream_synced f(path);
    if (!f.is_open())
    {
        throw PiPedalException("Can't write to bank file.");
    }
    json_writer writer(f, true);
    writer.write(bankFile);

    lastBank = this->bankIndex.addBank(lastBank, bankFile.name());
    this->SaveBankIndex();
    return lastBank;
}

void Storage::SetGovernorSettings(const std::string &governor)
{
    userSettings.governor_ = governor;
    SaveUserSettings();
}
void Storage::SaveUserSettings()
{
    std::filesystem::path path = this->dataRoot / USER_SETTINGS_FILENAME;
    {
        pipedal::ofstream_synced f(path);
        if (!f.is_open())
        {
            throw PiPedalException("Unable to write to " + ((std::string)path));
        }
        json_writer writer(f, false);
        writer.write(userSettings);
    }
}
void Storage::SetShowStatusMonitor(bool show)
{
    this->userSettings.showStatusMonitor_ = show;
    SaveUserSettings();
}
bool Storage::GetShowStatusMonitor() const
{
    return this->userSettings.showStatusMonitor_;
}

std::string Storage::GetGovernorSettings() const
{
    return this->userSettings.governor_;
}

bool Storage::SetWifiConfigSettings(const WifiConfigSettings &wifiConfigSettings)
{
    WifiConfigSettings copyToSave = wifiConfigSettings;
    WifiConfigSettings previousValue;
    previousValue.Load();
    if (!copyToSave.hasPassword_)
    {
        copyToSave.hasPassword_ = previousValue.hasPassword_;
        copyToSave.password_ = previousValue.password_;
        copyToSave.hasSavedPassword_ = previousValue.hasPassword_;
    }
    else
    {
        if (copyToSave.IsEnabled())
        {
            copyToSave.hasSavedPassword_ = copyToSave.hasPassword_;
        }
    }
    bool configChanged = copyToSave.ConfigurationChanged(previousValue);
    copyToSave.Save();
    this->wifiConfigSettings = copyToSave;
    this->wifiConfigSettings.hasPassword_ = false;
    this->wifiConfigSettings.password_ = "";

    return configChanged;
}

void Storage::SetWifiDirectConfigSettings(const WifiDirectConfigSettings &wifiDirectConfigSettings)
{
    WifiDirectConfigSettings copyToSave = wifiDirectConfigSettings;
    copyToSave.rebootRequired_ = false;
    if (!copyToSave.enable_)
    {
        copyToSave.pinChanged_ = false;
    }

    WifiDirectConfigSettings copyToStore = wifiDirectConfigSettings;
    copyToStore.pinChanged_ = false;
    this->wifiDirectConfigSettings = copyToStore;
}

void Storage::LoadUserSettings()
{
    std::filesystem::path path = this->dataRoot / USER_SETTINGS_FILENAME;

    try
    {
        if (std::filesystem::is_regular_file(path))
        {
            std::ifstream f(path);
            if (!f.is_open())
            {
                throw PiPedalException("Unable to read from " + ((std::string)path));
            }
            json_reader reader(f);
            reader.read(&userSettings);
        }
    }
    catch (const std::exception &)
    {
    }
    this->wifiConfigSettings.valid_ = true;
}
void Storage::LoadWifiConfigSettings()
{
    this->wifiConfigSettings.Load();
    this->wifiConfigSettings.valid_ = this->wifiConfigSettings.hasPassword_ && !this->wifiConfigSettings.countryCode_.empty();
}
void Storage::LoadWifiDirectConfigSettings()
{

    WifiDirectConfigSettings settings;
    settings.enable_ = false;
    settings.Load();
    settings.pinChanged_ = false;
    settings.valid_ = true;
    this->wifiDirectConfigSettings = settings;
}

WifiConfigSettings Storage::GetWifiConfigSettings()
{
    WifiConfigSettings result = this->wifiConfigSettings;
    result.hasPassword_ = false;
    result.password_ = "";
    return result;
}
WifiDirectConfigSettings Storage::GetWifiDirectConfigSettings()
{
    return this->wifiDirectConfigSettings;
}

void Storage::SaveCurrentPreset(const CurrentPreset &currentPreset)
{
    try
    {
        std::filesystem::path path = GetCurrentPresetPath();

        pipedal::ofstream_synced f(path);
        json_writer writer(f, true);
        writer.write(currentPreset);
    }
    catch (std::exception &)
    {
        // called from destructor. Must be nothrow().
    }
}
bool Storage::RestoreCurrentPreset(CurrentPreset *pResult)
{
    std::filesystem::path path = GetCurrentPresetPath();
    if (std::filesystem::exists(path))
    {
        try
        {
            std::ifstream f(path);
            json_reader reader(f);
            reader.read(pResult);
            std::filesystem::remove(path); // one-shot only, restore the state from the last *orderly* shutdown.
        }
        catch (const std::exception &e)
        {
            Lv2Log::warning(SS("Failed to restore current preset. " << e.what()));

            std::filesystem::remove(path); // one-shot only, restore the state from the last *orderly* shutdown.
            return false;
        }
        return true;
    }
    else
    {
        return false;
    }
}
bool Storage::HasPluginPresets(const std::string &pluginUri) const
{
    for (const auto &entry : this->pluginPresetIndex.entries_)
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
    for (const auto &entry : this->pluginPresetIndex.entries_)
    {
        if (entry.pluginUri_ == pluginUri)
        {
            return this->GetPluginPresetsDirectory() / entry.fileName_;
        }
    }
    throw PiPedalArgumentException("Plugin preset file not found.");
}

void Storage::MergePluginPresets(const std::string &pluginUri, const PluginPresets &presets)
{
    std::string name;
    std::filesystem::path path;
    bool presetAdded = false;
    PluginPresets existingPresets;
    if (!HasPluginPresets(pluginUri))
    {
        name = SS(pluginPresetIndex.nextInstanceId_ << ".json");
        path = GetPluginPresetsDirectory() / name;
        presetAdded = true;
    }
    else
    {
        path = GetPluginPresetPath(pluginUri);
        try
        {
            std::ifstream f(path);
            if (f.is_open())
            {
                json_reader reader(f);
                reader.read(&existingPresets);
            }
        }
        catch (const std::exception &e)
        {
            Lv2Log::error(SS("Storage::MergePluginPresets: Can't reading existing plugin presets." << e.what()));
        }
    }
    for (const PluginPreset &preset : presets.presets_)
    {

        existingPresets.MergePreset(preset);
    }
    auto tempPath = path.string() + ".$$$";
    {
        pipedal::ofstream_synced os;
        os.open(tempPath, std::ios_base::trunc);
        if (os.fail())
        {
            throw PiPedalException(SS("Can't write to " << path));
        }
        json_writer writer(os, true);
        writer.write(existingPresets);
    }
    if (std::filesystem::exists(path))
    {
        std::filesystem::remove(path);
    }
    std::filesystem::rename(tempPath, path);

    if (presetAdded)
    {
        pluginPresetIndex.entries_.push_back(
            PluginPresetIndexEntry(
                pluginUri, name));
        pluginPresetIndex.nextInstanceId_++;
        this->pluginPresetIndexChanged = true;
        SavePluginPresetIndex();
    }
}

void Storage::SavePluginPresets(const std::string &pluginUri, const PluginPresets &presets)
{
    std::string name;
    std::filesystem::path path;
    bool presetAdded = false;
    if (!HasPluginPresets(pluginUri))
    {
        name = SS(pluginPresetIndex.nextInstanceId_ << ".json");
        path = GetPluginPresetsDirectory() / name;
        presetAdded = true;
    }
    else
    {
        path = GetPluginPresetPath(pluginUri);
    }
    auto tempPath = path.string() + ".$$$";
    {
        pipedal::ofstream_synced os;
        os.open(tempPath, std::ios_base::trunc);
        if (os.fail())
        {
            throw PiPedalException(SS("Can't write to " << path));
        }
        json_writer writer(os, true);
        writer.write(presets);
    }
    if (std::filesystem::exists(path))
    {
        std::filesystem::remove(path);
    }
    std::filesystem::rename(tempPath, path);

    if (presetAdded)
    {
        pluginPresetIndex.entries_.push_back(
            PluginPresetIndexEntry(
                pluginUri, name));
        pluginPresetIndex.nextInstanceId_++;
        this->pluginPresetIndexChanged = true;
        SavePluginPresetIndex();
    }
}

PluginPresets Storage::GetPluginPresets(const std::string &pluginUri) const
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
PluginUiPresets Storage::GetPluginUiPresets(const std::string &pluginUri) const
{
    PluginPresets presets = GetPluginPresets(pluginUri);
    PluginUiPresets result;
    result.pluginUri_ = presets.pluginUri_;
    for (size_t i = 0; i < presets.presets_.size(); ++i)
    {
        const auto &preset = presets.presets_[i];
        result.presets_.push_back(
            PluginUiPreset{
                preset.label_,
                preset.instanceId_});
    }
    return result;
}

PluginPresetValues Storage::GetPluginPresetValues(const std::string &pluginUri, uint64_t instanceId)
{
    auto presets = GetPluginPresets(pluginUri);
    for (const auto &preset : presets.presets_)
    {
        if (preset.instanceId_ == instanceId)
        {
            PluginPresetValues result;

            for (const auto &valuePair : preset.controlValues_)
            {
                result.controls.push_back(ControlValue(valuePair.first.c_str(), valuePair.second));
            }
            result.state = preset.state_;
            result.lilvPresetUri = preset.lilvPresetUri_;
            return result;
        }
    }
    throw PiPedalException("Plugin preset not found.");
}

uint64_t Storage::SavePluginPreset(
    const std::string &name,
    const PedalboardItem &pedalboardItem)
{
    const std::string &pluginUri = pedalboardItem.uri();
    auto presets = GetPluginPresets(pluginUri);
    uint64_t result = -1;
    bool existing = false;
    for (size_t i = 0; i < presets.presets_.size(); ++i)
    {
        auto &preset = presets.presets_[i];
        if (preset.label_ == name)
        {
            existing = true;
            result = preset.instanceId_;
            presets.presets_[i] = PluginPreset(preset.instanceId_, preset.label_, pedalboardItem);
            break;
        }
    }
    if (!existing)
    {
        result = presets.nextInstanceId_++;
        presets.presets_.push_back(
            PluginPreset(result,
                         name,
                         pedalboardItem));
    }
    this->SavePluginPresets(pluginUri, presets);
    return result;
}

uint64_t Storage::SavePluginPreset(
    const std::string &pluginUri,
    const std::string &name,
    float inputVolume,
    float outputVolume,
    const std::map<std::string, float> &values,
    const Lv2PluginState &lv2State)
{
    auto presets = GetPluginPresets(pluginUri);
    uint64_t result = -1;
    bool existing = false;
    for (size_t i = 0; i < presets.presets_.size(); ++i)
    {
        auto &preset = presets.presets_[i];
        if (preset.label_ == name)
        {
            existing = true;
            preset.controlValues_ = values;
            preset.state_ = lv2State;

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
                values,
                lv2State));
    }
    this->SavePluginPresets(pluginUri, presets);
    return result;
}

void Storage::UpdatePluginPresets(const PluginUiPresets &pluginPresets)
{
    // handles deletions, renaming, and reordering only.
    // If you need to add a preset, you need to call SavePluginPreset or DuplicatePluginPreset instead.

    PluginPresets presets = this->GetPluginPresets(pluginPresets.pluginUri_);
    PluginPresets newPresets;
    newPresets.pluginUri_ = pluginPresets.pluginUri_;
    newPresets.nextInstanceId_ = presets.nextInstanceId_;

    for (const auto &preset : pluginPresets.presets_)
    {
        PluginPreset newPreset = presets.GetPreset(preset.instanceId_);
        newPreset.label_ = preset.label_;
        newPresets.presets_.push_back(std::move(newPreset));
    }
    SavePluginPresets(newPresets.pluginUri_, newPresets);
}

static std::string stripCopySuffix(const std::string &s)
{
    int pos = s.length() - 1;
    if (pos >= 0 && s[pos] == ')')
    {
        --pos;
        while (pos >= 0 && s[pos] >= '0' && s[pos] <= '9')
        {
            --pos;
        }
        if (pos >= 0 && s[pos] == '(')
        {
            --pos;
        }
        while (pos >= 0 && s[pos] == ' ')
        {
            --pos;
        }
        return s.substr(0, pos + 1);
    }
    return s;
}
uint64_t Storage::CopyPluginPreset(const std::string &pluginUri, uint64_t presetId)
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
    presets.presets_.insert(presets.presets_.begin() + pos + 1, t);
    SavePluginPresets(presets.pluginUri_, presets);
    return t.instanceId_;
}

std::map<std::string, bool> Storage::GetFavorites() const
{
    std::map<std::string, bool> result;

    std::filesystem::path fileName = this->dataRoot / "favorites.json";
    if (!std::filesystem::exists(fileName))
    {
        fileName = this->configRoot / "defaultFavorites.json";
    }
    std::ifstream f;
    f.open(fileName);
    if (f.is_open())
    {
        json_reader reader(f);
        reader.read(&result);
    }
    return result;
}
void Storage::SetFavorites(const std::map<std::string, bool> &favorites)
{
    std::filesystem::path fileName = this->dataRoot / "favorites.json";
    pipedal::ofstream_synced f;
    f.open(fileName);
    if (f.is_open())
    {
        json_writer writer(f, true);
        writer.write(favorites);
    }
}

pipedal::JackServerSettings Storage::GetJackServerSettings()
{
    JackServerSettings result;
    std::filesystem::path fileName = this->dataRoot / "AudioConfig.json";
    std::ifstream f;
    f.open(fileName);
    if (f.is_open())
    {
        json_reader reader(f);
        reader.read(&result);
    }
#if JACK_HOST
    result.Initialize();
#endif

    return result;
}
void Storage::SetJackServerSettings(const pipedal::JackServerSettings &jackConfiguration)
{
    std::filesystem::path fileName = this->dataRoot / "AudioConfig.json";
    pipedal::ofstream_synced f;
    f.open(fileName);
    if (f.is_open())
    {
        json_writer writer(f, false);
        writer.write(jackConfiguration);
    }
#if JACK_HOST
    jackConfiguration.Write();
#endif
}

void Storage::SetSystemMidiBindings(const std::vector<MidiBinding> &bindings)
{
    std::filesystem::path fileName = this->dataRoot / "config" / "SystemMidiBindings.json";
    pipedal::ofstream_synced f;
    f.open(fileName);
    if (f.is_open())
    {
        json_writer writer(f, true);
        writer.write(bindings);
    }
}
static bool hasBinding(std::vector<MidiBinding> &bindings, const std::string &name)
{
    for (auto &binding : bindings)
    {
        if (binding.symbol() == name)
        {
            return true;
        }
    }
    return false;
}
std::vector<MidiBinding> Storage::GetSystemMidiBindings()
{
    std::vector<MidiBinding> result;

    std::filesystem::path fileName = this->dataRoot / "config" / "SystemMidiBindings.json";
    if (!std::filesystem::exists(fileName))
    {
        // pick up from legacy location?
        fileName = this->configRoot / "config" / "SystemMidiBindings.json";
    }
    std::ifstream f;
    f.open(fileName);
    if (f.is_open())
    {
        try
        {
            json_reader reader(f);
            reader.read(&result);
            if (result.size() == 2)
            {
                result.push_back(MidiBinding::SystemBinding("stopHotspot"));
                result.push_back(MidiBinding::SystemBinding("startHotspot"));
                result.push_back(MidiBinding::SystemBinding("shutdown"));
                result.push_back(MidiBinding::SystemBinding("reboot"));
            }
            if (!hasBinding(result, "prevBank"))
            {
                result.insert(result.begin(), MidiBinding::SystemBinding("nextBank")); // reverse order.
                result.insert(result.begin(), MidiBinding::SystemBinding("prevBank"));
            }
            if (!hasBinding(result, "snapshot1"))
            {
                auto position = 4;
                for (int i = 0; i < 6; ++i)
                {
                    result.insert(result.begin() + position, MidiBinding::SystemBinding(SS("snapshot" << (i + 1))));
                    ++position;
                }
            }
            return result;
        }
        catch (const std::exception &e)
        {
            Lv2Log::warning(SS("Can't read file " << fileName << ". " << e.what()));
        }
    }
    result.clear();
    result.push_back(MidiBinding::SystemBinding("prevBank"));
    result.push_back(MidiBinding::SystemBinding("nextBank"));
    result.push_back(MidiBinding::SystemBinding("prevProgram"));
    result.push_back(MidiBinding::SystemBinding("nextProgram"));

    result.push_back(MidiBinding::SystemBinding("snapshot1"));
    result.push_back(MidiBinding::SystemBinding("snapshot2"));
    result.push_back(MidiBinding::SystemBinding("snapshot3"));
    result.push_back(MidiBinding::SystemBinding("snapshot4"));
    result.push_back(MidiBinding::SystemBinding("snapshot5"));
    result.push_back(MidiBinding::SystemBinding("snapshot6"));

    result.push_back(MidiBinding::SystemBinding("stopHotspot"));
    result.push_back(MidiBinding::SystemBinding("startHotspot"));
    result.push_back(MidiBinding::SystemBinding("shutdown"));
    result.push_back(MidiBinding::SystemBinding("reboot"));
    return result;
}

static bool containsDirectorySeparator(const std::string &value)
{
    if (value.find("/") != std::string::npos)
        return true; // linux
    if (value.find("\\") != std::string::npos)
        return true; // windows
    if (value.find("::") != std::string::npos)
        return true; // mac
    return false;
}

static void ThrowPermissionDeniedError()
{
    throw std::logic_error("Permission denied.");
}

static bool ensureNoDotDot(const std::filesystem::path &path)
{
    for (auto segment_ : path)
    {
        std::string segment = segment_.string();
        if (segment.starts_with("."))
        {
            // the linux rule: any path that consists of all '.'s.
            bool valid = false;
            for (auto c : segment)
            {
                if (c != '.')
                {
                    valid = true;
                    break;
                }
            }
            if (!valid)
            {
                return false;
            }
        }
    }
    return true;
}

static void AddFilesToResult(
    FileRequestResult &result,
    const ModFileTypes::ModDirectory *modDirectoryInfo, // yyx
    const UiFileProperty &fileProperty,
    const fs::path &rootPath)
{

    if (!fs::exists(rootPath))
    {
        return; // silently without error.
    }
    auto &resultFiles = result.files_;

    std::set<std::string> validExtensions = fileProperty.GetPermittedFileExtensions(
        modDirectoryInfo ? modDirectoryInfo->modType : "");

    try
    {
        for (auto const &dir_entry : std::filesystem::directory_iterator(rootPath))
        {
            const auto &path = dir_entry.path();
            auto name = path.filename().string();
            if (dir_entry.is_regular_file())
            {
                std::string extension = UiFileProperty::GetFileExtension(path);

                bool match = false;
                if (name.length() > 0 && name[0] != '.') // don't show hidden files.
                {
                    bool match = validExtensions.size() == 0 || validExtensions.contains(extension) || validExtensions.contains(".*");

                    if (match && !name.starts_with("."))
                    {
                        resultFiles.push_back(
                            FileEntry(path, name, false, false));
                    }
                }
            }
            else if (dir_entry.is_directory())
            {
                resultFiles.push_back(FileEntry{path, name, true, fs::is_symlink(path)});
            }
        }
    }
    catch (const std::exception &error)
    {
        throw std::logic_error("GetFileList failed. Directory not found: " + rootPath.string());
    }

    // sort lexicographically

    auto collator = Locale::GetInstance()->GetCollator();

    std::sort(resultFiles.begin(), resultFiles.end(), [&collator](const FileEntry &l, const FileEntry &r)
              {
        if (l.isDirectory_ != r.isDirectory_)
        {
            return l.isDirectory_ > r.isDirectory_;
        }
        return collator->Compare(l.displayName_,r.displayName_) < 0; });
}

static void AddTracksToResult(
    const fs::path &audioRootDirectory,
    FileRequestResult &result,
    const ModFileTypes::ModDirectory *modDirectoryInfo, // yyx
    const UiFileProperty &fileProperty,
    const fs::path &rootPath)
{

    if (!fs::exists(rootPath))
    {
        return; // silently without error.
    }
    auto &resultFiles = result.files_;

    std::set<std::string> validExtensions = fileProperty.GetPermittedFileExtensions(
        modDirectoryInfo ? modDirectoryInfo->modType : "");

    try
    {
        // Add directories first.
        for (auto const &dir_entry : std::filesystem::directory_iterator(rootPath))
        {
            const auto &path = dir_entry.path();
            auto name = path.filename().string();
            if (dir_entry.is_directory())
            {
                resultFiles.push_back(FileEntry{path, name, true, fs::is_symlink(path)});
            }
        }
        auto collator = Locale::GetInstance()->GetCollator();
        std::sort(
            resultFiles.begin(), 
            resultFiles.end(),
            [collator](const FileEntry &l, const FileEntry &r)
                  {
            if (l.isDirectory_ != r.isDirectory_)
            {
                return l.isDirectory_ > r.isDirectory_;
            }
            return collator->Compare(l.displayName_ ,r.displayName_) < 0; 
        });
        // Add audio files.
        auto audioFiles = AudioDirectoryInfo::Create(rootPath,
            GetShadowIndexDirectory(audioRootDirectory,rootPath)
        );
        for (const auto &audioFile : audioFiles->GetFiles())
        {
            fs::path audioFilePath = rootPath / audioFile.fileName();
            std::string extension = UiFileProperty::GetFileExtension(audioFilePath);
            if (validExtensions.size() == 0 || validExtensions.contains(extension) || validExtensions.contains(".*"))
            {
                resultFiles.push_back(
                    FileEntry(
                        audioFilePath,
                        audioFile.title(),
                        false,
                        std::make_shared<AudioFileMetadata>(audioFile)));
            }
        }
    }
    catch (const std::exception &error)
    {
        throw std::logic_error("GetFileList failed. Directory not found: " + rootPath.string());
    }
}

FileRequestResult Storage::GetModFileList2(const std::string &relativePath, const UiFileProperty &fileProperty)
{
    FileRequestResult result;
    fs::path uploadsDirectory = GetPluginUploadDirectory();

    if (relativePath.empty())
    {
        // return the synthetic root.
        result.isProtected_ = true;

        for (const auto &modDirectory : fileProperty.modDirectories())
        {
            const auto directoryInfo = ModFileTypes::GetModDirectory(modDirectory);
            if (directoryInfo)
            {
                result.files_.push_back(
                    FileEntry(uploadsDirectory / directoryInfo->pipedalPath, directoryInfo->displayName, true, true));
            }
        }
        if (fileProperty.useLegacyModDirectory())
        {
            result.files_.push_back(
                FileEntry(uploadsDirectory / fileProperty.directory(), fs::path(fileProperty.directory()).filename().string(), true, true));
        }
        result.breadcrumbs_.push_back({"", "Home"});
        result.currentDirectory_ = relativePath;
        return result;
    }
    fs::path modDirectoryPath;

    result.breadcrumbs_.push_back({"", "Home"});
    fs::path fsRelativePath{relativePath};

    const ModFileTypes::ModDirectory *rootModDirectory = nullptr;
    for (const auto &modDirectory : fileProperty.modDirectories())
    {
        const ModFileTypes::ModDirectory *modDirectoryInfo = ModFileTypes::GetModDirectory(modDirectory);
        if (modDirectoryInfo)
        {
            if (IsSubdirectory(fsRelativePath, uploadsDirectory / modDirectoryInfo->pipedalPath))
            {
                rootModDirectory = modDirectoryInfo;
                modDirectoryPath = uploadsDirectory / modDirectoryInfo->pipedalPath;
                result.breadcrumbs_.push_back({modDirectoryPath.string(), modDirectoryInfo->displayName});
                break;
            }
        }
    }
    if (modDirectoryPath.empty() && fileProperty.useLegacyModDirectory())
    {
        if (IsSubdirectory(fsRelativePath, uploadsDirectory / fileProperty.directory()))
        {
            modDirectoryPath = uploadsDirectory / fileProperty.directory();
            result.breadcrumbs_.push_back({modDirectoryPath.string(), fs::path(fileProperty.directory()).filename().string()});
        }
        else
        {
            ThrowPermissionDeniedError();
        }
    }

    // add reaming path segments as breadcrumbs.
    {
        fs::path modPath{modDirectoryPath};
        fs::path rp{relativePath};
        auto iRp = rp.begin();
        // skip past one or more segments in the modDirectoryPath.
        for (auto iModPath = modPath.begin(); iModPath != modPath.end(); ++iModPath)
        {
            if (iRp != rp.end())
            {
                ++iRp;
            }
        }
        while (iRp != rp.end())
        {
            result.breadcrumbs_.push_back({*iRp, *iRp});
            ++iRp;
        }
    }

    if (IsInAudioTracksDirectory(relativePath))
    {
        AddTracksToResult(this->GetPluginUploadDirectory(),result, rootModDirectory, fileProperty, relativePath);
    }
    else
    {
        AddFilesToResult(result, rootModDirectory, fileProperty, relativePath);
    }
    result.currentDirectory_ = relativePath;
    return result;
}

static bool IsChildDirectory(const fs::path &child, const fs::path &parent)
{
    auto iChild = child.begin();
    for (auto i = parent.begin(); i != parent.end(); ++i)
    {
        if (iChild == child.end() || (*i) != *iChild)
        {
            return false;
        }
        ++iChild;
    }
    return true;
}

FileRequestResult Storage::GetFileList2(const std::string &relativePath_, const UiFileProperty &fileProperty)
{
    std::string absolutePath = relativePath_;
    if (!ensureNoDotDot(absolutePath))
    {
        ThrowPermissionDeniedError();
    }
    if (hasSyntheticModRoot(fileProperty))
    {
        return Storage::GetModFileList2(absolutePath, fileProperty);
    }

    FileRequestResult result;

    if (fileProperty.directory().empty())
    {

        throw std::runtime_error("fileProperty.directory() not specified.");
    }
    std::filesystem::path pluginRootDirectory = this->GetPluginUploadDirectory() / fileProperty.directory();
    if (absolutePath.empty())
    {
        absolutePath = pluginRootDirectory.string();
    }

    // handle resource directories.
    if (!fileProperty.resourceDirectory().empty())
    {
        if (fileProperty.resourceDirectory() == absolutePath)
        {
            absolutePath = pluginRootDirectory;
        }
    }

    if (!IsChildDirectory(absolutePath, pluginRootDirectory))
    {
        absolutePath = pluginRootDirectory;
    }

    result.currentDirectory_ = absolutePath;

    {
        // watch out for resource files!!!
        result.breadcrumbs_.push_back({"", "Home"});
        fs::path fsAbsolutePath{absolutePath};
        auto iAbsolutePath = fsAbsolutePath.begin();
        for (auto i = pluginRootDirectory.begin(); i != pluginRootDirectory.end(); ++i)
        {
            if (iAbsolutePath == fsAbsolutePath.end() || (*i) != *iAbsolutePath)
            {
                throw std::runtime_error("Directory is not a subdirectory of the plugin root directory.");
            }
            ++iAbsolutePath;
        }
        auto cumulativePath = pluginRootDirectory;

        while (iAbsolutePath != fsAbsolutePath.end())
        {
            cumulativePath /= (*iAbsolutePath);
            result.breadcrumbs_.push_back({cumulativePath.string(), iAbsolutePath->string()});
            ++iAbsolutePath;
        }
    }
    if (!IsSubdirectory(absolutePath, pluginRootDirectory))
    {
        throw std::runtime_error(SS("Improper location. " << absolutePath));
    }

    const ModFileTypes::ModDirectory *pModDirectory = nullptr;
    if (fileProperty.modDirectories().size() > 0)
    {
        pModDirectory = ModFileTypes::GetModDirectory(fileProperty.modDirectories()[0]);
    }

    if (IsInAudioTracksDirectory(absolutePath))
    {
        AddTracksToResult(GetPluginUploadDirectory(), result, pModDirectory, fileProperty, absolutePath);
    }
    else
    {
        AddFilesToResult(result, pModDirectory, fileProperty, absolutePath);
    }
    return result;
}

bool Storage::IsValidSampleFileName(const std::filesystem::path &fileName)
{
    if (!fileName.is_absolute())
    {
        return false;
    }

    std::filesystem::path audioFilePath = this->GetPluginUploadDirectory();

    std::filesystem::path parentDirectory = fileName.parent_path();

    auto iTarget = parentDirectory.begin();

    for (auto i = audioFilePath.begin(); i != audioFilePath.end(); ++i)
    {
        if (iTarget == parentDirectory.end())
        {
            return false;
        }
        if (*i != *iTarget)
        {
            return false;
        }
        ++iTarget;
    }
    while (iTarget != parentDirectory.end())
    {
        if (iTarget->string() == "..")
        {
            return false;
        }
        ++iTarget;
    }
    return true;
}
void Storage::DeleteSampleFile(const std::filesystem::path &fileName)
{
    if (!IsValidSampleFileName(fileName))
    {
        throw std::logic_error("Permission denied.");
    }
    if (!std::filesystem::exists(fileName))
    {
        throw std::logic_error("File not found.");
    }
    try
    {
        if (std::filesystem::is_directory(fileName))
        {
            if (fileName.string().length() <= 1) // guard against rm -rf /  (bitter experience)
            {
                throw std::logic_error("Invalid filename.");
            }
            if (std::filesystem::is_symlink(fileName))
            {
                std::filesystem::remove(fileName);
            }
            else
            {
                if (fileName.string().length() > 1) // guard against rm -rf /  (bitter experience)
                {
                    std::filesystem::remove_all(fileName);
                }
            }
        }
        else
        {
            std::filesystem::remove(fileName);
        }
    }
    catch (const std::exception &)
    {
        throw std::logic_error("Permission denied.");
    }
}
std::filesystem::path Storage::MakeUserFilePath(const std::string &directory, const std::string &filename)
{
    std::filesystem::path filePath{filename};

    std::filesystem::path result = fs::path(directory) / filename;
    if (!result.is_absolute())
    {
        result = this->GetPluginUploadDirectory() / result;
    }
    if (!this->IsValidSampleFileName(result))
    {
        throw std::logic_error(SS("Invalid upload path: " << result));
    }
    return result;
}
std::string Storage::UploadUserFile(const std::string &directory,
                                    UiFileProperty::ptr uiFileProperty,
                                    const std::string &filename,
                                    std::istream &stream, size_t contentLength)
{
    std::filesystem::path path;
    if (directory.length() != 0)
    {
        path = this->MakeUserFilePath(directory, filename);
    }
    else
    {
        throw std::logic_error("Directory argument not supplied.");
    }
    fs::path relativePath;
    try
    {
        relativePath = MakeRelativePath(path, this->GetPluginUploadDirectory());
    }
    catch (const std::exception &e)
    {
        throw std::logic_error("Permission denied. Path is outside the upload storage directory.");
    }

    if (!uiFileProperty->IsValidExtension(relativePath))
    {
        throw std::logic_error("Permission denied. Invalid file extension for this directory.");
    }

    {
        try
        {
            std::filesystem::create_directories(path.parent_path());

            pipedal::ofstream_synced f(path, std::ios_base::trunc | std::ios_base::binary);
            if (!f.is_open())
            {
                throw std::logic_error(SS("Can't create file " << path << "."));
            }
            std::vector<uint8_t> buffer;
            size_t BUFFER_SIZE = 64 * 1024;
            buffer.resize(BUFFER_SIZE);
            char *pBuffer = (char *)&(buffer[0]);
            while (contentLength != 0)
            {
                size_t thisTime = std::min(BUFFER_SIZE, contentLength);
                stream.read(pBuffer, (std::streamsize)thisTime);
                if (!stream)
                {
                    throw std::runtime_error("Unable to read body input stream.");
                }
                f.write(pBuffer, (std::streamsize)thisTime);
                if (!f)
                {
                    throw std::runtime_error("Failed to write to upload file.");
                }
                contentLength -= thisTime;
            }
        }
        catch (const std::exception &e)
        {
            Lv2Log::error(SS("Upload failed. " << e.what()));
            std::filesystem::remove(path);
            throw;
        }
    }
    return path.string();
}

std::string Storage::CreateNewSampleDirectory(const std::string &relativePath, const UiFileProperty &uiFileProperty)
{
    if (uiFileProperty.directory().empty())
    {
        throw std::runtime_error("Invalid UI File Property.");
    }
    std::filesystem::path path = this->GetPluginUploadDirectory() / uiFileProperty.directory() / relativePath;
    if (!this->IsValidSampleFileName(path))
    {
        throw std::runtime_error("Invalid file name.");
    }
    if (std::filesystem::exists(path))
    {
        throw std::runtime_error("A directory with that name already exists.");
    }
    std::filesystem::create_directories(path);
    return path;
}
std::string Storage::RenameFilePropertyFile(
    const std::string &oldRelativePath,
    const std::string &newRelativePath,
    const UiFileProperty &uiFileProperty)
{
    if (uiFileProperty.directory().empty())
    {
        throw std::runtime_error("Invalid UI File Property.");
    }
    std::filesystem::path oldPath = this->GetPluginUploadDirectory() / uiFileProperty.directory() / oldRelativePath;
    if (!this->IsValidSampleFileName(oldPath))
    {
        throw std::runtime_error("Invalid file name.");
    }
    if (!std::filesystem::exists(oldPath))
    {
        throw std::runtime_error("Original path does not exist.");
    }

    std::filesystem::path newPath = this->GetPluginUploadDirectory() / uiFileProperty.directory() / newRelativePath;
    if (!this->IsValidSampleFileName(newPath))
    {
        throw std::runtime_error("Invalid file name.");
    }
    if (std::filesystem::exists(newPath))
    {
        if (std::filesystem::is_directory(newPath))
        {
            throw std::runtime_error("A directory with that name already exists.");
        }
        else
        {
            throw std::runtime_error("A file with that name already exists.");
        }
    }

    std::filesystem::rename(oldPath, newPath);
    return newPath;
}

void Storage::FillSampleDirectoryTree(FilePropertyDirectoryTree *node, const std::filesystem::path &directory) const
{
    for (auto child : std::filesystem::directory_iterator(directory))
    {
        if (child.is_directory())
        {
            const auto &childPath = child.path();
            FilePropertyDirectoryTree::ptr childTree = std::make_unique<FilePropertyDirectoryTree>(childPath);
            FillSampleDirectoryTree(childTree.get(), childPath);
            node->children_.push_back(std::move(childTree));
        }
    }
    auto collator = Locale::GetInstance()->GetCollator();
    std::sort(node->children_.begin(), node->children_.end(),
              [&collator](const FilePropertyDirectoryTree::ptr &left, const FilePropertyDirectoryTree::ptr &right)
              {
                  return collator->Compare(left->directoryName_, right->directoryName_) < 0;
              });
}

static void GetAllExtensions(std::set<std::string> &result, const std::filesystem::path &path)
{
    assert(fs::is_directory(path));

    for (const auto &dirEnt : fs::directory_iterator(path))
    {
        if (dirEnt.is_directory())
        {
            GetAllExtensions(result, dirEnt.path());
        }
        else
        {
            std::string filename = dirEnt.path().filename();
            if (dirEnt.path().has_extension())
            {
                std::string extension = dirEnt.path().extension();
                result.insert(extension);
            }
        }
    }
}

static std::set<std::string> GetAllExtensions(const std::filesystem::path &path)
{
    std::set<std::string> result;
    if (fs::is_regular_file(path))
    {
        result.insert(UiFileProperty::GetFileExtension(path));
    }
    else
    {
        GetAllExtensions(result, path);
    }
    return result;
}
FilePropertyDirectoryTree::ptr Storage::GetFilePropertydirectoryTree(const UiFileProperty &uiFileProperty, const std::filesystem::path &selectedPath)
{
    fs::path uploadDirectory = this->GetPluginUploadDirectory();

    fs::path relativePath;
    try
    {
        relativePath = MakeRelativePath(selectedPath, uploadDirectory);
    }
    catch (const std::exception &e)
    {
        // not an upload directory.
        throw std::runtime_error(SS("Permission denied: " << selectedPath));
    }

    std::set<std::string> fileExtensions = GetAllExtensions(selectedPath);

    if (hasSyntheticModRoot(uiFileProperty))
    {

        if (relativePath.empty())
        {
            throw std::runtime_error(SS("Permission denied: " << selectedPath));
        }
        FilePropertyDirectoryTree::ptr result = std::make_unique<FilePropertyDirectoryTree>("", "Home");
        result->isProtected_ = true;
        std::string parentModDirectory = uiFileProperty.getParentModDirectory(relativePath);

        for (const auto &modDirectory : uiFileProperty.modDirectories())
        {
            auto modDirectoryInfo = ModFileTypes::GetModDirectory(modDirectory);
            if (modDirectoryInfo)
            {
                if (IsSubset(fileExtensions, modDirectoryInfo->fileExtensions) ||
                    modDirectoryInfo->fileExtensions.contains(".*") ||
                    modDirectoryInfo->fileExtensions.empty() // Private directory that accepts files of any type.
                )
                {
                    auto childPath = uploadDirectory / modDirectoryInfo->pipedalPath;
                    FilePropertyDirectoryTree::ptr child = std::make_unique<FilePropertyDirectoryTree>(
                        childPath, modDirectoryInfo->displayName);
                    FillSampleDirectoryTree(child.get(), childPath);
                    result->children_.push_back(std::move(child));
                }
            }
        }
        if (uiFileProperty.useLegacyModDirectory())
        {
            auto childPath = uploadDirectory / uiFileProperty.directory();
            FilePropertyDirectoryTree::ptr child = std::make_unique<FilePropertyDirectoryTree>(
                childPath);
            FillSampleDirectoryTree(child.get(), childPath);
            result->children_.push_back(std::move(child));
        }
        return result;
    }
    else
    {
        if (uiFileProperty.directory().empty())
        {
            throw std::runtime_error("Invalid uiFileProperty");
        }
        if (!ensureNoDotDot(uiFileProperty.directory()))
        {
            throw std::runtime_error("Invalid uiFileProperty");
        }
        std::filesystem::path rootDirectory = uploadDirectory / uiFileProperty.directory();
        FilePropertyDirectoryTree::ptr result = std::make_unique<FilePropertyDirectoryTree>(rootDirectory.string(), "Home");

        FillSampleDirectoryTree(result.get(), rootDirectory);
        return result;
    }
}

bool Storage::IsInAudioTracksDirectory(const std::filesystem::path &path) const
{
    std::filesystem::path audioTracksDirectory =
        this->GetPluginUploadDirectory() / "shared/audio/Tracks";
    return IsSubdirectory(path, audioTracksDirectory);
}

bool Storage::IsInUploadsDirectory(const std::filesystem::path &path) const
{
    return IsSubdirectory(path, this->GetPluginUploadDirectory());
}

const PluginPresetIndex &Storage::GetPluginPresetIndex()
{
    return pluginPresetIndex;
}

JSON_MAP_BEGIN(UserSettings)
JSON_MAP_REFERENCE(UserSettings, governor)
JSON_MAP_REFERENCE(UserSettings, showStatusMonitor)
JSON_MAP_END()

JSON_MAP_BEGIN(CurrentPreset)
JSON_MAP_REFERENCE(CurrentPreset, modified)
JSON_MAP_REFERENCE(CurrentPreset, preset)
JSON_MAP_END()
