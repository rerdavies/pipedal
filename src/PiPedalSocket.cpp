#include "pch.h"

#include "PiPedalSocket.hpp"
#include "json.hpp"
#include "viewstream.hpp"
#include "PiPedalVersion.hpp"
#include <atomic>
#include "Lv2Log.hpp"
#include "JackConfiguration.hpp"
#include <future>
#include <atomic>

#include "ShutdownClient.hpp"

using namespace std;
using namespace pipedal;


class NotifyMidiListenerBody {
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

class ListenForMidiEventBody {
public: 
    bool listenForControlsOnly_;
    int64_t handle_;
    DECLARE_JSON_MAP(ListenForMidiEventBody);
};

JSON_MAP_BEGIN(ListenForMidiEventBody)
JSON_MAP_REFERENCE(ListenForMidiEventBody, listenForControlsOnly)
JSON_MAP_REFERENCE(ListenForMidiEventBody, handle)
JSON_MAP_END()


class OnLoadPluginPresetBody {
public: 
    int64_t instanceId_;
    std::vector<ControlValue> controlValues_;
    DECLARE_JSON_MAP(OnLoadPluginPresetBody);
};

JSON_MAP_BEGIN(OnLoadPluginPresetBody)
JSON_MAP_REFERENCE(OnLoadPluginPresetBody, instanceId)
JSON_MAP_REFERENCE(OnLoadPluginPresetBody, controlValues)
JSON_MAP_END()

class LoadPluginPresetBody {
public: 
    int64_t instanceId_;
    std::string presetName_;
    DECLARE_JSON_MAP(LoadPluginPresetBody);
};

JSON_MAP_BEGIN(LoadPluginPresetBody)
JSON_MAP_REFERENCE(LoadPluginPresetBody, instanceId)
JSON_MAP_REFERENCE(LoadPluginPresetBody, presetName)
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


class GetLv2ParameterBody
{
public:
    int64_t instanceId_;
    std::string uri_;

    DECLARE_JSON_MAP(GetLv2ParameterBody);
};
JSON_MAP_BEGIN(GetLv2ParameterBody)
JSON_MAP_REFERENCE(GetLv2ParameterBody, instanceId)
JSON_MAP_REFERENCE(GetLv2ParameterBody, uri)
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

class RenameBankBody {
public:
    int64_t bankId_;
    std::string newName_;
    DECLARE_JSON_MAP(RenameBankBody);

};
JSON_MAP_BEGIN(RenameBankBody)
JSON_MAP_REFERENCE(RenameBankBody,bankId)
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

class PedalBoardItemEnabledBody
{
public:
    int64_t clientId_ = -1;
    int64_t instanceId_ = -1;
    bool enabled_ = true;

    DECLARE_JSON_MAP(PedalBoardItemEnabledBody);
};
JSON_MAP_BEGIN(PedalBoardItemEnabledBody)
JSON_MAP_REFERENCE(PedalBoardItemEnabledBody, clientId)
JSON_MAP_REFERENCE(PedalBoardItemEnabledBody, instanceId)
JSON_MAP_REFERENCE(PedalBoardItemEnabledBody, enabled)
JSON_MAP_END()

class SetCurrentPedalBoardBody
{
public:
    int64_t clientId_ = -1;
    PedalBoard pedalBoard_;

    DECLARE_JSON_MAP(SetCurrentPedalBoardBody);
};

JSON_MAP_BEGIN(SetCurrentPedalBoardBody)
JSON_MAP_REFERENCE(SetCurrentPedalBoardBody, clientId)
JSON_MAP_REFERENCE(SetCurrentPedalBoardBody, pedalBoard)
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




static void requestShutdown(bool restart)
{
    if (ShutdownClient::CanUseShutdownClient())
    {
        ShutdownClient::RequestShutdown(restart);
    }
    else {
        // ONLY works when interactively logged in.
        std::stringstream s;
        s << "/usr/sbin/shutdown ";
        if (restart)
        {
            s << "-r";
        } else 
        {
            s << "-P";
        }
        s << " now";

        if (system(s.str().c_str()) != EXIT_SUCCESS)
        {
            Lv2Log::error("shutdown failed.");
            if (restart) {
                throw new PiPedalStateException("Restart request failed.");
            } else {
                throw new PiPedalStateException("Shutdown request failed.");
            }
        }
    }
}


class PiPedalSocketHandler : public SocketHandler, public IPiPedalModelSubscriber
{
private:
    std::recursive_mutex writeMutex;
    PiPedalModel &model;
    static std::atomic<uint64_t> nextClientId;

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
        int64_t subscriptionHandle = -1;
        int64_t instanceId = -1;
        bool waitingForAck = false;
        std::string key;
    };

