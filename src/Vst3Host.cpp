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
#include "PiPedalHost.hpp"
#include "vst3/Vst3Host.hpp"
#include "Lv2Log.hpp"
#include <unordered_map>
#include <set>
#include <unordered_set>
#include <filesystem>
#include "public.sdk/source/vst/hosting/module.h"
#include "public.sdk/source/vst/hosting/hostclasses.h"
#include "public.sdk/source/vst/hosting/plugprovider.h"
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
#include "json.hpp"

using namespace std;
using namespace pipedal;
using namespace VST3::Hosting;
using namespace Steinberg;
using namespace Steinberg::Vst;

namespace pipedal
{

	class Vst3HostApplication : public HostApplication
	{
	public:
		Vst3HostApplication() {}
		//--- IHostApplication ---------------
		tresult PLUGIN_API getName(String128 name)
		{
			return VST3::StringConvert::convert("PiPedal", name) ? kResultTrue : kInternalError;
		}
	};

	class Vst3HostImpl : public Vst3Host
	{
	private:
		OPtr<Vst3HostApplication> hostApplication;
		std::string cacheFilePath;

	public:
		Vst3HostImpl(const std::string &cacheFilePath);

		const Vst3Host::PluginList &RescanPlugins() override;
		const Vst3Host::PluginList &RefreshPlugins() override;
		const Vst3Host::PluginList &getPluginList() override { return pluginList; }

		std::unique_ptr<Vst3Effect> CreatePlugin(long instanceId, const std::string &url, IHost *pHost) override;
		std::unique_ptr<Vst3Effect> CreatePlugin(PedalBoardItem &pedalBoardItem, IHost *pHost) override;

	private:
		Lv2PluginUiInfo *GetPluginInfo(const std::string &uri);

		void LoadPluginCache();
		void SavePluginCache();

		bool havePluginsChanged() const;
		PluginList pluginList;

		void EnsureContext();
		std::unique_ptr<Vst3PluginInfo> MakeDetailedPluginInfo(const PluginFactory &factory, const ClassInfo &classInfo, const std::string &path);

		void MakeVst3Info(const std::filesystem::path &path, Vst3Host::PluginList &list);
	};

	Vst3Host::Ptr Vst3Host::CreateInstance(const std::string &cacheFilePath)
	{
		return std::make_unique<Vst3HostImpl>(cacheFilePath);
	}

	//--------------------------------------------------------------

} // namespace pipedal

Vst3HostImpl::Vst3HostImpl(const std::string &cacheFilePath)
	: cacheFilePath(cacheFilePath)
{
	this->hostApplication = new Vst3HostApplication();
	EnsureContext();
}

