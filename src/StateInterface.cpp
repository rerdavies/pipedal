// Copyright (c) 2023 Robin Davies
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

#include "StateInterface.hpp"
#include "ss.hpp"
#include "Base64.hpp"
#include "json.hpp"
#include <sstream>
#include <string.h>

using namespace pipedal;

LV2_State_Status StateInterface::FnStateStoreFunction(
    LV2_State_Handle handle,
    uint32_t key,
    const void *value,
    size_t size,
    uint32_t type,
    uint32_t flags)
{
    SaveCallState *pCallState = (SaveCallState *)handle;

    pCallState->status = pCallState->pThis->StateStoreFunction(
        pCallState,
        key,
        value,
        size,
        type,
        flags

    );
    return pCallState->status;
}

LV2_State_Status StateInterface::StateStoreFunction(
    SaveCallState *pCallState,
    uint32_t key,
    const void *value,
    size_t size,
    uint32_t type,
    uint32_t flags)
{
    Lv2PluginState &state = pCallState->state;

    std::string  strKey = map.UridToString(key);
    Lv2PluginStateEntry& entry = state.values_[strKey];

    std::string atomType = map.UridToString(type);
    entry.atomType_ = atomType;
    entry.flags_ = flags;
    entry.value_.resize(size);
    uint8_t *p = (uint8_t*)value;

    for (size_t i = 0; i < size; ++i)
    {
        entry.value_[i] = p[i];
    }
    return LV2_State_Status::LV2_STATE_SUCCESS;
}

static void CheckState(LV2_State_Status status)
{
    const char *lv2Error = nullptr;

    switch (status)
    {
    case LV2_State_Status::LV2_STATE_SUCCESS:
        return;
    default:
        lv2Error = "Unknown error.";
        break;
    case LV2_State_Status::LV2_STATE_ERR_BAD_TYPE:
        lv2Error = "Invalid type.";
        break;
    case LV2_State_Status::LV2_STATE_ERR_BAD_FLAGS:
        lv2Error = "Unsupported flags.";
        break;
    case LV2_State_Status::LV2_STATE_ERR_NO_FEATURE:
        lv2Error = "Feature not supported.";
        break;
    case LV2_State_Status::LV2_STATE_ERR_NO_PROPERTY:
        lv2Error = "No such property.";
        break;
    case LV2_State_Status::LV2_STATE_ERR_NO_SPACE:
        lv2Error = "Insufficient memory.";
        break;
    }
    throw std::logic_error(lv2Error);
}
Lv2PluginState StateInterface::Save()
{
    SaveCallState callState;
    callState.pThis = this;
    callState.status = LV2_State_Status::LV2_STATE_SUCCESS;

    LV2_Feature **features = &(this->features[0]);
    LV2_Handle instanceHandle = lilv_instance_get_handle(pInstance);
    try
    {
        LV2_State_Status status =
            pluginStateInterface->save(
                instanceHandle,
                FnStateStoreFunction,
                (LV2_State_Handle)&callState,
                LV2_STATE_IS_POD | LV2_STATE_IS_PORTABLE,
                features);
        CheckState(status);
        CheckState(callState.status);
        callState.state.isValid_ = true;
    }
    catch (const std::exception &e)
    {
        throw std::logic_error(SS("State save failed. " << e.what()));
    }

    return std::move(callState.state);
}

void StateInterface::RestoreState(LilvState*pLv2State)
{
    lilv_state_restore(pLv2State,this->pInstance,nullptr,nullptr,LV2_STATE_IS_POD,&(features[0]));
}
void StateInterface::Restore(const Lv2PluginState &state)
{
    RestoreCallState callState;
    callState.pThis = this;
    callState.pState = &state;
    callState.status = LV2_State_Status::LV2_STATE_SUCCESS;

    LV2_Feature **features = &(this->features[0]);
    LV2_Handle instanceHandle = lilv_instance_get_handle(pInstance);
    try
    {
        LV2_State_Status status =
            pluginStateInterface->restore(
                instanceHandle,
                FnStateRetreiveFunction,
                (LV2_State_Handle)&callState,
                LV2_STATE_IS_POD,
                features);
        CheckState(status);
        CheckState(callState.status);
    }
    catch (const std::exception &e)
    {
        throw std::logic_error(SS("State restore failed. " << e.what()));
    }
}
/*static*/ const void *StateInterface::FnStateRetreiveFunction(
    LV2_State_Handle handle,
    uint32_t key,
    size_t *size,
    uint32_t *type,
    uint32_t *flags)
{
    RestoreCallState*pCallState = (RestoreCallState*)handle;
    return pCallState->pThis->StateRetrieveFunction(
        pCallState,
        key,
        size,
        type,
        flags);

}
const void *StateInterface::StateRetrieveFunction(
    RestoreCallState *pCallState,
    uint32_t key,
    size_t *size,
    uint32_t *type,
    uint32_t *flags)
{
    std::string strKey = map.UridToString(key);
    auto & values = pCallState->pState->values_;
    auto iEntry = values.find(strKey);
    if (iEntry == values.end())
    {
        *size = 0;
        *type = urids.atom__Atom;
        return nullptr;
    }

    const Lv2PluginStateEntry& entry = iEntry->second;
    *size = entry.value_.size();
    *type = map.GetUrid(entry.atomType_.c_str());
    *flags = entry.flags_;
    return entry.value_.data();

}

