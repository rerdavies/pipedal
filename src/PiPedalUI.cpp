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

using namespace pipedal;

PiPedalUI::PiPedalUI(PluginHost *pHost, const LilvNode *uiNode, const std::filesystem::path &resourcePath)
{
    auto pWorld = pHost->getWorld();

    LilvNodes *fileNodes = lilv_world_find_nodes(pWorld, uiNode, pHost->lilvUris.pipedalUI__fileProperties, nullptr);
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

    LilvNodes *portNotifications = lilv_world_find_nodes(pWorld, uiNode, pHost->lilvUris.ui__portNotification, nullptr);
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

    lilv_nodes_free(fileNodes);
}

UiFileType::UiFileType(PluginHost *pHost, const LilvNode *node)
{
    auto pWorld = pHost->getWorld();

    AutoLilvNode label = lilv_world_get(
        pWorld,
        node,
        pHost->lilvUris.rdfs__label,
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
        pHost->lilvUris.pipedalUI__fileExtension,
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
        pHost->lilvUris.pipedalUI__mimeType,
        nullptr);
    if (mimeType)
    {
        this->mimeType_ = mimeType.AsString();
    }
    if (fileExtension_ == "")
    {
        fileExtension_ = MimeTypes::ExtensionFromMimeType(mimeType_);
    }
    if (mimeType_ == "")
    {
        mimeType_ = MimeTypes::MimeTypeFromExtension(fileExtension_);
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
        pHost->lilvUris.rdfs__label,
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
        pHost->lilvUris.lv2core__index,
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
        pHost->lilvUris.pipedalUI__directory,
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

    AutoLilvNode patchProperty = lilv_world_get(
        pWorld,
        node,
        pHost->lilvUris.pipedalUI__patchProperty,
        nullptr);
    if (patchProperty)
    {
        this->patchProperty_ = patchProperty.AsUri();
    }
    else
    {
        throw std::logic_error("PiPedal FileProperty is missing pipedalui:patchProperty value.");
    }

    AutoLilvNode portGroup = lilv_world_get(pWorld,node,pHost->lilvUris.portgroups__group,nullptr);
    if (portGroup)
    {
        this->portGroup_ = portGroup.AsUri();
    }

    this->fileTypes_ = UiFileType::GetArray(pHost, node, pHost->lilvUris.pipedalUI__fileTypes);
}

std::vector<UiFileType> UiFileType::GetArray(PluginHost *pHost, const LilvNode *node, const LilvNode *uri)
{
    std::vector<UiFileType> result;
    LilvWorld *pWorld = pHost->getWorld();

    LilvNodes *fileTypeNodes = lilv_world_find_nodes(pWorld, node, pHost->lilvUris.pipedalUI__fileTypes, nullptr);
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

bool UiFileProperty::IsValidExtension(const std::string &extension) const
{
    if (fileTypes_.size() == 0)
    {
        return true;
    }

    for (auto &fileType : fileTypes_)
    {
        if (fileType.fileExtension() == extension)
        {
            return true;
        }
    }
    return false;
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

    AutoLilvNode portIndex = lilv_world_get(pWorld, node, pHost->lilvUris.ui__portIndex, nullptr);
    if (!portIndex)
    {
        this->portIndex_ = -1;
    }
    else
    {
        this->portIndex_ = (uint32_t)lilv_node_as_int(portIndex);
    }
    AutoLilvNode symbol = lilv_world_get(pWorld, node, pHost->lilvUris.lv2__symbol, nullptr);
    if (!symbol)
    {
        this->symbol_ = "";
    }
    else
    {
        this->symbol_ = symbol.AsString();
    }
    AutoLilvNode plugin = lilv_world_get(pWorld, node, pHost->lilvUris.ui__plugin, nullptr);
    if (!plugin)
    {
        this->plugin_ = "";
    }
    else
    {
        this->plugin_ = plugin.AsUri();
    }
    AutoLilvNode protocol = lilv_world_get(pWorld, node, pHost->lilvUris.ui__protocol, nullptr);
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

UiFileProperty::UiFileProperty(const std::string &name, const std::string &patchProperty, const std::string &directory)
    : label_(name),
      patchProperty_(patchProperty),
      directory_(directory)
{
}
PiPedalUI::PiPedalUI(std::vector<UiFileProperty::ptr> &&fileProperties)
{
    this->fileProperties_ = std::move(fileProperties);
}

UiFileType::UiFileType(const std::string&label, const std::string &fileType) 
: label_(label)
, fileExtension_(fileType)
{
    if (fileType.starts_with('.'))
    {
        fileExtension_ = fileType;
        mimeType_ = MimeTypes::MimeTypeFromExtension(fileType);
        if (mimeType_ == "")
        {
            mimeType_ = "application/octet-stream";
        }
    } else {
        fileExtension_ = MimeTypes::ExtensionFromMimeType(fileType); // (may be blank, esp. for audio/* and video/*.
        mimeType_ = fileType;
    }
    if (mimeType_ == "*")
    {
        mimeType_ = "application/octet-stream";
    }
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
JSON_MAP_END()