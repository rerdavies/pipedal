// Copyright (c) 2024 Robin Davies
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

#include "PresetBundle.hpp"
#include "PiPedalModel.hpp"
#include <vector>
#include "Pedalboard.hpp"
#include "Banks.hpp"
#include "json.hpp"
#include <sstream>
#include "lv2/atom/atom.h"
#include "ZipFile.hpp"
#include <set>

using namespace pipedal;

static std::string ToString(const std::vector<uint8_t> &value)
{
    const char *start = (const char *)(&value[0]);
    const char *end = start + value.size();
    while (end != start && end[-1] == 0)
    {
        --end;
    }

    return std::string(start, end);
}

static std::vector<uint8_t> ToBinary(const std::string &value)
{
    std::vector<uint8_t> result(value.length() + 1);
    for (size_t i = 0; i < value.length(); ++i)
    {
        result[i] = (uint8_t)value[i];
    }
    result[value.length()] = 0;
    return result;
}

class PresetBundleWriterImpl : public PresetBundleWriter
{
public:
    using self = PresetBundleWriterImpl;
    using super = PresetBundleWriter;

    PresetBundleWriterImpl()
    {
    }

    void LoadPresets(PiPedalModel &model, const std::string &presetJson)
    {
        BankFile bankFile;

        std::istringstream s(presetJson);
        json_reader reader(s);

        reader.read(&bankFile);
        GatherMediaPaths(model, bankFile);
        configFiles["bankFile.json"] = presetJson;

        pluginUploadDirectory = model.GetPluginUploadDirectory();
    }

    void LoadPluginPresets(PiPedalModel &model, const std::string pluginPresetJson)
    {
        PluginPresets pluginPresets;

        std::istringstream s(pluginPresetJson);
        json_reader reader(s);

        reader.read(&pluginPresets);

        GatherMediaPaths(model, pluginPresets);
        configFiles["pluginPresets.json"] = pluginPresetJson;

        pluginUploadDirectory = model.GetPluginUploadDirectory();
    }
    virtual ~PresetBundleWriterImpl() noexcept;

    virtual void WriteToFile(const std::filesystem::path &filePath) override;

private:
    void AddUsedPlugin(PiPedalModel &model, const std::string &pluginUri, const std::string &name);

    void GatherMediaPaths(PiPedalModel &model, BankFile &bankFile);
    void GatherMediaPaths(PiPedalModel &model, PluginPresets &pluginPreset);

    std::map<std::string, std::string> configFiles;
    std::set<std::string> mediaPaths;
    std::set<std::string> usedPlugins;
    std::vector<std::map<std::string, std::string>> pluginMetadata;

    std::filesystem::path pluginUploadDirectory;
};

void PresetBundleWriterImpl::WriteToFile(const std::filesystem::path &filePath)
{

    ZipFileWriter::ptr zipFile = ZipFileWriter::Create(filePath);

    for (const auto &configFile : configFiles)
    {

        zipFile->WriteFile(configFile.first, configFile.second.data(), configFile.second.size());
    }
    std::string metadata;
    {
        std::ostringstream ss;
        json_writer writer(ss);
        writer.write(this->pluginMetadata);
        metadata = ss.str();
        zipFile->WriteFile("pluginsUsed.json", metadata.data(), metadata.length());
    }

    for (const auto &mediaPath : mediaPaths)
    {
        std::string zipName = (std::filesystem::path("media") / std::filesystem::path(mediaPath)).string();
        std::filesystem::path sourcePath = this->pluginUploadDirectory / std::filesystem::path(mediaPath);
        if (std::filesystem::exists(sourcePath))
        {
            zipFile->WriteFile(zipName, sourcePath);
        }
        else
        {
            Lv2Log::warning(SS("Media file not found: " << sourcePath));
        }
    }
    zipFile->Close();
}

void PresetBundleWriterImpl::GatherMediaPaths(PiPedalModel &model, BankFile &bankFile)
{
    for (auto &preset : bankFile.presets())
    {
        Pedalboard &pedalboard = preset->preset();
        auto items = pedalboard.GetAllPlugins();
        for (auto plugin : items)
        {
            AddUsedPlugin(model, plugin->uri(), plugin->pluginName());
            const Lv2PluginState &state = plugin->lv2State();
            for (const auto &value : state.values_)
            {
                if (value.second.atomType_ == LV2_ATOM__Path)
                {
                    std::string path = ToString(value.second.value_);
                    if (!mediaPaths.contains(path))
                    {
                        mediaPaths.insert(std::move(path));
                    }
                }
            }
        }
    }
}
void PresetBundleWriterImpl::GatherMediaPaths(PiPedalModel &model, PluginPresets &pluginPresets)
{
    AddUsedPlugin(model, pluginPresets.pluginUri_, "");

    for (auto &preset : pluginPresets.presets_)
    {
        const Lv2PluginState &state = preset.state_;
        for (const auto &value : state.values_)
        {
            if (value.second.atomType_ == LV2_ATOM__Path)
            {
                std::string path = ToString(value.second.value_);
                if (!mediaPaths.contains(path))
                {
                    mediaPaths.insert(std::move(path));
                }
            }
        }
    }
}

