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

#include "ss.hpp"
#include <assert.h>
#include "PiPedalHost.hpp"

#include "vst3/Vst3Host.hpp"
#include "Lv2Log.hpp"
#include <unordered_map>
#include <set>
#include <unordered_set>
#include <filesystem>

#include "RingBufferReader.hpp"

#include "public.sdk/source/vst/hosting/module.h"
#include "public.sdk/source/vst/hosting/hostclasses.h"
#include "public.sdk/source/vst/hosting/plugprovider.h"
#include "public.sdk/source/common/memorystream.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "pluginterfaces/vst/ivstunits.h"
#include "public.sdk/source/vst/utility/stringconvert.h"
#include "public.sdk/source/vst/hosting/eventlist.h"
#include "public.sdk/source/vst/hosting/parameterchanges.h"
#include "public.sdk/source/vst/hosting/processdata.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "pluginterfaces/vst/ivstmidicontrollers.h"

#include "public.sdk/samples/vst-hosting/audiohost/source/media/iparameterclient.h"
#include "public.sdk/samples/vst-hosting/audiohost/source/media/imediaserver.h"

#include <array>

#include <sstream>
#include "LiteralVersion.hpp"

#include "Vst3MidiToEvent.hpp"

#include "vst3/Vst3EffectImpl.hpp"
#include "PiPedalHost.hpp"

using namespace pipedal;

std::unique_ptr<Vst3Effect> Vst3Effect::CreateInstance(uint64_t instanceId, const Vst3PluginInfo &info, IHost *pHost)
{
	std::unique_ptr<Vst3EffectImpl> result = std::make_unique<Vst3EffectImpl>();

	result->Load(instanceId, info, pHost);


	return std::move(result);
}

static double NaNGuard(double value)
{
	if (std::isnan(value)) return 0;
	if (std::isinf(value)) return 0;
	return value;
}

static uint32_t GetRefCount(FUnknown *p)
{
	p->addRef();
	return p->release();
}

//------------------------------------------------------------------------
// From Vst2Wrapper
static MidiCCMapping initMidiCtrlerAssignment(IComponent *component, IMidiMapping *midiMapping)
{
	MidiCCMapping midiCCMapping{};

	if (!midiMapping || !component)
		return midiCCMapping;

	int32 busses = std::min<int32>(component->getBusCount(kEvent, kInput), kMaxMidiMappingBusses);

	if (midiCCMapping[0][0].empty())
	{
		for (int32 b = 0; b < busses; b++)
			for (int32 i = 0; i < kMaxMidiChannels; i++)
				midiCCMapping[b][i].resize(Vst::kCountCtrlNumber);
	}

	ParamID paramID;
	for (int32 b = 0; b < busses; b++)
	{
		for (int16 ch = 0; ch < kMaxMidiChannels; ch++)
		{
			for (int32 i = 0; i < Vst::kCountCtrlNumber; i++)
			{
				paramID = kNoParamId;
				if (midiMapping->getMidiControllerAssignment(b, ch, (CtrlNumber)i, paramID) ==
					kResultTrue)
				{
					// TODO check if tag is associated to a parameter
					midiCCMapping[b][ch][i] = paramID;
				}
				else
					midiCCMapping[b][ch][i] = kNoParamId;
			}
		}
	}
	return midiCCMapping;
}

//------------------------------------------------------------------------
static void assignBusBuffers(const IAudioClient::Buffers &buffers, HostProcessData &processData,
							 bool unassign = false)
{
	// Set outputs
	auto bufferIndex = 0;
	for (auto busIndex = 0; busIndex < processData.numOutputs; busIndex++)
	{
		auto channelCount = processData.outputs[busIndex].numChannels;
		for (auto chanIndex = 0; chanIndex < channelCount; chanIndex++)
		{
			if (bufferIndex < buffers.numOutputs)
			{
				processData.setChannelBuffer(BusDirections::kOutput, busIndex, chanIndex,
											 unassign ? nullptr : buffers.outputs[bufferIndex]);
				bufferIndex++;
			}
		}
	}

	// Set inputs
	bufferIndex = 0;
	for (auto busIndex = 0; busIndex < processData.numInputs; busIndex++)
	{
		auto channelCount = processData.inputs[busIndex].numChannels;
		for (auto chanIndex = 0; chanIndex < channelCount; chanIndex++)
		{
			if (bufferIndex < buffers.numInputs)
			{
				processData.setChannelBuffer(BusDirections::kInput, busIndex, chanIndex,
											 unassign ? nullptr : buffers.inputs[bufferIndex]);

				bufferIndex++;
			}
		}
	}
}

