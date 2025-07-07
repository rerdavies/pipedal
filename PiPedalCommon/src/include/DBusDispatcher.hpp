#pragma once
#include <memory>
#include <functional>
#include <thread>
#include <mutex>
#include <vector>
#include <chrono>
#include <atomic>

namespace sdbus {
    class IConnection;
};


class DBusDispatcher {
    DBusDispatcher(const DBusDispatcher&) = delete;
    DBusDispatcher(DBusDispatcher&&) = delete;
    DBusDispatcher & operator=(const DBusDispatcher&) = delete;
    DBusDispatcher & operator=(const DBusDispatcher&&) = delete;
public:
    DBusDispatcher(bool systemBus = true);
    ~DBusDispatcher();

    void Run();
    // Stop everything, shutting down gracefully.
    void Stop();
    void WaitForClose();
    bool IsFinished();

    // Stop, but don't wait. Suitable for use in a signal handler.
    // The callback is executed by the dispatcher thread, and can therefore call non-signal-safe functions.
    void SignalStop(std::function<void(void)> &&callback);

    bool IsStopping();

    using PostHandle = uint64_t;
    using PostCallback = std::function<void()>;
    PostHandle Post(PostCallback&&fn);

    using clock = std::chrono::steady_clock;

    template<class REP,class PERIOD> 
    DBusDispatcher::PostHandle PostDelayed(const std::chrono::duration<REP,PERIOD>&delay,PostCallback&&fn)
    {
        return PostDelayed(
            std::chrono::duration_cast<clock::duration,REP,PERIOD>(delay),
            std::move(fn));
    }

    DBusDispatcher::PostHandle PostDelayed(const clock::duration&delay,PostCallback&&fn);

    bool CancelPost(DBusDispatcher::PostHandle handle);

    sdbus::IConnection&Connection() { return *busConnection;}

private:
    std::atomic<bool> isFinished;
    std::atomic<bool> signalStopRequested;
    bool signalStopExecuted = false;
    std::function<void(void)> signalStopCallback;
    uint64_t NextHandle() {
        return nextHandle.fetch_add(1);
    }
    int GetEventTimeoutMs();
    std::atomic<PostHandle> nextHandle;
    int eventFd = -1;
    bool closed = false;
    void ThreadProc();
    void WakeThread();
    std::unique_ptr<sdbus::IConnection> busConnection;

    struct CallbackEntry {
        PostCallback callback;
        clock::time_point time;
        PostHandle handle;
    };

    std::atomic<bool> stopping;

    std::vector<CallbackEntry> postedEvents;
    bool threadStarted;
    std::thread serviceThread;
    std::mutex postMutex;
};