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

#pragma once

#include "lilv/lilv.h"
#include "lv2/state.lv2/state.h"
#include <cstddef>
#include "json_variant.hpp"
#include "MapFeature.hpp"
#include "IHost.hpp"

namespace pipedal
{
    class Lv2PluginStateEntry: public JsonSerializable {
    public:
        std::int32_t flags_ = 0;
        std::string atomType_;
        std::vector<uint8_t> value_;
    private:
        virtual void write_json(json_writer &writer) const;
        virtual void read_json(json_reader &reader);

    };
    class Lv2PluginState: public JsonSerializable
    {
    public:
        std::map<std::string,Lv2PluginStateEntry> values_;

        std::string ToString() const;
    private:
        virtual void write_json(json_writer &writer) const;
        virtual void read_json(json_reader &reader);

    };


    // state callback interface for plugin's LV2_State
    class StateInterface
    {
    public:
        StateInterface(IHost *host, LilvInstance *pInstance, const LV2_State_Interface *pluginStateInterface);

    public:
        Lv2PluginState Save();
        void Restore(const Lv2PluginState &state);

    private:
        struct Urids {
        public:
            void Init(MapFeature&map)
            {
                atom__Atom = map.GetUrid(LV2_ATOM__Atom);
                atom__String = map.GetUrid(LV2_ATOM__String);
                atom__Float = map.GetUrid(LV2_ATOM__Float);
            }
            LV2_URID atom__Atom;
            LV2_URID atom__String;
            LV2_URID atom__Float;
        };
        Urids urids;
        struct SaveCallState
        {
            StateInterface *pThis;
            LV2_State_Status status;
            Lv2PluginState state;
        };
        struct RestoreCallState
        {
            StateInterface *pThis;
            LV2_State_Status status;
            const Lv2PluginState *pState;
        };
        static LV2_State_Status FnStateStoreFunction(
            LV2_State_Handle handle,
            uint32_t key,
            const void *value,
            size_t size,
            uint32_t type,
            uint32_t flags);
        LV2_State_Status StateStoreFunction(
            SaveCallState *pCallState,
            uint32_t key,
            const void *value,
            size_t size,
            uint32_t type,
            uint32_t flags);
        static const void *FnStateRetreiveFunction(
            LV2_State_Handle handle,
            uint32_t key,
            size_t *size,
            uint32_t *type,
            uint32_t *flags);
        const void *StateRetrieveFunction(
            RestoreCallState*pCallState,
            uint32_t key,
            size_t *size,
            uint32_t *type,
            uint32_t *flags);

    private:
        MapFeature &map;
        LilvInstance *pInstance;
        const LV2_State_Interface *pluginStateInterface;
        std::vector<LV2_Feature *> features;
    };
}