//------------------------------------------------------------------------
static void unassignBusBuffers(const IAudioClient::Buffers &buffers, HostProcessData &processData)
{
	assignBusBuffers(buffers, processData, true);
}

//------------------------------------------------------------------------
//  Vst3Processor
//------------------------------------------------------------------------
Vst3EffectImpl::Vst3EffectImpl()
{
	buffers.inputs = nullptr;
	buffers.outputs = nullptr;
	componentHandler.setEffect(this);
}

//------------------------------------------------------------------------
Vst3EffectImpl::~Vst3EffectImpl()
{
	delete[] buffers.inputs;
	delete[] buffers.outputs;
	terminate();
}

//------------------------------------------------------------------------

void Vst3EffectImpl::SetControl(int index, float value)
{
	this->parameterValues[index] = value;
	ParamID paramId = lv2ToVstParam[index];

	double normalizedValue = controller->plainParamToNormalized(paramId, value);
	this->controller->setParamNormalized(paramId, normalizedValue);

	{
		RtInversionGuard inversionGuard; // boost priority to prevent priority inversion.
		std::lock_guard guard{parameterMutex};

		paramTransferrer.addChange(paramId, normalizedValue, 0);
	}
	if (!isProcessing)
	{
		Run(0, nullptr); // pump parameter changes on UI thread.
	}
}

void Vst3EffectImpl::Load(uint64_t instanceId, const Vst3PluginInfo &info, IHost *pHost)
{
	processContext = {};
	processContext.tempo = 120;


	this->pHost = pHost;
	this->instanceId = instanceId;
	this->info = info;

	size_t nControls = info.pluginInfo_.controls().size();

	inputParameterChanges.setMaxParameters(nControls);
	outputParameterChanges.setMaxParameters(nControls);

	lv2ToVstParam.resize(nControls);
	parameterValues.resize(nControls);
	for (size_t i = 0; i < nControls; ++i)
	{
		const auto &control = info.pluginInfo_.controls()[i];
		ParamID paramId;
		std::stringstream ss(control.symbol());
		ss >> paramId;

		assert(i == control.index());

		lv2ToVstParam[control.index()] = paramId;

		parameterValues[control.index()] = control.default_value();
		if (control.is_bypass())
		{
			bypassControl = i;
		}
	}

	std::string error;
	this->module = Module::create(info.filePath_, error);

	if (!module)
	{
		throw Vst3Exception(SS("Vst3 load failed: " << error << "(" << info.filePath_ << ")"));
	}
	const PluginFactory &factory = module->getFactory();

	ClassInfo myClassInfo;
	bool foundClassInfo = false;

	VST3::Optional<VST3::UID> myUid = VST3::UID::fromString(info.uid_);

	if (!myUid)
	{
		throw Vst3Exception(SS("Invalid UID"));
	}
	for (const ClassInfo &classInfo : factory.classInfos())
	{
		if (classInfo.category() == kVstAudioEffectClass &&
			classInfo.ID() == *myUid)
		{
			myClassInfo = classInfo;
			foundClassInfo = true;
		}
	}
	if (!foundClassInfo)
	{
		throw Vst3Exception(SS("Vst3 load failed: effect class not found in " << info.filePath_));
	}

	plugProvider = owned(NEW PlugProvider(factory, myClassInfo, true));

	this->component = plugProvider->getComponent();
	this->processor = component;

	this->controller = plugProvider->getController();
	controller->queryInterface(IMidiMapping::iid, (void **)&midiMapping);

	paramTransferrer.setMaxParameters(1000);
	if (midiMapping)
		midiCCMapping = initMidiCtrlerAssignment(component, midiMapping);

	setBlockSize((Steinberg::int32)pHost->GetMaxAudioBufferSize());
	setSamplerate((SampleRate)pHost->GetSampleRate());

	ProcessSetup setup{kRealtime, kSample32, blockSize, sampleRate};

	initProcessData();

	if (processor->setupProcessing(setup) != kResultOk)
	{
		throw Vst3Exception(SS(info.pluginInfo_.name() << ": setupProcessing() failed."));
	}

	if (component->setActive(true) != kResultOk)
		throw Vst3Exception(SS(info.pluginInfo_.name() << ": setActive() failed."));

	OPtr<IBStream> state = new MemoryStream();

	assert(GetRefCount(state.get()) == 1);
	this->supportsState = false;
	if (component->getState(state) == kResultOk)
	{
		this->supportsState = true;
		state->seek(0, IBStream::kIBSeekSet);
		controller->setComponentState(state);
		refreshControlValues();
	}
	IComponentHandler *handler;

	controller->setComponentHandler(&componentHandler);

	paramTransferrer.setMaxParameters(1000);

	if (midiMapping)
		midiCCMapping = initMidiCtrlerAssignment(component, midiMapping);

	component->setActive(true);

	for (size_t i = 0; i < this->lv2ToVstParam.size(); ++i)
	{
		fireControlChanged(i, (float)(controller->getParamNormalized(lv2ToVstParam[i])));
	}
}
//------------------------------------------------------------------------
//------------------------------------------------------------------------
// void Vst3PluginImpl::createLocalMediaServer (const Name& name)
// {
// 	mediaServer = createMediaServer (name);
// 	mediaServer->registerAudioClient (this);
// 	mediaServer->registerMidiClient (this);
// }

