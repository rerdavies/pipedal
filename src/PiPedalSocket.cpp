// Copyright (c) 2022-2023 Robin Davies
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

#include "pch.h"

#include "PiPedalSocket.hpp"
#include "json.hpp"
#include "viewstream.hpp"
#include "PiPedalVersion.hpp"
#include <atomic>
#include <limits>
#include "Lv2Log.hpp"
#include "JackConfiguration.hpp"
#include <future>
#include <atomic>
#include "Ipv6Helpers.hpp"
#include "Promise.hpp"
#include <mutex>

#include "AdminClient.hpp"
#include "WifiConfigSettings.hpp"
#include "WifiDirectConfigSettings.hpp"
#include "WifiChannels.hpp"
#include "SysExec.hpp"
#include "PiPedalAlsa.hpp"
#include <filesystem>

using namespace std;
using namespace pipedal;


class GetPatchPropertyBody {
public:
    uint64_t instanceId_;
    std::string propertyUri_;
    DECLARE_JSON_MAP(GetPatchPropertyBody);
};

JSON_MAP_BEGIN(GetPatchPropertyBody)
JSON_MAP_REFERENCE(GetPatchPropertyBody, instanceId)
JSON_MAP_REFERENCE(GetPatchPropertyBody, propertyUri)
JSON_MAP_END()


class Lv2StateChangedBody {
public:
    uint64_t instanceId_;
    Lv2PluginState state_;
    DECLARE_JSON_MAP(Lv2StateChangedBody);
};

JSON_MAP_BEGIN(Lv2StateChangedBody)
JSON_MAP_REFERENCE(Lv2StateChangedBody, instanceId)
JSON_MAP_REFERENCE(Lv2StateChangedBody, state)
JSON_MAP_END()



class SetPatchPropertyBody {
public:
    uint64_t instanceId_;
    std::string propertyUri_;
    json_variant value_;
    DECLARE_JSON_MAP(SetPatchPropertyBody);
};

JSON_MAP_BEGIN(SetPatchPropertyBody)
JSON_MAP_REFERENCE(SetPatchPropertyBody, instanceId)
JSON_MAP_REFERENCE(SetPatchPropertyBody, propertyUri)
JSON_MAP_REFERENCE(SetPatchPropertyBody, value)
JSON_MAP_END()

class NotifyMidiListenerBody
{
public:
    int64_t clientHandle_;
    bool isNote_;
    int32_t noteOrControl_;
    DECLARE_JSON_MAP(NotifyMidiListenerBody);
};
JSON_MAP_BEGIN(NotifyMidiListenerBody)
JSON_MAP_REFERENCE(NotifyMidiListenerBody, clientHandle)
JSON_MAP_REFERENCE(NotifyMidiListenerBody, isNote)
JSON_MAP_REFERENCE(NotifyMidiListenerBody, noteOrControl)
JSON_MAP_END()

class NotifyAtomOutputBody
{
public:
    int64_t clientHandle_;
    uint64_t instanceId_;
    std::string propertyUri_;
    raw_json_string atomJson_;

    DECLARE_JSON_MAP(NotifyAtomOutputBody);
};
JSON_MAP_BEGIN(NotifyAtomOutputBody)
JSON_MAP_REFERENCE(NotifyAtomOutputBody, clientHandle)
JSON_MAP_REFERENCE(NotifyAtomOutputBody, instanceId)
JSON_MAP_REFERENCE(NotifyAtomOutputBody, propertyUri)
JSON_MAP_REFERENCE(NotifyAtomOutputBody, atomJson)
JSON_MAP_END()

class ListenForMidiEventBody
{
public:
    bool listenForControlsOnly_;
    int64_t handle_;
    DECLARE_JSON_MAP(ListenForMidiEventBody);
};

JSON_MAP_BEGIN(ListenForMidiEventBody)
JSON_MAP_REFERENCE(ListenForMidiEventBody, listenForControlsOnly)
JSON_MAP_REFERENCE(ListenForMidiEventBody, handle)
JSON_MAP_END()

class MonitorPatchPropertyBody
{
public:
    uint64_t instanceId_;
    int64_t clientHandle_;
    std::string propertyUri_;
    DECLARE_JSON_MAP(MonitorPatchPropertyBody);
};

JSON_MAP_BEGIN(MonitorPatchPropertyBody)
JSON_MAP_REFERENCE(MonitorPatchPropertyBody, instanceId)
JSON_MAP_REFERENCE(MonitorPatchPropertyBody, clientHandle)
JSON_MAP_REFERENCE(MonitorPatchPropertyBody,propertyUri)
JSON_MAP_END()

class OnLoadPluginPresetBody
{
public:
    int64_t instanceId_;
    std::vector<ControlValue> controlValues_;
    DECLARE_JSON_MAP(OnLoadPluginPresetBody);
};

JSON_MAP_BEGIN(OnLoadPluginPresetBody)
JSON_MAP_REFERENCE(OnLoadPluginPresetBody, instanceId)
JSON_MAP_REFERENCE(OnLoadPluginPresetBody, controlValues)
JSON_MAP_END()

class LoadPluginPresetBody
{
public:
    uint64_t pluginInstanceId_;
    uint64_t presetInstanceId_;
    DECLARE_JSON_MAP(LoadPluginPresetBody);
};

JSON_MAP_BEGIN(LoadPluginPresetBody)
JSON_MAP_REFERENCE(LoadPluginPresetBody, pluginInstanceId)
JSON_MAP_REFERENCE(LoadPluginPresetBody, presetInstanceId)
JSON_MAP_END()

class FromToBody
{
public:
    int64_t from_ = -1;
    int64_t to_ = -1;

    DECLARE_JSON_MAP(FromToBody);
};

JSON_MAP_BEGIN(FromToBody)
JSON_MAP_REFERENCE(FromToBody, from)
JSON_MAP_REFERENCE(FromToBody, to)
JSON_MAP_END()

class MonitorResultBody
{
public:
    int64_t subscriptionHandle_ = -1;
    float value_;

    DECLARE_JSON_MAP(MonitorResultBody);
};
JSON_MAP_BEGIN(MonitorResultBody)
JSON_MAP_REFERENCE(MonitorResultBody, subscriptionHandle)
JSON_MAP_REFERENCE(MonitorResultBody, value)
JSON_MAP_END()


class MonitorPortBody
{
public:
    int64_t instanceId_ = -1;
    std::string key_;
    float_t updateRate_ = 0;

    DECLARE_JSON_MAP(MonitorPortBody);
};
JSON_MAP_BEGIN(MonitorPortBody)
JSON_MAP_REFERENCE(MonitorPortBody, instanceId)
JSON_MAP_REFERENCE(MonitorPortBody, key)
JSON_MAP_REFERENCE(MonitorPortBody, updateRate)
JSON_MAP_END()

class SaveCurrentPresetAsBody
{
public:
    int64_t clientId_ = -1;
    std::string name_;
    int64_t saveAfterInstanceId_ = -1;

