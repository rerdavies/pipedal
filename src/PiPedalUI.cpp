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
#include "PiPedalHost.hpp"

using namespace pipedal;

PiPedalUI::PiPedalUI(PiPedalHost *pHost, const LilvNode *uiNode)
{
    auto pWorld = pHost->getWorld();

    LilvNodes *fileNodes = lilv_world_find_nodes(pWorld, uiNode, pHost->lilvUris.pipedalUI__fileProperties, nullptr);
    LILV_FOREACH(nodes, i, fileNodes)
    {
        const LilvNode *fileNode = lilv_nodes_get(fileNodes, i);
        try
        {
            PiPedalFileProperty::ptr fileUI = std::make_shared<PiPedalFileProperty>(pHost, fileNode);
            this->fileProperites_.push_back(std::move(fileUI));
        }
        catch (const std::exception &e)
        {
            pHost->LogError(e.what());
        }
    }
    lilv_nodes_free(fileNodes);
}

PiPedalFileType::PiPedalFileType(PiPedalHost*pHost, const LilvNode*node) {
    auto pWorld = pHost->getWorld();

    AutoLilvNode name = lilv_world_get(
        pWorld,
        node,
        pHost->lilvUris.lv2Core__name,
        nullptr);
    if (name)
    {
        this->name_ = name.AsString();
    }
    else
    {
        throw std::logic_error("pipedal_ui:fileType is missing name property.");
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
        throw std::logic_error("pipedal_ui:fileType is missing fileExtension property.");
    }

}
PiPedalFileProperty::PiPedalFileProperty(PiPedalHost *pHost, const LilvNode *node)
{
    auto pWorld = pHost->getWorld();

    AutoLilvNode name = lilv_world_get(
        pWorld,
        node,
        pHost->lilvUris.lv2Core__name,
        nullptr);
    if (name)
    {
        this->name_ = name.AsString();
    }
    else
    {
        this->name_ = "File";
    }

    AutoLilvNode directory = lilv_world_get(
        pWorld,
        node,
        pHost->lilvUris.pipedalUI__directory,
        nullptr);
    if (directory)
    {
        this->directory_ = name.AsString();
    }
    else
    {
        throw std::logic_error("PiPedal FileProperty is missing a pipedalui:directory value.");
    }

    AutoLilvNode patchProperty = lilv_world_get(
        pWorld,
        node,
        pHost->lilvUris.pipedalUI__patchProperty,
        nullptr);
    if (patchProperty)
    {
        this->patchProperty_ = patchProperty.AsUri();
    } else {
        throw std::logic_error("PiPedal FileProperty is missing pipedalui:patchProperty value.");
    }
    AutoLilvNode defaultFile = lilv_world_get(
        pWorld,
        node,
        pHost->lilvUris.pipedalUI__defaultFile,
        nullptr);
    this->defaultFile_ = defaultFile.AsString();


    this->fileTypes_ = PiPedalFileType::GetArray(pHost,node,pHost->lilvUris.pipedalUI__fileTypes);
}

std::vector<PiPedalFileType> PiPedalFileType::GetArray(PiPedalHost*pHost, const LilvNode*node,const LilvNode*uri)
{
    std::vector<PiPedalFileType> result;
    LilvWorld* pWorld = pHost->getWorld();

    LilvNodes *fileTypeNodes = lilv_world_find_nodes(pWorld, node, pHost->lilvUris.pipedalUI__fileTypes, nullptr);
    LILV_FOREACH(nodes, i, fileTypeNodes)
    {
        const LilvNode *fileTypeNode = lilv_nodes_get(fileTypeNodes, i);
        try
        {
            PiPedalFileType fileType = PiPedalFileType(pHost, fileTypeNode);
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


JSON_MAP_BEGIN(PiPedalFileType)
    JSON_MAP_REFERENCE(PiPedalFileType,name)
    JSON_MAP_REFERENCE(PiPedalFileType,fileExtension)
JSON_MAP_END()

JSON_MAP_BEGIN(PiPedalFileProperty)
    JSON_MAP_REFERENCE(PiPedalFileProperty,patchProperty)
    JSON_MAP_REFERENCE(PiPedalFileProperty,name)
    JSON_MAP_REFERENCE(PiPedalFileProperty,defaultFile)
    JSON_MAP_REFERENCE(PiPedalFileProperty,fileTypes)
JSON_MAP_END()