//------------------------------------------------------------------------

//------------------------------------------------------------------------
void Vst3EffectImpl::terminate()
{
	// mediaServer = nullptr;

	if (!processor)
		return;

	processor->setProcessing(false);
	component->setActive(false);
}

//------------------------------------------------------------------------
void Vst3EffectImpl::initProcessData()
{
	// processData.prepare will be done in setBlockSize

	buffers.numInputs = info.pluginInfo_.audio_inputs();
	buffers.numOutputs = info.pluginInfo_.audio_outputs();
	buffers.numSamples = 0;
	buffers.inputs = new float *[buffers.numInputs + 1];
	buffers.outputs = new float *[buffers.numOutputs + 1];

	for (size_t i = 0; i < buffers.numInputs + 1; ++i)
	{
		buffers.inputs[i] = nullptr;
	}
	for (size_t i = 0; i < buffers.numOutputs + 1; ++i)
	{
		buffers.outputs[i] = nullptr;
	}

	processData.inputEvents = &eventList;
	processData.inputParameterChanges = &inputParameterChanges;
	processData.outputParameterChanges = &outputParameterChanges;
	processData.processContext = &processContext;

	setSamplerate(pHost->GetSampleRate());
	setBlockSize((Steinberg::int32)pHost->GetMaxAudioBufferSize());
}

//------------------------------------------------------------------------
IMidiClient::IOSetup Vst3EffectImpl::getMidiIOSetup() const
{
	IMidiClient::IOSetup iosetup;
	auto count = component->getBusCount(MediaTypes::kEvent, BusDirections::kInput);
	for (int32_t i = 0; i < count; i++)
	{
		BusInfo info;
		if (component->getBusInfo(MediaTypes::kEvent, BusDirections::kInput, i, info) != kResultOk)
			continue;

		auto busName = VST3::StringConvert::convert(info.name, 128);
		iosetup.inputs.push_back(busName);
	}

	count = component->getBusCount(MediaTypes::kEvent, BusDirections::kOutput);
	for (int32_t i = 0; i < count; i++)
	{
		BusInfo info;
		if (component->getBusInfo(MediaTypes::kEvent, BusDirections::kOutput, i, info) !=
			kResultOk)
			continue;

		auto busName = VST3::StringConvert::convert(info.name, 128);
		iosetup.outputs.push_back(busName);
	}

	return iosetup;
}

//------------------------------------------------------------------------
IAudioClient::IOSetup Vst3EffectImpl::getIOSetup() const
{
	IAudioClient::IOSetup iosetup;
	auto count = component->getBusCount(MediaTypes::kAudio, BusDirections::kOutput);
	for (int32_t i = 0; i < count; i++)
	{
		BusInfo info;
		if (component->getBusInfo(MediaTypes::kAudio, BusDirections::kOutput, i, info) !=
			kResultOk)
			continue;

		for (int32_t j = 0; j < info.channelCount; j++)
		{
			auto channelName = VST3::StringConvert::convert(info.name, 128);
			iosetup.outputs.push_back(channelName + " " + std::to_string(j));
		}
	}

	count = component->getBusCount(MediaTypes::kAudio, BusDirections::kInput);
	for (int32_t i = 0; i < count; i++)
	{
		BusInfo info;
		if (component->getBusInfo(MediaTypes::kAudio, BusDirections::kInput, i, info) != kResultOk)
			continue;

		for (int32_t j = 0; j < info.channelCount; j++)
		{
			auto channelName = VST3::StringConvert::convert(info.name, 128);
			iosetup.inputs.push_back(channelName + " " + std::to_string(j));
		}
	}

	return iosetup;
}