    DECLARE_JSON_MAP(SaveCurrentPresetAsBody);
};
JSON_MAP_BEGIN(SaveCurrentPresetAsBody)
JSON_MAP_REFERENCE(SaveCurrentPresetAsBody, clientId)
JSON_MAP_REFERENCE(SaveCurrentPresetAsBody, name)
JSON_MAP_REFERENCE(SaveCurrentPresetAsBody, saveAfterInstanceId)
JSON_MAP_END()

class SavePluginPresetAsBody
{
public:
    int64_t instanceId_ = -1;
    std::string name_;

    DECLARE_JSON_MAP(SavePluginPresetAsBody);
};
JSON_MAP_BEGIN(SavePluginPresetAsBody)
JSON_MAP_REFERENCE(SavePluginPresetAsBody, instanceId)
JSON_MAP_REFERENCE(SavePluginPresetAsBody, name)
JSON_MAP_END()

class RenameBankBody
{
public:
    int64_t bankId_;
    std::string newName_;
    DECLARE_JSON_MAP(RenameBankBody);
};
JSON_MAP_BEGIN(RenameBankBody)
JSON_MAP_REFERENCE(RenameBankBody, bankId)
JSON_MAP_REFERENCE(RenameBankBody, newName)
JSON_MAP_END()

class RenamePresetBody
{
public:
    int64_t clientId_ = -1;
    int64_t instanceId_ = -1;
    std::string name_;

    DECLARE_JSON_MAP(RenamePresetBody);
};
JSON_MAP_BEGIN(RenamePresetBody)
JSON_MAP_REFERENCE(RenamePresetBody, clientId)
JSON_MAP_REFERENCE(RenamePresetBody, instanceId)
JSON_MAP_REFERENCE(RenamePresetBody, name)
JSON_MAP_END()

class CopyPresetBody
{
public:
    int64_t clientId_ = -1;
    int64_t fromId_ = -1;
    int64_t toId_ = -1;

    DECLARE_JSON_MAP(CopyPresetBody);
};
JSON_MAP_BEGIN(CopyPresetBody)
JSON_MAP_REFERENCE(CopyPresetBody, clientId)
JSON_MAP_REFERENCE(CopyPresetBody, fromId)
JSON_MAP_REFERENCE(CopyPresetBody, toId)
JSON_MAP_END()

class CopyPluginPresetBody
{
public:
    std::string pluginUri_;
    uint64_t instanceId_;

    DECLARE_JSON_MAP(CopyPluginPresetBody);
};
JSON_MAP_BEGIN(CopyPluginPresetBody)
JSON_MAP_REFERENCE(CopyPluginPresetBody, pluginUri)
JSON_MAP_REFERENCE(CopyPluginPresetBody, instanceId)
JSON_MAP_END()

class PedalboardItemEnabledBody
{
public:
    int64_t clientId_ = -1;
    int64_t instanceId_ = -1;
    bool enabled_ = true;

    DECLARE_JSON_MAP(PedalboardItemEnabledBody);
};
JSON_MAP_BEGIN(PedalboardItemEnabledBody)
JSON_MAP_REFERENCE(PedalboardItemEnabledBody, clientId)
JSON_MAP_REFERENCE(PedalboardItemEnabledBody, instanceId)
JSON_MAP_REFERENCE(PedalboardItemEnabledBody, enabled)
JSON_MAP_END()

class UpdateCurrentPedalboardBody
{
public:
    int64_t clientId_ = -1;
    Pedalboard pedalboard_;

    DECLARE_JSON_MAP(UpdateCurrentPedalboardBody);
};

JSON_MAP_BEGIN(UpdateCurrentPedalboardBody)
JSON_MAP_REFERENCE(UpdateCurrentPedalboardBody, clientId)
JSON_MAP_REFERENCE(UpdateCurrentPedalboardBody, pedalboard)
JSON_MAP_END()

class ChannelSelectionChangedBody
{
public:
    int64_t clientId_ = -1;
    JackChannelSelection *jackChannelSelection_ = nullptr;

    DECLARE_JSON_MAP(ChannelSelectionChangedBody);
};
JSON_MAP_BEGIN(ChannelSelectionChangedBody)
JSON_MAP_REFERENCE(ChannelSelectionChangedBody, clientId)
JSON_MAP_REFERENCE(ChannelSelectionChangedBody, jackChannelSelection)
JSON_MAP_END()

class PresetsChangedBody
{
public:
    int64_t clientId_ = -1;
    PresetIndex *presets_ = nullptr;

    DECLARE_JSON_MAP(PresetsChangedBody);
};
JSON_MAP_BEGIN(PresetsChangedBody)
JSON_MAP_REFERENCE(PresetsChangedBody, clientId)
JSON_MAP_REFERENCE(PresetsChangedBody, presets)
JSON_MAP_END()

class ControlChangedBody
{
public:
    int64_t clientId_;
    int64_t instanceId_;
    std::string symbol_;
    float value_;

    DECLARE_JSON_MAP(ControlChangedBody);
};

JSON_MAP_BEGIN(ControlChangedBody)
JSON_MAP_REFERENCE(ControlChangedBody, clientId)
JSON_MAP_REFERENCE(ControlChangedBody, instanceId)
JSON_MAP_REFERENCE(ControlChangedBody, symbol)
JSON_MAP_REFERENCE(ControlChangedBody, value)
JSON_MAP_END()

class PatchPropertyChangedBody
{
public:
    int64_t clientId_;
    int64_t instanceId_;
    std::string propertyUri_;
    json_variant value_;

    DECLARE_JSON_MAP(PatchPropertyChangedBody);
};

JSON_MAP_BEGIN(PatchPropertyChangedBody)
JSON_MAP_REFERENCE(PatchPropertyChangedBody, clientId)
JSON_MAP_REFERENCE(PatchPropertyChangedBody, instanceId)
JSON_MAP_REFERENCE(PatchPropertyChangedBody, propertyUri)
JSON_MAP_REFERENCE(PatchPropertyChangedBody, value)
JSON_MAP_END()

class Vst3ControlChangedBody
{
public:
    int64_t clientId_;
    int64_t instanceId_;
    std::string symbol_;
    float value_;
    std::string state_;

    DECLARE_JSON_MAP(Vst3ControlChangedBody);
};
JSON_MAP_BEGIN(Vst3ControlChangedBody)
JSON_MAP_REFERENCE(Vst3ControlChangedBody, clientId)
JSON_MAP_REFERENCE(Vst3ControlChangedBody, instanceId)
JSON_MAP_REFERENCE(Vst3ControlChangedBody, symbol)
JSON_MAP_REFERENCE(Vst3ControlChangedBody, value)
JSON_MAP_REFERENCE(Vst3ControlChangedBody, state)
JSON_MAP_END()


class PiPedalSocketHandler : public SocketHandler, public IPiPedalModelSubscriber
{
private:
    AdminClient &GetAdminClient() const
    {
        return model.GetAdminClient();
    }
    void RequestShutdown(bool restart);

    std::recursive_mutex writeMutex;
    PiPedalModel &model;
    static std::atomic<uint64_t> nextClientId;
    std::string imageList;

    uint64_t clientId;
    // pedalboard is mutable and not thread-safe.
    std::recursive_mutex subscriptionMutex;
    struct VuSubscription
    {
        int64_t subscriptionHandle;
        int64_t instanceId;
    };
    std::vector<VuSubscription> activeVuSubscriptions;

