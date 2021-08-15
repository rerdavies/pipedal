#pragma once

#include "json.hpp"
#include "MidiBinding.hpp"

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

class PedalBoardItem: public JsonWritable {
    long instanceId_ = 0;
    std::string uri_;
    std::string pluginName_;
    bool isEnabled_ = true;
    std::vector<ControlValue> controlValues_;
    std::vector<PedalBoardItem> topChain_;
    std::vector<PedalBoardItem> bottomChain_;
    std::vector<MidiBinding> midiBindings_;
public:
    ControlValue*GetControlValue(const std::string&symbol);
    GETTER_SETTER(instanceId)
    GETTER_SETTER_REF(uri)
    GETTER_SETTER_REF(pluginName)
    GETTER_SETTER(isEnabled)
    GETTER_SETTER_VEC(controlValues)
    GETTER_SETTER_VEC(topChain)
    GETTER_SETTER_VEC(bottomChain)
    GETTER_SETTER_VEC(midiBindings)


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
        writer.write_member("controlValues",controlValues_);
        if (isSplit())
        {
            writer.write_member("topChain",topChain_);
            writer.write_member("bottomChain",bottomChain_);
        }
    }

    DECLARE_JSON_MAP(PedalBoardItem);
};


class PedalBoard {
    std::string name_;
    std::vector<PedalBoardItem> items_;
    long nextInstanceId_ = 0;
    long NextInstanceId() { return ++nextInstanceId_; }

public:
    bool SetControlValue(long pedalItemId, const std::string &symbol, float value);
    bool SetItemEnabled(long pedalItemId, bool enabled);

    PedalBoardItem*GetItem(long pedalItemId);
    const PedalBoardItem*GetItem(long pedalItemId) const;

    bool HasItem(long pedalItemid) const { return GetItem(pedalItemid) != nullptr; }

    GETTER_SETTER_REF(name)
    GETTER_SETTER_VEC(items)


    DECLARE_JSON_MAP(PedalBoard);

    PedalBoardItem MakeEmptyItem();
    PedalBoardItem MakeSplit();


    static PedalBoard MakeDefault();
};

#undef GETTER_SETTER_REF
#undef GETTER_SETTER_VEC
#undef GETTER_SETTER



} // namespace pipedal