//------------------------------------------------------------------------
void Vst3EffectImpl::preprocess(Buffers &buffers, int64_t continousFrames)
{
	processData.numSamples = buffers.numSamples;
	processContext.continousTimeSamples = continousFrames;
	assignBusBuffers(buffers, processData);

	{
		std::lock_guard guard{parameterMutex};

		paramTransferrer.transferChangesTo(inputParameterChanges);
	}
	outputParameterChanges.clearQueue();
}

//------------------------------------------------------------------------
bool Vst3EffectImpl::process(Buffers &buffers, int64_t continousFrames)
{
	if (!processor || !isProcessing)
		return false;
	buffers.numSamples = continousFrames;
	preprocess(buffers, continousFrames);

	if (processor->process(processData) != kResultOk)
		return false;

	postprocess(buffers);

	return true;
}
//------------------------------------------------------------------------
void Vst3EffectImpl::postprocess(Buffers &buffers)
{
	eventList.clear();
	inputParameterChanges.clearQueue();
	unassignBusBuffers(buffers, processData);
}

void Vst3EffectImpl::SendControlChanges(RealtimeRingBufferWriter *realtimeRingBufferWriter)
{
	for (auto i = 0; i < outputParameterChanges.getParameterCount(); ++i)
	{
		IParamValueQueue *queue = outputParameterChanges.getParameterData(i);
		auto points = queue->getPointCount();
		if (points != 0)
		{
			Steinberg::Vst::ParamValue value;
			Steinberg::int32 sampleOffset;
			queue->getPoint(points - 1, sampleOffset, value);

			auto paramId = queue->getParameterId();

			int thisControlId = -1;
			for (size_t lv2Id = 0; lv2Id < this->lv2ToVstParam.size(); ++lv2Id)
			{
				if (lv2ToVstParam[lv2Id] == paramId)
				{
					thisControlId = (int)lv2Id;
					break;
				}
			}
			if (thisControlId != -1)
			{
				// realtimeRingBufferWriter->NotifyVst3ControlValue(this->instanceId,thisControlId,(float)value);
			}
		}
	}
}

//------------------------------------------------------------------------
bool Vst3EffectImpl::setSamplerate(SampleRate value)
{
	if (sampleRate == value)
		return true;

	sampleRate = value;
	processContext.sampleRate = sampleRate;
	if (blockSize == 0)
		return true;
	return true;
}

//------------------------------------------------------------------------
bool Vst3EffectImpl::setBlockSize(int32 value)
{
	blockSize = value;
	if (sampleRate == 0)
		return true;

	processData.prepare(*component, blockSize, kSample32);
	return true;
}

//------------------------------------------------------------------------

//------------------------------------------------------------------------
bool Vst3EffectImpl::isPortInRange(int32 port, int32 channel) const
{
	return port < kMaxMidiMappingBusses && !midiCCMapping[port][channel].empty();
}

//------------------------------------------------------------------------
bool Vst3EffectImpl::processVstEvent(const IMidiClient::Event &event, int32 port)
{
	auto vstEvent = midiToEvent(event.type, event.channel, event.data0, event.data1);
	if (vstEvent)
	{
		vstEvent->busIndex = port;
		if (eventList.addEvent(*vstEvent) != kResultOk)
		{
			assert(false && "Event was not added to EventList!");
		}

		return true;
	}

	return false;
}

//------------------------------------------------------------------------
bool Vst3EffectImpl::processParamChange(const IMidiClient::Event &event, int32 port)
{
	auto paramMapping = [port, this](int32 channel, MidiData data1) -> ParamID
	{
		if (!isPortInRange(port, channel))
			return kNoParamId;

		return midiCCMapping[port][channel][data1];
	};

	auto paramChange =
		midiToParameter(event.type, event.channel, event.data0, event.data1, paramMapping);
	if (paramChange)
	{
		int32 index = 0;
		IParamValueQueue *queue =
			inputParameterChanges.addParameterData((*paramChange).first, index);
		if (queue)
		{
			if (queue->addPoint(event.timestamp, (*paramChange).second, index) != kResultOk)
			{
				assert(false && "Parameter point was not added to ParamValueQueue!");
			}
		}

		return true;
	}

	return false;
}