PresetBundleWriter::ptr PresetBundleWriter::CreatePresetsFile(PiPedalModel &model, const std::string &presetJson)
{
    auto result = std::make_unique<PresetBundleWriterImpl>();
    result->LoadPresets(model, presetJson);

    return result;
}
PresetBundleWriter::ptr PresetBundleWriter::CreatePluginPresetsFile(PiPedalModel &model, const std::string &pluginPresetJson)
{
    auto result = std::make_unique<PresetBundleWriterImpl>();
    result->LoadPluginPresets(model, pluginPresetJson);
    return result;
}

class PresetBundleReaderImpl : public PresetBundleReader
{
public:
    PresetBundleReaderImpl()
    {
    }

    void LoadPresetsFile(PiPedalModel &model, const std::filesystem::path &path)
    {
        this->pluginUploadDirectory = model.GetPluginUploadDirectory();
        zipFile = ZipFileReader::Create(path);
        auto s = zipFile->GetFileInputStream("pluginPresets.json");
        json_reader reader(s);
        reader.read(&pluginPresets);
    }
    void LoadPluginPresetFile(PiPedalModel &model, const std::filesystem::path &path)
    {
        this->pluginUploadDirectory = model.GetPluginUploadDirectory();
        zipFile = ZipFileReader::Create(path);
        auto s = zipFile->GetFileInputStream("bankFile.json");
        json_reader reader(s);
        reader.read(&bankFile);
    }
    virtual ~PresetBundleReaderImpl() noexcept;

    virtual void ExtractMediaFiles() override;
    virtual std::string GetPresetJson() override;
    virtual std::string GetPluginPresetsJson() override;

private:
    void ExtractMediaFile(const std::string &zipFileName);
    bool IsSameFile(const std::string &zipFileName, const std::filesystem::path &filePath)
    {
        return zipFile->CompareFiles(zipFileName, filePath);
    }
    void RenameMediaFileProperty(const std::string oldName, const std::string &newName);

    std::filesystem::path pluginUploadDirectory;
    // BankFile bankFile;
    ZipFileReader::ptr zipFile;

    BankFile bankFile;
    PluginPresets pluginPresets;
};

void PresetBundleReaderImpl::RenameMediaFileProperty(const std::string oldName, const std::string &newName)
{
    for (auto &preset : bankFile.presets())
    {
        Pedalboard &pedalboard = preset->preset();
        auto items = pedalboard.GetAllPlugins();
        for (auto plugin : items)
        {
            Lv2PluginState &state = plugin->lv2State();
            for (auto &value : state.values_)
            {
                if (value.second.atomType_ == LV2_ATOM__Path)
                {
                    std::string path = ToString(value.second.value_);
                    if (path == oldName)
                    {
                        state.values_.at(value.first).value_ = ToBinary(newName);
                    }
                }
            }
        }
    }
    for (auto preset : pluginPresets.presets_)
    {
        Lv2PluginState state = preset.state_;
        for (auto &value : state.values_)
        {
            if (value.second.atomType_ == LV2_ATOM__Path)
            {
                std::string path = ToString(value.second.value_);
                if (path == oldName)
                {
                    state.values_.at(value.first).value_ = ToBinary(newName);
                }
            }
        }
    }
}

PresetBundleReader::ptr PresetBundleReader::LoadPresetsFile(PiPedalModel &model, const std::filesystem::path &filePath)
{
    std::unique_ptr<PresetBundleReaderImpl> result = std::make_unique<PresetBundleReaderImpl>();
    result->LoadPresetsFile(model, filePath);
    return result;
}
PresetBundleReader::ptr PresetBundleReader::LoadPluginPresetsFile(PiPedalModel &model, const std::filesystem::path &filePath)
{
    std::unique_ptr<PresetBundleReaderImpl> result = std::make_unique<PresetBundleReaderImpl>();
    result->LoadPluginPresetsFile(model, filePath);
    return result;
}

