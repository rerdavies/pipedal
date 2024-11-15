// Copyright (c) 2024 Robin Davies
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
#include <memory>
#include <functional>
#include <chrono>

namespace pipedal {

    class HotspotManager {
        
        // no move, no delete.
        HotspotManager(const HotspotManager&) = delete;
        HotspotManager(HotspotManager&&) = delete;
        HotspotManager & operator=(const HotspotManager&) = delete;
        HotspotManager & operator=(const HotspotManager&&) = delete;
    protected: 

        HotspotManager() {} // use Create().
    public:
        using clock = std::chrono::steady_clock;
        
        using ptr = std::unique_ptr<HotspotManager>;

        static bool HasWifiDevice();

        static ptr Create();

        virtual ~HotspotManager() noexcept { }

        virtual void Open() = 0;
        virtual void Reload() = 0;
        virtual void Close() = 0;

        virtual std::vector<std::string> GetKnownWifiNetworks() = 0;


        using NetworkChangingListener = std::function<void(bool ethernetConnected,bool hotspotEnabled)>;

        virtual void SetNetworkChangingListener(NetworkChangingListener &&listener) = 0;

        using PostHandle = uint64_t;
        using PostCallback = std::function<void()>;

        virtual PostHandle Post(PostCallback&&fn) = 0;
        virtual PostHandle PostDelayed(const clock::duration&delay,PostCallback&&fn) = 0;
        virtual bool CancelPost(PostHandle handle) = 0;


        template<class REP,class PERIOD> 
        PostHandle PostDelayed(const std::chrono::duration<REP,PERIOD>&delay,PostCallback&&fn)
        {
            return PostDelayed(
                std::chrono::duration_cast<clock::duration,REP,PERIOD>(delay),
                std::move(fn));
        }



    };

}