    struct PortMonitorSubscription
    {
        PortMonitorSubscription(
            int64_t subscriptionHandle,
            int64_t instanceId,
            const std::string& key)
        :subscriptionHandle(subscriptionHandle),
        instanceId(instanceId),
        key(key)
        {

        }
        ~PortMonitorSubscription() {
            Close();
        }
        PortMonitorSubscription() {}
        PortMonitorSubscription(const PortMonitorSubscription&) = delete;
        PortMonitorSubscription& operator=(const PortMonitorSubscription&) = delete;
        void Close() {
            std::lock_guard lock {pmMutex};
            closed = true;
        }


        std::mutex pmMutex;

        bool closed = false;
        int64_t subscriptionHandle = -1;
        int64_t instanceId = -1;
        std::string key;

        static constexpr float INVALID_VALUE = std::numeric_limits<float>::max();
        float lastValue = INVALID_VALUE;
        float currentValue = INVALID_VALUE;

        bool pendingValue = false;
        bool waitingForAck = false;
    };

    std::mutex activePortMonitorsMutex;
    std::vector<std::shared_ptr<PortMonitorSubscription>> activePortMonitors;

public:
    virtual int64_t GetClientId() { return clientId; }

    virtual ~PiPedalSocketHandler()
    {
        for (int i = 0; i < this->activePortMonitors.size(); ++i)
        {
            model.UnmonitorPort(activePortMonitors[i]->subscriptionHandle);
        }
        for (int i = 0; i < this->activeVuSubscriptions.size(); ++i)
        {
            model.RemoveVuSubscription(activeVuSubscriptions[i].subscriptionHandle);
        }

        model.RemoveNotificationSubsription(this);
    }

    virtual void Close()
    {
        SocketHandler::Close();
    }

    PiPedalSocketHandler(PiPedalModel &model)
        : model(model), clientId(++nextClientId)
    {
        std::stringstream imageList;
        const std::filesystem::path &webRoot = model.GetWebRoot() / "img";
        bool firstTime = true;
        try {
            for (const auto &entry : std::filesystem::directory_iterator(webRoot))
            {
                if (!firstTime)
                {
                    imageList << ";";
                }
                firstTime = false;
                imageList << entry.path().filename().string();
            }
        } catch (const std::exception&)
        {
            Lv2Log::error("Can't list files in %s. Image files will not be pre-loaded in the client.", webRoot.c_str());
        }
        this->imageList = imageList.str();
    }

private:
    void JsonReply(int replyTo, const char *message, const char *json)
    {
        std::stringstream s(ios_base::out);

        json_writer writer(s, true);

        writer.start_array();
        {
            writer.start_object();
            {
                if (replyTo != -1)
                {
                    writer.write_member("reply", replyTo);
                    writer.write_raw(",");
                }
                writer.write_member("message", message);
            }
            writer.end_object();
            writer.write_raw(",");
            writer.write_raw(json);
        }
        writer.end_array();

        {
            std::lock_guard<std::recursive_mutex> guard(this->writeMutex);
            this->send(s.str());
        }
    }
    // void JsonSend(const char *message, const char *json)
    // {
    //     JsonReply(-1, message, json);
    // }
    template <typename T>
    void Reply(int replyTo, const char *message, const T &value)
    {
        std::stringstream s(ios_base::out);

        json_writer writer(s, true);
        writer.start_array();
        {
            writer.start_object();
            {
                if (replyTo != -1)
                {
                    writer.write_member("reply", replyTo);
                    writer.write_raw(",");
                }
                writer.write_member("message", message);
            }
            writer.end_object();
            writer.write_raw(",");
            writer.write(value);
        }
        writer.end_array();
        {
            std::lock_guard<std::recursive_mutex> guard(this->writeMutex);
            this->send(s.str());
        }
    }
    void Reply(int replyTo, const char *message)
    {
        if (replyTo == -1)
            return;
        std::stringstream s(ios_base::out);

        json_writer writer(s, true);
        writer.start_array();
        {
            writer.start_object();
            {
                if (replyTo != -1)
                {
                    writer.write_member("reply", replyTo);
                    writer.write_raw(",");
                }
                writer.write_member("message", message);
            }
            writer.end_object();
        }
        writer.end_array();

        {
            std::lock_guard<std::recursive_mutex> guard(this->writeMutex);
            this->send(s.str());
        }
    }

private:
    class IRequestReservation
    {
        int reservationId;

    public:
        IRequestReservation(int reservationId)
            : reservationId(reservationId)
        {
        }
        int GetReservationid() const { return reservationId; }
        virtual ~IRequestReservation() {}
        virtual void onResult(json_reader *pReader) = 0;
        virtual void onError(const std::exception &e) = 0;
    };

    class RequestReservationBase : public IRequestReservation
    {
    protected:
        virtual void onResult(json_reader &reader) = 0;
    };
    template <typename REPLY>
    class RequestReservation : public IRequestReservation
    {
    public:
        using ResponseFn = std::function<void(const REPLY &)>;
        using ErrorFn = std::function<void(const std::exception &)>;

    private:
        ResponseFn responseFn;
        bool hasErrorFn;
        ErrorFn errorFn;

        virtual void onError(const std::exception &e)
        {
            if (hasErrorFn)
            {
                errorFn(e);
            }
            else
            {
                throw e;
            }
        };

        virtual void onResult(json_reader *pReader)
        {
            REPLY value;
            try
            {
                if (pReader == nullptr)
                {
                    throw PiPedalException("No value provide for reply.");
                }
                pReader->read(&value);
                responseFn(value);
            }
            catch (const std::exception &e)
            {
                onError(e);
            }
        };

    public:
        RequestReservation(int reservationId, ResponseFn onResponse, ErrorFn errorFn)
            : IRequestReservation(reservationId), responseFn(onResponse), errorFn(errorFn), hasErrorFn(true)
        {
        }
        RequestReservation(int reservationId, ResponseFn onResponse)
            : IRequestReservation(reservationId), responseFn(onResponse), hasErrorFn(false)
        {
        }
    };
    std::recursive_mutex requestMutex;
    std::vector<IRequestReservation *> requestReservations;
    std::atomic<int> nextRequestId{1};

public:
    template <typename REPLY, typename T>
    void Request(const char *message, const T &body,
                 std::function<void(const REPLY &)> onSuccess,
                 std::function<void(const std::exception &error)> onError)
    {
        try
        {
            RequestReservation<REPLY> *reservation = new RequestReservation<REPLY>(
                (int)++nextRequestId,
                onSuccess,
                onError);
            {
                std::lock_guard<std::recursive_mutex> lock(requestMutex);
                requestReservations.push_back(reservation);
            }
            std::stringstream s(ios_base::out);

            json_writer writer(s, true);
            writer.start_array();
            {
                writer.start_object();
                {
                    writer.write_member("replyTo", reservation->GetReservationid());
                    writer.write_raw(",");
                    writer.write_member("message", message);
                }
                writer.end_object();
                writer.write_raw(",");
                writer.write(body);
            }
            writer.end_array();
            {
                std::lock_guard<std::recursive_mutex> guard(this->writeMutex);
                this->send(s.str());
            }
        }
        catch (const std::exception &e)
        {
            onError(PiPedalException(e.what()));
        }
    }

