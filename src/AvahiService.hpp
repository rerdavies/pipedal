/*
 * MIT License
 * 
 * Copyright (c) 2022 Robin E. R. Davies
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#include <thread>
#include <string>
#include <atomic>

// forward declarations.
class AvahiEntryGroup;
class AvahiThreadedPoll;
#include <atomic>

#include <avahi-client/client.h>


namespace pipedal {

    class AvahiService {
    public:
        ~AvahiService() { Stop(); }
        void Announce(
            int portNumber, const std::string &name, const std::string &instanceId, const std::string&mdnsName, bool wait);

        void Unannounce(bool wait);
    private: 
        enum class ServiceState {
            Unitialized,
            Initializing,
            Settling,
            Requested,
            Uncommited,
            Registering,
            Collision,
            Reset,
            Established,
            Failed,
            Closed
        };

        std::atomic<ServiceState> serviceState;
        void Wait();
        void WaitForUnannounce();
        void SetState(ServiceState serviceState);

        std::atomic<bool> terminated = false;
        bool stopped = false;
        bool started = false;
        bool createPending = false;
        bool makeAnnouncement = false;

        void Start();
        void Stop();
        static void entry_group_callback(AvahiEntryGroup*g, AvahiEntryGroupState state, void *userData);
        void EntryGroupCallback(AvahiEntryGroup*g, AvahiEntryGroupState state);

        static void client_callback(AvahiClient *c, AvahiClientState state, AVAHI_GCC_UNUSED void * userdata);
        void ClientCallback(AvahiClient *c, AvahiClientState state);


        void create_group(AvahiClient *c);

        int clientErrno = 0;

        int portNumber = -1;
        std::string instanceId;
        std::string serviceName;
        std::string mdnsName;

        AvahiClient *client = NULL;
        AvahiEntryGroup *group = NULL;
        AvahiThreadedPoll *threadedPoll = NULL;
        char*avahiNameString = NULL;

    };

}

