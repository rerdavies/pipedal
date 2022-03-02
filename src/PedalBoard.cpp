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
#include "PedalBoard.hpp"


using namespace pipedal;


static const PedalBoardItem* GetItem_(const std::vector<PedalBoardItem>&items,long pedalBoardItemId)
{
    for (size_t i = 0; i < items.size(); ++i)
    {
        auto &item = items[i];
        if (items[i].instanceId() == pedalBoardItemId)
        {
            return &(items[i]);
        }
        if (item.isSplit())
        {
            const PedalBoardItem* t = GetItem_(item.topChain(),pedalBoardItemId);
            if (t != nullptr) return t;
            t = GetItem_(item.bottomChain(),pedalBoardItemId);
            if (t != nullptr) return t;
        }
    }
    return nullptr;
}

const PedalBoardItem*PedalBoard::GetItem(long pedalItemId) const
{
    return GetItem_(this->items(),pedalItemId);
}
PedalBoardItem*PedalBoard::GetItem(long pedalItemId)
 {
     return const_cast<PedalBoardItem*>(GetItem_(this->items(),pedalItemId));
 }

ControlValue* PedalBoardItem::GetControlValue(const std::string&symbol)
{
    for (size_t i = 0; i < this->controlValues().size(); ++i)
    {
        if (this->controlValues()[i].key() == symbol)
        {
            return &(this->controlValues()[i]);
        }
    }
    return nullptr;
}

bool PedalBoard::SetItemEnabled(long pedalItemId, bool enabled)
{
    PedalBoardItem*item = GetItem(pedalItemId);
    if (!item) return false;
    if (item->isEnabled() != enabled)
    {
        item->isEnabled(enabled);
        return true;
    }
    return false;

}


bool PedalBoard::SetControlValue(long pedalItemId, const std::string &symbol, float value)
{
    PedalBoardItem*item = GetItem(pedalItemId);
    if (!item) return false;
    ControlValue*controlValue = item->GetControlValue(symbol);
    if (controlValue == nullptr) return false;
    if (controlValue->value() != value)
    {
        controlValue->value(value);
        return true;
    }
    return false;
}


PedalBoardItem PedalBoard::MakeEmptyItem()
{
    uint64_t instanceId = NextInstanceId();

    PedalBoardItem result;
    result.instanceId(instanceId);
    result.uri(EMPTY_PEDALBOARD_ITEM_URI);
    result.pluginName("");
    result.isEnabled(true);
    return result;
}


PedalBoardItem PedalBoard::MakeSplit()
{
    uint64_t instanceId = NextInstanceId();

    PedalBoardItem result;
    result.instanceId(instanceId);
    result.uri(SPLIT_PEDALBOARD_ITEM_URI);
    result.pluginName("");
    result.isEnabled(true);

    result.topChain().push_back(MakeEmptyItem());
    result.bottomChain().push_back(MakeEmptyItem());
    result.controlValues().push_back(
        ControlValue(SPLIT_SPLITTYPE_KEY,0));
    result.controlValues().push_back(
        ControlValue(SPLIT_SELECT_KEY,0));
    result.controlValues().push_back(
        ControlValue(SPLIT_MIX_KEY,0));
    result.controlValues().push_back(
        ControlValue(SPLIT_PANL_KEY,0));
    result.controlValues().push_back(
        ControlValue(SPLIT_VOLL_KEY,-3));
    result.controlValues().push_back(
        ControlValue(SPLIT_PANR_KEY,0));
    result.controlValues().push_back(
        ControlValue(SPLIT_VOLR_KEY,-3));
    
    return result;
}



PedalBoard PedalBoard::MakeDefault()
{
    // copy insanity. but it happens so rarely.
    PedalBoard result;

    auto split = result.MakeSplit();
    split.topChain().push_back(result.MakeEmptyItem());
    split.bottomChain().push_back(result.MakeEmptyItem());


    result.items().push_back(result.MakeEmptyItem());
    result.items().push_back(result.MakeEmptyItem());
    result.items().push_back(result.MakeEmptyItem());

    result.items().push_back(split);

    result.items().push_back(result.MakeEmptyItem());
    result.items().push_back(result.MakeEmptyItem());
    result.items().push_back(result.MakeEmptyItem());
    result.name("Default Preset");

    return result;
}









JSON_MAP_BEGIN(ControlValue)
    JSON_MAP_REFERENCE(ControlValue,key)
    JSON_MAP_REFERENCE(ControlValue,value)
JSON_MAP_END()


bool IsPedalBoardSplitItem(const PedalBoardItem*self, const std::vector<PedalBoardItem>&value)
{
    return self->uri() == SPLIT_PEDALBOARD_ITEM_URI;
}

JSON_MAP_BEGIN(PedalBoardItem)
    JSON_MAP_REFERENCE(PedalBoardItem,instanceId)
    JSON_MAP_REFERENCE(PedalBoardItem,uri)
    JSON_MAP_REFERENCE(PedalBoardItem,isEnabled)
    JSON_MAP_REFERENCE(PedalBoardItem,controlValues)
    JSON_MAP_REFERENCE(PedalBoardItem,pluginName)
    JSON_MAP_REFERENCE_CONDITIONAL(PedalBoardItem,topChain,IsPedalBoardSplitItem)
    JSON_MAP_REFERENCE_CONDITIONAL(PedalBoardItem,bottomChain,&IsPedalBoardSplitItem)
    JSON_MAP_REFERENCE(PedalBoardItem,midiBindings)
JSON_MAP_END()


JSON_MAP_BEGIN(PedalBoard)
    JSON_MAP_REFERENCE(PedalBoard,name)
    JSON_MAP_REFERENCE(PedalBoard,items)
    JSON_MAP_REFERENCE(PedalBoard,nextInstanceId)
JSON_MAP_END()