    template <typename T>
    void Send(const char *message, const T &body)
    {
        Reply(-1, message, body);
    }
    void Send(const char *message)
    {
        Reply(-1, message);
    }

    void SendError(int replyTo, std::exception &e)
    {
        Reply(replyTo, "error", e.what());
    }
    void SendError(int replyTo, const std::string &what)
    {
        Reply(replyTo, "error", what);
    }

    std::shared_ptr<PortMonitorSubscription> getPortMonitorSubscription(uint64_t subscriptionId)
    {
        std::shared_ptr<PortMonitorSubscription> result;
        {
            std::lock_guard lock (activePortMonitorsMutex);

            for (int i = 0; i < this->activePortMonitors.size(); ++i)
            {
                if (activePortMonitors[i]->subscriptionHandle == subscriptionId)
                {
                    result = activePortMonitors[i];
                    break;
                }
            }
        }
        return result;
    }

    void SendMonitorPortMessage(std::shared_ptr<PortMonitorSubscription> &subscription, float value)
    {
        // running on RT_output thread, or on Socket thread.
        {
            std::lock_guard lock {subscription->pmMutex};
            if (subscription->closed) return;
            if (value == subscription->currentValue) 
                return;
            if (subscription->waitingForAck)
            {
                subscription->pendingValue = true;
                subscription->currentValue = value;
                return;
            }
            subscription->currentValue = value;
            subscription->lastValue = value;
            subscription->waitingForAck = true;
        }
        SendMonitorPortMessage_Inner(subscription,value);
    }

    // post-sync send.
    void SendMonitorPortMessage_Inner(std::shared_ptr<PortMonitorSubscription> &subscription, float value)
    {
        auto subscriptionHandle_ = subscription->subscriptionHandle;
        MonitorResultBody body;
        body.subscriptionHandle_ = subscriptionHandle_;
        body.value_ = value;
        this->Request<bool, MonitorResultBody>( // send the message and wait for a response.
            "onMonitorPortOutput",
            body,
            [this, subscriptionHandle_](const bool &result)
            {
                // running on PiPedalSocket thread.
                std::shared_ptr<PortMonitorSubscription> subscription = getPortMonitorSubscription(subscriptionHandle_);
                if (!subscription) return;

                float value;
                {
                    std::unique_lock lock {subscription->pmMutex};
                    if (subscription->closed) return;
                    if (subscription->pendingValue)
                    {
                        value = subscription->currentValue;
                        subscription->lastValue = value;
                        subscription->pendingValue = false;
                        subscription->waitingForAck = true;

                        lock.unlock();

                        SendMonitorPortMessage_Inner(subscription, value);
                        return;
                    }
                    else
                    {
                        subscription->waitingForAck = false;
                    }
                }
            },
            [](const std::exception &e)
            {
                Lv2Log::debug("Failed to monitor port output. (%s)", e.what());
            });
    }
    void MonitorPort(int replyTo,MonitorPortBody &body)
    {
        std::lock_guard<std::recursive_mutex> guard(subscriptionMutex);
        int64_t subscriptionHandle = model.MonitorPort(
            body.instanceId_,
            body.key_,
            body.updateRate_,
            [this](int64_t subscriptionHandle_, float value)
            {
                std::shared_ptr<PortMonitorSubscription> subscription = getPortMonitorSubscription(subscriptionHandle_);

                if (subscription)
                {
                    SendMonitorPortMessage(subscription, value);
                }
            }

        );
        {
            std::lock_guard lock(activePortMonitorsMutex);
            activePortMonitors.push_back(std::make_shared<PortMonitorSubscription>(subscriptionHandle, body.instanceId_, body.key_));
        }

        this->Reply(replyTo, "monitorPort", subscriptionHandle);
    }

    /***********************/

