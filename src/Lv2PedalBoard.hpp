#pragma once
#include "PedalBoard.hpp"
#include "Lv2Host.hpp"
#include "Lv2Effect.hpp"
#include "BufferPool.hpp"
#include <functional>
#include <lv2/urid/urid.h>
#include <functional>

namespace pipedal {



class RealtimeVuBuffers;
class RealtimeParameterRequest;

class Lv2PedalBoard {
    IHost *pHost = nullptr;

    BufferPool bufferPool;
    std::vector<float*> pedalBoardInputBuffers;
    std::vector<float*> pedalBoardOutputBuffers;

    std::vector<std::shared_ptr<IEffect> > effects;
    std::vector<IEffect* > realtimeEffects; // std::shared_ptr is not thread-safe!!

    using Action = std::function<void()>;
    using ProcessAction = std::function<void(uint32_t frames)>;

    std::vector<Action> activateActions;

    std::vector<ProcessAction> processActions;

    std::vector<Action> deactivateActions;

    float *CreateNewAudioBuffer();

    enum class MappingType {
        Linear,
        Circular,
        Momentary,
        Latched,
    };
    class MidiMapping {
    public:
        std::shared_ptr<Lv2PluginInfo> pluginInfo; // lifecycle
        const Lv2PortInfo *pPortInfo = nullptr; // owned by port.
        int instanceId = -1;
        int effectIndex = -1;
        int controlIndex = -1;
        int key; // key to the note or control. internal use only.
        bool hasLastValue = false;
        float lastValue = -1;
        MappingType mappingType;
        MidiBinding midiBinding;
    };

    std::vector<MidiMapping> midiMappings;


    std::vector<float*> PrepareItems(
        const std::vector<PedalBoardItem> & items,
        std::vector<float*> inputBuffers
        );

    void PrepareMidiMap(const PedalBoard&pedalBoard);
    void PrepareMidiMap(const PedalBoardItem&pedalBoardItem);

    std::vector<float*> AllocateAudioBuffers(int nChannels);
    int CalculateChainInputs(const std::vector<float *> &inputBuffers, const std::vector<PedalBoardItem> &items);
    void AppendParameterRequest(uint8_t*atomBuffer, LV2_URID uridParameter);
public:
    Lv2PedalBoard() { }
    ~Lv2PedalBoard() { }

    void Prepare(IHost *pHost,const PedalBoard&pedalBoard);

    int GetIndexOfInstanceId(long instanceId) {
        for (int i = 0; i < this->realtimeEffects.size(); ++i)
        {
            if (this->realtimeEffects[i]->GetInstanceId() == instanceId) return i;
        }
        return -1;
    }
    IEffect*GetEffect(long instanceId) {
        for (int i = 0; i < realtimeEffects.size(); ++i) {
            if (realtimeEffects[i]->GetInstanceId() == instanceId)
            {
                return realtimeEffects[i];
            }
        }
        return nullptr;
    }
    void Activate();
    void Deactivate();
    bool Run(float**inputBuffers, float**outputBuffers,uint32_t samples);

    void ResetAtomBuffers();


    void ProcessParameterRequests(RealtimeParameterRequest *pParameterRequests);
    void GatherParameterRequests(RealtimeParameterRequest *pParameterRequests);


    std::vector<float*> &GetInputBuffers() { return this->pedalBoardInputBuffers;}
    std::vector<float*> &GetoutputBuffers() { return this->pedalBoardOutputBuffers;}



    int GetControlIndex(long instanceId,const std::string&symbol);
    void SetControlValue(int effectIndex, int portIndex, float value);
    void SetBypass(int effectIndex,bool enabled);

    void ComputeVus(RealtimeVuBuffers *vuConfiguration, uint32_t samples);

    float GetControlOutputValue(int effectIndex, int portIndex);

    typedef void (MidiCallbackFn) (void* data,uint64_t intanceId, int controlIndex, float value);
    void OnMidiMessage(size_t size, uint8_t*data,
            void *callbackHandle,
            MidiCallbackFn*pfnCallback);
        
};

} // namespace