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

#include "Vst3Effect.hpp"
#include <vector>

#include "RtInversionGuard.hpp"
#include "Vst3RtStream.hpp"

#pragma once

namespace pipedal
{
	using namespace std;
	using namespace pipedal;
	using namespace VST3::Hosting;
	using namespace Steinberg;
	using namespace Steinberg::Vst;

	enum
	{
		kMaxMidiMappingBusses = 4,
		kMaxMidiChannels = 16
	};
	using Controllers = std::vector<int32>;
	using Channels = std::array<Controllers, kMaxMidiChannels>;
	using Busses = std::array<Channels, kMaxMidiMappingBusses>;
	using MidiCCMapping = Busses;

	class Vst3EffectImpl : public Vst3Effect,
						   public IAudioClient,
						   public IMidiClient
	{
	public:
		//--------------------------------------------------------------------
		using Name = std::string;

		Vst3EffectImpl();
		virtual ~Vst3EffectImpl();
		void Load(uint64_t instanceId, const Vst3PluginInfo &info, IHost *pHost);

		// IEffect
		virtual bool IsVst3() const { return true; }
		virtual uint64_t GetInstanceId() const { return instanceId; }
		virtual int GetControlIndex(const std::string &symbol) const
		{
			for (size_t i = 0; i < info.pluginInfo_.controls().size(); ++i)
			{
				if (info.pluginInfo_.controls()[i].symbol() == symbol)
				{
					return (int)i;
				}
			}
			return -1;
		}

		bool SupportsState() const { return supportsState; }

		bool GetState(std::vector<uint8_t> *state);

		void SetState(const std::vector<uint8_t> state);


        const Lv2PluginUiInfo&  GetCurrentPluginInfo();

		virtual void CheckSync();

		//- PluginHost Interfaces.
		virtual void SetControl(int index, float value);
		virtual float GetControlValue(int index) const
		{
			return this->parameterValues[index];
		}

        virtual std::vector<Vst3ProgramList> GetProgramList(int32_t programListId);

		int bypassControl = -1;
		bool bypassEnable = false;

		virtual void SetBypass(bool enable)
		{
			if (bypassControl != -1)
			{
				SetControl(bypassControl, enable ? 1 : 0);
			}
			else
			{
				bypassEnable = enable;
			}
		}

		virtual void Activate();
		virtual void Deactivate();

		virtual float GetOutputControlValue(int controlIndex) const
		{
			return this->parameterValues[controlIndex];
		}

		virtual int GetNumberOfInputAudioPorts() const
		{
			return info.pluginInfo_.audio_inputs();
		}
		virtual int GetNumberOfOutputAudioPorts() const
		{
			return info.pluginInfo_.audio_outputs();
		}
		virtual float *GetAudioInputBuffer(int index) const { return buffers.inputs[index]; }
		virtual float *GetAudioOutputBuffer(int index) const { return buffers.outputs[index]; }
		virtual void ResetAtomBuffers() {}
		virtual void RequestParameter(LV2_URID uridUri) {}					// no vst equivalent.
		virtual void GatherPatchProperties(RealtimePatchPropertyRequest *pRequest) {} // no vst equivalent.

		virtual void SetAudioInputBuffer(int index, float *buffer)
		{
			buffers.inputs[index] = buffer;
		}
		virtual void SetAudioOutputBuffer(int index, float *buffer)
		{
			buffers.outputs[index] = buffer;
		}


		virtual void Run(uint32_t samples, RealtimeRingBufferWriter *realtimeRingBufferWriter)
		{
			process(buffers, samples);
			SendControlChanges(realtimeRingBufferWriter);
		}

		// Vst3Effect interfaces.

		virtual void Prepare(int32_t sampleRate, size_t maxBufferSize, int inputChannels, int outputChannels);
		virtual void Unprepare();

		// IAudioClient
		bool process(Buffers &buffers, int64_t continousFrames) override;
		bool setSamplerate(SampleRate value) override;
		bool setBlockSize(int32 value) override;
		IAudioClient::IOSetup getIOSetup() const override;

		// IMidiClient
		bool onEvent(const Event &event, int32_t port) override;
		IMidiClient::IOSetup getMidiIOSetup() const override;

		// IParameterClient


		//--------------------------------------------------------------------
	private:
		bool supportsState = false;
		IHost *pHost;
		Module::Ptr module;
		IPtr<IMidiMapping> midiMapping;
		IPtr<PlugProvider> plugProvider;

		RtStreamPool streamPool;

		void SendControlChanges(RealtimeRingBufferWriter *realtimeRingBufferWriter);
		//--------------------------------------------------------------------
	private:
		std::mutex parameterMutex;
		Buffers buffers;
		std::vector<ParamID> lv2ToVstParam;
		std::vector<float> parameterValues;

		// void createLocalMediaServer(const Name &name);
		uint64_t instanceId;
		Vst3PluginInfo info;
		void terminate();
		void initProcessData();
		void updateBusBuffers(Buffers &buffers, HostProcessData &processData);
		void preprocess(Buffers &buffers, int64_t continousFrames);
		void postprocess(Buffers &buffers);
		bool isPortInRange(int32 port, int32 channel) const;
		bool processVstEvent(const IMidiClient::Event &event, int32 port);
		bool processParamChange(const IMidiClient::Event &event, int32 port);

		SampleRate sampleRate = 0;
		int32 blockSize = 0;
		HostProcessData processData;
		ProcessContext processContext;
		EventList eventList;
		ParameterChanges inputParameterChanges;
		ParameterChanges outputParameterChanges;
		IComponent *component = nullptr;
		IEditController *controller = nullptr;
		FUnknownPtr<IAudioProcessor> processor;

		ParameterChangeTransfer paramTransferrer;

		MidiCCMapping midiCCMapping;
		// IMediaServerPtr mediaServer;
		bool isProcessing = false;

		Name name;

	private:
		ssize_t ParamIdToLv2Id(ParamID id) const;

		class ComponentHandler : public IComponentHandler
		{
		private:
			Vst3EffectImpl *pEffect = nullptr;
		public:
			ComponentHandler() { }

			void setEffect(Vst3EffectImpl*effect)
			{
				pEffect = effect;
			}
			tresult PLUGIN_API beginEdit(ParamID id) override
			{
				return pEffect->beginEdit(id);
			}
			tresult PLUGIN_API performEdit(ParamID id, ParamValue valueNormalized) override
			{
				return pEffect->performEdit(id, valueNormalized);
			}
			tresult PLUGIN_API endEdit(ParamID id) override
			{
				return pEffect->endEdit(id);
			}
			tresult PLUGIN_API restartComponent(int32 flags) override
			{
				return pEffect->restartComponent(flags);
			}

		private:
			tresult PLUGIN_API queryInterface(const TUID _iid, void ** obj) override
			{
				if (_iid == IComponentHandler_iid)
				{
					addRef();
					*((FUnknown**)obj) = this;
					return kResultOk;
				}

				return kNoInterface;
			}
			uint32 PLUGIN_API addRef() override { return 1000; }
			uint32 PLUGIN_API release() override { return 1000; }
		};

		ComponentHandler componentHandler;
		void transferControllerStateToComponent();
		void refreshControlValues();

		virtual tresult beginEdit(ParamID id);
		virtual tresult performEdit(ParamID id, ParamValue valueNormalized);
		virtual tresult endEdit(ParamID id);
		virtual tresult restartComponent(int32 flags);

	private:
		ControlChangedHandler controlChangedHandler = [] (int control,float value) {};
		void fireControlChanged(int control, float  normalizedValue);
		
	public:
		virtual void SetControlChangedHandler(const ControlChangedHandler& handler) 
		{
			controlChangedHandler = handler;
		}
        virtual void RemoveControlChangedHandler() {
			controlChangedHandler = [] (int,float){};
		}

	};

}