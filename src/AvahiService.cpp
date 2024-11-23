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

#include "AvahiService.hpp"
#include <string.h>
#include <thread>
#include <avahi-client/client.h>
#include <avahi-client/publish.h>
#include <avahi-common/alternative.h>
#include <avahi-common/simple-watch.h>
#include <avahi-common/thread-watch.h>
#include <avahi-common/malloc.h>
#include <avahi-common/error.h>
#include <avahi-common/timeval.h>
#include <unistd.h>
#include "Lv2Log.hpp"
#include "ss.hpp"
#include <chrono>

using namespace pipedal;

void AvahiService::Announce(
    int portNumber,
    const std::string &name,
    const std::string &instanceId,
    const std::string &mdnsName,
    bool wait)
{
    if (terminated)
        return;
    // We should update an existing group if the name doesn't change.
    // but in practice, the name is the only thing that's going to change without restarting the pipedal server.
    if (this->serviceName == name)
    {
        return;
    }

    // this->name = avahi_strdup(name.c_str());
    if (!started)
    {
        this->portNumber = portNumber;
        this->instanceId = instanceId;
        this->mdnsName = mdnsName;
        this->serviceName = name;

        started = true;
        makeAnnouncement = true;
        createPending = true;
        Start();
        if (wait) {
            Wait();
        }
    }
    else
    {
        // already started, so we have to use poll_lock instead.
        avahi_threaded_poll_lock(threadedPoll);

        // all state that we have to protect with the lock now that the avahi serv3ce is running.
        this->portNumber = portNumber;
        this->instanceId = instanceId;
        this->mdnsName = mdnsName;
        this->serviceName = name;
        this->makeAnnouncement = true;

        // we've requested a start but have not created the group.
        if (this->createPending)
        {
            // the first create will use our freshly updated parameters.
            // no action required.
        }
        else
        {
            // otherwise we have to use a lock to effect the update.
            if (group)
            {
                avahi_entry_group_reset(group); // unannounce the previous
                SetState(ServiceState::Reset);
                create_group(client);
            }
            else
            {
                Lv2Log::error("Failed to update mDNS service because of a synch problem.");
            }
        }
        avahi_threaded_poll_unlock(threadedPoll);
        if (wait)
        {
            Wait();
        }
    }
}

void AvahiService::Unannounce(bool wait)
{
    if (terminated)
        return;
    if (this->threadedPoll)
    {
        avahi_threaded_poll_lock(threadedPoll);
        if (started)
        {
            if (this->createPending)
            {
                // if service is starting up, and we haven't yet made our first announcement,
                // just signal that we don't want to announc.
                this->makeAnnouncement = false;
            }
            else
            {
                this->makeAnnouncement = false;
                // we have previously made a successful announcement. Retract it.
                if (group)
                {
                    int rc = avahi_entry_group_reset(group);
                    if (rc < 0)
                    {
                        Lv2Log::error("Avahi: failed to un-announce.");
                    }
                }
            }
        }
        avahi_threaded_poll_unlock(threadedPoll);
        if (wait)
        {
            WaitForUnannounce();
        }
    }
}



void AvahiService::entry_group_callback(AvahiEntryGroup *g, AvahiEntryGroupState state, void *userData)
{
    ((AvahiService *)userData)->EntryGroupCallback(g, state);
}

void AvahiService::SetState(ServiceState serviceState)
{
    this->serviceState = serviceState;
}

void AvahiService::EntryGroupCallback(AvahiEntryGroup *g, AvahiEntryGroupState state)
{
    group = g;
    /* Called whenever the entry group state changes */
    switch (state)
    {
    case AVAHI_ENTRY_GROUP_ESTABLISHED:
        SetState(ServiceState::Established);
        Lv2Log::info(SS("DNS/SD group established."));

        /* The entry group has been established successfully */
        break;
    case AVAHI_ENTRY_GROUP_COLLISION:
    {
        /* A service name collision with a remote service
         * happened. Let's pick a new name */
        SetState(ServiceState::Collision);
        char *n = avahi_alternative_service_name(avahiNameString);
        avahi_free(avahiNameString);
        avahiNameString = n;
        Lv2Log::warning(SS("Service name collision, renaming service to '" << avahiNameString << "'\n"));
        /* And recreate the services */
        create_group(avahi_entry_group_get_client(g));
        break;
    }
    case AVAHI_ENTRY_GROUP_FAILURE:
        Lv2Log::error(SS("Entry group failure: " << avahi_strerror(avahi_client_errno(avahi_entry_group_get_client(g))) << "\n"));
        Lv2Log::error(SS("DNS/SD service shutting down."));
        /* Some kind of failure happened while we were registering our services */
        terminated = true;
        avahi_threaded_poll_quit(threadedPoll);
        SetState(ServiceState::Failed);
        break;
    case AVAHI_ENTRY_GROUP_UNCOMMITED:
        SetState(ServiceState::Uncommited);
        break;
    case AVAHI_ENTRY_GROUP_REGISTERING:;
        SetState(ServiceState::Registering);
        break;

    }
}