void Lv2PluginState::write_json(json_writer &writer) const 
{
    writer.start_array();
    writer.write(isValid_);
    writer.write_raw(",");
    writer.write(values_);
    writer.end_array();
}
void Lv2PluginState::read_json(json_reader &reader) {
    reader.consume('[');
    reader.read(&isValid_);
    reader.consume(',');
    reader.read(&values_);
    reader.consume(']');
}


void Lv2PluginStateEntry::write_json(json_writer &writer) const
{
    writer.start_object();
    writer.write_member("flags",flags_);
    writer.write_raw(",");
    writer.write_member("atomType",atomType_);
    writer.write_raw(",");
    if (atomType_ == LV2_ATOM__String || atomType_ == LV2_ATOM__Path || atomType_ == LV2_ATOM__URI)
    {
        const char *p = (const char*)&(value_[0]);
        size_t size = value_.size();
        while (size > 0 && p[size-1] == '\0')
        {
            --size;
        }
        std::string value { p,size};
        writer.write_member("value",value);
    } else if (atomType_ == LV2_ATOM__Float)
    {
        if (value_.size() != sizeof(float))
        {
            throw std::logic_error("Invalid float property in LV2PluginState");
        }
        writer.write_member("value",*(float*)&(value_[0]));
    } else {
        std::string base64 = macaron::Base64::Encode(value_);
        writer.write_member("value",base64);
    }
    writer.end_object();
}
void Lv2PluginStateEntry::read_json(json_reader &reader)
{
    reader.start_object();
    reader.read_member("flags",&flags_);
    reader.consume(',');
    reader.read_member("atomType",&atomType_);
    reader.consume(',');
    if (atomType_ == LV2_ATOM__String || atomType_ == LV2_ATOM__Path || atomType_ == LV2_ATOM__URI)
    {
        std::string v;
        reader.read_member("value",&v);
        value_.resize(v.length()+1);
        for (size_t i = 0; i < v.length(); ++i)
        {
            value_[i] = (uint8_t)v[i];
        }
        value_[v.length()] = 0;
    } else if (atomType_ == LV2_ATOM__Float)
    {
        float v;
        reader.read_member("value",&v);
        value_.resize(sizeof(float));
        float *pVal = (float*)&(value_[0]);
        *pVal = v;
    } else {
        std::string v;
        reader.read_member("value",&v);
        value_ = macaron::Base64::Decode(v);
    }
    reader.end_object();
}

StateInterface::StateInterface(
    IHost *host,
    const LV2_Feature** features, 
    LilvInstance *pInstance,
    const LV2_State_Interface *pluginStateInterface)
    : map(host->GetMapFeature()),
        pInstance(pInstance),
        pluginStateInterface(pluginStateInterface)
{
    while (*features != nullptr)
    {
        this->features.push_back((LV2_Feature*)(*features));
        ++features;
    }
    this->features.push_back(nullptr);
}

std::string Lv2PluginState::ToString() const {
    std::stringstream ss;
    json_writer writer(ss);
    writer.write(*this);
    return ss.str();
}


bool Lv2PluginStateEntry::operator==(const Lv2PluginStateEntry&other) const
{
    return this->atomType_ == other.atomType_ && this->value_ == other.value_;
}

bool Lv2PluginState::IsEqual(const Lv2PluginState&other) const
{
    if (other.isValid_ != this->isValid_) return false;
    return (other.values_ == this->values_);
}
