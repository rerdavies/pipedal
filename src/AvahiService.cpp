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

using namespace pipedal;

void AvahiService::Announce(
    int portNumber,
    const std::string &name,
    const std::string &instanceId,
    const std::string &mdnsName)
{
    Unannounce();

    this->portNumber = portNumber;
    this->instanceId = instanceId;
    this->mdnsName = mdnsName;

    this->name = avahi_strdup(name.c_str());

    Start();
}

void AvahiService::Unannounce()
{
    Stop();
    if (name)
    {
        avahi_free(name);
        this->name = nullptr;
    }
}

void AvahiService::entry_group_callback(AvahiEntryGroup *g, AvahiEntryGroupState state, void *userData)
{
    ((AvahiService *)userData)->EntryGroupCallback(g, state);
}
void AvahiService::EntryGroupCallback(AvahiEntryGroup *g, AvahiEntryGroupState state)
{
    group = g;
    /* Called whenever the entry group state changes */
    switch (state)
    {
    case AVAHI_ENTRY_GROUP_ESTABLISHED:
        /* The entry group has been established successfully */
        Lv2Log::debug(SS("Service " << name << " successfully established."));
        break;
    case AVAHI_ENTRY_GROUP_COLLISION:
    {
        char *n;
        /* A service name collision with a remote service
             * happened. Let's pick a new name */
        n = avahi_alternative_service_name(name);
        avahi_free(name);
        name = n;
        Lv2Log::debug(SS("Service name collision, renaming service to '" << name << "'\n"));
        /* And recreate the services */
        create_group(avahi_entry_group_get_client(g));
        break;
    }
    case AVAHI_ENTRY_GROUP_FAILURE:
        Lv2Log::error(SS("Entry group failure: " << avahi_strerror(avahi_client_errno(avahi_entry_group_get_client(g))) << "\n"));
        /* Some kind of failure happened while we were registering our services */
        avahi_threaded_poll_quit(threadedPoll);
        break;
    case AVAHI_ENTRY_GROUP_UNCOMMITED:
    case AVAHI_ENTRY_GROUP_REGISTERING:;
    }
}


static int toRawDns(char*rawResult,size_t size, const std::string &name)
{
    // from standard name format to <nn>xyz<nn>foo<nn>com<00> format
    size_t len = name.length();

    rawResult[0] = 0;
    if (len+1 >= size-1)  return 0;

    rawResult[len+1] = 0;
    int count = 0;
    for (int i = len-1; i >= 0; --i)
    {
        if (name[i] == '.')
        {
            rawResult[i+1] = (char)count;
            count = 0;
        } else {
            rawResult[i+1] = name[i];
            ++count;
        }
    }
    rawResult[0] = count;
    return len+1;

}
void AvahiService::create_group(AvahiClient *c)
{
    char *n;
    int ret;
    assert(c);
    /* If this is the first time we're called, let's create a new
     * entry group if necessary */
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
    if (avahi_entry_group_is_empty(group))
    {
        Lv2Log::debug(SS("Adding service '" << name));

        std::string instanceTxtRecord = SS("pipedal_id=" << this->instanceId);

#define PIPEDAL_SERVICE_TYPE "_pipedal._tcp"

        if ((ret = avahi_entry_group_add_service(
                 group,
                 AVAHI_IF_UNSPEC,
                 AVAHI_PROTO_UNSPEC,
                 (AvahiPublishFlags)0,
                 name,
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
    }
    return;
collision:
    /* A service name collision with a local service happened. Let's
     * pick a new name */
    n = avahi_alternative_service_name(name);
    avahi_free(name);
    name = n;
    Lv2Log::warning(SS("Service name collision, renaming service to '" << name << "'"));
    avahi_entry_group_reset(group);
    create_group(c);
    return;
fail:
    avahi_threaded_poll_quit(threadedPoll);
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
            Lv2Log::info("Avahi connection lost. Reconnecting.");

            if (group)
            {
                avahi_entry_group_reset(group);
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
                avahi_threaded_poll_quit(threadedPoll);
            }
        }
        else
        {
            Lv2Log::error(SS("Client failure: " << avahi_strerror(avahi_client_errno(c))));
            avahi_threaded_poll_quit(threadedPoll);
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
        break;
    case AVAHI_CLIENT_CONNECTING:;
    }
}

void AvahiService::Start()
{

    int error;
    int ret = 1;
    struct timeval tv;
    this->clientErrno = 0;
    while (true)
    {
        /* Allocate main loop object */
        if (!(this->threadedPoll = avahi_threaded_poll_new()))
        {
            Lv2Log::error("Failed to create Avahi poll object.");
            goto fail;
        }

        /* Allocate a new client */
        client = avahi_client_new(avahi_threaded_poll_get(threadedPoll), AvahiClientFlags::AVAHI_CLIENT_NO_FAIL, client_callback, (void *)this, &error);
        /* Check wether creating the client object succeeded */
        if (!client)
        {
            Lv2Log::error(SS("Failed to create client: " << avahi_strerror(error)));
            goto fail;
        }

        avahi_threaded_poll_start(threadedPoll);
        ret = 0;
        return;
    fail:
        /* Cleanup things */
        if (client)
        {
            avahi_client_free(client);
            client = nullptr;
        }
        if (group)
        {
            avahi_entry_group_reset(group);
            group = nullptr;
        }
        if (threadedPoll)
        {
            avahi_threaded_poll_free(threadedPoll);
            threadedPoll = nullptr;
        }
    }
    return;
}

void AvahiService::Stop()
{

    if (threadedPoll)
    {
        avahi_threaded_poll_stop(threadedPoll);
    }
    /* Cleanup things */
    // client owns the group?
    // if (group)
    // {
    //     avahi_entry_group_reset(group);
    //     group = nullptr;
    // }`

    if (client)
    {
        avahi_client_free(client);
        client = nullptr;
    }
    if (threadedPoll)
    {
        avahi_threaded_poll_free(threadedPoll);
        threadedPoll = nullptr;
    }
    group = nullptr;
}