static bool hasSubCategory(const ClassInfo &classInfo, const std::string &subcategory)
{
	for (auto &category : classInfo.subCategories())
	{
		if (category == subcategory)
		{
			return true;
		}
	}
	return false;
}
static PluginType getVst3PluginType(const ClassInfo &classInfo)
{
	if (hasSubCategory(classInfo, "Instrument"))
	{
		return PluginType::InstrumentPlugin;
	}
	if (!hasSubCategory(classInfo, "Fx"))
	{
		return PluginType::InvalidPlugin;
	}

	// kFxAnalyzer			= "Fx|Analyzer";	///< Scope, FFT-Display, Loudness Processing...
	if (hasSubCategory(classInfo, "Analyzer"))
	{
		return PluginType::AnalyserPlugin;
	}
	// kFxDelay				= "Fx|Delay";		///< Delay, Multi-tap Delay, Ping-Pong Delay...
	if (hasSubCategory(classInfo, "Delay"))
	{
		return PluginType::DelayPlugin;
	}

	// kFxDistortion			= "Fx|Distortion";	///< Amp Simulator, Sub-Harmonic, SoftClipper...
	if (hasSubCategory(classInfo, "Distortion"))
	{
		return PluginType::DistortionPlugin;
	}
	// kFxDynamics			= "Fx|Dynamics";	///< Compressor, Expander, Gate, Limiter, Maximizer, Tape Simulator, EnvelopeShaper...
	if (hasSubCategory(classInfo, "Dynamics"))
	{
		return PluginType::DynamicsPlugin;
	}
	// kFxEQ					= "Fx|EQ";			///< Equalization, Graphical EQ...
	if (hasSubCategory(classInfo, "EQ"))
	{
		return PluginType::EQPlugin;
	}

	// kFxFilter				= "Fx|Filter";		///< WahWah, ToneBooster, Specific Filter,...
	if (hasSubCategory(classInfo, "Filter"))
	{
		return PluginType::FilterPlugin;
	}

	// kFx					= "Fx";				///< others type (not categorized)
	// kFxSpatial				= "Fx|Spatial";		///< MonoToStereo, StereoEnhancer,...
	if (hasSubCategory(classInfo, "Spatial"))
	{
		return PluginType::SpatialPlugin;
	}

	// kFxGenerator			= "Fx|Generator";	///< Tone Generator, Noise Generator...
	if (hasSubCategory(classInfo, "Generator"))
	{
		return PluginType::GeneratorPlugin;
	}

	// kFxReverb				= "Fx|Reverb";		///< Reverberation, Room Simulation, Convolution Reverb...
	if (hasSubCategory(classInfo, "Reverb"))
	{
		return PluginType::ReverbPlugin;
	}
	// kFxPitchShift			= "Fx|Pitch Shift";	///< Pitch Processing, Pitch Correction, Vocal Tuning...
	if (hasSubCategory(classInfo, "Pitch Shift"))
	{
		return PluginType::PitchPlugin;
	}

	// kFxModulation			= "Fx|Modulation";	///< Phaser, Flanger, Chorus, Tremolo, Vibrato, AutoPan, Rotary, Cloner...
	if (hasSubCategory(classInfo, "Modulation"))
	{
		return PluginType::ModulatorPlugin;
	}

	// kFxRestoration			= "Fx|Restoration";	///< Denoiser, Declicker,...
	if (hasSubCategory(classInfo, "Restoration"))
	{
		return PluginType::UtilityPlugin;
	}
	// kFxSurround			= "Fx|Surround";	///< dedicated to surround processing: LFE Splitter, Bass Manager...
	if (hasSubCategory(classInfo, "Surround"))
	{
		return PluginType::UtilityPlugin;
	}

	// kFxTools				= "Fx|Tools";		///< Volume, Mixer, Tuner...
	if (hasSubCategory(classInfo, "Tools"))
	{
		return PluginType::UtilityPlugin;
	}
	// kFxMastering			= "Fx|Mastering";	///< Dither, Noise Shaping,...
	if (hasSubCategory(classInfo, "Mastering"))
	{
		return PluginType::UtilityPlugin;
	}

	// kFxNetwork				= "Fx|Network";		///< using Network
	if (hasSubCategory(classInfo, "Network"))
	{
		return PluginType::UtilityPlugin;
	}
	// kFxInstrument			= "Fx|Instrument";	///< Fx which could be loaded as Instrument too
	// kFxInstrumentExternal	= "Fx|Instrument|External";	///< Fx which could be loaded as Instrument too and is external (wrapped Hardware)
	if (hasSubCategory(classInfo, "Instrument"))
	{
		return PluginType::InstrumentPlugin;
	}
	return PluginType::Plugin;
}

void Vst3HostImpl::EnsureContext()
{
	PluginContextFactory::instance().setPluginContext(this->hostApplication);
}