void PresetBundleReaderImpl::ExtractMediaFiles()
{
    auto zipFiles = zipFile->GetFiles();
    for (const auto &zipFile : zipFiles)
    {
        if (zipFile != "bankFile.json")
        {
            if (!zipFile.ends_with('/')) // don't process diretories.
            {
                ExtractMediaFile(zipFile);
            }
        }
    }
}

static void ExtractFileVersion(std::string &baseName, int &n)
{
    if (baseName.ends_with(')'))
    {
        size_t endPos = baseName.length() - 1;
        int startPos = baseName.find_last_of('(');
        if (startPos != std::string::npos)
        {
            std::string number = baseName.substr(startPos + 1, endPos - (startPos + 1));
            std::istringstream ss(number);
            ss >> n;
            if (!ss.fail())
            {
                while (startPos > 0 && baseName[startPos - 1] == ' ')
                {
                    --startPos;
                }
                baseName = baseName.substr(0, startPos);
                return;
            }
        }
    }
    n = 0;
}
static std::filesystem::path NextFileName(const std::filesystem::path &fileName)
{
    int n = 0;
    auto fileNameOnly = fileName.filename().string();
    auto extPos = fileNameOnly.find_last_of('.');
    if (extPos == std::string::npos)
    {
        extPos = fileNameOnly.length();
    }
    std::string baseName = fileNameOnly.substr(0, extPos);
    ExtractFileVersion(baseName, n);
    if (n >= 1024)
    {
        // something has gone badly wrong.
        throw new std::runtime_error(SS("Can't increase the version number of " << fileName));
    }
    std::string newFileName = SS(baseName << "(" << (n + 1) << ')' << fileNameOnly.substr(extPos));
    return fileName.parent_path() / newFileName;
}
void PresetBundleReaderImpl::ExtractMediaFile(const std::string &zipFileName)
{
    namespace fs = std::filesystem;
    if (zipFileName.starts_with("media/"))
    {
        std::string baseName = zipFileName.substr(6);
        fs::path targetFileName = this->pluginUploadDirectory / std::filesystem::path(baseName);
        bool renamed = false;
        while (true)
        {
            if (fs::exists(targetFileName))
            {
                if (IsSameFile(zipFileName, targetFileName))
                {
                    break;
                }
                else
                {
                    renamed = true;
                    targetFileName = NextFileName(targetFileName);
                }
            }
            else
            {
                zipFile->ExtractTo(zipFileName, targetFileName);
                break;
            }
        }
        if (renamed)
        {
            std::string newName = fs::path(baseName).parent_path() / targetFileName.filename();
            RenameMediaFileProperty(baseName, newName);
        }
    }
}

std::string PresetBundleReaderImpl::GetPresetJson()
{
    std::ostringstream ss;
    json_writer writer(ss);
    writer.write(this->bankFile);
    return ss.str();
}

std::string PresetBundleReaderImpl::GetPluginPresetsJson()
{
    std::ostringstream ss;
    json_writer writer(ss);
    writer.write(this->pluginPresets);
    return ss.str();
}

PresetBundleReaderImpl::~PresetBundleReaderImpl() noexcept
{
}
PresetBundleReader::~PresetBundleReader() noexcept
{
}

PresetBundleWriterImpl::~PresetBundleWriterImpl() noexcept
{
}
PresetBundleWriter::~PresetBundleWriter() noexcept
{
}

class PluginMetadata
{
public:
    PluginMetadata(std::shared_ptr<Lv2PluginInfo> &pluginInfo)
    {
        metadata_["uri"] = pluginInfo->uri();
        metadata_["name"] = pluginInfo->name();
        metadata_["brand"] = pluginInfo->brand();
        metadata_["authorName"] = pluginInfo->author_name();
        metadata_["authorHomePage"] = pluginInfo->author_homepage();
    }
    PluginMetadata(const std::string &uri, const std::string &name)
    {
        metadata_["uri"] = uri;
        metadata_["name"] = name.empty() ? uri : name;
    }

    std::map<std::string, std::string> metadata_;
};

void PresetBundleWriterImpl::AddUsedPlugin(PiPedalModel &model, const std::string &pluginUri, const std::string &name)
{
    if (!usedPlugins.contains(pluginUri))
    {
        usedPlugins.insert(pluginUri);

        auto pluginInfo = model.GetPluginInfo(pluginUri);

        if (pluginInfo)
        {
            // use dictionaries because it's more version tolerant.
            PluginMetadata metadata(pluginInfo);
            this->pluginMetadata.push_back(std::move(metadata.metadata_));
        }
        else
        {
            PluginMetadata metadata(pluginUri, name);
            this->pluginMetadata.push_back(std::move(metadata.metadata_));
        }
    }
}
