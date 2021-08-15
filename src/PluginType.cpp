#include "pch.h"
#include <boost/bimap/bimap.hpp>
#include "PluginType.hpp"



using namespace pipedal;


class PluginTypeMapEntry {
public:
    PluginType pedal_type;
    const std::string uri;
};


#define CORE_URI_PREFIX "http://lv2plug.in/ns/lv2core#"

#define URI_TO_TYPE_MAP_ENTRY(TYPE) \
    {std::string(CORE_URI_PREFIX #TYPE),PluginType::TYPE  }

static std::pair<std::string,PluginType> urisToNames[] {
    { "", PluginType::None},
    URI_TO_TYPE_MAP_ENTRY(EmptyPlugin),
    URI_TO_TYPE_MAP_ENTRY(SplitterPlugin),
    URI_TO_TYPE_MAP_ENTRY(Plugin),
    URI_TO_TYPE_MAP_ENTRY(InvalidPlugin),
    URI_TO_TYPE_MAP_ENTRY(Plugin),
    URI_TO_TYPE_MAP_ENTRY(AllpassPlugin),
    URI_TO_TYPE_MAP_ENTRY(AmplifierPlugin),
    URI_TO_TYPE_MAP_ENTRY(AnalyserPlugin),
    URI_TO_TYPE_MAP_ENTRY(BandpassPlugin),
    URI_TO_TYPE_MAP_ENTRY(ChorusPlugin),
    URI_TO_TYPE_MAP_ENTRY(CombPlugin),
    URI_TO_TYPE_MAP_ENTRY(CompressorPlugin),
    URI_TO_TYPE_MAP_ENTRY(ConstantPlugin),
    URI_TO_TYPE_MAP_ENTRY(ConverterPlugin),
    URI_TO_TYPE_MAP_ENTRY(DelayPlugin),
    URI_TO_TYPE_MAP_ENTRY(DistortionPlugin),
    URI_TO_TYPE_MAP_ENTRY(DynamicsPlugin),
    URI_TO_TYPE_MAP_ENTRY(EQPlugin),
    URI_TO_TYPE_MAP_ENTRY(EnvelopePlugin),
    URI_TO_TYPE_MAP_ENTRY(ExpanderPlugin),
    URI_TO_TYPE_MAP_ENTRY(FilterPlugin),
    URI_TO_TYPE_MAP_ENTRY(FlangerPlugin),
    URI_TO_TYPE_MAP_ENTRY(FunctionPlugin),
    URI_TO_TYPE_MAP_ENTRY(GatePlugin),
    URI_TO_TYPE_MAP_ENTRY(GeneratorPlugin),
    URI_TO_TYPE_MAP_ENTRY(HighpassPlugin),
    URI_TO_TYPE_MAP_ENTRY(InstrumentPlugin),
    URI_TO_TYPE_MAP_ENTRY(LimiterPlugin),
    URI_TO_TYPE_MAP_ENTRY(LowpassPlugin),
    URI_TO_TYPE_MAP_ENTRY(MixerPlugin),
    URI_TO_TYPE_MAP_ENTRY(ModulatorPlugin),
    URI_TO_TYPE_MAP_ENTRY(MultiEQPlugin),
    URI_TO_TYPE_MAP_ENTRY(OscillatorPlugin),
    URI_TO_TYPE_MAP_ENTRY(ParaEQPlugin),
    URI_TO_TYPE_MAP_ENTRY(PhaserPlugin),
    URI_TO_TYPE_MAP_ENTRY(PitchPlugin),
    URI_TO_TYPE_MAP_ENTRY(ReverbPlugin),
    URI_TO_TYPE_MAP_ENTRY(SimulatorPlugin),
    URI_TO_TYPE_MAP_ENTRY(SpatialPlugin),
    URI_TO_TYPE_MAP_ENTRY(SpectralPlugin),
    URI_TO_TYPE_MAP_ENTRY(UtilityPlugin),
    URI_TO_TYPE_MAP_ENTRY(WaveshaperPlugin),
    URI_TO_TYPE_MAP_ENTRY(MIDIPlugin),
};

PluginType pipedal::uri_to_plugin_type(const std::string&uri)
{
    for (auto& i: urisToNames)
    {
        if (i.first == uri)
        {
            return i.second;
        }
    }
    return PluginType::None;
}


const std::string& pipedal::plugin_type_to_uri(PluginType type)
{
    for (auto& i: urisToNames)
    {
        if (i.second == type)
        {
            return i.first;
        }
    }
    throw std::invalid_argument("Plugin type not valid.");
}

#define STRING_TO_TYPE_MAP_ENTRY(TYPE) \
    {std::string(#TYPE),PluginType::TYPE  }

static std::pair<std::string,PluginType> strings_to_type_map[] {
    { "", PluginType::None},
    STRING_TO_TYPE_MAP_ENTRY(InvalidPlugin),
    STRING_TO_TYPE_MAP_ENTRY(Plugin),
    STRING_TO_TYPE_MAP_ENTRY(AllpassPlugin),
    STRING_TO_TYPE_MAP_ENTRY(AmplifierPlugin),
    STRING_TO_TYPE_MAP_ENTRY(AnalyserPlugin),
    STRING_TO_TYPE_MAP_ENTRY(BandpassPlugin),
    STRING_TO_TYPE_MAP_ENTRY(ChorusPlugin),
    STRING_TO_TYPE_MAP_ENTRY(CombPlugin),
    STRING_TO_TYPE_MAP_ENTRY(CompressorPlugin),
    STRING_TO_TYPE_MAP_ENTRY(ConstantPlugin),
    STRING_TO_TYPE_MAP_ENTRY(ConverterPlugin),
    STRING_TO_TYPE_MAP_ENTRY(DelayPlugin),
    STRING_TO_TYPE_MAP_ENTRY(DistortionPlugin),
    STRING_TO_TYPE_MAP_ENTRY(DynamicsPlugin),
    STRING_TO_TYPE_MAP_ENTRY(EQPlugin),
    STRING_TO_TYPE_MAP_ENTRY(EnvelopePlugin),
    STRING_TO_TYPE_MAP_ENTRY(ExpanderPlugin),
    STRING_TO_TYPE_MAP_ENTRY(FilterPlugin),
    STRING_TO_TYPE_MAP_ENTRY(FlangerPlugin),
    STRING_TO_TYPE_MAP_ENTRY(FunctionPlugin),
    STRING_TO_TYPE_MAP_ENTRY(GatePlugin),
    STRING_TO_TYPE_MAP_ENTRY(GeneratorPlugin),
    STRING_TO_TYPE_MAP_ENTRY(HighpassPlugin),
    STRING_TO_TYPE_MAP_ENTRY(InstrumentPlugin),
    STRING_TO_TYPE_MAP_ENTRY(LimiterPlugin),
    STRING_TO_TYPE_MAP_ENTRY(LowpassPlugin),
    STRING_TO_TYPE_MAP_ENTRY(MixerPlugin),
    STRING_TO_TYPE_MAP_ENTRY(ModulatorPlugin),
    STRING_TO_TYPE_MAP_ENTRY(MultiEQPlugin),
    STRING_TO_TYPE_MAP_ENTRY(OscillatorPlugin),
    STRING_TO_TYPE_MAP_ENTRY(ParaEQPlugin),
    STRING_TO_TYPE_MAP_ENTRY(PhaserPlugin),
    STRING_TO_TYPE_MAP_ENTRY(PitchPlugin),
    STRING_TO_TYPE_MAP_ENTRY(ReverbPlugin),
    STRING_TO_TYPE_MAP_ENTRY(SimulatorPlugin),
    STRING_TO_TYPE_MAP_ENTRY(SpatialPlugin),
    STRING_TO_TYPE_MAP_ENTRY(SpectralPlugin),
    STRING_TO_TYPE_MAP_ENTRY(UtilityPlugin),
    STRING_TO_TYPE_MAP_ENTRY(WaveshaperPlugin),
    STRING_TO_TYPE_MAP_ENTRY(MIDIPlugin),
};

PluginType pipedal::string_to_plugin_type(const std::string&uri)
{
    for (auto& i: strings_to_type_map)
    {
        if (i.first == uri)
        {
            return i.second;
        }
    }
    return PluginType::None;
}


const std::string &pipedal::plugin_type_to_string(PluginType type)
{
    for (auto& i: strings_to_type_map)
    {
        if (i.second == type)
        {
            return i.first;
        }
    }
    throw std::invalid_argument("Plugin type not valid.");

}




class PluginTypeEnumConverter: public json_enum_converter<PluginType> {
public:
    virtual PluginType fromString(const std::string&value) const
    {
        return string_to_plugin_type(value);
    }
    virtual const std::string& toString(PluginType value) const
    {
        return plugin_type_to_string(value);
    }
    

} g_plugin_type_converter;


json_enum_converter<PluginType> *pipedal::get_plugin_type_enum_converter()
{
    return &g_plugin_type_converter;
}