//------------------------------------------------------------------------
bool Vst3EffectImpl::onEvent(const IMidiClient::Event &event, int32_t port)
{
	// Try to create Event first.
	if (processVstEvent(event, port))
		return true;

	// In case this is no event it must be a parameter.
	if (processParamChange(event, port))
		return true;

	// TODO: Something else???

	return true;
}

//------------------------------------------------------------------------

void Vst3EffectImpl::Activate()
{
	processor->setProcessing(true); // != kResultOk
	this->isProcessing  = true;
}

void Vst3EffectImpl::Deactivate()
{
	if (isProcessing)
	{
		Run(0, nullptr); // make sure all pending events have been processed.
		isProcessing = false;
	}
}

void Vst3EffectImpl::Prepare(int32_t sampleRate, size_t maxBufferSize, int inputChannels, int outputChannels)
{
}
void Vst3EffectImpl::Unprepare()
{
	if (isProcessing)
	{
		Deactivate();
	}
}

ssize_t Vst3EffectImpl::ParamIdToLv2Id(ParamID id) const
{
	for (size_t i = 0; i < this->lv2ToVstParam.size(); ++i)
	{
		if (lv2ToVstParam[i] == id)
		{
			return (ssize_t)i;
		}
	}
	return -1;
}

tresult Vst3EffectImpl::beginEdit(ParamID id)
{
	return kResultOk;
}
tresult Vst3EffectImpl::performEdit(ParamID id, ParamValue valueNormalized)
{
	int ix = ParamIdToLv2Id(id);
	if (ix != -1)
	{
		fireControlChanged(ix, (float)valueNormalized);
	}
	return kResultOk;
}

tresult Vst3EffectImpl::endEdit(ParamID id)
{
	return kResultOk;
}

void Vst3EffectImpl::transferControllerStateToComponent()
{
	OPtr<IRtStream> bStream = this->streamPool.AllocateBStream();
	assert(GetRefCount(bStream.get()) == 1);

	RtInversionGuard inversionGuard; // boost priority to prevent priority inversion.
	std::lock_guard guard{parameterMutex};

	paramTransferrer.removeChanges();

	// assume that parameters are straightforward and uncomplicated if they
	// didn't provide BStream-based state management
	// Vst2 used to do this. It's not clear that this is still legal in Vst3.

	for (int index = 0; index < this->lv2ToVstParam.size(); ++index)
	{
		ParamID paramId = lv2ToVstParam[index];
		paramTransferrer.addChange(paramId,
									controller->plainParamToNormalized(paramId, parameterValues[index]),
									0);
	}
	if (!this->isProcessing)
	{
		Run(0,nullptr);
	}
}


tresult Vst3EffectImpl::restartComponent(int32 flags)
{
	if (flags & (RestartFlags::kParamValuesChanged))
	{
		refreshControlValues();
		transferControllerStateToComponent();
	}

	return kResultOk;
}

static std::vector<uint8_t> StreamToVec(IBStream *stream)
{
	Steinberg::int64 length;
	stream->seek(0, IBStream::kIBSeekEnd, &length);
	stream->seek(0, IBStream::kIBSeekSet);
	std::vector<uint8_t> result;
	result.resize(length);
	Steinberg::int32 numRead = 0;
	stream->read(&result[0], length, &numRead);
	if (numRead != length)
	{
		throw Vst3Exception("Failed to read.");
	}
	return result;
}

void Vst3EffectImpl::CheckSync()
{
	OPtr<IBStream> stream{new MemoryStream()};

	if (this->controller->getState(stream) == kResultOk)
	{
		std::vector<uint8_t> initialState = StreamToVec(stream);
		OPtr<IBStream> stream2{new MemoryStream()};
		this->component->getState(stream2);
		stream2->seek(0, IBStream::kIBSeekSet);
		this->controller->setState(stream2);

		OPtr<IBStream> stream3{new MemoryStream()};
		this->controller->getState(stream3);
		std::vector<uint8_t> newState = StreamToVec(stream3);

		if (initialState.size() != newState.size())
		{
			throw Vst3Exception("State changed.");
		}
		for (size_t i = 0; i < newState.size(); ++i)
		{
			if (initialState[i] != newState[i])
			{
				throw Vst3Exception("State changed.");
			}
		}
	}
}