static int toRawDns(char *rawResult, size_t size, const std::string &name)
{
    // from standard name format to <nn>xyz<nn>foo<nn>com<00> format
    size_t len = name.length();

    rawResult[0] = 0;
    if (len + 1 >= size - 1)
        return 0;

    rawResult[len + 1] = 0;
    int count = 0;
    for (int i = len - 1; i >= 0; --i)
    {
        if (name[i] == '.')
        {
            rawResult[i + 1] = (char)count;
            count = 0;
        }
        else
        {
            rawResult[i + 1] = name[i];
            ++count;
        }
    }
    rawResult[0] = count;
    return len + 1;
}
void AvahiService::create_group(AvahiClient *c)
{
    this->createPending = false;

    char *n;
    int ret;
    assert(c);
    /* If this is the first time we're called, let's create a new
     * entry group if necessary */
    SetState(ServiceState::Requested);
    if (!group)
    {
        if (!(group = avahi_entry_group_new(c, entry_group_callback, (void *)this)))
        {
            Lv2Log::error(SS("avahi_entry_group_new() failed: " << avahi_strerror(avahi_client_errno(c))));
            goto fail;
        }
    }
    /* If the group is empty (either because it was just created, or
     * because it was reset previously, add our entries.  */
    if (this->makeAnnouncement && avahi_entry_group_is_empty(group))
    {
        Lv2Log::debug(SS("Adding service '" << serviceName << "'"));

        std::string instanceTxtRecord = SS("id=" << this->instanceId);

#define PIPEDAL_SERVICE_TYPE "_pipedal._tcp"

        if (avahiNameString)
        {
            avahi_free(avahiNameString);
            avahiNameString = nullptr;
        }
        avahiNameString = avahi_strdup(serviceName.c_str());
        if ((ret = avahi_entry_group_add_service(
                 group,
                 AVAHI_IF_UNSPEC,
                 AVAHI_PROTO_INET, // IPv4 for now, until we figure out why dnsqmasq borks IPv6 routing.)
                 (AvahiPublishFlags)0,
                 avahiNameString,
                 PIPEDAL_SERVICE_TYPE,
                 NULL,
                 NULL,
                 portNumber,

                 // txt records.
                 instanceTxtRecord.c_str(),
                 NULL)) < 0)
        {
            if (ret == AVAHI_ERR_COLLISION)
                goto collision;
            Lv2Log::error(SS("Failed to add _pipedal._tcp service: " << avahi_strerror(ret)));
            goto fail;
        }

        /* Tell the server to register the service */
        if ((ret = avahi_entry_group_commit(group)) < 0)
        {
            Lv2Log::error(SS("Failed to commit entry group: " << avahi_strerror(ret)));
            goto fail;
        }
        Lv2Log::info(SS("DNS/SD service announced. (" << this->avahiNameString << ")"));
    }
    return;
collision:
    /* A service name collision with a local service happened. Let's
     * pick a new avahiNameString */
    n = avahi_alternative_service_name(avahiNameString);
    avahi_free(avahiNameString);
    avahiNameString = n;
    serviceName = avahiNameString;
    Lv2Log::warning(SS("Service name collision, renaming service to '" << avahiNameString << "'"));
    avahi_entry_group_reset(group);
    create_group(c);
    return;
fail:
    terminated = true;
    avahi_threaded_poll_quit(threadedPoll);
    Lv2Log::error("DNS/SD service shutting down.");
    SetState(ServiceState::Failed);

}

void AvahiService::client_callback(AvahiClient *c, AvahiClientState state, AVAHI_GCC_UNUSED void *userdata)
{
    ((AvahiService *)userdata)->ClientCallback(c, state);
}

void AvahiService::ClientCallback(AvahiClient *c, AvahiClientState state)
{
    assert(c);
    /* Called whenever the client or server state changes */
    switch (state)
    {
    case AVAHI_CLIENT_S_RUNNING:
        /* The server has startup successfully and registered its host
         * name on the network, so it's time to create our services */
        create_group(c);
        break;
    case AVAHI_CLIENT_FAILURE:
        this->clientErrno = avahi_client_errno(c);

        if (this->clientErrno == AVAHI_ERR_DISCONNECTED)
        {
            // tear the client down and restart it
            Lv2Log::info("DNS/SD connection lost. Reconnecting.");

            if (group)
            {
                avahi_entry_group_free(group);
                group = nullptr;
            }

            if (client)
            {
                avahi_client_free(client);
                client = nullptr;
            }
            int error;
            client = avahi_client_new(avahi_threaded_poll_get(threadedPoll), AvahiClientFlags::AVAHI_CLIENT_NO_FAIL, client_callback, (void *)this, &error);
            /* Check wether creating the client object succeeded */
            if (!client)
            {
                Lv2Log::error(SS("Failed to create client: " << avahi_strerror(error)));
                terminated = true;
                avahi_threaded_poll_quit(threadedPoll);
                SetState(ServiceState::Failed);
            }
        }
        else
        {
            terminated = true;
            Lv2Log::error(SS("DNS/SD client failure: " << avahi_strerror(avahi_client_errno(c))));
            avahi_threaded_poll_quit(threadedPoll);
            SetState(ServiceState::Failed);
        }
        break;
    case AVAHI_CLIENT_S_COLLISION:
        /* Let's drop our registered services. When the server is back
         * in AVAHI_SERVER_RUNNING state we will register them
         * again with the new host name. */
    case AVAHI_CLIENT_S_REGISTERING:
        /* The server records are now being established. This
         * might be caused by a host name change. We need to wait
         * for our own records to register until the host name is
         * properly esatblished. */
        if (group)
            avahi_entry_group_reset(group);
        SetState(ServiceState::Settling);
        break;
    case AVAHI_CLIENT_CONNECTING:
        SetState(ServiceState::Settling);
        break;
    }
}

