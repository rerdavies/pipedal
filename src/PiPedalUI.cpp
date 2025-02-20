/*
 * MIT License
 *
 * Copyright (c) 2023 Robin E. R. Davies
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "PiPedalUI.hpp"
#include "PluginHost.hpp"
#include "ss.hpp"
#include "MimeTypes.hpp"
#include "AutoLilvNode.hpp"
#include "ModFileTypes.hpp"
#include <algorithm>
#include "util.hpp"
#include "MimeTypes.hpp"

using namespace pipedal;


PiPedalUI::PiPedalUI(PluginHost *pHost, const LilvNode *uiNode, const std::filesystem::path &resourcePath)
{
    auto pWorld = pHost->getWorld();

    AutoLilvNodes  fileNodes = lilv_world_find_nodes(pWorld, uiNode, pHost->lilvUris->pipedalUI__fileProperties, nullptr);
    LILV_FOREACH(nodes, i, fileNodes)
    {
        const LilvNode *fileNode = lilv_nodes_get(fileNodes, i);
        try
        {
            UiFileProperty::ptr fileUI = std::make_shared<UiFileProperty>(pHost, fileNode, resourcePath);
            this->fileProperties_.push_back(std::move(fileUI));
        }
        catch (const std::exception &e)
        {
            pHost->LogWarning(SS("Failed to read pipedalui::fileProperties. " << e.what()));
        }
    }
    AutoLilvNodes  frequencyPlotNodes = lilv_world_find_nodes(pWorld, uiNode, pHost->lilvUris->pipedalUI__frequencyPlot, nullptr);
    LILV_FOREACH(nodes, i, frequencyPlotNodes)
    {
        const LilvNode *frequencyPlotNode = lilv_nodes_get(frequencyPlotNodes, i);
        try
        {
            UiFrequencyPlot::ptr frequencyPlotUI =
                std::make_shared<UiFrequencyPlot>(pHost, frequencyPlotNode, resourcePath);
            this->frequencyPlots_.push_back(std::move(frequencyPlotUI));
        }
        catch (const std::exception &e)
        {
            pHost->LogWarning(SS("Failed to read pipedalui::frequencyPlots. " << e.what()));
        }
    }

    AutoLilvNodes portNotifications = lilv_world_find_nodes(pWorld, uiNode, pHost->lilvUris->ui__portNotification, nullptr);
    LILV_FOREACH(nodes, i, portNotifications)
    {
        const LilvNode *portNotificationNode = lilv_nodes_get(portNotifications, i);
        try
        {
            UiPortNotification::ptr portNotification = std::make_shared<UiPortNotification>(pHost, portNotificationNode);
            this->portNotifications_.push_back(std::move(portNotification));
        }
        catch (const std::exception &e)
        {
            pHost->LogWarning(SS("Failed to read ui:portNotifications. " << e.what()));
        }
    }
}

UiFileType::UiFileType(PluginHost *pHost, const LilvNode *node)
{
    auto pWorld = pHost->getWorld();

    AutoLilvNode label = lilv_world_get(
        pWorld,
        node,
        pHost->lilvUris->rdfs__label,
        nullptr);
    if (label)
    {
        this->label_ = label.AsString();
    }
    else
    {
        throw std::logic_error("pipedal_ui:fileType is missing label property.");
    }
    AutoLilvNode fileExtension = lilv_world_get(
        pWorld,
        node,
        pHost->lilvUris->pipedalUI__fileExtension,
        nullptr);
    if (fileExtension)
    {
        this->fileExtension_ = fileExtension.AsString();
    }
    else
    {
        this->fileExtension_ = "";
    }
    AutoLilvNode mimeType = lilv_world_get(
        pWorld,
        node,
        pHost->lilvUris->pipedalUI__mimeType,
        nullptr);
    if (mimeType)
    {
        this->mimeType_ = mimeType.AsString();
    }
    if (fileExtension_ == "")
    {
        fileExtension_ = MimeTypes::instance().ExtensionFromMimeType(mimeType_);
    }
    if (mimeType_ == "")
    {
        mimeType_ = MimeTypes::instance().MimeTypeFromExtension(fileExtension_);
        if (mimeType_ == "")
        {
            mimeType_ = "application/octet-stream";
        }
    }
}



UiFileProperty::UiFileProperty(PluginHost *pHost, const LilvNode *node, const std::filesystem::path &resourcePath)
{
    auto pWorld = pHost->getWorld();

    AutoLilvNode label = lilv_world_get(
        pWorld,
        node,
        pHost->lilvUris->rdfs__label,
        nullptr);
    if (label)
    {
        this->label_ = label.AsString();
    }
    else
    {
        this->label_ = "File";
    }
    AutoLilvNode index = lilv_world_get(
        pWorld,
        node,
        pHost->lilvUris->lv2core__index,
        nullptr);
    if (index)
    {
        this->index_ = index.AsInt(-1);
    }
    else
    {
        this->index_ = -1;
    }

    AutoLilvNode directory = lilv_world_get(
        pWorld,
        node,
        pHost->lilvUris->pipedalUI__directory,
        nullptr);
    if (directory)
    {
        this->directory_ = directory.AsString();
        if (!IsDirectoryNameValid(this->directory_))
        {
            throw std::logic_error("Pipedal FileProperty::directory must have only alpha-numeric characters.");
        }
    }
    if (directory_.length() == 0)
    {
        throw std::logic_error("PipedalUI::fileProperty: must specify at least a directory.");
    }

    this->modDirectories_.push_back(directory_);

    AutoLilvNode patchProperty = lilv_world_get(
        pWorld,
        node,
        pHost->lilvUris->pipedalUI__patchProperty,
        nullptr);
    if (patchProperty)
    {
        this->patchProperty_ = patchProperty.AsUri();
    }
    else
    {
        throw std::logic_error("PiPedal FileProperty is missing pipedalui:patchProperty value.");
    }

    AutoLilvNode portGroup = lilv_world_get(pWorld,node,pHost->lilvUris->portgroups__group,nullptr);
    if (portGroup)
    {
        this->portGroup_ = portGroup.AsUri();
    }
    AutoLilvNode resourceDirectory = lilv_world_get(pWorld,node,pHost->lilvUris->pipedalUI__resourceDirectory,nullptr);
    if (resourceDirectory)
    {
        this->resourceDirectory_ =  resourceDirectory.AsString();
    }
    this->fileTypes_ = UiFileType::GetArray(pHost, node, pHost->lilvUris->pipedalUI__fileTypes);

    PrecalculateFileExtensions();
}

std::vector<UiFileType> UiFileType::GetArray(PluginHost *pHost, const LilvNode *node, const LilvNode *uri)
{
    std::vector<UiFileType> result;
    LilvWorld *pWorld = pHost->getWorld();

    LilvNodes *fileTypeNodes = lilv_world_find_nodes(pWorld, node, pHost->lilvUris->pipedalUI__fileTypes, nullptr);
    LILV_FOREACH(nodes, i, fileTypeNodes)
    {
        const LilvNode *fileTypeNode = lilv_nodes_get(fileTypeNodes, i);
        try
        {
            UiFileType fileType = UiFileType(pHost, fileTypeNode);
            result.push_back(std::move(fileType));
        }
        catch (const std::exception &e)
        {
            pHost->LogError(e.what());
        }
    }
    lilv_nodes_free(fileTypeNodes);
    return result;
}

bool pipedal::IsAlphaNumeric(const std::string &value)
{
    for (char c : value)
    {
        if (
            (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c == '_')

        )
        {
            continue;
        }
        return false;
    }
    return true;
}

bool UiFileProperty::IsDirectoryNameValid(const std::string &value)
{
    if (value.length() == 0)
        return false;
    if (value.find_first_of('/') != std::string::npos)
    {
        return false;
    }
    if (value.find_first_of('\\') != std::string::npos)
    {
        return false;
    }
    if (value.find_first_of("::") != std::string::npos)
    {
        return false;
    }
    if (value.find_first_of(":") != std::string::npos)
    {
        return false;
    }
    return true;
}

// bool UiFileProperty::IsValidExtension(const std::filesystem::path&relativePath,const std::string &extension) const
// {
//     std::string modDirectory = this->getParentModDirectory(relativePath);
//     const auto&validExtensions = GetPermittedFileExtensions(modDirectory);
//     return validExtensions.contains(extension);
// }

std::string UiFileProperty::GetFileExtension(const std::filesystem::path&path)
{
    if (path.has_extension())
    {
        return ToLower(path.extension());
    }
    return "";
}

bool UiFileProperty::IsValidExtension(const std::filesystem::path&relativePath) const
{
    std::string extension = GetFileExtension(relativePath);
    if (this->fileTypes().size() != 0) {
        for (const auto&fileType: fileTypes())
        {
            UiFileType x;
            if (fileType.IsValidExtension(extension)) return true;
        }
        return false;
    }
    if (this->modDirectories().size() != 0)
    {
        std::string modDirectory = this->getParentModDirectory(relativePath);
        const auto&validExtensions = GetPermittedFileExtensions(modDirectory);
        if (validExtensions.contains(extension)) return true;
        return  validExtensions.contains(".*");
    }
    return true;
}

UiPortNotification::UiPortNotification(PluginHost *pHost, const LilvNode *node)
{
    // ui:portNotification
    // [
    //         ui:portIndex 3;
    //         ui:plugin <http://two-play.com/plugins/toob-convolution-reverb>;
    //         ui:protocol ui:floatProtocol;
    //         // pipedal_ui:style pipedal_ui:text ;
    //         // pipedal_ui:redLevel 0;
    //         // pipedal_ui:yellowLevel -12;
    // ]
    LilvWorld *pWorld = pHost->getWorld();

    AutoLilvNode portIndex = lilv_world_get(pWorld, node, pHost->lilvUris->ui__portIndex, nullptr);
    if (!portIndex)
    {
        this->portIndex_ = -1;
    }
    else
    {
        this->portIndex_ = (uint32_t)lilv_node_as_int(portIndex);
    }
    AutoLilvNode symbol = lilv_world_get(pWorld, node, pHost->lilvUris->lv2__symbol, nullptr);
    if (!symbol)
    {
        this->symbol_ = "";
    }
    else
    {
        this->symbol_ = symbol.AsString();
    }
    AutoLilvNode plugin = lilv_world_get(pWorld, node, pHost->lilvUris->ui__plugin, nullptr);
    if (!plugin)
    {
        this->plugin_ = "";
    }
    else
    {
        this->plugin_ = plugin.AsUri();
    }
    AutoLilvNode protocol = lilv_world_get(pWorld, node, pHost->lilvUris->ui__protocol, nullptr);
    if (!protocol)
    {
        this->protocol_ = "";
    }
    else
    {
        this->protocol_ = protocol.AsUri();
    }
    if (this->portIndex_ == -1 && this->symbol_ == "")
    {
        pHost->LogWarning("ui:portNotification specifies neither a ui:portIndex nor an lv2:symbol.");
    }
}

UiFileProperty::UiFileProperty(const std::string&label, const std::string&patchProperty, const ModFileTypes&modFileTypes)
: label_(label),
    patchProperty_(patchProperty),
    directory_("")
{
    setModFileTypes(modFileTypes);
}

void UiFileProperty::setModFileTypes(const ModFileTypes&modFileTypes)
{
    modDirectories_.resize(0);
    fileTypes_.resize(0);
    for (const std::string &type : modFileTypes.fileTypes())
    {
        fileTypes().push_back(UiFileType(SS(type << " file"), SS('.' << type)));
    }

    if (modFileTypes.rootDirectories().size() != 0)
    {
        for (const std::string &rootDirectory : modFileTypes.rootDirectories())
        {
            modDirectories().push_back(rootDirectory);
        }
    } else {
        if (fileTypes().size() == 0)
        {
            // add everything.
            for (const auto&modDirectory: split(ModFileTypes::DEFAULT_FILE_TYPES,','))
            {
                modDirectories().push_back(modDirectory);
            }
        } else {
            // add only those mod directories that contain one or more of our fileTypes.
            for (const auto&modDirectory: split(ModFileTypes::DEFAULT_FILE_TYPES,','))
             {
                modDirectories().push_back(modDirectory);
            }
        }
    }

    PrecalculateFileExtensions();


}


UiFileProperty::UiFileProperty(const std::string &name, const std::string &patchProperty, const std::string &directory)
    : label_(name),
      patchProperty_(patchProperty),
      directory_(directory)
{
}
PiPedalUI::PiPedalUI(
    std::vector<UiFileProperty::ptr> &&fileProperties, 
    std::vector<UiFrequencyPlot::ptr>&& frequencyPlots)
{
    this->fileProperties_ = std::move(fileProperties);
    this->frequencyPlots_ = std::move(frequencyPlots);
}
PiPedalUI::PiPedalUI(
    std::vector<UiFileProperty::ptr> &&fileProperties)
{
    this->fileProperties_ = std::move(fileProperties);
}

bool UiFileType::IsValidExtension(const std::string&extension) const {
    if (fileExtension_.length() != 0)
    {
        if (fileExtension_ == ".*")
        {
            return true;
        }
        return fileExtension_ == extension;
    }
    if (mimeType_.length() != 0)
    {
        return MimeTypes::instance().IsValidExtension(mimeType_,extension);
    }
    return false;
}

UiFileType::UiFileType(const std::string&label, const std::string &fileType) 
: label_(label)
, fileExtension_(fileType)
{
    if (fileType.starts_with('.'))
    {
        fileExtension_ = fileType;
        mimeType_ = MimeTypes::instance().MimeTypeFromExtension(fileType);
        if (mimeType_ == "")
        {
            mimeType_ = "application/octet-stream";
        }
    } else {
        mimeType_ = fileType;
    }
    if (mimeType_ == "*")
    {
        mimeType_ = "application/octet-stream";
    }
}

static float GetFloat(LilvWorld *pWorld,const LilvNode*node,const LilvNode*property,float defaultValue)
{
    AutoLilvNode value = lilv_world_get(
        pWorld,
        node,
        property,
        nullptr);
    return value.AsFloat(defaultValue);
}


UiFrequencyPlot::UiFrequencyPlot(PluginHost*pHost, const LilvNode*node,
    const std::filesystem::path&resourcePath)
{
    auto pWorld = pHost->getWorld();

    AutoLilvNode patchProperty = lilv_world_get(
        pWorld,
        node,
        pHost->lilvUris->pipedalUI__patchProperty,
        nullptr);
    if (patchProperty)
    {
        this->patchProperty_ = patchProperty.AsUri();
    }
    else
    {
        throw std::logic_error("PiPedal FileProperty is missing pipedalui:patchProperty value.");
    }


    AutoLilvNode index = lilv_world_get(
        pWorld,
        node,
        pHost->lilvUris->lv2core__index,
        nullptr);
    if (index)
    {
        this->index_ = index.AsInt(-1);
    }
    else
    {
        this->index_ = -1;
    }

    AutoLilvNode portGroup = lilv_world_get(pWorld,node,pHost->lilvUris->portgroups__group,nullptr);
    if (portGroup)
    {
        this->portGroup_ = portGroup.AsUri();
    }
    this->xLeft_ = GetFloat(pWorld,node,pHost->lilvUris->pipedalUI__xLeft,100);
    this->xRight_ = GetFloat(pWorld,node,pHost->lilvUris->pipedalUI__xRight,22000);
    this->yTop_ = GetFloat(pWorld,node,pHost->lilvUris->pipedalUI__yTop,5);
    this->yBottom_ = GetFloat(pWorld,node,pHost->lilvUris->pipedalUI__yBottom,-35);
    this->xLog_ = GetFloat(pWorld,node,pHost->lilvUris->pipedalUI__xLog,-35);
    this->width_ = GetFloat(pWorld,node,pHost->lilvUris->pipedalUI__width,60);
}


static std::set<std::string> emptySet;

const std::set<std::string>& UiFileProperty::GetPermittedFileExtensions(const std::string &modDirectory) const {
    auto result = fileExtensionsByModDirectory.find(modDirectory);
    if (result != fileExtensionsByModDirectory.end())
    {
        return result->second;
    }
    if (modDirectory == this->directory_)
    {
        std::set<std::string> result;
        for (auto &x : this->fileTypes_)
        {

        }
        return emptySet;
    }
    return emptySet;
}

void UiFileProperty::PrecalculateFileExtensions()
{
    std::set<std::string> explicitlySpecifiedExtensions;
    bool allAudioTypes = false;
    bool allVideoTypes = false;
    for (const auto &fileType: this->fileTypes())
    {
        if (!fileType.fileExtension().empty())
        {
            explicitlySpecifiedExtensions.insert(fileType.fileExtension());
        } else if (fileType.mimeType().length() != 0)
        {
            if (fileType.mimeType() == "audio/*")
            {
                allAudioTypes = true;
            } else if (fileType.mimeType() == "video/*")
            {
                allVideoTypes = true;
            } else {
                // The principle risk being that there is more than one commonly used extension. e.g. midi.
                // If the right extension isn't picked, the client should explicitly list which extensions are wanted
                // instead of using a mime type. :-/ Not a good implementation, but not a use case that's actually going to
                // come up.
                std::string extension =  MimeTypes::instance().ExtensionFromMimeType(fileType.mimeType());
                if (extension.length() == 0)
                {
                    Lv2Log::warning(SS("Unknown MIME type: " << fileType.mimeType()));
                } else {
                    explicitlySpecifiedExtensions.insert(extension);
                }
            }
        }
    }
    if (allAudioTypes)
    {
        const auto&audioExtensions = MimeTypes::instance().AudioExtensions();
        explicitlySpecifiedExtensions.insert(
            audioExtensions.begin(),audioExtensions.end()
        );
    }

    if (allVideoTypes)
    {
        const auto&videoExtensions = MimeTypes::instance().VideoExtensions();
        explicitlySpecifiedExtensions.insert(
            videoExtensions.begin(),videoExtensions.end()
        );

    }
    if (this->modDirectories().size() == 0) // not specified? Add all directories, (and prune later, if we have filetypes).
    {
        this->modDirectories_ = split(ModFileTypes::DEFAULT_FILE_TYPES,',');      
    }
    if (fileTypes().size() != 0)
    {
        // delete any modDirectories that do not contain one of the given filetyeps.
        auto iter = this->modDirectories_.begin();
        while(iter != this->modDirectories_.end())
        {
            const auto *pDirectory = ModFileTypes::GetModDirectory(*iter);
            if (pDirectory == nullptr)
            {
                // a custom directory! How cool is that!?
                this->fileExtensionsByModDirectory[*iter] = explicitlySpecifiedExtensions;
                ++iter;
            } else {
                std::set<std::string> extensions = Intersect(pDirectory->fileExtensions,explicitlySpecifiedExtensions);
                if (extensions.size() == 0)
                {
                    iter = modDirectories_.erase(iter);
                } else {
                    this->fileExtensionsByModDirectory[*iter] = std::move(extensions);
                    ++iter;
                }
            }
        }
    } else {
        auto iter = this->modDirectories_.begin();
        while(iter != this->modDirectories_.end())
        {
            const auto *pDirectory = ModFileTypes::GetModDirectory(*iter);
            if (pDirectory) 
            {
                this->fileExtensionsByModDirectory[*iter] = pDirectory->fileExtensions;
            } else {
                Lv2Log::warning(SS("Unknown Mod Directory: " << *iter));
            }
            ++iter;
        }

    }
}
// std::set<std::string> modDirectoryFileTypes = rootModDirectory->fileExtensions;
    // if (fileProperty.fileTypes().size() != 0)
    // {
    //     std::set<std::string> specifiedFileTypes;

    //     for (const auto&fileType: fileProperty.fileTypes())
    //     {
    //         if (!fileType.fileExtension().empty())
    //         {

    //         }
    //     }
    // }



std::string UiFileProperty::getParentModDirectory(const std::filesystem::path&path) const
{
    for (const auto &modDirectory: this->modDirectories_)
    {
        const auto pModDirectory = ModFileTypes::GetModDirectory(modDirectory);
        if (pModDirectory)
        {
            if (IsChildDirectory(path,pModDirectory->pipedalPath))
            {
                return modDirectory;
            }
        }
    }
    if (this->directory_.length() != 0)
    {
        if (IsChildDirectory(path,this->directory_))
        {
            return this->directory_;
        }   
    }
    return "";
}


JSON_MAP_BEGIN(UiPortNotification)
JSON_MAP_REFERENCE(UiPortNotification, portIndex)
JSON_MAP_REFERENCE(UiPortNotification, symbol)
JSON_MAP_REFERENCE(UiPortNotification, plugin)
JSON_MAP_REFERENCE(UiPortNotification, protocol)
JSON_MAP_END()

JSON_MAP_BEGIN(UiFileType)
JSON_MAP_REFERENCE(UiFileType, label)
JSON_MAP_REFERENCE(UiFileType, mimeType)
JSON_MAP_REFERENCE(UiFileType, fileExtension)
JSON_MAP_END()

JSON_MAP_BEGIN(UiFileProperty)
JSON_MAP_REFERENCE(UiFileProperty, label)
JSON_MAP_REFERENCE(UiFileProperty, index)
JSON_MAP_REFERENCE(UiFileProperty, directory)
JSON_MAP_REFERENCE(UiFileProperty, patchProperty)
JSON_MAP_REFERENCE(UiFileProperty, fileTypes)
JSON_MAP_REFERENCE(UiFileProperty, portGroup)
JSON_MAP_REFERENCE(UiFileProperty, resourceDirectory)
JSON_MAP_REFERENCE(UiFileProperty, modDirectories)
JSON_MAP_REFERENCE(UiFileProperty, useLegacyModDirectory)
JSON_MAP_END()

JSON_MAP_BEGIN(UiFrequencyPlot)
JSON_MAP_REFERENCE(UiFrequencyPlot, patchProperty)
JSON_MAP_REFERENCE(UiFrequencyPlot, index)
JSON_MAP_REFERENCE(UiFrequencyPlot, portGroup)
JSON_MAP_REFERENCE(UiFrequencyPlot, xLeft)
JSON_MAP_REFERENCE(UiFrequencyPlot, xRight)
JSON_MAP_REFERENCE(UiFrequencyPlot, xLog)
JSON_MAP_REFERENCE(UiFrequencyPlot, yTop)
JSON_MAP_REFERENCE(UiFrequencyPlot, yBottom)
JSON_MAP_REFERENCE(UiFrequencyPlot, width)

JSON_MAP_END()