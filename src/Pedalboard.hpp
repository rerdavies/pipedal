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

#include "json.hpp"
#include "json_variant.hpp"
#include "MidiBinding.hpp"
#include "StateInterface.hpp"

namespace pipedal {

#define SPLIT_PEDALBOARD_ITEM_URI  "uri://two-play/pipedal/pedalboard#Split"
#define EMPTY_PEDALBOARD_ITEM_URI  "uri://two-play/pipedal/pedalboard#Empty"

#define SPLIT_SPLITTYPE_KEY "splitType"
#define SPLIT_SELECT_KEY "select"
#define SPLIT_MIX_KEY "mix"
#define SPLIT_PANL_KEY "panL"
#define SPLIT_VOLL_KEY "volL"
#define SPLIT_PANR_KEY "panR"
#define SPLIT_VOLR_KEY "volR"


#define GETTER_SETTER_REF(name) \
    const decltype(name##_)& name() const { return name##_;} \
    void name(const decltype(name##_) &value) { name##_ = value; }

#define GETTER_SETTER_VEC(name) \
    decltype(name##_)& name()  { return name##_;}  \
    const decltype(name##_)& name() const  { return name##_;} 



#define GETTER_SETTER(name) \
    decltype(name##_) name() const { return name##_;} \
    void name(decltype(name##_) value) { name##_ = value; }

class ControlValue {
private:
    std::string key_;
    float value_;
public:
    ControlValue()
    {

    }
    ControlValue(const char*key, float value)
    :key_(key)
    , value_(value)
    {

    }
    GETTER_SETTER_REF(key)
    GETTER_SETTER_REF(value)

    DECLARE_JSON_MAP(ControlValue);


};

class PedalboardItem: public JsonMemberWritable {
    int64_t instanceId_ = 0;
    std::string uri_;
    std::string pluginName_;
    bool isEnabled_ = true;
    std::vector<ControlValue> controlValues_;
    std::vector<PedalboardItem> topChain_;
    std::vector<PedalboardItem> bottomChain_;
    std::vector<MidiBinding> midiBindings_;
    std::string vstState_;
    uint32_t stateUpdateCount_ = 0;
    Lv2PluginState lv2State_;
    std::string lilvPresetUri_;
public:
    ControlValue*GetControlValue(const std::string&symbol);
    
    bool hasLv2State() const {
        return lv2State_.isValid_ != 0;
    }
    GETTER_SETTER(instanceId)
    GETTER_SETTER_REF(uri)
    GETTER_SETTER_REF(vstState);
    GETTER_SETTER_REF(pluginName)
    GETTER_SETTER(isEnabled)
    GETTER_SETTER_VEC(controlValues)
    GETTER_SETTER_VEC(topChain)
    GETTER_SETTER_VEC(bottomChain)
    GETTER_SETTER_VEC(midiBindings)
    GETTER_SETTER(stateUpdateCount)
    GETTER_SETTER_REF(lv2State)
    Lv2PluginState&lv2State() { return lv2State_; } // non-const version.
    GETTER_SETTER_REF(lilvPresetUri)


    bool isSplit() const
    {
        return uri_ == SPLIT_PEDALBOARD_ITEM_URI;
    }
    bool isEmpty() const {
        return uri_ == EMPTY_PEDALBOARD_ITEM_URI;
    }

    virtual void write_members(json_writer&writer) const {
        writer.write_member("instanceId",instanceId_);
        writer.write_member("uri",uri_);
        writer.write_member("pluginName",pluginName_);
        writer.write_member("isEnabled",isEnabled_);
        if (isSplit())
        {
            writer.write_member("topChain",topChain_);
            writer.write_member("bottomChain",bottomChain_);
        }
    }

    DECLARE_JSON_MAP(PedalboardItem);
};


class Pedalboard {
    std::string name_;
    float input_volume_db_ = 0;
    float output_volume_db_ = 0;

    std::vector<PedalboardItem> items_;
    uint64_t nextInstanceId_ = 0;
    uint64_t NextInstanceId() { return ++nextInstanceId_; }

public:
    static constexpr int64_t INPUT_VOLUME_ID = -2; // synthetic PedalboardItem for input volume.
    static constexpr int64_t OUTPUT_VOLUME_ID = -3; // synthetic PedalboardItem for output volume.
    bool SetControlValue(int64_t pedalItemId, const std::string &symbol, float value);
    bool SetItemEnabled(int64_t pedalItemId, bool enabled);

    PedalboardItem*GetItem(int64_t pedalItemId);
    const PedalboardItem*GetItem(int64_t pedalItemId) const;
    std::vector<PedalboardItem*>GetAllPlugins();

    bool HasItem(int64_t pedalItemid) const { return GetItem(pedalItemid) != nullptr; }

    GETTER_SETTER_REF(name)
    GETTER_SETTER_VEC(items)
    GETTER_SETTER(input_volume_db)
    GETTER_SETTER(output_volume_db)


    DECLARE_JSON_MAP(Pedalboard);

    PedalboardItem MakeEmptyItem();
    PedalboardItem MakeSplit();


    static Pedalboard MakeDefault();
};

#undef GETTER_SETTER_REF
#undef GETTER_SETTER_VEC
#undef GETTER_SETTER



} // namespace pipedal