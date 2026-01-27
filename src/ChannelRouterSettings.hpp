// Copyright (c) 2026 Robin Davies
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

#include <vector>
#include "json.hpp"
#include "Pedalboard.hpp"
#include <memory>

namespace pipedal
{

    class ChannelRouterSettings
    {
    protected:
        static constexpr int64_t CHANNEL_ROUTER_MAIN_INSERT_ID = -4; // Reserved Instance ID for Router Main Inserts.
        static constexpr int64_t CHANNEL_ROUTER_AUX_INSERT_ID = -5;  // Reserved Instance ID for Router Aux inserts.

        bool configured_ = false;
        int64_t channelRouterPresetId_ = -1;

        std::vector<int64_t> mainInputChannels_ = {1, 1};
        std::vector<int64_t> mainOutputChannels_ = {0, 1};
        Pedalboard mainInserts_;

        std::vector<int64_t> auxInputChannels_ = {-1, -1};
        std::vector<int64_t> auxOutputChannels_ = {-1, -1};
        Pedalboard auxInserts_;

        std::vector<int64_t> sendInputChannels_ = {-1, -1};
        std::vector<int64_t> sendOutputChannels_ = {-1, -1};

        std::vector<ControlValue> controlValues_;

    public: 
        using self = ChannelRouterSettings;
        using ptr = std::shared_ptr<self>;

        ChannelRouterSettings();

        uint64_t numberOfAudioInputChannels() const;
        uint64_t numberOfAudioOutputChannels() const;
        
        JSON_GETTER_SETTER(configured)
        JSON_GETTER_SETTER(channelRouterPresetId)
        JSON_GETTER_SETTER_REF(mainInputChannels)
        JSON_GETTER_SETTER_REF(mainOutputChannels)
        JSON_GETTER_SETTER_REF(mainInserts)
        JSON_GETTER_SETTER_REF(auxInputChannels)
        JSON_GETTER_SETTER_REF(auxOutputChannels)
        JSON_GETTER_SETTER_REF(auxInserts)
        JSON_GETTER_SETTER_REF(sendInputChannels)
        JSON_GETTER_SETTER_REF(sendOutputChannels)
        JSON_GETTER_SETTER_REF(controlValues)


        DECLARE_JSON_MAP(ChannelRouterSettings);

    };

        // just the channel selecttions.
    class ChannelSelection {
    public:
        ChannelSelection() = default;
        ChannelSelection(ChannelRouterSettings&settings);
        ChannelSelection(const ChannelSelection&other) = default;
        ChannelSelection(ChannelSelection&&other) = default;
        ChannelSelection& operator=(const ChannelSelection&other) = default;
        ChannelSelection& operator=(ChannelSelection&&other) = default;
        ~ChannelSelection() = default;

        const std::vector<int64_t>&mainInputChannels() const { return mainInputChannels_; }
        const std::vector<int64_t>&mainOutputChannels() const { return mainOutputChannels_; }
        const std::vector<int64_t>&auxInputChannels() const { return auxInputChannels_; }
        const std::vector<int64_t>&auxOutputChannels() const { return auxOutputChannels_; }
        const std::vector<int64_t>&sendInputChannels() const { return sendInputChannels_; }
        const std::vector<int64_t>&sendOutputChannels() const { return sendOutputChannels_; }    

    private:
        void normalizeChannelSelection();

        std::vector<int64_t> mainInputChannels_;
        std::vector<int64_t> mainOutputChannels_;
        std::vector<int64_t> auxInputChannels_;
        std::vector<int64_t> auxOutputChannels_;
        std::vector<int64_t> sendInputChannels_;
        std::vector<int64_t> sendOutputChannels_;
    };

}