    void handleMessage(int reply, int replyTo, const std::string &message, json_reader *pReader)
    {
        if (reply != -1)
        {
            IRequestReservation *reservation = nullptr;
            {
                std::lock_guard<std::recursive_mutex> guard(this->requestMutex);
                for (auto i = this->requestReservations.begin(); i != this->requestReservations.end(); ++i)
                {
                    if ((*i)->GetReservationid() == reply)
                    {
                        reservation = (*i);
                        requestReservations.erase(i);
                        break;
                    }
                }
            }
            // be careful not to take down the whole server because of a client that's shutting down.
            if (reservation != nullptr)
            {
                try
                {
                    reservation->onResult(pReader);
                }
                catch (const std::exception &e)
                {
                    Lv2Log::error("Socket: Invalid reply '%s'. (%s)", message.c_str(), e.what());
                }
                delete reservation;
            }
            else
            {
                Lv2Log::warning("Socket: Reply '%s' with nobody waiting.", message.c_str());
            }
            return;
        }
        if (message == "setControl")
        {
            ControlChangedBody message;
            pReader->read(&message);
            this->model.SetControl(message.clientId_, message.instanceId_, message.symbol_, message.value_);
        } 
        else if (message == "previewControl")
        {
            ControlChangedBody message;
            pReader->read(&message);
            this->model.PreviewControl(message.clientId_, message.instanceId_, message.symbol_, message.value_);
        } else if (message == "setControl")
        {
            ControlChangedBody message;
            pReader->read(&message);
            this->model.SetControl(message.clientId_, message.instanceId_, message.symbol_, message.value_);
        } 
        else if (message == "setInputVolume")
        {
            float value;
            pReader->read(&value);
            this->model.SetInputVolume(value);
        }
        else if (message == "setOutputVolume")
        {
            float value;
            pReader->read(&value);
            this->model.SetOutputVolume(value);
        }
        else if (message == "previewInputVolume")
        {
            float value;
            pReader->read(&value);
            this->model.PreviewInputVolume(value);
        }
        else if (message == "previewOutputVolume")
        {
            float value;
            pReader->read(&value);
            this->model.PreviewOutputVolume(value);
        }

        else if (message == "listenForMidiEvent")
        {
            ListenForMidiEventBody body;
            pReader->read(&body);
            this->model.ListenForMidiEvent(this->clientId, body.handle_, body.listenForControlsOnly_);
        }
        else if (message == "cancelListenForMidiEvent")
        {
            uint64_t handle;
            pReader->read(&handle);
            this->model.CancelListenForMidiEvent(this->clientId, handle);
        }
        else if (message == "monitorPatchProperty")
        {
            MonitorPatchPropertyBody body;
            pReader->read(&body);
            this->model.MonitorPatchProperty(this->clientId, body.clientHandle_, body.instanceId_, body.propertyUri_);
        }
        else if (message == "cancelMonitorPatchProperty")
        {
            int64_t handle;
            pReader->read(&handle);
            this->model.CancelMonitorPatchProperty(this->clientId, handle);
        }

        else if (message == "getJackStatus")
        {
            JackHostStatus status = model.GetJackStatus();
            this->Reply(replyTo, "getJackStatus", status);
        }
        else if (message == "getAlsaDevices")
        {
            std::vector<AlsaDeviceInfo> devices = model.GetAlsaDevices();
            this->Reply(replyTo, "getAlsaDevices", devices);
        }
        else if (message == "getWifiChannels")
        {
            std::string country;
            pReader->read(&country);
            std::vector<WifiChannel> channels = pipedal::getWifiChannels(country.c_str());
            this->Reply(replyTo, "getWifiChannels", channels);
        }
        else if (message == "getPluginPresets")
        {
            std::string uri;
            pReader->read(&uri);
            this->Reply(replyTo, "getPluginPresets", this->model.GetPluginUiPresets(uri));
        }
        else if (message == "loadPluginPreset")
        {
            LoadPluginPresetBody body;
            pReader->read(&body);
            this->model.LoadPluginPreset(body.pluginInstanceId_, body.presetInstanceId_);
        }
        else if (message == "setJackServerSettings")
        {
            JackServerSettings jackServerSettings;
            pReader->read(&jackServerSettings);
            this->model.SetJackServerSettings(jackServerSettings);
            this->Reply(replyTo, "setJackserverSettings");
        }
        else if (message == "setGovernorSettings")
        {
            std::string governor;
            pReader->read(&governor);
            std::string fromAddress = this->getFromAddress();
            // if (!IsOnLocalSubnet(fromAddress))
            // {
            //     throw PiPedalException("Permission denied. Not on local subnet.");
            // }
            this->model.SetGovernorSettings(governor);
            this->Reply(replyTo, "setGovernorSettings");
        }
        else if (message == "setWifiConfigSettings")
        {
            WifiConfigSettings wifiConfigSettings;
            pReader->read(&wifiConfigSettings);
            if (!GetAdminClient().CanUseShutdownClient())
            {
                throw PiPedalException("Can't change server settings when running interactively.");
            }
            std::string fromAddress = this->getFromAddress();
            // if (!IsOnLocalSubnet(fromAddress))
            // {
            //     throw PiPedalException("Permission denied. Not on local subnet.");
            // }

            this->model.SetWifiConfigSettings(wifiConfigSettings);
            this->Reply(replyTo, "setWifiConfigSettings");
        }
        else if (message == "getWifiConfigSettings")
        {
            this->Reply(replyTo, "getWifiConfigSettings", model.GetWifiConfigSettings());
        }
        else if (message == "setWifiDirectConfigSettings")
        {
            WifiDirectConfigSettings wifiDirectConfigSettings;
            pReader->read(&wifiDirectConfigSettings);
            if (!GetAdminClient().CanUseShutdownClient())
            {
                throw PiPedalException("Can't change server settings when running interactively.");
            }
            std::string fromAddress = this->getFromAddress();
            // if (!IsOnLocalSubnet(fromAddress))
            // {
            //     throw PiPedalException("Permission denied. Not on local subnet.");
            // }

            this->model.SetWifiDirectConfigSettings(wifiDirectConfigSettings);
            this->Reply(replyTo, "setWifiDirectConfigSettings");
        }
        else if (message == "getWifiDirectConfigSettings")
        {
            this->Reply(replyTo, "getWifiDirectConfigSettings", model.GetWifiDirectConfigSettings());
        }

        else if (message == "getGovernorSettings")
        {
            this->Reply(replyTo, "getGovernorSettings", model.GetGovernorSettings());
        }

        else if (message == "getJackServerSettings")
        {
            this->Reply(replyTo, "getJackServerSettings", model.GetJackServerSettings());
        }
        else if (message == "getBankIndex")
        {
            BankIndex bankIndex = model.GetBankIndex();
            this->Reply(replyTo, "getBankIndex", bankIndex);
        }
        else if (message == "getJackConfiguration")
        {
            JackConfiguration configuration = this->model.GetJackConfiguration();
            this->Reply(replyTo, "getJackConfiguration", configuration);
        }
        else if (message == "getJackSettings")
        {
            JackChannelSelection selection = this->model.GetJackChannelSelection();
            this->Reply(replyTo, "getJackSettings", selection);
        }
        else if (message == "saveCurrentPreset")
        {
            this->model.SaveCurrentPreset(this->clientId);
        }
        else if (message == "saveCurrentPresetAs")
        {
            SaveCurrentPresetAsBody body;
            pReader->read(&body);
            int64_t result = this->model.SaveCurrentPresetAs(this->clientId, body.name_, body.saveAfterInstanceId_);
            Reply(replyTo, "saveCurrentPresetsAs", result);
        }
        else if (message == "savePluginPresetAs")
        {
            SavePluginPresetAsBody body;
            pReader->read(&body);
            int64_t result = this->model.SavePluginPresetAs(body.instanceId_, body.name_);
            Reply(replyTo, "saveCurrentPresetsAs", result);
        }
        else if (message == "getPresets")
        {
            PresetIndex presets;
            this->model.GetPresets(&presets);
            Reply(replyTo, "getPresets", presets);
        }
        else if (message == "setPedalboardItemEnable")
        {
            PedalboardItemEnabledBody body;
            pReader->read(&body);
            model.SetPedalboardItemEnable(body.clientId_, body.instanceId_, body.enabled_);
        }
        else if (message == "updateCurrentPedalboard")
        {
            {
                UpdateCurrentPedalboardBody body;

                pReader->read(&body);
                this->model.UpdateCurrentPedalboard(body.clientId_, body.pedalboard_);
            }
        }
        else if (message == "currentPedalboard")
        {
            auto pedalboard = model.GetCurrentPedalboardCopy();
            Reply(replyTo, "currentPedalboard", pedalboard);
        }
        else if (message == "plugins")
        {
            auto ui_plugins = model.GetLv2Host().GetUiPlugins();
            Reply(replyTo, "plugins", ui_plugins);
        }
        else if (message == "pluginClasses")
        {
            auto classes = model.GetLv2Host().GetLv2PluginClass();
            Reply(replyTo, "pluginClasses", classes);
        }
        else if (message == "hello")
        {
            this->model.AddNotificationSubscription(this);
            Reply(replyTo, "ehlo", clientId);
        }
        else if (message == "setJackSettings")
        {
            JackChannelSelection jackSettings;
            pReader->read(&jackSettings);
            this->model.SetJackChannelSelection(this->clientId, jackSettings);
        }
        else if (message == "setShowStatusMonitor")
        {
            bool showStatusMonitor;
            pReader->read(&showStatusMonitor);
            this->model.SetShowStatusMonitor(showStatusMonitor);
        }
        else if (message == "getShowStatusMonitor")
        {
            Reply(replyTo, "getShowStatusMonitor", this->model.GetShowStatusMonitor());
        }
        else if (message == "version")
        {
            PiPedalVersion version(this->model);

            Reply(replyTo, "version", version);
        }
        else if (message == "loadPreset")
        {
            int64_t instanceId = 0;
            pReader->read(&instanceId);
            model.LoadPreset(this->clientId, instanceId);
        }
        else if (message == "updatePresets")
        {
            PresetIndex newIndex;
            pReader->read(&newIndex);
            bool result = model.UpdatePresets(this->clientId, newIndex);
            this->Reply(replyTo, "updatePresets", result);
        }
        else if (message == "updatePluginPresets")
        {
            PluginUiPresets pluginPresets;
            pReader->read(&pluginPresets);
            model.UpdatePluginPresets(pluginPresets);
            this->Reply(replyTo, "updatePluginPresets", true);
        }
        else if (message == "moveBank")
        {
            FromToBody body;
            pReader->read(&body);
            model.MoveBank(this->clientId, body.from_, body.to_);
            this->Reply(replyTo, "moveBank");
        }
        else if (message == "shutdown")
        {
            PresetIndex newIndex;

            RequestShutdown(false);
            this->Reply(replyTo, "shutdown");
        }
        else if (message == "restart")
        {
            PresetIndex newIndex;
            RequestShutdown(true);
            this->Reply(replyTo, "restart");
        }
        else if (message == "deletePresetItem")
        {
            int64_t instanceId = 0;
            pReader->read(&instanceId);
            int64_t result = model.DeletePreset(this->clientId, instanceId);
            this->Reply(replyTo, "deletePresetItem", result);
        }
        else if (message == "deleteBankItem")
        {
            int64_t instanceId = 0;
            pReader->read(&instanceId);
            uint64_t result = model.DeleteBank(this->clientId, instanceId);
            this->Reply(replyTo, "deleteBankItem", result);
        }
        else if (message == "renameBank")
        {
            RenameBankBody body;
            pReader->read(&body);

            std::stringstream tOut;
            json_writer tWriter(tOut);
            tWriter.write(body.newName_);
            std::string tJson = tOut.str();
            std::stringstream tIn(tJson);
            json_reader tReader(tIn);
            std::string tResult;
            tReader.read(&tResult);

            body.newName_ = tResult;

            try
            {
                model.RenameBank(this->clientId, body.bankId_, body.newName_);
                this->Reply(replyTo, "renameBank");
            }
            catch (const std::exception &e)
            {
                this->SendError(replyTo, std::string(e.what()));
            }
        }
        else if (message == "openBank")
        {
            int64_t bankId = -1;
            pReader->read(&bankId);
            try
            {
                model.OpenBank(this->clientId, bankId);
                ;
                this->Reply(replyTo, "openBank");
            }
            catch (const std::exception &e)
            {
                this->SendError(replyTo, std::string(e.what()));
            }
        }
        else if (message == "saveBankAs")
        {
            RenameBankBody body;
            pReader->read(&body);
            try
            {
                int64_t newId = model.SaveBankAs(this->clientId, body.bankId_, body.newName_);
                this->Reply(replyTo, "saveBankAs", newId);
            }
            catch (const std::exception &e)
            {
                this->SendError(replyTo, std::string(e.what()));
            }
        }
        else if (message == "renamePresetItem")
        {
            RenamePresetBody body;
            pReader->read(&body);

            bool result = model.RenamePreset(body.clientId_, body.instanceId_, body.name_);
            this->Reply(replyTo, "renamePresetItem", result);
        }
        else if (message == "copyPreset")
        {
            CopyPresetBody body;
            pReader->read(&body);
            int64_t result = model.CopyPreset(body.clientId_, body.fromId_, body.toId_);
            this->Reply(replyTo, "copyPreset", result);
        }
        else if (message == "copyPluginPreset")
        {
            CopyPluginPresetBody body;
            pReader->read(&body);
            uint64_t result = model.CopyPluginPreset(body.pluginUri_, body.instanceId_);
            this->Reply(replyTo, "copyPluginPreset", result);
        }
        else if (message == "setPatchProperty")
        {
            SetPatchPropertyBody body;
            pReader->read(&body);
            model.SendSetPatchProperty(clientId,body.instanceId_,body.propertyUri_,body.value_,
                [this, replyTo] () {
                    this->JsonReply(replyTo, "setPatchProperty", "true");

                },
                [this, replyTo] (const std::string&error) {
                    this->SendError(replyTo, error.c_str());

                }
            );

        }
        
        else if (message == "getPatchProperty")
        {
            GetPatchPropertyBody body;
            pReader->read(&body);

            model.SendGetPatchProperty(
                this->clientId,
                body.instanceId_,
                body.propertyUri_,
                [this, replyTo](const std::string &jsonResult)
                {
                    this->JsonReply(replyTo, "getPatchProperty", jsonResult.c_str());
                },
                [this, replyTo](const std::string &error)
                {
                    this->SendError(replyTo, error.c_str());
                });
        }
        else if (message == "monitorPort")
        {

            MonitorPortBody body;
            pReader->read(&body);

            MonitorPort(replyTo,body);
        }
        else if (message == "unmonitorPort")
        {
            int64_t subscriptionHandle;
            pReader->read(&subscriptionHandle);
            {
                {
                    std::lock_guard  guard(activePortMonitorsMutex);
                    for (auto i = this->activePortMonitors.begin(); i != this->activePortMonitors.end(); ++i)
                    {
                        if ((*i)->subscriptionHandle == subscriptionHandle)
                        {
                            auto subscription = (*i);
                            subscription->Close();

                            this->activePortMonitors.erase(i);
                            break;
                        }
                    }
                }
                model.UnmonitorPort(subscriptionHandle);
            }
        }
        else if (message == "addVuSubscription")
        {
            int64_t instanceId = -1;

            pReader->read(&instanceId);

            int64_t subscriptionHandle = model.AddVuSubscription(instanceId);

            {
                std::lock_guard<std::recursive_mutex> guard(subscriptionMutex);
                activeVuSubscriptions.push_back(VuSubscription{subscriptionHandle, instanceId});
            }
            this->Reply(replyTo, "addVuSubscription", subscriptionHandle);
        }
        else if (message == "removeVuSubscription")
        {
            int64_t subscriptionHandle = -1;
            pReader->read(&subscriptionHandle);
            {
                std::lock_guard<std::recursive_mutex> guard(subscriptionMutex);

                for (auto i = activeVuSubscriptions.begin(); i != activeVuSubscriptions.end(); ++i)
                {
                    if (i->subscriptionHandle == subscriptionHandle)
                    {
                        activeVuSubscriptions.erase(i);
                        break;
                    }
                }
            }
            model.RemoveVuSubscription(subscriptionHandle);
        }
        else if (message == "imageList")
        {

            this->Reply(replyTo, "imageList", imageList);
        }
        else if (message == "getFavorites")
        {
            std::map<std::string, bool> favorites = this->model.GetFavorites();
            this->Reply(replyTo, "getFavorites", favorites);
        }
        else if (message == "setFavorites")
        {
            std::map<std::string, bool> favorites;
            pReader->read(&favorites);
            this->model.SetFavorites(favorites);
        }
        else if (message == "setSystemMidiBindings")
        {
            std::vector<MidiBinding> bindings;
            pReader->read(&bindings);
            this->model.SetSystemMidiBindings(bindings);
        }
        else if (message == "getSystemMidiBindings")
        {
            std::vector<MidiBinding> bindings = this->model.GetSystemMidiBidings();
            this->Reply(replyTo,"getSystemMidiBindings",bindings);
        }
        else if (message == "requestFileList")
        {
            UiFileProperty fileProperty;
            pReader->read(&fileProperty);
            std::vector<std::string> list = this->model.GetFileList(fileProperty);
            this->Reply(replyTo,"requestFileList",list);
        } 
        else if (message == "newPreset")
        {
            int64_t presetId = this->model.CreateNewPreset();
            this->Reply(replyTo,"newPreset",presetId);
        } 
        else if (message == "deleteUserFile")
        {
            std::string fileName;
            pReader->read(&fileName);

            this->model.DeleteSampleFile(fileName);
            this->Reply(replyTo,"deleteUserFile",true);
        }
        else if (message == "setOnboarding")
        {
            bool value;
            pReader->read(&value);
            this->model.SetOnboarding(value);
        }
        else
        {
            Lv2Log::error("Unknown message received: %s", message.c_str());
            SendError(replyTo, std::string("Unknown message: ") + message);
        }
    }

protected:
    virtual void onReceive(const std::string_view &text)
    {
        view_istream<char> s(text);

        json_reader reader(s);
        // read top level object until we have message
        int64_t replyTo = -1;
        int64_t reply = -1;
        std::string message;

        try
        {
            reader.consume('[');
            reader.consume('{');

            while (true)
            {
                if (reader.peek() == '}')
                {
                    reader.consume('}');
                    break;
                }
                std::string name = reader.read_string();
                reader.consume(':');
                if (name == "reply")
                {
                    reader.read(&reply);
                }
                if (name == "replyTo")
                {
                    reader.read(&replyTo);
                }
                if (name == "message")
                {
                    message = reader.read_string();
                }
                if (reader.peek() == ',')
                {
                    reader.consume(',');
                    continue;
                }
            }
            if (reader.peek() == ',')
            {
                reader.consume(',');
                handleMessage(reply, replyTo, message, &reader);
            }
            else
            {
                handleMessage(reply, replyTo, message, nullptr);
            }
            // Handle message.
        }
        catch (const PiPedalException &e)
        {
            SendError(replyTo, e.what());
        }
        catch (const std::exception &e)
        {
            SendError(replyTo, e.what());
        }
    }

private:
    virtual void OnLv2StateChanged(int64_t instanceId, const Lv2PluginState &state)
    {
        Lv2StateChangedBody message { (uint64_t)instanceId, state};
        Send("onLv2StateChanged",message);

    }
    virtual void OnErrorMessage(const std::string&message)
    {
        Send("onErrorMessage",message);
    }
    // virtual void OnPatchPropertyChanged(int64_t clientId, int64_t instanceId,const std::string& propertyUri,const json_variant& value)
    // {
    //     PatchPropertyChangedBody body;
    //     body.clientId_ = clientId;
    //     body.instanceId_ = instanceId;
    //     body.propertyUri_ = propertyUri;
    //     body.value_ = value;
    //     Send("onPatchPropertyChanged",body);
    // }

