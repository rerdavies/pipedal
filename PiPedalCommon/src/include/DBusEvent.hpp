#pragma once
#include <functional>
#include <atomic>
#include <map>
#include <vector>
#include <mutex>
#include <memory>

namespace impl {
    extern std::atomic<uint64_t> NextHandle;
}

using DBusEventHandle=uint64_t;
constexpr DBusEventHandle INVALID_DBUS_EVENT_HANDLE = 0;


template <typename... ARG_TYPES>
class DBusEvent {
public:
    using Callback=std::function<void (ARG_TYPES...)>;


    DBusEventHandle add(Callback&&cb)
    {
        auto handle = impl::NextHandle.fetch_add(1,std::memory_order::acq_rel);
        {
            std::lock_guard<std::mutex> lock{m};
            callbacks[handle] = std::make_shared<Callback>(std::move(cb));
        }
        return handle;
    }
    void remove(DBusEventHandle h)
    {
        std::lock_guard<std::mutex> lock{m};
        callbacks.erase(h);
    }

    void fire(ARG_TYPES...args)
    {
        std::vector<std::shared_ptr<Callback>> cbs;
        cbs.reserve(callbacks.size());
        {
            std::lock_guard<std::mutex> lock{m};
            for (const auto &cb: callbacks)
            {
                cbs.push_back(cb.second);
            }
        }
        for (auto&cb: cbs)
        {
            (*cb)(args...);
        }
        {
            // destruct shared_ptrs with mutex held.
            std::lock_guard<std::mutex> lock{m};
            cbs.clear(); 
        }
    }
private:
    std::map<DBusEventHandle,std::shared_ptr<Callback>> callbacks;
    std::mutex m;
};