void Vst3Host::Private::UpdateControlInfo(IEditController *controller, Lv2PluginUiInfo &pluginInfo)
{
	// update UI text and control info.
	FUnknownPtr<IUnitInfo> iUnitInfo(controller);
	pluginInfo.controls().resize(0);
	pluginInfo.port_groups().resize(0);

	if (iUnitInfo)
	{
		auto nUnits = iUnitInfo->getUnitCount();
		for (int i = 0; i < nUnits; ++i)
		{
			Steinberg::Vst::UnitInfo unitInfo;
			iUnitInfo->getUnitInfo(i, unitInfo);
			Lv2PluginUiPortGroup portGroup(
				SS(unitInfo.id), VST3::StringConvert::convert(unitInfo.name), SS(unitInfo.parentUnitId), unitInfo.programListId);

			pluginInfo.port_groups().push_back(portGroup);
		}
	}

	auto nParams = controller->getParameterCount();
	for (int32 param = 0; param < nParams; ++param)
	{
		Steinberg::Vst::ParameterInfo info;
		tresult tErr = controller->getParameterInfo(param, info);

		Lv2PluginUiControlPort port;
		port.index(param);		  // used for LV2 purposes.
		port.symbol(SS(info.id)); // Used for VST3 purposes. convert to int to specifiy a vst control id.

		std::string shortTitle = VST3::StringConvert::convert(info.shortTitle);
		std::string title = VST3::StringConvert::convert(info.title);
		if (shortTitle.length() == 0)
		{
			shortTitle = title;
		}
		port.name(shortTitle);
		port.comment(title);

		// guard against NaN value (mda Talkbox)
		port.display_priority(param);

		bool isList = (info.flags & ParameterInfo::kIsList) != ParameterInfo::kNoFlags;
		bool canAutomate = (info.flags & ParameterInfo::kCanAutomate) != ParameterInfo::kNoFlags;
		bool isProgramChange = (info.flags & ParameterInfo::kIsProgramChange) != ParameterInfo::kNoFlags;
		bool isReadOnly = (info.flags & ParameterInfo::kIsReadOnly) != ParameterInfo::kNoFlags;
		bool notOnGui = (info.flags & ParameterInfo::kIsHidden) != ParameterInfo::kNoFlags;
		bool isBypass = (info.flags & ParameterInfo::kIsBypass) != ParameterInfo::kNoFlags;

		port.is_bypass(isBypass);

		port.not_on_gui(isReadOnly | notOnGui);

		port.min_value(controller->normalizedParamToPlain(info.id, 0));
		port.max_value(controller->normalizedParamToPlain(info.id, 1));

		double t = controller->normalizedParamToPlain(info.id, info.defaultNormalizedValue);
		if (std::isnan(t) || std::isinf(t))
		{
			t = 0;
		}

		port.default_value(t);

		port.range_steps(info.stepCount == 0 ? 0 : info.stepCount + 1);
		if (isList && !isProgramChange)
		{
			port.enumeration_property(true);
			for (int i = 0; i <= info.stepCount; ++i)
			{
				double normalizedValue;
				if (info.stepCount == 0)
				{
					normalizedValue = 0;
				}
				else
				{
					normalizedValue = i * 1.0 / (info.stepCount);
				}

				String128 strValue;
				if (controller->getParamStringByValue(info.id, normalizedValue, strValue) == kResultOk)
				{
					Lv2ScalePoint scalePoint((float)controller->normalizedParamToPlain(info.id, normalizedValue), VST3::StringConvert::convert(strValue));
					port.scale_points().push_back(scalePoint);
				}
			}
		}
		if (info.unitId != 0)
		{
			port.port_group(SS(info.unitId));
		}
		std::string units = VST3::StringConvert::convert(info.units);
		if (units.size() != 0)
		{
			if (units == "dB")
			{
				port.units(Units::db);
			}
			else if (units == "%")
			{
				port.units(Units::pc);
			}
			else if (units == "Hz")
			{
				port.units(Units::hz);
			}
			else if (units == "kHz")
			{
				port.units(Units::khz);
			}
			else if (units == "sec" || units == "s")
			{
				port.units(Units::s);
			}
			else if (units == "ms")
			{
				port.units(Units::ms);
			}
			else if (units == "cent")
			{
				port.units(Units::cent);
			}
			else
			{
				port.units(Units::custom);
				port.custom_units(units);
			}
		}

		pluginInfo.controls().push_back(port);
	}
}

static void UpdateBusInfo(IComponent *component, Lv2PluginUiInfo &pluginInfo)
{

	int audioOutputs = 0;
	auto count = component->getBusCount(MediaTypes::kAudio, BusDirections::kOutput);
	for (int32_t i = 0; i < count; i++)
	{
		BusInfo busInfo;
		if (component->getBusInfo(MediaTypes::kAudio, BusDirections::kOutput, i, busInfo) ==
			kResultOk)
		{
			auto channelName = VST3::StringConvert::convert(busInfo.name, 128);

			if (busInfo.busType == BusTypes::kMain) // aux channels not currently supported.
			{
				audioOutputs += busInfo.channelCount;
			}
		}
	}
	pluginInfo.audio_outputs(audioOutputs);

	count = component->getBusCount(MediaTypes::kAudio, BusDirections::kInput);
	int audioInputs = 0;
	for (int32_t i = 0; i < count; i++)
	{
		BusInfo busInfo;
		if (component->getBusInfo(MediaTypes::kAudio, BusDirections::kInput, i, busInfo) ==
			kResultOk)
		{
			auto channelName = VST3::StringConvert::convert(busInfo.name, 128);
			if (busInfo.busType == BusTypes::kMain) // aux channels not currently supported.
			{
				audioInputs += busInfo.channelCount;
			}
		}
	}

	pluginInfo.audio_inputs(audioInputs);

	count = component->getBusCount(MediaTypes::kEvent, BusDirections::kInput);
	if (count >= 1)
	{
		pluginInfo.has_midi_input(1);
	}
	count = component->getBusCount(MediaTypes::kEvent, BusDirections::kOutput);
	if (count >= 1)
	{
		pluginInfo.has_midi_output(1);
	}
}