    virtual void OnSystemMidiBindingsChanged(const std::vector<MidiBinding>&bindings) {
        Send("onSystemMidiBindingsChanged",bindings);
    }

    virtual void OnFavoritesChanged(const std::map<std::string, bool> &favorites)
    {
        Send("onFavoritesChanged", favorites);
    }

    virtual void OnShowStatusMonitorChanged(bool show)
    {
        Send("onShowStatusMonitorChanged", show);
    }

    virtual void OnChannelSelectionChanged(int64_t clientId, const JackChannelSelection &channelSelection)
    {
        ChannelSelectionChangedBody body;
        body.clientId_ = clientId;
        body.jackChannelSelection_ = const_cast<JackChannelSelection *>(&channelSelection);
        Send("onChannelSelectionChanged", body);
    }
    virtual void OnPresetsChanged(int64_t clientId, const PresetIndex &presets)
    {
        PresetsChangedBody body;
        body.clientId_ = clientId;
        body.presets_ = const_cast<PresetIndex *>(&presets);
        Send("onPresetsChanged", body);
    }
    virtual void OnPluginPresetsChanged(const std::string &pluginUri)
    {
        Send("onPluginPresetsChanged", pluginUri);
    }
    virtual void OnJackConfigurationChanged(const JackConfiguration &jackConfiguration)
    {
        Send("onJackConfigurationChanged", jackConfiguration);
    }