bool Vst3EffectImpl::GetState(std::vector<uint8_t>*state)
{
	OPtr<IBStream> stream{new MemoryStream()};

	if (component->getState(stream) != kResultOk)
	{
		return false;
	}
	Steinberg::int64 length = 0;
	if (stream->seek(0, IBStream::kIBSeekEnd, &length) != kResultOk)
	{
		return false;
	}
	stream->seek(0, IBStream::kIBSeekSet);
	std::vector<uint8_t> result;

	result.resize(length);
	int64 ix = 0;
	while (length != 0)
	{
		int32 thisTime;
		if (length > 0x10000000)
		{
			thisTime = 0x10000000;
		}
		else
		{
			thisTime = (int32)length;
		}
		if (stream->read((void *)(&result[ix]), thisTime, &thisTime) != kResultOk)
		{
			throw Vst3Exception(this->info.pluginInfo_.name() + ": Failed to read state.");
		}
		if (thisTime == 0)
		{
			throw Vst3Exception(this->info.pluginInfo_.name() + ": Unexpected end of file while reading state.");
		}
		ix += thisTime;
		length -= thisTime;
	}
	*state = std::move(result);
	return true;
}
void Vst3EffectImpl::SetState(const std::vector<uint8_t> state)
{
	OPtr<IBStream> stream{new MemoryStream((void *)(&state[0]), state.size())};

	if (controller->setComponentState(stream) != kResultOk)
	{
		throw Vst3Exception(this->info.pluginInfo_.name() + " Failed to restore state.");
	}
	refreshControlValues();
	stream->seek(0,IBStream::kIBSeekSet);
	if (component->setState(stream) != kResultOk)
	{
		transferControllerStateToComponent();
	}
}

void Vst3EffectImpl::refreshControlValues()
{
	for (size_t i = 0; i < lv2ToVstParam.size(); ++i)
	{
		fireControlChanged(i, controller->getParamNormalized(lv2ToVstParam[i]));
	}
}
std::vector<Vst3ProgramList> Vst3EffectImpl::GetProgramList(int32_t programListId)
{
	std::vector<Vst3ProgramList> result;

	FUnknownPtr<IUnitInfo> iUnitInfo(controller);
	if (!iUnitInfo)
		throw Vst3Exception("Invalid unit info");

	int count = iUnitInfo->getProgramListCount();
	for (int i = 0; i < count; ++i)
	{
		ProgramListInfo programListInfo;
		if (iUnitInfo->getProgramListInfo(i, programListInfo) != kResultOk)
		{
			break;
		}
		if (programListInfo.id == programListId)
		{
			Vst3ProgramList list;

			list.name = VST3::StringConvert::convert(programListInfo.name);

			for (int prog = 0; prog < programListInfo.programCount; ++prog)
			{
				Vst::String128 progName;
				if (iUnitInfo->getProgramName(programListInfo.id, prog, progName) == kResultOk)
				{
					Vst3ProgramListEntry entry;
					entry.id = prog;
					entry.name = VST3::StringConvert::convert(progName);
					list.programs.push_back(std::move(entry));
				}
			}
			result.push_back(std::move(list));
		}
	}
	return result;
}

void Vst3EffectImpl::fireControlChanged(int control, float normalizedValue)
{
	normalizedValue = NaNGuard(normalizedValue);
	float plainValue = NaNGuard(this->controller->normalizedParamToPlain(this->lv2ToVstParam[control], normalizedValue));
	if (parameterValues[control] != plainValue)
	{
		parameterValues[control] = plainValue;
		controlChangedHandler(control,plainValue);
	}
}


const Lv2PluginUiInfo& Vst3EffectImpl::GetCurrentPluginInfo()
{
	Vst3Host::Private::UpdateControlInfo(this->controller,this->info.pluginInfo_);
	return this->info.pluginInfo_;
}

JSON_MAP_BEGIN(Vst3PluginInfo)
JSON_MAP_REFERENCE(Vst3PluginInfo, filePath)
JSON_MAP_REFERENCE(Vst3PluginInfo, version)
JSON_MAP_REFERENCE(Vst3PluginInfo, uid)
JSON_MAP_REFERENCE(Vst3PluginInfo, pluginInfo)

JSON_MAP_END()