unique_ptr<Vst3PluginInfo> Vst3HostImpl::MakeDetailedPluginInfo(const PluginFactory &factory, const ClassInfo &classInfo, const std::string &path)
{
	IPtr<PlugProvider> plugProvider = owned(NEW PlugProvider(factory, classInfo, true));
	{
		OPtr<IComponent> component = plugProvider->getComponent();
		OPtr<IEditController> controller = plugProvider->getController();

		FUnknownPtr<IMidiMapping> midiMapping(controller);

		unique_ptr<Vst3PluginInfo> info = make_unique<Vst3PluginInfo>();

		info->filePath_ = path;
		info->version_ = classInfo.version();
		info->uid_ = classInfo.ID().toString();

		Lv2PluginUiInfo &pluginInfo = info->pluginInfo_;

		pluginInfo.uri(SS("vst3:" << classInfo.ID().toString()));

		pluginInfo.is_vst3(true);
		pluginInfo.name(classInfo.name());
		pluginInfo.author_name(classInfo.vendor());

		pluginInfo.plugin_type(
			getVst3PluginType(classInfo));
		pluginInfo.plugin_display_type(""); // fill this in later. VSTHost has list of name translations; we don;'t.

		Private::UpdateControlInfo(controller, pluginInfo);
		UpdateBusInfo(component, pluginInfo);

		return info;
	}
}
void Vst3HostImpl::MakeVst3Info(const std::filesystem::path &path, Vst3Host::PluginList &list)
{

	EnsureContext();
	unique_ptr<Vst3PluginInfo> result = std::make_unique<Vst3PluginInfo>();
	result->filePath_ = path;

	string error;
	Module::Ptr module = Module::create(path.string(), error);

	if (!module)
	{
		Lv2Log::error(SS("Vst3 load failed: " << error << "(" << path.string() << ")"));
		return;
	}
	const PluginFactory &factory = module->getFactory();

	for (const ClassInfo &classInfo : factory.classInfos())
	{
		if (classInfo.category() == kVstAudioEffectClass)
		{
			bool found = false;
			std::string newUid = classInfo.ID().toString();
			for (const auto &existingPlugin : list)
			{
				if (existingPlugin->uid_ == newUid)
				{
					found = true;
					break; // first instance takes precedence (e.g. ~/.vst3 takes precedence over /usr/lib/vst3 )
				}
			}
			if (!found)
			{
				std::unique_ptr<Vst3PluginInfo> pluginInfo = MakeDetailedPluginInfo(factory, classInfo, path);

				list.push_back(std::move(pluginInfo));
			}
		}
	}
}

bool Vst3HostImpl::havePluginsChanged() const
{

	std::unordered_set<std::string> modules;

	for (const auto &plugin : pluginList)
	{
		if (!std::filesystem::exists(plugin->filePath_))
		{
			return true;
		}
		modules.insert(plugin->filePath_);
	}
	Module::PathList pathList = Module::getModulePaths();

	for (const auto &path : pathList)
	{
		if (!modules.contains(path))
		{
			return true;
		}
	}
	return false;
}

const Vst3Host::PluginList &Vst3HostImpl::RefreshPlugins()
{
	if (cacheFilePath.length() != 0)
	{
		LoadPluginCache();
	}
	if (havePluginsChanged())
	{
		RescanPlugins();
	}
	return pluginList;
}

const Vst3Host::PluginList &Vst3HostImpl::RescanPlugins()
{
	Module::PathList pathList = Module::getModulePaths();

	PluginList list;

	// pathList.insert(pathList.begin(),"/home/pi/.vst3/mda-vst3.vst3");

	for (auto &path : pathList)
	{
		if (path == std::string("/home/pi/.vst3/mda-vst3.vst3"))
		{
			Lv2Log::debug("Processing MDA plugins");
		}
		filesystem::path p{path};
		MakeVst3Info(p, list);
	}
	this->pluginList = std::move(list);
	SavePluginCache();
	return pluginList;
}