    virtual void OnLoadPluginPreset(int64_t instanceId, const std::vector<ControlValue> &controlValues)
    {
        OnLoadPluginPresetBody body;
        body.instanceId_ = instanceId;
        body.controlValues_ = const_cast<std::vector<ControlValue> &>(controlValues);
        Send("onLoadPluginPreset", body);
    }

    int updateRequestOutstanding = 0;
    bool vuUpdateDropped = false;

    virtual void OnVuMeterUpdate(const std::vector<VuUpdate> &updates)
    {
        std::lock_guard<std::recursive_mutex> guard(subscriptionMutex);
        if (updateRequestOutstanding < 5) // throttle to accomodate a web page that can't keep up.
        {
            vuUpdateDropped = false;
            for (int i = 0; i < updates.size(); ++i)
            {
                const VuUpdate &vuUpdate = updates[i];
                bool interested = false;
                for (int i = 0; i < this->activeVuSubscriptions.size(); ++i)
                {
                    if (activeVuSubscriptions[i].instanceId == vuUpdate.instanceId_)
                    {
                        interested = true;
                        break;
                    }
                }
                if (interested)
                {
                    updateRequestOutstanding++;
                    this->Request<bool, VuUpdate>(
                        "onVuUpdate",
                        vuUpdate,
                        [this](const bool &result)
                        {
                            this->updateRequestOutstanding--;
                        },
                        [this](const std::exception &)
                        {
                            this->updateRequestOutstanding--;
                        });
                }
            }
        }
        else
        {
            if (!vuUpdateDropped)
            {
                vuUpdateDropped = true;
                Lv2Log::debug("Vu Update(s) dropped.");
            }
        }
    }

    virtual void OnVst3ControlChanged(int64_t clientId, int64_t instanceId, const std::string &key, float value, const std::string &state)
    {
        Vst3ControlChangedBody body;
        body.clientId_ = clientId;
        body.instanceId_ = instanceId;
        body.symbol_ = key;
        body.value_ = value;
        body.state_ = state;
        Send("onVst3ControlChanged", body);
    }

    virtual void OnControlChanged(int64_t clientId, int64_t instanceId, const std::string &key, float value)
    {
        ControlChangedBody body;
        body.clientId_ = clientId;
        body.instanceId_ = instanceId;
        body.symbol_ = key;
        body.value_ = value;
        Send("onControlChanged", body);
    }
    virtual void OnInputVolumeChanged(float value)
    {
        ControlChangedBody body;
        Send("onInputVolumeChanged", value);
    }
    virtual void OnOutputVolumeChanged(float value)
    {
        ControlChangedBody body;
        Send("onOutputVolumeChanged", value);
    }

    class DeferredValue
    {
    public:
        int64_t instanceId;
        std::string symbol;
        float value;
    };

    std::vector<DeferredValue> deferredValues;
    bool midiValueChangedOutstanding = false;

