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
#include "Pedalboard.hpp"


using namespace pipedal;


static const PedalboardItem* GetItem_(const std::vector<PedalboardItem>&items,long pedalboardItemId)
{
    for (size_t i = 0; i < items.size(); ++i)
    {
        auto &item = items[i];
        if (items[i].instanceId() == pedalboardItemId)
        {
            return &(items[i]);
        }
        if (item.isSplit())
        {
            const PedalboardItem* t = GetItem_(item.topChain(),pedalboardItemId);
            if (t != nullptr) return t;
            t = GetItem_(item.bottomChain(),pedalboardItemId);
            if (t != nullptr) return t;
        }
    }
    return nullptr;
}

static void GetAllItems(std::vector<PedalboardItem*> & result, std::vector<PedalboardItem>&items)
{
    for (auto& item: items)
    {
        if (item.isSplit())
        {
            GetAllItems(result,item.topChain());
            GetAllItems(result,item.bottomChain());
        }
        result.push_back(&item);
    }    
}
std::vector<PedalboardItem*> Pedalboard::GetAllPlugins()
{
    std::vector<PedalboardItem*> result;
    GetAllItems(result,this->items());
    return result;
}


const PedalboardItem*Pedalboard::GetItem(long pedalItemId) const
{
    return GetItem_(this->items(),pedalItemId);
}
PedalboardItem*Pedalboard::GetItem(long pedalItemId)
 {
     return const_cast<PedalboardItem*>(GetItem_(this->items(),pedalItemId));
 }


ControlValue* PedalboardItem::GetControlValue(const std::string&symbol)
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

bool Pedalboard::SetItemEnabled(long pedalItemId, bool enabled)
{
    PedalboardItem*item = GetItem(pedalItemId);
    if (!item) return false;
    if (item->isEnabled() != enabled)
    {
        item->isEnabled(enabled);
        return true;
    }
    return false;

}


bool Pedalboard::SetControlValue(long pedalItemId, const std::string &symbol, float value)
{
    PedalboardItem*item = GetItem(pedalItemId);
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


PedalboardItem Pedalboard::MakeEmptyItem()
{
    uint64_t instanceId = NextInstanceId();

    PedalboardItem result;
    result.instanceId(instanceId);
    result.uri(EMPTY_PEDALBOARD_ITEM_URI);
    result.pluginName("");
    result.isEnabled(true);
    return result;
}


PedalboardItem Pedalboard::MakeSplit()
{
    uint64_t instanceId = NextInstanceId();

    PedalboardItem result;
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



Pedalboard Pedalboard::MakeDefault()
{
    // copy insanity. but it happens so rarely.
    Pedalboard result;

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


bool IsPedalboardSplitItem(const PedalboardItem*self, const std::vector<PedalboardItem>&value)
{
    return self->uri() == SPLIT_PEDALBOARD_ITEM_URI;
}







JSON_MAP_BEGIN(ControlValue)
    JSON_MAP_REFERENCE(ControlValue,key)
    JSON_MAP_REFERENCE(ControlValue,value)
JSON_MAP_END()



JSON_MAP_BEGIN(PedalboardItem)
    JSON_MAP_REFERENCE(PedalboardItem,instanceId)
    JSON_MAP_REFERENCE(PedalboardItem,uri)
    JSON_MAP_REFERENCE(PedalboardItem,isEnabled)
    JSON_MAP_REFERENCE(PedalboardItem,controlValues)
    JSON_MAP_REFERENCE(PedalboardItem,pluginName)
    JSON_MAP_REFERENCE_CONDITIONAL(PedalboardItem,topChain,IsPedalboardSplitItem)
    JSON_MAP_REFERENCE_CONDITIONAL(PedalboardItem,bottomChain,&IsPedalboardSplitItem)
    JSON_MAP_REFERENCE(PedalboardItem,midiBindings)
    JSON_MAP_REFERENCE(PedalboardItem,lv2State)
JSON_MAP_END()


JSON_MAP_BEGIN(Pedalboard)
    JSON_MAP_REFERENCE(Pedalboard,name)
    JSON_MAP_REFERENCE(Pedalboard,items)
    JSON_MAP_REFERENCE(Pedalboard,nextInstanceId)
JSON_MAP_END()