static std::string ParseVst3Url(const std::string &url)
{
	if (!url.starts_with("vst3:"))
	{
		throw Vst3Exception("Not a valid vst3 url.: " + url);
	}
	std::string strUrl = url.substr(5);
	return strUrl;
}

std::unique_ptr<Vst3Effect> Vst3HostImpl::CreatePlugin(long instanceId, const std::string &url, IHost *pHost)
{
	EnsureContext();

	auto uuid = ParseVst3Url(url);

	std::unique_ptr<Vst3Effect> result;

	for (const auto &plugin : pluginList)
	{
		if (plugin->uid_ == uuid)
		{
			return Vst3Effect::CreateInstance(instanceId, *(plugin.get()), pHost);
		}
	}
	throw Vst3Exception("Plugin not found.");
}

static inline uint8_t Hex(char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'z')
		return c - 'a' + 10;
	if (c >= 'A' && c <= 'Z')
		return c - 'A' + 10;

	throw Vst3Exception("Invalid state bundle.");
}

static std::vector<uint8_t> HexToByteArray(const std::string &hexState)
{
	std::vector<uint8_t> result;
	result.resize(hexState.length() / 2);
	for (int i = 0; i < result.size(); ++i)
	{
		result[i] = Hex(hexState[i * 2]) * 16 + Hex(hexState[i * 2 + 1]);
	}
	return result;
}
std::unique_ptr<Vst3Effect> Vst3HostImpl::CreatePlugin(PedalBoardItem &pedalBoardItem, IHost *pHost)
{
	std::unique_ptr<Vst3Effect> result = CreatePlugin(pedalBoardItem.instanceId(), pedalBoardItem.uri(), pHost);
	auto pluginInfo = this->GetPluginInfo(pedalBoardItem.uri());
	if (!pluginInfo)
	{
		throw Vst3Exception(SS("Plugin " << pedalBoardItem.pluginName() << " not found."));
	}
	if (pedalBoardItem.vstState().length() != 0)
	{
		std::vector<uint8_t> state = HexToByteArray(pedalBoardItem.vstState());
		result->SetState(state);
	}
	else
	{
		for (const ControlValue &controlValue : pedalBoardItem.controlValues())
		{
			int32_t index = -1;
			for (size_t i = 0; i < pluginInfo->controls().size(); ++i)
			{
				if (pluginInfo->controls()[i].symbol() == controlValue.key())
				{
					index = (int32_t)(pluginInfo->controls()[i].index());
					break;
				}
			}
			if (index != -1)
			{
				result->SetControl(index, controlValue.value());
			}
		}
	}
	for (ControlValue &controlValue : pedalBoardItem.controlValues())
	{
		int32_t index = -1;
		for (size_t i = 0; i < pluginInfo->controls().size(); ++i)
		{
			if (pluginInfo->controls()[i].symbol() == controlValue.key())
			{
				index = (int32_t)(pluginInfo->controls()[i].index());
				break;
			}
		}
		if (index != -1)
		{
			float t = result->GetControlValue(index);
			if (std::isnan(t) || std::isinf(t))
			{
				t = 0;
			}
			controlValue.value(t);
		} else {
			Lv2Log::warning(SS(pedalBoardItem.pluginName() << ": Control key not found. key=" << controlValue.key() ));
		}
	}

	result->GetCurrentPluginInfo();

	return result;
}

#include <fstream>

void Vst3HostImpl::LoadPluginCache()
{
	if (cacheFilePath.length() != 0)
	{
		std::ifstream f(cacheFilePath);
		if (f.is_open())
		{
			json_reader reader(f);

			reader.read(&(this->pluginList));
		}
	}
}
void Vst3HostImpl::SavePluginCache()
{
	if (cacheFilePath.length() != 0)
	{
		std::ofstream f(cacheFilePath);
		if (f.is_open())
		{
			json_writer writer(f);

			writer.write(this->pluginList);
		}
	}
}

Lv2PluginUiInfo *Vst3HostImpl::GetPluginInfo(const std::string &uri)
{
	for (const auto &pluginInfo : pluginList)
	{
		if (pluginInfo->pluginInfo_.uri() == uri)
		{
			return &(pluginInfo->pluginInfo_);
		}
	}
	return nullptr;
}
