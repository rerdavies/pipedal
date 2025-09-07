#include "DBusDispatcher.hpp"
#include "DBusLog.hpp"
#include <sdbus-c++/IConnection.h>
#include <poll.h>
#include <sys/eventfd.h>
#include "ss.hpp"
#include <chrono>
#include <cassert>
#include "util.hpp"

DBusDispatcher::DBusDispatcher(bool systemBus)
{
    // SignalStop requires this to be true.

    if (systemBus)
    {
        busConnection = sdbus::createSystemBusConnection();
    }
    else
    {
        busConnection = sdbus::createSessionBusConnection();
    }
}
DBusDispatcher::~DBusDispatcher()
{
    Stop();
    busConnection = nullptr;
}

DBusDispatcher::PostHandle DBusDispatcher::Post(DBusDispatcher::PostCallback &&fn)
{
    auto nextHandle = NextHandle();
    {
        std::lock_guard lock{postMutex};
        this->postedEvents.push_back(CallbackEntry{std::move(fn), clock::time_point::min(), nextHandle});
    }
    WakeThread();
    return nextHandle;
}

bool DBusDispatcher::CancelPost(DBusDispatcher::PostHandle handle)
{
    {
        std::lock_guard lock{postMutex};
        for (auto i = postedEvents.begin(); i != postedEvents.end(); ++i)
        {
            if (i->handle == handle)
            {
                postedEvents.erase(i);
                return true;
            }
        }
        return false;
    }
}

DBusDispatcher::PostHandle DBusDispatcher::PostDelayed(const clock::duration &delay, PostCallback &&fn)
{
    PostHandle nextHandle = NextHandle();
    {
        std::lock_guard lock{postMutex};

        this->postedEvents.emplace_back(
            CallbackEntry{
                std::move(fn),
                clock::now() + delay,
                nextHandle});
    }
    WakeThread();
    return nextHandle;
}

void DBusDispatcher::Run()
{
    this->eventFd = eventfd(0, 0);
    threadStarted = true;
    this->serviceThread = std::thread(
        [this]()
        { this->ThreadProc(); });
}

void DBusDispatcher::WakeThread()
{
    uint64_t val = 1;
    if (eventFd != -1)
    {
        std::ignore = write(eventFd, &val, sizeof(val));
    }
}
void DBusDispatcher::ThreadProc()
{
    pipedal::SetThreadName("dbusd");
    try
    {

        while (true)
        {
            bool idle;
            bool quitting;
            while (true)
            {
                idle = true;
                quitting = false;

                while (busConnection->processPendingEvent())
                {
                    idle = false;
                }
                {
                    // process one event.
                    PostCallback callback;
                    auto now = clock::now();
                    {
                        std::lock_guard lock{postMutex};
                        for (auto iter = postedEvents.begin(); iter != postedEvents.end(); ++iter)
                        {
                            if (now >= iter->time)
                            {
                                callback = std::move(iter->callback);
                                postedEvents.erase(iter);
                                idle = false;
                                break;
                            }
                        }
                        quitting = this->stopping; // transfer with the mutex held.
                    }
                    if (callback)
                    {
                        callback();
                    }
                }
                if (idle)
                {
                    break;
                }
            }
            if (idle && signalStopRequested)
            {
                signalStopRequested = false;
                if (!signalStopExecuted) // only ever do this once.
                {
                    signalStopExecuted = true;
                    std::atomic_thread_fence(std::memory_order::acquire);
                    signalStopCallback();
                    signalStopCallback = std::function<void(void)>();
                    this->stopping = true;
                    idle = false;
                }
            }
            if (idle)
            {
                if (quitting)
                {
                    if (postedEvents.size() == 0)
                    {
                        break;
                    }
                }
                sdbus::IConnection::PollData pollData = busConnection->getEventLoopPollData();

                struct pollfd pollFds[2];

                pollFds[0].fd = pollData.fd;
                pollFds[0].events = pollData.events;
                pollFds[0].revents = 0;
                pollFds[1].fd = this->eventFd;
                pollFds[1].events = POLLIN;
                pollFds[1].revents = 0;
                int pollTimeout = pollData.getPollTimeout();
                int eventTimeout = GetEventTimeoutMs();
                if (eventTimeout != -1 && eventTimeout < pollTimeout)
                {
                    pollTimeout = eventTimeout; /*+-*/
                }
                if (pollTimeout > 250 || pollTimeout == -1)
                {
                    pollTimeout = 250; // poll for quit every 250 ms.
                }
                if (pollTimeout != 0)
                {
                    poll(pollFds, 2, pollTimeout);

                    if (pollFds[1].revents & POLLIN) // received a wakup event?
                    {
                        uint64_t counter;
                        std::ignore = read(pollFds[1].fd, &counter, sizeof(counter)); // reset the wakeup event.
                    }
                }
            }
        }
    }
    catch (const std::exception &e)
    {
        std::string msg = SS("Abnormal termination. " << e.what());
        LogError("DBusDispatcher", "ThreadProc", msg);
    }
    isFinished = true;
    LogTrace("DBusDispatcher", "ThreadProc", "Service thread terminated.");
}

void DBusDispatcher::WaitForClose()
{
    try
    {
        if (threadStarted)
        {
            threadStarted = false;
            serviceThread.join();
        }
    }
    catch (const std::exception &)
    {
    }
    if (eventFd != -1)
    {
        close(eventFd);
        eventFd = -1;
    }
}

bool DBusDispatcher::IsFinished()
{
    return isFinished;
}
void DBusDispatcher::Stop()
{
    if (!closed)
    {
        closed = true;

        this->stopping = true;
        WakeThread();
        WaitForClose();
    }
}

void DBusDispatcher::SignalStop(std::function<void(void)> &&callback)
{
    this->signalStopCallback = std::move(callback);
    std::atomic_thread_fence(::std::memory_order::release);
    this->signalStopRequested = true;
}

int DBusDispatcher::GetEventTimeoutMs()
{
    std::lock_guard lock{postMutex};

    clock::time_point nextTime = clock::time_point::max();
    for (const auto &event : this->postedEvents)
    {
        if (event.time == clock::time_point::min())
            return 0;
        if (event.time < nextTime)
        {
            nextTime = event.time;
        }
    }
    if (nextTime == clock::time_point::max())
    {
        return -1;
    }
    auto clockDuration = nextTime - clock::now();
    auto durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(clockDuration);
    auto ticks = durationMs.count();
    if (ticks > 250)
        return 250;
    if (ticks < 0)
        return 0;
    return (int)ticks;
}

bool DBusDispatcher::IsStopping()
{
    std::lock_guard lock{postMutex};
    return stopping;
}
