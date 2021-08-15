#pragma once

#include <string>
#include "json.hpp"


namespace pipedal {

enum class PluginType {
    None,

    EmptyPlugin, // Pedalboards only.
    SplitterPlugin, // Pedalboards only.
    
    InvalidPlugin,
    Plugin,
    AllpassPlugin,
    AmplifierPlugin,
    AnalyserPlugin,
    BandpassPlugin,
    ChorusPlugin,
    CombPlugin,
    CompressorPlugin,
    ConstantPlugin,
    ConverterPlugin,
    DelayPlugin,
    DistortionPlugin,
    DynamicsPlugin,
    EQPlugin,
    EnvelopePlugin,
    ExpanderPlugin,
    FilterPlugin,
    FlangerPlugin,
    FunctionPlugin,
    GatePlugin,
    GeneratorPlugin,
    HighpassPlugin,
    InstrumentPlugin,
    LimiterPlugin,
    LowpassPlugin,
    MixerPlugin,
    ModulatorPlugin,
    MultiEQPlugin,
    OscillatorPlugin,
    ParaEQPlugin,
    PhaserPlugin,
    PitchPlugin,
    ReverbPlugin,
    SimulatorPlugin,
    SpatialPlugin,
    SpectralPlugin,
    UtilityPlugin,
    WaveshaperPlugin,
    MIDIPlugin,
};

const std::string &plugin_type_to_uri(PluginType type);
PluginType uri_to_plugin_type(const std::string&uri);

PluginType string_to_plugin_type(const std::string&value);
const std::string& plugin_type_to_string(PluginType value);


json_enum_converter<PluginType> *get_plugin_type_enum_converter();


}; // namespace pipedal.