    virtual void OnMidiValueChanged(int64_t instanceId, const std::string &symbol, float value)
    {
        if (midiValueChangedOutstanding)
        {
            for (size_t i = 0; i < deferredValues.size(); ++i)
            {
                auto &deferredValue = deferredValues[i];
                if (deferredValue.instanceId == instanceId && deferredValue.symbol == symbol)
                {
                    deferredValue.value = value;
                    return;
                }
            }
            DeferredValue newValue{instanceId, symbol, value};
            deferredValues.push_back(newValue);
        }
        else
        {
            midiValueChangedOutstanding = true;
            ControlChangedBody body;
            body.clientId_ = -1;
            body.instanceId_ = instanceId;
            body.symbol_ = symbol;
            body.value_ = value;
            Request<bool>(
                "onMidiValueChanged", body,
                [this](const bool &value)
                {
                    this->midiValueChangedOutstanding = false;
                    if (this->deferredValues.size() != 0)
                    {
                        DeferredValue value = deferredValues[0];
                        deferredValues.erase(deferredValues.begin());
                        this->OnMidiValueChanged(value.instanceId, value.symbol, value.value);
                    }
                },
                [](const std::exception &e) {

                });
        }
    }

    int outstandingNotifyAtomOutputs = 0;

    class PendingNotifyAtomOutput
    {
    public:
        int64_t clientHandle;
        uint64_t instanceId;
        std::string propertyUri;
        std::string json;
    };

    std::vector<PendingNotifyAtomOutput> pendingNotifyAtomOutputs;

    void OnAckNotifyPatchProperty()
    {
        std::lock_guard<std::recursive_mutex> guard(subscriptionMutex);
        if (--outstandingNotifyAtomOutputs <= 0)
        {
            outstandingNotifyAtomOutputs = 0;
            if (pendingNotifyAtomOutputs.size() != 0)
            {
                PendingNotifyAtomOutput t = pendingNotifyAtomOutputs[0];
                pendingNotifyAtomOutputs.erase(pendingNotifyAtomOutputs.begin());
                OnNotifyPatchProperty(
                    t.clientHandle,
                    t.instanceId,
                    t.propertyUri,
                    t.json);
            }
        }
    }

    virtual void OnNotifyPatchProperty(int64_t clientHandle, uint64_t instanceId, const std::string &atomProperty, const std::string &atomJson)
    {
        NotifyAtomOutputBody body;
        body.clientHandle_ = clientHandle;
        body.instanceId_ = instanceId;
        body.propertyUri_ = atomProperty;
        body.atomJson_.Set(atomJson);

        // flow control. We can only have one in-flight NotifyAtomOutput at a time.
        // Subsequent notifications are held until we receive an ack. 
        //
        // If a duplicate atomProperty is queued, overwrite the previous value.
        {
            std::lock_guard<std::recursive_mutex> guard(subscriptionMutex);

            if (outstandingNotifyAtomOutputs != 0)
            {
                for (size_t i = 0; i < pendingNotifyAtomOutputs.size(); ++i)
                {
                    auto &output = pendingNotifyAtomOutputs[i];
                    if (output.clientHandle == clientHandle && output.instanceId == instanceId && output.propertyUri == atomProperty)
                    {
                        // better to erase than overwrite, since it provides better idempotence.
                        pendingNotifyAtomOutputs.erase(pendingNotifyAtomOutputs.begin()+i);
                        break;
                    }
                }
                pendingNotifyAtomOutputs.push_back(PendingNotifyAtomOutput{clientHandle, instanceId, atomProperty, atomJson});
                return;
            }
            ++outstandingNotifyAtomOutputs;
        }

        Request<bool>(
            "onNotifyPatchProperty", body,
            [this](const bool &value)
            {
                this->OnAckNotifyPatchProperty();
            },
            [](const std::exception &e) {

            });
    }
    virtual void OnNotifyMidiListener(int64_t clientHandle, bool isNote, uint8_t noteOrControl)
    {
        NotifyMidiListenerBody body;
        body.clientHandle_ = clientHandle;
        body.isNote_ = isNote;
        body.noteOrControl_ = noteOrControl;

        Send("onNotifyMidiListener", body);
    }

    virtual void OnBankIndexChanged(const BankIndex &bankIndex)
    {
        Send("onBanksChanged", bankIndex);
    }

    virtual void OnJackServerSettingsChanged(const JackServerSettings &jackServerSettings)
    {
        Send("onJackServerSettingsChanged", jackServerSettings);
    }
    virtual void OnWifiConfigSettingsChanged(const WifiConfigSettings &wifiConfigSettings)
    {
        Send("onWifiConfigSettingsChanged", wifiConfigSettings);
    }

    virtual void OnWifiDirectConfigSettingsChanged(const WifiDirectConfigSettings &wifiConfigSettings)
    {
        Send("onWifiDirectConfigSettingsChanged", wifiConfigSettings);
    }
    virtual void OnGovernorSettingsChanged(const std::string &governor)
    {
        Send("onGovernorSettingsChanged", governor);
    }

    virtual void OnPedalboardChanged(int64_t clientId, const Pedalboard &pedalboard)
    {
        UpdateCurrentPedalboardBody body;
        body.clientId_ = clientId;
        body.pedalboard_ = pedalboard;
        Send("onPedalboardChanged", body);
    }

    virtual void OnItemEnabledChanged(int64_t clientId, int64_t pedalItemId, bool enabled)
    {
        PedalboardItemEnabledBody body;
        body.clientId_ = clientId;
        body.instanceId_ = pedalItemId;
        body.enabled_ = enabled;
        Send("onItemEnabledChanged", body);
    }
};

std::atomic<uint64_t> PiPedalSocketHandler::nextClientId = 0;

class PiPedalSocketFactory : public ISocketFactory
{
private:
    PiPedalModel &model;

public:
    virtual ~PiPedalSocketFactory()
    {
    }
    PiPedalSocketFactory(PiPedalModel &model)
        : model(model)
    {
    }

public:
    virtual bool wants(const uri &request)
    {
        if (request.segment(0) == "pipedal")
            return true;
        return false;
    }
    virtual std::shared_ptr<SocketHandler> CreateHandler(const uri &request)
    {
        return std::make_shared<PiPedalSocketHandler>(model);
    }
};

std::shared_ptr<ISocketFactory> pipedal::MakePiPedalSocketFactory(PiPedalModel &model)
{
    return std::make_shared<PiPedalSocketFactory>(model);
}

void PiPedalSocketHandler::RequestShutdown(bool restart)
{
    if (GetAdminClient().CanUseShutdownClient())
    {
        GetAdminClient().RequestShutdown(restart);
    }
    else
    {
        // ONLY works when interactively logged in.
        std::stringstream s;
        s << "/usr/sbin/shutdown ";
        if (restart)
        {
            s << "-r";
        }
        else
        {
            s << "-P";
        }
        s << " now";

        if (sysExec(s.str().c_str()) != EXIT_SUCCESS)
        {
            Lv2Log::error("shutdown failed.");
            if (restart)
            {
                throw new PiPedalStateException("Restart request failed.");
            }
            else
            {
                throw new PiPedalStateException("Shutdown request failed.");
            }
        }
    }
}