void AvahiService::Start()
{

    int error;
    int ret = 1;
    struct timeval tv;
    this->clientErrno = 0;
    SetState(ServiceState::Initializing);
    for (int retry = 0; retry < 3; ++retry)
    {
        /* Allocate main loop object */
        if (!this->threadedPoll)
        {
            if (!(this->threadedPoll = avahi_threaded_poll_new()))
            {
                Lv2Log::error("Failed to create Avahi poll object.");
                goto fail;
            }
            avahi_threaded_poll_start(threadedPoll);
        }

        if (!client)
        {
            /* Allocate a new client */
            client = avahi_client_new(avahi_threaded_poll_get(threadedPoll), AvahiClientFlags::AVAHI_CLIENT_NO_FAIL, client_callback, (void *)this, &error);
            /* Check wether creating the client object succeeded */
            if (!client)
            {
                Lv2Log::error(SS("Failed to create client: " << avahi_strerror(error)));
                goto fail;
            }
        }

        ret = 0;
        return;
    fail:
        Stop();
        sleep(1);
        SetState(ServiceState::Failed);
    }
    return;
}

void AvahiService::Stop()
{
    if (stopped)
        return;
    this->stopped = true;

    Unannounce(true);
    sleep(1); // let traffic setting. avahi doens't seem to shut down cleany otherwise.

    if (threadedPoll)
    {
        avahi_threaded_poll_lock(threadedPoll);
        if (group)
        {
            avahi_entry_group_free(group);
            group = nullptr;
        }
        avahi_threaded_poll_unlock(threadedPoll);
        avahi_threaded_poll_lock(threadedPoll);
        if (client)
        {
            avahi_client_free(client);
            client = nullptr;
        }
        avahi_threaded_poll_unlock(threadedPoll);

        avahi_threaded_poll_stop(threadedPoll);
        avahi_threaded_poll_free(threadedPoll);
        SetState(ServiceState::Closed);
        threadedPoll = nullptr;
    }

    /* Cleanup things */
    if (avahiNameString)
    {
        avahi_free(avahiNameString);
        avahiNameString = nullptr;
    }
    Lv2Log::info("DNS/SD service stopped.");
}

void AvahiService::Wait()
{
    using clock = std::chrono::steady_clock;
    auto start = clock::now();
    auto waitTime = start + std::chrono::duration_cast<clock::duration>(std::chrono::seconds(5));
    while (true)
    {
        bool done;
        switch (serviceState)
        {
            default:
            case ServiceState::Unitialized:
                done = true;
                break;
            case ServiceState::Initializing:
            case ServiceState::Settling:
            case ServiceState::Requested:
            case ServiceState::Uncommited:
            case ServiceState::Registering:
            case ServiceState::Collision:
                done = false;
                break;
            case ServiceState::Reset:
            case ServiceState::Established:
            case ServiceState::Failed:
            case ServiceState::Closed:
                done = true;
                break;
        };
        if (done) break;
        if (clock::now() > waitTime)
        {
            Lv2Log::error("DNS/SD announcement timed out.");
            done = true;
            break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
}
void AvahiService::WaitForUnannounce()
{
    using clock = std::chrono::steady_clock;
    auto start = clock::now();
    auto waitTime = start + std::chrono::duration_cast<clock::duration>(std::chrono::seconds(5));

    while (true)
    {
        bool done;
        switch (serviceState)
        {
            default:
                throw std::runtime_error("Invalid state");

            case ServiceState::Initializing:
            case ServiceState::Settling:
            case ServiceState::Requested:
            case ServiceState::Registering:
            case ServiceState::Collision:
                done = false;
                break;
            case ServiceState::Uncommited:
            case ServiceState::Unitialized:
            case ServiceState::Reset:
            case ServiceState::Established:
            case ServiceState::Failed:
            case ServiceState::Closed:
                done = true;
                break;
        };
        if (done) break;
        if (clock::now() > waitTime)
        {
            Lv2Log::error("DNS/SD unannounce timed out.");
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

}

