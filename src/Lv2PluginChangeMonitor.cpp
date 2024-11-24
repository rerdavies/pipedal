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

#include "Lv2PluginChangeMonitor.hpp"
#include "Lv2Log.hpp"
#include <sys/inotify.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <Finally.hpp>
#include <chrono>
#include <sys/eventfd.h>
#include "PiPedalModel.hpp"
#include "util.hpp"

using namespace pipedal;

Lv2PluginChangeMonitor::Lv2PluginChangeMonitor(PiPedalModel&model)
:model(model)
{
    shutdown_eventfd = eventfd(0, 0);
    monitorThread = std::make_unique<std::thread>([this]() { ThreadProc();});
}

void Lv2PluginChangeMonitor::Shutdown()
{
    if (monitorThread)
    {
        terminateThread = true;
        uint64_t val = 1;
        auto _ = write(shutdown_eventfd,(void*)&val, sizeof(val));
        monitorThread->join();
        monitorThread = nullptr;
        close(shutdown_eventfd);
    }
}

Lv2PluginChangeMonitor::~Lv2PluginChangeMonitor()
{
    Shutdown();
}

void Lv2PluginChangeMonitor::ThreadProc()
{
    SetThreadName("Lv2Change");
    using clock = std::chrono::steady_clock;

    int inotify_fd = inotify_init();
    if (inotify_fd == -1) {
        Lv2Log::error("Failed to initialize inotify");
        return;
    }

    Finally f1 ([inotify_fd]() {
        close(inotify_fd);

    });
    // Add the directory to the inotify watch list
    int watch_descriptor = inotify_add_watch(inotify_fd, "/usr/lib/lv2", IN_MODIFY | IN_CREATE | IN_DELETE);
    if (watch_descriptor == -1) {
        Lv2Log::error("Failed to add directory to inotify watch list");
        return;
    }
    Finally f2([inotify_fd,watch_descriptor]() {
        inotify_rm_watch(inotify_fd, watch_descriptor);
    });

    bool updating = false;
    clock::time_point updateTime;

    // Monitor for file system events
    while (true) {
        struct pollfd pfds[2] = {
            {.fd = inotify_fd, .events = POLLIN},
            {.fd = shutdown_eventfd, .events = POLLIN}
        };
        int ret = poll(pfds, 2,500);  // infinite wait
        if (ret == -1) {
            Lv2Log::error("Error in poll()");
            break;
        }
        if (ret == 0)
        {
            // timeout.
            if (updating && clock::now() >= updateTime)
            {
                updating = false;
                model.OnLv2PluginsChanged();
            }
            continue;
        }
        if (pfds[1].revents & POLLIN) {
            // Shutdown event received
            break;
        } 

        char buffer[4096];
        ssize_t num_bytes = read(inotify_fd, buffer, sizeof(buffer));
        if (num_bytes == -1) {
            Lv2Log::error("Error reading from inotify");
            break;
        }

        size_t i = 0;
        bool updated = false;
        while (i < static_cast<size_t>(num_bytes)) {
            struct inotify_event* event = reinterpret_cast<struct inotify_event*>(&buffer[i]);
            if (event->len > 0) {
                if (event->mask & IN_MODIFY) {
                    updated = true;                    
                } else if (event->mask & IN_CREATE) {
                    updated = true;
                } else if (event->mask & IN_DELETE) {
                    updated = true;
                }
            }
            i += sizeof(struct inotify_event) + event->len;
        }
        if (updated)
        {
            updating = true;
            updateTime = clock::now() + std::chrono::duration_cast<clock::duration>(std::chrono::seconds(5));
        }
    }

    // Clean up
}