    std::vector<PortMonitorSubscription> activePortMonitors;

public:
    virtual int64_t GetClientId() { return clientId; }

    virtual ~PiPedalSocketHandler()
    {
        for (int i = 0; i < this->activePortMonitors.size(); ++i)
        {
            model.unmonitorPort(activePortMonitors[i].subscriptionHandle);
        }
        for (int i = 0; i < this->activeVuSubscriptions.size(); ++i)
        {
            model.removeVuSubscription(activeVuSubscriptions[i].subscriptionHandle);
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
            std::lock_guard guard(this->writeMutex);
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
            std::lock_guard guard(this->writeMutex);
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
            std::lock_guard guard(this->writeMutex);
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
    std::mutex requestMutex;
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
                std::lock_guard lock(requestMutex);
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
                std::lock_guard guard(this->writeMutex);
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

    bool waitingForPortMonitorAck(uint64_t subscriptionId)
    {
        {
            std::lock_guard guard(subscriptionMutex);
            for (int i = 0; i < this->activePortMonitors.size(); ++i)
            {
                auto &portMonitor = activePortMonitors[i];
                if (portMonitor.subscriptionHandle == subscriptionId)
                {
                    if (portMonitor.waitingForAck)
                        return true;
                    portMonitor.waitingForAck = true;
                    return false;
                }
            }
        }
        return true; // subscription has been closed. don't sent to client.
    }
    void portMonitorAck(uint64_t subscriptionId)
    {
        {
            std::lock_guard guard(subscriptionMutex);
            for (int i = 0; i < this->activePortMonitors.size(); ++i)
            {
                auto &portMonitor = activePortMonitors[i];
                if (portMonitor.subscriptionHandle == subscriptionId)
                {
                    portMonitor.waitingForAck = false;
                    break;
                }
            }
        }
    }

    void handleMessage(int reply, int replyTo, const std::string &message, json_reader *pReader)
    {
        if (reply != -1)
        {
            IRequestReservation *reservation = nullptr;
            {
                std::lock_guard guard(this->requestMutex);
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
        if (message == "previewControl")
        {
            ControlChangedBody message;
            pReader->read(&message);
            this->model.previewControl(message.clientId_, message.instanceId_, message.symbol_, message.value_);
        }
        else if (message == "setControl")
        {
            ControlChangedBody message;
            pReader->read(&message);
            this->model.setControl(message.clientId_, message.instanceId_, message.symbol_, message.value_);
        }
        else if (message == "listenForMidiEvent")
        {
            ListenForMidiEventBody body;
            pReader->read(&body);
            this->model.listenForMidiEvent(this->clientId,body.handle_, body.listenForControlsOnly_);
        }
        else if (message == "cancelListenForMidiEvent")
        {
            uint64_t handle;
            pReader->read(&handle);
            this->model.cancelListenForMidiEvent(this->clientId,handle);
        }
        else if (message == "getJackStatus")
        {
            JackHostStatus status = model.getJackStatus();
            this->Reply(replyTo,"getJackStatus",status);
        }
        else if (message == "getPluginPresets")
        {
            std::string uri;
            pReader->read(&uri);
            this->Reply(replyTo,"getPluginPresets",this->model.GetPluginPresets(uri));
        }
        else if (message == "loadPluginPreset")
        {
            LoadPluginPresetBody body;
            pReader->read(&body);
            this->model.LoadPluginPreset(body.instanceId_,body.presetName_);

        }
        else if (message == "setJackServerSettings") {
            JackServerSettings jackServerSettings;
            pReader->read(&jackServerSettings);
            this->model.SetJackServerSettings(jackServerSettings);
            this->Reply(replyTo,"setJackserverSettings");

        }
        else if (message == "getJackServerSettings") {
            this->Reply(replyTo, "getJackServerSettings", model.GetJackServerSettings());

        }
        else if (message == "getBankIndex") {
            BankIndex bankIndex = model.getBankIndex();
            this->Reply(replyTo, "getBankIndex", bankIndex);

        }
        else if (message == "getJackConfiguration")
        {
            JackConfiguration configuration = this->model.getJackConfiguration();
            this->Reply(replyTo, "getJackConfiguration", configuration);
        }
        else if (message == "getJackSettings")
        {
            JackChannelSelection selection = this->model.getJackChannelSelection();
            this->Reply(replyTo, "getJackSettings", selection);
        }
        else if (message == "saveCurrentPreset")
        {
            this->model.saveCurrentPreset(this->clientId);
        }
        else if (message == "saveCurrentPresetAs")
        {
            SaveCurrentPresetAsBody body;
            pReader->read(&body);
            int64_t result = this->model.saveCurrentPresetAs(this->clientId, body.name_, body.saveAfterInstanceId_);
            Reply(replyTo, "saveCurrentPresetsAs", result);
        }
        else if (message == "getPresets")
        {
            PresetIndex presets;
            this->model.getPresets(&presets);
            Reply(replyTo, "getPresets", presets);
        }
        else if (message == "setPedalBoardItemEnable")
        {
            PedalBoardItemEnabledBody body;
            pReader->read(&body);
            model.setPedalBoardItemEnable(body.clientId_, body.instanceId_, body.enabled_);
        }
        else if (message == "setCurrentPedalBoard")
        {
            {
                SetCurrentPedalBoardBody body;

                pReader->read(&body);
                this->model.setPedalBoard(body.clientId_, body.pedalBoard_);
            }
        }
        else if (message == "currentPedalBoard")
        {
            auto pedalBoard = model.getCurrentPedalBoardCopy();
            Reply(replyTo, "currentPedalBoard", pedalBoard);
        }
        else if (message == "plugins")
        {
            auto ui_plugins = model.getPlugins().GetUiPlugins();
            Reply(replyTo, "plugins", ui_plugins);
        }
        else if (message == "pluginClasses")
        {
            auto classes = model.getPlugins().GetLv2PluginClass();
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
            this->model.setJackChannelSelection(this->clientId, jackSettings);
        }
        else if (message == "version")
        {
            PiPedalVersion version;

            Reply(replyTo, "version", version);
        }
        else if (message == "loadPreset")
        {
            int64_t instanceId = 0;
            pReader->read(&instanceId);
            model.loadPreset(this->clientId, instanceId);
        }
        else if (message == "updatePresets")
        {
            PresetIndex newIndex;
            pReader->read(&newIndex);
            bool result = model.updatePresets(this->clientId, newIndex);
            this->Reply(replyTo, "updatePresets", result);
        }
        else if (message == "moveBank")
        {
            FromToBody body;
            pReader->read(&body);
            model.moveBank(this->clientId, body.from_, body.to_);
            this->Reply(replyTo, "moveBank");
        }
        else if (message == "shutdown")
        {
            PresetIndex newIndex;
            
            requestShutdown(false);
            this->Reply(replyTo,"shutdown");
        }
        else if (message == "restart")
        {
            PresetIndex newIndex;
            requestShutdown(true);
            this->Reply(replyTo,"restart");
        }
        else if (message == "deletePresetItem")
        {
            int64_t instanceId = 0;
            pReader->read(&instanceId);
            int64_t result = model.deletePreset(this->clientId, instanceId);
            this->Reply(replyTo, "deletePresetItem", result);
        }
        else if (message == "deleteBankItem")
        {
            int64_t instanceId = 0;
            pReader->read(&instanceId);
            uint64_t result = model.deleteBank(this->clientId, instanceId);
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


            try {
                model.renameBank(this->clientId,body.bankId_, body.newName_);
                        this->Reply(replyTo, "renameBank");
            } catch (const std::exception &e)
            {
                this->SendError(replyTo,std::string(e.what()));
            }
        }
        else if (message == "openBank")
        {
            int64_t bankId = -1;
            pReader->read(&bankId);
            try {
                model.openBank(this->clientId,bankId);;
                this->Reply(replyTo, "openBank");
            } catch (const std::exception &e)
            {
                this->SendError(replyTo,std::string(e.what()));
            }
        }
        else if (message == "saveBankAs")
        {
            RenameBankBody body;
            pReader->read(&body);
            try {
                int64_t newId = model.saveBankAs(this->clientId,body.bankId_, body.newName_);
                this->Reply(replyTo, "saveBankAs",newId);
            } catch (const std::exception &e)
            {
                this->SendError(replyTo,std::string(e.what()));
            }
        }
        else if (message == "renamePresetItem")
        {
            RenamePresetBody body;
            pReader->read(&body);

            bool result = model.renamePreset(body.clientId_, body.instanceId_, body.name_);
            this->Reply(replyTo, "renamePresetItem", result);
        }
        else if (message == "copyPreset")
        {
            CopyPresetBody body;
            pReader->read(&body);
            int64_t result = model.copyPreset(body.clientId_, body.fromId_, body.toId_);
            this->Reply(replyTo, "copyPreset", result);
        } 
        else if (message =="getLv2Parameter")
        {
            GetLv2ParameterBody body;
            pReader->read(&body);
            
            model.getLv2Parameter(
                this->clientId,
                body.instanceId_,
                body.uri_,
                [this,replyTo] (const std::string &jsonResult)
                {
                    this->JsonReply(replyTo,"getLv2Parameter",jsonResult.c_str());
                },
                [this,replyTo] (const std::string &error) {
                    this->SendError(replyTo,error.c_str());
                });
            
        }
        else if (message == "monitorPort")
        {

            MonitorPortBody body;
            pReader->read(&body);

            int64_t subscriptionHandle = model.monitorPort(
                body.instanceId_,
                body.key_,
                body.updateRate_,
                [this](int64_t subscriptionHandle_, float value)
                {
                    MonitorResultBody body;
                    body.subscriptionHandle_ = subscriptionHandle_;
                    body.value_ = value;
                    if (!waitingForPortMonitorAck(subscriptionHandle_)) //only one outstanding response.
                    {
                        this->Request<bool, MonitorResultBody>( // send the message and wait for a response.
                            "onMonitorPortOutput",
                            body,
                            [this, subscriptionHandle_](const bool &result)
                            {
                                this->portMonitorAck(subscriptionHandle_); // please can we have some more.
                                                                           // (....complexity not worth the effort. :-( )
                            },
                            [](const std::exception &e)
                            {
                                Lv2Log::debug("Failed to monitor port output. (%s)", e.what());
                            });
                    }
                });
            {
                std::lock_guard guard(subscriptionMutex);
                activePortMonitors.push_back(
                    PortMonitorSubscription{subscriptionHandle, body.instanceId_,false, body.key_});
            }

            this->Reply(replyTo, "monitorPort", subscriptionHandle);
        }
        else if (message == "unmonitorPort")
        {
            int64_t subscriptionHandle;
            pReader->read(&subscriptionHandle);
            {
                {
                    std::lock_guard guard(subscriptionMutex);
                    for (auto i = this->activePortMonitors.begin(); i != this->activePortMonitors.end(); ++i)
                    {
                        if ((*i).subscriptionHandle == subscriptionHandle)
                        {
                            this->activePortMonitors.erase(i);
                            break;
                        }
                    }
                }
                model.unmonitorPort(subscriptionHandle);
            }
        }
        else if (message == "addVuSubscription")
        {
            int64_t instanceId = -1;

            pReader->read(&instanceId);

            int64_t subscriptionHandle = model.addVuSubscription(instanceId);

            {
                std::lock_guard guard(subscriptionMutex);
                activeVuSubscriptions.push_back(VuSubscription{subscriptionHandle, instanceId});
            }
            this->Reply(replyTo, "addVuSubscriptin", subscriptionHandle);
        }
        else if (message == "removeVuSubscription")
        {
            int64_t subscriptionHandle = -1;
            pReader->read(&subscriptionHandle);
            {
                std::lock_guard guard(subscriptionMutex);

                for (auto i = activeVuSubscriptions.begin(); i != activeVuSubscriptions.end(); ++i)
                {
                    if (i->subscriptionHandle == subscriptionHandle)
                    {
                        activeVuSubscriptions.erase(i);
                        break;
                    }
                }
            }
            model.removeVuSubscription(subscriptionHandle);
        }
        else
        {
            Lv2Log::error("Unknown message received: %s", message.c_str());
            SendError(replyTo, std::string("Unknown message: ") + message);
        }
    }

protected:
    virtual void
    onReceive(const std::string_view &text)
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
        catch (const std::exception & e)
        {
            SendError(replyTo, e.what());
        }
    }

public:
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
    virtual void OnJackConfigurationChanged(const JackConfiguration &jackConfiguration)
    {
        Send("onJackConfigurationChanged", jackConfiguration);
    }

    virtual void OnLoadPluginPreset(int64_t instanceId,const std::vector<ControlValue>&controlValues) {
        OnLoadPluginPresetBody body;
        body.instanceId_ = instanceId;
        body.controlValues_ = const_cast<std::vector<ControlValue>&>(controlValues);
        Send("onLoadPluginPreset",body);
    }

    
    int updateRequestOutstanding = 0;
    bool vuUpdateDropped = false;

    virtual void OnVuMeterUpdate(const std::vector<VuUpdate> &updates)
    {
        std::lock_guard guard(subscriptionMutex);
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

    virtual void OnControlChanged(int64_t clientId, int64_t instanceId, const std::string &key, float value)
    {
        ControlChangedBody body;
        body.clientId_ = clientId;
        body.instanceId_ = instanceId;
        body.symbol_ = key;
        body.value_ = value;
        Send("onControlChanged", body);
    }

    class DeferredValue {
    public:
        int64_t instanceId;
        std::string symbol;
        float value;
    };

    std::vector<DeferredValue> deferredValues;
    bool midiValueChangedOutstanding = false;


    virtual void OnMidiValueChanged(int64_t instanceId, const std::string&symbol, float value) {
        if (midiValueChangedOutstanding)
        {
            for (size_t i = 0; i < deferredValues.size(); ++i) {
                auto &deferredValue = deferredValues[i];
                if (deferredValue.instanceId == instanceId && deferredValue.symbol == symbol)
                {
                    deferredValue.value = value;
                    return;
                }
            }
            DeferredValue newValue {instanceId,symbol,value};
            deferredValues.push_back(newValue);
        } else {
            midiValueChangedOutstanding = true;
            ControlChangedBody body;
            body.clientId_ = -1;
            body.instanceId_ = instanceId;
            body.symbol_ = symbol;
            body.value_ = value;
            Request<bool>("onMidiValueChanged", body,
            [this] (const bool& value)
            {
                this->midiValueChangedOutstanding = false;
                if (this->deferredValues.size() != 0)
                {
                    DeferredValue value = deferredValues[0];
                    deferredValues.erase(deferredValues.begin());
                    this->OnMidiValueChanged(value.instanceId,value.symbol,value.value);
                }

            },
            [] (const std::exception &e) {
                
            }
            );
        }
    }

    virtual void OnNotifyMidiListener(int64_t clientHandle, bool isNote, uint8_t noteOrControl) 
    {
        NotifyMidiListenerBody body;
        body.clientHandle_ = clientHandle;
        body.isNote_ = isNote;
        body.noteOrControl_ = noteOrControl;

        Send("onNotifyMidiListener",body);
    }



    virtual void OnBankIndexChanged(const BankIndex& bankIndex) 
    {
        Send("onBanksChanged",bankIndex);
    }

    virtual void OnJackServerSettingsChanged(const JackServerSettings& jackServerSettings) 
    {
        Send("onJackServerSettingsChanged",jackServerSettings);
    }

    virtual void OnPedalBoardChanged(int64_t clientId, const PedalBoard &pedalBoard)
    {
        SetCurrentPedalBoardBody body;
        body.clientId_ = clientId;
        body.pedalBoard_ = pedalBoard;
        Send("onPedalBoardChanged", body);
    }


    virtual void OnItemEnabledChanged(int64_t clientId, int64_t pedalItemId, bool enabled)
    {
        PedalBoardItemEnabledBody body;
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

