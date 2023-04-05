// Copyright (c) 2022 Robin Davies
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
#include "PluginHost.hpp"
#include <lilv/lilv.h>
#include <stdexcept>
#include <locale>
#include <string_view>
#include "Lv2Log.hpp"
#include <functional>
#include "Pedalboard.hpp"
#include "Lv2Effect.hpp"
#include "Lv2Pedalboard.hpp"
#include "OptionsFeature.hpp"
#include "JackConfiguration.hpp"
#include "lv2/urid.lv2/urid.h"
#include "lv2/ui.lv2/ui.h"
#include "lv2.h"
#include "lv2/atom.lv2/atom.h"
#include "lv2/time/time.h"
#include "lv2/state/state.h"
#include "lv2/lv2plug.in/ns/ext/buf-size/buf-size.h"
#include "lv2/lv2plug.in/ns/ext/presets/presets.h"
#include "lv2/lv2plug.in/ns/ext/port-props/port-props.h"
#include "lv2/lv2plug.in/ns/extensions/units/units.h"
#include "lv2/lv2plug.in/ns/ext/port-groups/port-groups.h"
#include <fstream>
#include "PiPedalException.hpp"
#include "StdErrorCapture.hpp"

#include "Locale.hpp"

using namespace pipedal;

#define MAP_CHECK()                                                           \
    {                                                                         \
        if (GetPluginClass("http://lv2plug.in/ns/lv2core#Plugin") == nullptr) \
            throw PiPedalStateException("Map is broken");                     \
    }

#define PLUGIN_MAP_CHECK()                                                           \
    {                                                                                \
        if (pHost->GetPluginClass("http://lv2plug.in/ns/lv2core#Plugin") == nullptr) \
            throw PiPedalStateException("Map is broken");                            \
    }

#define P_LV2_CORE_URI "http://lv2plug.in/ns/lv2core#"
#define P_LV2_PPROPS "http://lv2plug.in/ns/ext/port-props#"

namespace pipedal
{
    static const char *LV2_AUDIO_PORT = P_LV2_CORE_URI "AudioPort";
    static const char *LV2_PLUGIN = P_LV2_CORE_URI "Plugin";
    static const char *LV2_CONTROL_PORT = P_LV2_CORE_URI "ControlPort";
    static const char *LV2_INPUT_PORT = P_LV2_CORE_URI "InputPort";
    static const char *LV2_OUTPUT_PORT = P_LV2_CORE_URI "OutputPort";
    static const char *LV2_INTEGER = P_LV2_CORE_URI "integer";
    static const char *LV2_ENUMERATION = P_LV2_CORE_URI "enumeration";
    static const char *LV2_PORT_LOGARITHMIC = P_LV2_PPROPS "logarithmic";
    static const char *LV2_PORT_RANGE_STEPS = P_LV2_PPROPS "rangeSteps";
    static const char *LV2_PORT_TRIGGER = P_LV2_PPROPS "trigger";
    static const char *LV2_PORT_DISPLAY_PRIORITY = P_LV2_PPROPS "displayPriority";
    static const char *LV2_MIDI_PLUGIN = "http://lv2plug.in/ns/lv2core#MIDIPlugin";

    static const char *LV2_ATOM_PORT = "http://lv2plug.in/ns/ext/atom#AtomPort";
    static const char *LV2_ATOM_SEQUENCE = "http://lv2plug.in/ns/ext/atom#Sequence";
    static const char *LV2_ATOM_SOUND = "http://lv2plug.in/ns/ext/atom#Sound";
    static const char *LV2_ATOM_VECTOR = "http://lv2plug.in/ns/ext/atom#Vector";
    static const char *LV2_ATOM_STRING = "http://lv2plug.in/ns/ext/atom#String";
    static const char *LV2_MIDI_EVENT = "http://lv2plug.in/ns/ext/midi#MidiEvent";
    static const char *LV2_DESIGNATION = "http://lv2plug.in/ns/lv2core#Designation";

    class PluginHost::Urids
    {
    public:
        Urids(MapFeature &mapFeature)
        {
            atom_Float = mapFeature.GetUrid(LV2_ATOM__Float);
            ui__portNotification = mapFeature.GetUrid(LV2_UI__portNotification);
            ui__plugin = mapFeature.GetUrid(LV2_UI__plugin);
            ui__protocol = mapFeature.GetUrid(LV2_UI__protocol);
            ui__floatProtocol = mapFeature.GetUrid(LV2_UI__protocol);
            ui__peakProtocol = mapFeature.GetUrid(LV2_UI__peakProtocol);
        }
        LV2_URID atom_Float;
        LV2_URID ui__portNotification;
        LV2_URID ui__plugin;
        LV2_URID ui__protocol;
        LV2_URID ui__floatProtocol;
        LV2_URID ui__peakProtocol;
    };
}

void PluginHost::SetConfiguration(const PiPedalConfiguration &configuration)
{
    this->vst3CachePath =
        std::filesystem::path(configuration.GetLocalStoragePath()) / "vst3cache.json";
    this->vst3Enabled = configuration.IsVst3Enabled();
}

void PluginHost::LilvUris::Initialize(LilvWorld *pWorld)
{
    rdfsComment = lilv_new_uri(pWorld, PluginHost::RDFS_COMMENT_URI);
    logarithic_uri = lilv_new_uri(pWorld, LV2_PORT_LOGARITHMIC);
    display_priority_uri = lilv_new_uri(pWorld, LV2_PORT_DISPLAY_PRIORITY);
    range_steps_uri = lilv_new_uri(pWorld, LV2_PORT_RANGE_STEPS);
    integer_property_uri = lilv_new_uri(pWorld, LV2_INTEGER);
    enumeration_property_uri = lilv_new_uri(pWorld, LV2_ENUMERATION);
    toggle_property_uri = lilv_new_uri(pWorld, LV2_CORE__toggled);
    not_on_gui_property_uri = lilv_new_uri(pWorld, LV2_PORT_PROPS__notOnGUI);
    midiEventNode = lilv_new_uri(pWorld, LV2_MIDI_EVENT);
    designationNode = lilv_new_uri(pWorld, LV2_DESIGNATION);
    portGroupUri = lilv_new_uri(pWorld, LV2_PORT_GROUPS__group);
    unitsUri = lilv_new_uri(pWorld, LV2_UNITS__unit);
    bufferType_uri = lilv_new_uri(pWorld, LV2_ATOM__bufferType);
    pset_Preset = lilv_new_uri(pWorld, LV2_PRESETS__Preset);
    rdfs_label = lilv_new_uri(pWorld, LILV_NS_RDFS "label");
    symbolUri = lilv_new_uri(pWorld, LV2_CORE__symbol);
    nameUri = lilv_new_uri(pWorld, LV2_CORE__name);

    lv2Core__name = lilv_new_uri(pWorld, LV2_CORE__name);
    pipedalUI__ui = lilv_new_uri(pWorld, PIPEDAL_UI__ui);
    pipedalUI__fileProperties = lilv_new_uri(pWorld, PIPEDAL_UI__fileProperties);
    pipedalUI__patchProperty = lilv_new_uri(pWorld, PIPEDAL_UI__patchProperty);
    pipedalUI__directory = lilv_new_uri(pWorld, PIPEDAL_UI__directory);
    pipedalUI__fileTypes = lilv_new_uri(pWorld, PIPEDAL_UI__fileTypes);
    pipedalUI__fileProperty = lilv_new_uri(pWorld, PIPEDAL_UI__fileProperty);
    pipedalUI__fileExtension = lilv_new_uri(pWorld, PIPEDAL_UI__fileExtension);
    pipedalUI__outputPorts = lilv_new_uri(pWorld, PIPEDAL_UI__outputPorts);
    pipedalUI__text = lilv_new_uri(pWorld, PIPEDAL_UI__text);


    ui__portNotification = lilv_new_uri(pWorld, LV2_UI__portNotification);
    ui__plugin = lilv_new_uri(pWorld, LV2_UI__plugin);
    ui__protocol = lilv_new_uri(pWorld, LV2_UI__protocol);
    ui__floatProtocol = lilv_new_uri(pWorld, LV2_UI__protocol);
    ui__peakProtocol = lilv_new_uri(pWorld, LV2_UI__peakProtocol);
    ui__portIndex = lilv_new_uri(pWorld,LV2_UI__portIndex);
    lv2__symbol = lilv_new_uri(pWorld,LV2_CORE__symbol);



    // ui:portNotification
    // [
    //         ui:portIndex 3;
    //         ui:plugin <http://two-play.com/plugins/toob-convolution-reverb>;
    //         ui:protocol ui:floatProtocol;
    //         // pipedal_ui:style pipedal_ui:text ;
    //         // pipedal_ui:redLevel 0;
    //         // pipedal_ui:yellowLevel -12;
    // ]

    time_Position = lilv_new_uri(pWorld, LV2_TIME__Position);
    time_barBeat = lilv_new_uri(pWorld, LV2_TIME__barBeat);
    time_beatsPerMinute = lilv_new_uri(pWorld, LV2_TIME__beatsPerMinute);
    time_speed = lilv_new_uri(pWorld, LV2_TIME__speed);

    appliesTo = lilv_new_uri(pWorld, LV2_CORE__appliesTo);
}

void PluginHost::LilvUris::Free()
{
    rdfsComment.Free();
    logarithic_uri.Free();
    display_priority_uri.Free();
    range_steps_uri.Free();
    integer_property_uri.Free();
    enumeration_property_uri.Free();
    toggle_property_uri.Free();
    not_on_gui_property_uri.Free();
    midiEventNode.Free();
    designationNode.Free();
    portGroupUri.Free();
    unitsUri.Free();
    bufferType_uri.Free();
    pset_Preset.Free();
    rdfs_label.Free();
    symbolUri.Free();
    nameUri.Free();

    time_Position.Free();
    time_barBeat.Free();
    time_beatsPerMinute.Free();
    time_speed.Free();

    appliesTo.Free();
    isA.Free();
}

static std::string nodeAsString(const LilvNode *node)
{
    if (node == nullptr)
    {
        return "";
    }
    if (lilv_node_is_uri(node))
    {
        return lilv_node_as_uri(node);
    }
    if (lilv_node_is_string(node))
    {
        return lilv_node_as_string(node);
    }
    if (lilv_node_is_blank(node))
    {
        return "";
    }
    return "#TypeError";
}
static float nodeAsFloat(const LilvNode *node, float default_ = 0)
{
    if (node == nullptr)
    {
        return default_;
    }
    if (lilv_node_is_float(node))
    {
        return lilv_node_as_float(node);
    }
    if (lilv_node_is_int(node))
    {
        return lilv_node_as_int(node);
    }
    if (lilv_node_is_blank(node))
    {
        return default_;
    }
    return default_;
}

void PluginHost::SetPluginStoragePath(const std::filesystem::path &path)
{
    mapPathFeature.SetPluginStoragePath(path);
}

PluginHost::PluginHost()
    : mapPathFeature("")
{
    pWorld = nullptr;

    LV2_Feature **features = new LV2_Feature *[10];
    lv2Features.push_back(mapFeature.GetMapFeature());
    lv2Features.push_back(mapFeature.GetUnmapFeature());

    logFeature.Prepare(&mapFeature);
    lv2Features.push_back(logFeature.GetFeature());
    optionsFeature.Prepare(mapFeature, 44100, this->GetMaxAudioBufferSize(), this->GetAtomBufferSize());

    mapPathFeature.Prepare(&mapFeature);
    lv2Features.push_back(mapPathFeature.GetMapPathFeature());
    lv2Features.push_back(mapPathFeature.GetMakePathFeature());
    lv2Features.push_back(optionsFeature.GetFeature());

    lv2Features.push_back(nullptr);

    this->urids = new Urids(mapFeature);

    pHostWorkerThread = std::make_shared<HostWorkerThread>();
}

void PluginHost::OnConfigurationChanged(const JackConfiguration &configuration, const JackChannelSelection &settings)
{
    this->sampleRate = configuration.sampleRate();
    if (configuration.isValid())
    {
        this->numberOfAudioInputChannels = settings.GetInputAudioPorts().size();
        this->numberOfAudioOutputChannels = settings.GetOutputAudioPorts().size();
        this->maxBufferSize = configuration.blockLength();
        optionsFeature.Prepare(this->mapFeature, configuration.sampleRate(), configuration.blockLength(), GetAtomBufferSize());
    }
}

PluginHost::~PluginHost()
{
    lilvUris.Free();
    free_world();
    delete urids;
}

void PluginHost::free_world()
{
    if (pWorld)
    {
        lilv_world_free(pWorld);
        pWorld = nullptr;
    }
}
std::shared_ptr<Lv2PluginClass> PluginHost::GetPluginClass(const std::string &uri) const
{
    auto it = this->classesMap.find(uri);
    if (it == this->classesMap.end())
        return nullptr;
    return (*it).second;
}

void PluginHost::AddJsonClassesToMap(std::shared_ptr<Lv2PluginClass> pluginClass)
{
    this->classesMap[pluginClass->uri()] = pluginClass;
    for (int i = 0; i < pluginClass->children_.size(); ++i)
    {
        std::shared_ptr<Lv2PluginClass> child = pluginClass->children_[i];
        AddJsonClassesToMap(pluginClass->children_[i]);
        child->parent_ = pluginClass.get();
    }
}
void PluginHost::LoadPluginClassesFromJson(std::filesystem::path jsonFile)
{
    std::ifstream f(jsonFile);
    json_reader reader(f);

    std::shared_ptr<Lv2PluginClass> classes;
    reader.read(&classes);

    AddJsonClassesToMap(classes);
    this->classesLoaded = true;

    MAP_CHECK();
}

std::shared_ptr<Lv2PluginClass> PluginHost::GetPluginClass(const LilvPluginClass *pClass)
{
    MAP_CHECK();

    std::string uri = nodeAsString(lilv_plugin_class_get_uri(pClass));
    std::shared_ptr<Lv2PluginClass> pResult = this->classesMap[uri];
    return pResult;
}
std::shared_ptr<Lv2PluginClass> PluginHost::MakePluginClass(const LilvPluginClass *pClass)
{
    std::string uri = nodeAsString(lilv_plugin_class_get_uri(pClass));
    auto t = this->classesMap.find(uri);
    if (t != classesMap.end())
    {
        return t->second;
    }
    const LilvNode *parentNode = lilv_plugin_class_get_parent_uri(pClass);
    std::string parent_uri = nodeAsString(parentNode);

    std::string name = nodeAsString(lilv_plugin_class_get_label(pClass));

    std::shared_ptr<Lv2PluginClass> result = std::make_shared<Lv2PluginClass>(
        name.c_str(), uri.c_str(), parent_uri.c_str());

    classesMap[uri] = result;
    return result;
}

void PluginHost::LoadPluginClassesFromLilv()
{
    const auto classes = lilv_world_get_plugin_classes(pWorld);

    LILV_FOREACH(plugin_classes, iPluginClass, classes)
    {
        const LilvPluginClass *pClass = lilv_plugin_classes_get(classes, iPluginClass);
        // Get it to prepopulate the map.
        MakePluginClass(pClass);
    }
    for (auto value : classesMap)
    {
        std::shared_ptr<Lv2PluginClass> lv2Class{value.second};

        auto t = GetPluginClass(lv2Class->uri());

        if (lv2Class && lv2Class->parent_uri().size() != 0)
        {
                std::shared_ptr<Lv2PluginClass> parentClass = GetPluginClass(lv2Class->parent_uri());
                if (parentClass)
                {
                    Lv2PluginClass *ccClass = const_cast<Lv2PluginClass *>(lv2Class.get());
                    ccClass->set_parent(parentClass);
                    Lv2PluginClass *ccParentClass = parentClass.get();
                    ccParentClass->add_child(lv2Class);
                }
        }
    }
}

void PluginHost::Load(const char *lv2Path)
{

    this->plugins_.clear();
    this->ui_plugins_.clear();
    if (!classesLoaded)
    {
        this->classesMap.clear();
    }

    free_world();

    Lv2Log::info("Scanning for LV2 Plugins");

    setenv("LV2_PATH", lv2Path, true);
    StdErrorCapture stdoutCapture; // captures lilv messages written to STDOUT.
    pWorld = lilv_world_new();
    {

        lilv_world_load_all(pWorld);
    }

    lilvUris.Initialize(pWorld);

    const LilvPlugins *plugins = lilv_world_get_all_plugins(pWorld);

    LoadPluginClassesFromLilv();

    LILV_FOREACH(plugins, iPlugin, plugins)
    {
        const LilvPlugin *lilvPlugin = lilv_plugins_get(plugins, iPlugin);

        std::shared_ptr<Lv2PluginInfo> pluginInfo = std::make_shared<Lv2PluginInfo>(this, pWorld, lilvPlugin);

        Lv2Log::debug("Plugin: " + pluginInfo->name());

        if (pluginInfo->hasCvPorts())
        {
                Lv2Log::debug("Plugin %s (%s) skipped. (Has CV ports).", pluginInfo->name().c_str(), pluginInfo->uri().c_str());
        }
#if !SUPPORT_MIDI
        else if (pluginInfo->plugin_class() == LV2_MIDI_PLUGIN)
        {
                Lv2Log::debug("Plugin %s (%s) skipped. (MIDI Plugin).", pluginInfo->name().c_str(), pluginInfo->uri().c_str());
        }
#endif
        else if (!pluginInfo->is_valid())
        {
                auto &ports = pluginInfo->ports();
                for (int i = 0; i < ports.size(); ++i)
                {
                    auto &port = ports[i];
                    if (!port->is_valid())
                    {
                        Lv2Log::debug("Plugin port %s:%s is invalid.", pluginInfo->name().c_str(), port->name().c_str());
                    }
                }
                Lv2Log::debug("Plugin %s (%s) skipped. Not valid.", pluginInfo->name().c_str(), pluginInfo->uri().c_str());
        }
        else
        {
                this->plugins_.push_back(pluginInfo);
        }
    }
    auto messages = stdoutCapture.GetOutputLines();

    for (const std::string &s : messages)
    {
        Lv2Log::info("lilv: " + s);
    }

    const auto &collation = std::use_facet<std::collate<char>>(std::locale());
    auto compare = [&collation](
                       const std::shared_ptr<Lv2PluginInfo> &left,
                       const std::shared_ptr<Lv2PluginInfo> &right)
    {
        const char *pb1 = left->name().c_str();
        const char *pb2 = right->name().c_str();
        return collation.compare(
                   pb1, pb1 + left->name().size(),
                   pb2, pb2 + right->name().size()) < 0;
    };
    std::sort(this->plugins_.begin(), this->plugins_.end(), compare);

    for (auto plugin : this->plugins_)
    {
        Lv2PluginUiInfo info(this, plugin.get());
        if (plugin->is_valid())
        {
#if SUPPORT_MIDI
                if (info.audio_inputs() == 0 && !info.has_midi_input())
                {
                    Lv2Log::debug("Plugin %s (%s) skipped. No inputs.", plugin->name().c_str(), plugin->uri().c_str());
                }
                else if (info.audio_outputs() == 0 && !info.has_midi_output())
                {
                    Lv2Log::debug("Plugin %s (%s) skipped. No audio outputs.", plugin->name().c_str(), plugin->uri().c_str());
                }
#else
                if (info.audio_inputs() == 0)
                {
                    Lv2Log::debug("Plugin %s (%s) skipped. No audio inputs.", plugin->name().c_str(), plugin->uri().c_str());
                }
                else if (info.audio_outputs() == 0)
                {
                    Lv2Log::debug("Plugin %s (%s) skipped. No audio outputs.", plugin->name().c_str(), plugin->uri().c_str());
                }
#endif
                else
                {
                    ui_plugins_.push_back(std::move(info));
                }
        }
    }

#if ENABLE_VST3
    if (vst3Enabled)
    {
        Lv2Log::info("Scanning for VST3 Plugins");
        this->vst3Host = Vst3Host::CreateInstance(
            this->vst3CachePath);
        this->vst3Host->RefreshPlugins();

        const auto &vst3PluginList = this->vst3Host->getPluginList();
        for (const auto &vst3Plugin : vst3PluginList)
        {
                // copy not move!
                ui_plugins_.push_back(vst3Plugin->pluginInfo_);
        }
        auto ui_compare = [&collation](
                              Lv2PluginUiInfo &left,
                              Lv2PluginUiInfo &right)
        {
            const char *pb1 = left.name().c_str();
            const char *pb2 = right.name().c_str();
            return collation.compare(
                       pb1, pb1 + left.name().size(),
                       pb2, pb2 + right.name().size()) < 0;
        };
        std::sort(this->ui_plugins_.begin(), this->ui_plugins_.end(), ui_compare);
    }
#endif
};

class NodesAutoFree
{
    LilvNodes *nodes;

    // const LilvNode* returns must not be freed, by convention.
    NodesAutoFree(const LilvNodes *nodes)
    {
    }

public:
    NodesAutoFree(LilvNodes *nodes)
    {
        this->nodes = nodes;
    }
    operator const LilvNodes *()
    {
        return this->nodes;
    }
    operator bool()
    {
        return this->nodes != nullptr;
    }
    ~NodesAutoFree()
    {
        lilv_nodes_free(this->nodes);
    }
};

static std::vector<std::string> nodeAsStringArray(const LilvNodes *nodes)
{
    std::vector<std::string> result;
    LILV_FOREACH(nodes, iNode, nodes)
    {
        result.push_back(nodeAsString(lilv_nodes_get(nodes, iNode)));
    }
    return result;
}

const char *PluginHost::RDFS_COMMENT_URI = "http://www.w3.org/2000/01/rdf-schema#"
                                           "comment";

LilvNode *PluginHost::get_comment(const std::string &uri)
{
    AutoLilvNode uriNode = lilv_new_uri(pWorld, uri.c_str());
    LilvNode *result = lilv_world_get(pWorld, uriNode, lilvUris.rdfsComment, nullptr);
    return result;
}

static bool ports_sort_compare(std::shared_ptr<Lv2PortInfo> &p1, const std::shared_ptr<Lv2PortInfo> &p2)
{
    return p1->index() < p2->index();
}

bool Lv2PluginInfo::HasFactoryPresets(PluginHost *lv2Host, const LilvPlugin *plugin)
{
    NodesAutoFree nodes = lilv_plugin_get_related(plugin, lv2Host->lilvUris.pset_Preset);
    bool result = false;
    LILV_FOREACH(nodes, iNode, nodes)
    {
        result = true;
        break;
    }
    return result;
}

Lv2PluginInfo::Lv2PluginInfo(PluginHost *lv2Host, LilvWorld *pWorld, const LilvPlugin *pPlugin)
{

    AutoLilvNode bundleUriNode = lilv_plugin_get_bundle_uri(pPlugin);
    if (!bundleUriNode)
        throw std::logic_error("Invalid bundle uri.");

    std::string bundleUri = bundleUriNode.AsUri();

    std::string bundlePath = lilv_file_uri_parse(bundleUri.c_str(), nullptr);
    if (bundlePath.length() == 0)
        throw std::logic_error("Bundle uri is not a file uri.");

    this->bundle_path_ = bundlePath;

    this->has_factory_presets_ = HasFactoryPresets(lv2Host, pPlugin);

    this->uri_ = nodeAsString(lilv_plugin_get_uri(pPlugin));

    AutoLilvNode name = (lilv_plugin_get_name(pPlugin));
    this->name_ = nodeAsString(name);

    AutoLilvNode author_name = (lilv_plugin_get_author_name(pPlugin));
    this->author_name_ = nodeAsString(author_name);

    AutoLilvNode author_homepage = (lilv_plugin_get_author_homepage(pPlugin));
    this->author_homepage_ = nodeAsString(author_homepage);

    const LilvPluginClass *pClass = lilv_plugin_get_class(pPlugin);
    this->plugin_class_ = nodeAsString(lilv_plugin_class_get_uri(pClass));

    NodesAutoFree required_features = lilv_plugin_get_required_features(pPlugin);
    this->required_features_ = nodeAsStringArray(required_features);

    NodesAutoFree supported_features = lilv_plugin_get_supported_features(pPlugin);
    this->supported_features_ = nodeAsStringArray(supported_features);

    NodesAutoFree optional_features = lilv_plugin_get_optional_features(pPlugin);
    this->optional_features_ = nodeAsStringArray(optional_features);

    NodesAutoFree extensions = lilv_plugin_get_extension_data(pPlugin);
    this->extensions_ = nodeAsStringArray(extensions);

    AutoLilvNode comment = lv2Host->get_comment(this->uri_);
    this->comment_ = nodeAsString(comment);

    uint32_t ports = lilv_plugin_get_num_ports(pPlugin);

    bool isValid = true;
    std::vector<std::string> portGroups;

    for (uint32_t i = 0; i < ports; ++i)
    {
        const LilvPort *pPort = lilv_plugin_get_port_by_index(pPlugin, i);

        std::shared_ptr<Lv2PortInfo> portInfo = std::make_shared<Lv2PortInfo>(lv2Host, pPlugin, pPort);
        if (!portInfo->is_valid())
        {
                isValid = false;
        }
        const auto &portGroup = portInfo->port_group();
        if (portGroup.size() != 0)
        {
                if (std::find(portGroups.begin(), portGroups.end(), portGroup) == portGroups.end())
                {
                    portGroups.push_back(portGroup);
                }
        }
        ports_.push_back(std::move(portInfo));
    }

    for (auto &portGroup : portGroups)
    {
        port_groups_.push_back(std::make_shared<Lv2PortGroup>(lv2Host, portGroup));
    }

    for (int i = 0; i < this->required_features_.size(); ++i)
    {
        if (!this->IsSupportedFeature(this->required_features_[i]))
        {
                Lv2Log::debug("%s (%s) requires feature %s.", this->name_.c_str(), this->uri_.c_str(), this->required_features_[i].c_str());
                isValid = false;
        }
    }

    std::sort(ports_.begin(), ports_.end(), ports_sort_compare);

    // Fetch pipedal plugin UI

    AutoLilvNode pipedalUINode = lilv_world_get(
        pWorld,
        lilv_plugin_get_uri(pPlugin),
        lv2Host->lilvUris.pipedalUI__ui,
        nullptr);
    if (pipedalUINode)
    {
        this->piPedalUI_ = std::make_shared<PiPedalUI>(lv2Host, pipedalUINode, std::filesystem::path(bundlePath));
    }
    // xxx lilv_world_get(pWorld,pluginUri,);
    // for (auto&portInfo: ports_)
    // {
    //     if (portInfo->is_control_port() && portInfo->is_output())
    //     {
    //         std::cout << "Dbg: " << "Has an output control port. " << this->uri_ << std::endl;
    //     }
    // }

    this->is_valid_ = isValid;
}

std::vector<std::string> supportedFeatures = {
    LV2_URID__map,
    LV2_URID__unmap,
    LV2_LOG__log,
    LV2_WORKER__schedule,
    LV2_OPTIONS__options,
    LV2_BUF_SIZE__boundedBlockLength,
    LV2_BUF_SIZE__fixedBlockLength,
    LV2_BUF_SIZE__powerOf2BlockLength,
    LV2_CORE__isLive,
    LV2_STATE__loadDefaultState,
    LV2_STATE__makePath,
    LV2_STATE__mapPath,

    // UI features that we can ignore, since we won't load their ui.
    "http://lv2plug.in/ns/extensions/ui#makeResident",

};

bool Lv2PluginInfo::IsSupportedFeature(const std::string &feature) const
{
    for (int i = 0; i < supportedFeatures.size(); ++i)
    {
        if (supportedFeatures[i] == feature)
                return true;
    }
    return false;
}
Lv2PluginInfo::~Lv2PluginInfo()
{
}

bool Lv2PortInfo::is_a(PluginHost *lv2Plugins, const char *classUri)
{
    return classes_.is_a(lv2Plugins, classUri);
}

static float getFloat(const LilvNode *pNode)
{
    if (lilv_node_is_float(pNode))
    {
        return lilv_node_as_float(pNode);
    }
    if (lilv_node_is_int(pNode))
    {
        return lilv_node_as_int(pNode);
    }
    throw std::invalid_argument("Invalid float node.");
}

static bool scale_points_sort_compare(const Lv2ScalePoint &v1, const Lv2ScalePoint &v2)
{
    return v1.value() < v2.value();
}
Lv2PortInfo::Lv2PortInfo(PluginHost *host, const LilvPlugin *plugin, const LilvPort *pPort)
{
    auto pWorld = host->pWorld;
    index_ = lilv_port_get_index(plugin, pPort);
    symbol_ = nodeAsString(lilv_port_get_symbol(plugin, pPort));

    AutoLilvNode name = lilv_port_get_name(plugin, pPort);
    name_ = nodeAsString(name);

    classes_ = host->GetPluginPortClass(plugin, pPort);

    AutoLilvNode minNode, maxNode, defaultNode;
    min_value_ = 0;
    max_value_ = 1;
    default_value_ = 0;
    lilv_port_get_range(plugin, pPort, defaultNode, minNode, maxNode);
    if (defaultNode)
    {
        default_value_ = getFloat(defaultNode);
    }
    if (minNode)
    {
        min_value_ = getFloat(minNode);
    }
    if (maxNode)
    {
        max_value_ = getFloat(maxNode);
    }
    if (default_value_ > max_value_)
        default_value_ = max_value_;
    if (default_value_ < min_value_)
        default_value_ = min_value_;

    this->is_logarithmic_ = lilv_port_has_property(plugin, pPort, host->lilvUris.logarithic_uri);

    NodesAutoFree priority_nodes = lilv_port_get_value(plugin, pPort, host->lilvUris.display_priority_uri);

    this->display_priority_ = -1;
    if (priority_nodes)
    {
        auto priority_node = lilv_nodes_get_first(priority_nodes);
        if (priority_node)
        {
                this->display_priority_ = lilv_node_as_int(priority_node);
        }
    }

    NodesAutoFree range_steps_nodes = lilv_port_get_value(plugin, pPort, host->lilvUris.range_steps_uri);
    this->range_steps_ = 0;
    if (range_steps_nodes)
    {
        auto range_steps_node = lilv_nodes_get_first(range_steps_nodes);
        if (range_steps_node)
        {
                this->range_steps_ = lilv_node_as_int(range_steps_node);
        }
    }
    this->integer_property_ = lilv_port_has_property(plugin, pPort, host->lilvUris.integer_property_uri);

    this->enumeration_property_ = lilv_port_has_property(plugin, pPort, host->lilvUris.enumeration_property_uri);

    this->toggled_property_ = lilv_port_has_property(plugin, pPort, host->lilvUris.toggle_property_uri);

    this->not_on_gui_ = lilv_port_has_property(plugin, pPort, host->lilvUris.not_on_gui_property_uri);

    LilvScalePoints *pScalePoints = lilv_port_get_scale_points(plugin, pPort);
    LILV_FOREACH(scale_points, iSP, pScalePoints)
    {
        const LilvScalePoint *pSP = lilv_scale_points_get(pScalePoints, iSP);
        float value = nodeAsFloat(lilv_scale_point_get_value(pSP));
        auto label = nodeAsString(lilv_scale_point_get_label(pSP));
        scale_points_.push_back(Lv2ScalePoint(value, label));
    }
    lilv_scale_points_free(pScalePoints);

    // sort is what we want, but the implementation is broken in g++ 8.3

    std::sort(scale_points_.begin(), scale_points_.end(), scale_points_sort_compare);

    is_input_ = is_a(host, LV2_INPUT_PORT);
    is_output_ = is_a(host, LV2_OUTPUT_PORT);

    is_control_port_ = is_a(host, LV2_CORE__ControlPort);
    is_audio_port_ = is_a(host, LV2_CORE__AudioPort);
    is_atom_port_ = is_a(host, LV2_ATOM_PORT);
    is_cv_port_ = is_a(host, LV2_CORE__CVPort);

    supports_midi_ = lilv_port_supports_event(plugin, pPort, host->lilvUris.midiEventNode);
    supports_time_position_ = lilv_port_supports_event(plugin, pPort, host->lilvUris.time_Position);

    AutoLilvNode designationValue = lilv_port_get(plugin, pPort, host->lilvUris.designationNode);
    designation_ = nodeAsString(designationValue);

    AutoLilvNode portGroup_value = lilv_port_get(plugin, pPort, host->lilvUris.portGroupUri);
    port_group_ = nodeAsString(portGroup_value);

    AutoLilvNode unitsValueUri = lilv_port_get(plugin, pPort, host->lilvUris.unitsUri);
    this->units_ = UriToUnits(nodeAsString(unitsValueUri));

    AutoLilvNode commentNode = lilv_port_get(plugin, pPort, host->lilvUris.rdfsComment);
    this->comment_ = nodeAsString(commentNode);

    AutoLilvNode bufferType = lilv_port_get(plugin, pPort, host->lilvUris.bufferType_uri);

    this->buffer_type_ = "";
    if (bufferType)
    {
        this->buffer_type_ = nodeAsString(bufferType);
    }

    is_valid_ = is_control_port_ ||
                ((is_input_ || is_output_) && (is_audio_port_ || is_atom_port_ || is_cv_port_));
}

Lv2PluginClasses PluginHost::GetPluginPortClass(const LilvPlugin *lilvPlugin, const LilvPort *lilvPort)
{
    std::vector<std::string> result;
    const LilvNodes *nodes = lilv_port_get_classes(lilvPlugin, lilvPort);
    if (nodes != nullptr)
    {
        LILV_FOREACH(nodes, iNode, nodes)
        {
                const LilvNode *node = lilv_nodes_get(nodes, iNode);
                std::string classUri = nodeAsString(node);
                result.push_back(classUri);
        }
    }
    return Lv2PluginClasses(result);
}

bool Lv2PluginClass::is_a(const std::string &classUri) const
{
    if (this->uri_ == classUri)
        return true;
    if (parent_)
    {
        return parent_->is_a(classUri);
    }
    // some classes have parent uris that are not themselves
    // classes because the class tree is incomplete.
    if (this->parent_uri_ == classUri)
    {
        return true;
    }
    return false;
}
bool Lv2PluginClasses::is_a(PluginHost *lv2Plugins, const char *classUri) const
{
    std::string classUri_(classUri);
    for (auto i : classes_)
    {
        if (lv2Plugins->is_a(classUri_, i))
        {
                return true;
        }
    }
    return false;
}

bool PluginHost::is_a(const std::string &class_, const std::string &target_class)
{
    if (class_ == target_class)
        return true;
    std::shared_ptr<Lv2PluginClass> pClass = this->GetPluginClass(target_class);
    if (pClass)
    {
        return pClass->is_a(class_);
    }
    return false;
}

Lv2PluginUiInfo::Lv2PluginUiInfo(PluginHost *pHost, const Lv2PluginInfo *plugin)
    : uri_(plugin->uri()),
      name_(plugin->name()),
      author_name_(plugin->author_name()),
      author_homepage_(plugin->author_homepage()),
      plugin_type_(uri_to_plugin_type(plugin->plugin_class())),
      audio_inputs_(0),
      audio_outputs_(0),
      description_(plugin->comment()),
      has_midi_input_(false),
      has_midi_output_(false)
{
    PLUGIN_MAP_CHECK();
    auto pluginClass = pHost->GetPluginClass(plugin->plugin_class());
    if (pluginClass)
    {
        this->plugin_display_type_ = pluginClass->display_name();
    }
    else
    {
        PLUGIN_MAP_CHECK();
        this->plugin_display_type_ = "Plugin";
        Lv2Log::warning("%s: Unknown plugin type: %s", plugin->uri().c_str(), plugin->plugin_class().c_str());
    }
    for (auto port : plugin->ports())
    {
        if (port->is_input())
        {
                if (port->is_control_port() && port->is_input())
                {
                    controls_.push_back(Lv2PluginUiControlPort(plugin, port.get()));
                }
                else if (port->is_atom_port())
                {
                    if (port->supports_midi())
                    {
                        has_midi_input_ = true;
                    }
                }
                else if (port->is_audio_port())
                {
                    ++audio_inputs_;
                }
        }
        else if (port->is_output())
        {
                if (port->is_atom_port() && port->supports_midi())
                {
                    this->has_midi_output_ = true;
                }
                else if (port->is_audio_port())
                {
                    ++audio_outputs_;
                }
        }
    }
    for (auto &portGroup : plugin->port_groups())
    {
        this->port_groups_.push_back(Lv2PluginUiPortGroup(portGroup.get()));
    }
    auto &piPedalUI = plugin->piPedalUI();

    if (piPedalUI)
    {
        this->fileProperties_ = piPedalUI->fileProperties();
        this->uiPortNotifications_ = piPedalUI->portNotifications();
    }
}

std::shared_ptr<Lv2PluginClass> PluginHost::GetLv2PluginClass() const
{
    return this->GetPluginClass(LV2_PLUGIN);
}

std::shared_ptr<Lv2PluginInfo> PluginHost::GetPluginInfo(const std::string &uri) const
{
    for (auto i = this->plugins_.begin(); i != this->plugins_.end(); ++i)
    {
        if ((*i)->uri() == uri)
        {
                return (*i);
        }
    }
    return nullptr;
}

Lv2Pedalboard *PluginHost::CreateLv2Pedalboard(Pedalboard &pedalboard)
{
    Lv2Pedalboard *pPedalboard = new Lv2Pedalboard();
    try
    {
        pPedalboard->Prepare(this, pedalboard);
        return pPedalboard;
    }
    catch (const std::exception &e)
    {
        delete pPedalboard;
        throw;
    }
}

struct StateCallbackData
{
    PluginHost *pHost;
    std::vector<ControlValue> *pResult;
};

void PluginHost::fn_LilvSetPortValueFunc(const char *port_symbol,
                                         void *user_data,
                                         const void *value,
                                         uint32_t size,
                                         uint32_t type)
{
    StateCallbackData *pData = (StateCallbackData *)user_data;
    PluginHost *pHost = pData->pHost;
    std::vector<ControlValue> *pResult = pData->pResult;
    LV2_URID valueType = (LV2_URID)type;
    if (valueType != pHost->urids->atom_Float)
    {
        throw PiPedalStateException("Preset format not supported.");
    }
    pResult->push_back(ControlValue(port_symbol, *(float *)value));
}

std::vector<ControlValue> PluginHost::LoadFactoryPluginPreset(
    PedalboardItem *pedalboardItem, const std::string &presetUri)
{

    std::vector<ControlValue> result;

    const LilvPlugins *plugins = lilv_world_get_all_plugins(this->pWorld);

    AutoLilvNode uriNode = lilv_new_uri(pWorld, pedalboardItem->uri().c_str());

    lilv_world_load_resource(pWorld, uriNode);

    const LilvPlugin *plugin = lilv_plugins_get_by_uri(plugins, uriNode);
    if (plugin == nullptr)
    {
        throw PiPedalStateException("No such plugin.");
    }

    LilvNodes *presets = lilv_plugin_get_related(plugin, lilvUris.pset_Preset);
    LILV_FOREACH(nodes, i, presets)
    {
        const LilvNode *preset = lilv_nodes_get(presets, i);
        lilv_world_load_resource(pWorld, preset);

        std::string thisPresetUri = nodeAsString(preset);
        if (presetUri == thisPresetUri)
        {

                /*********************************/

                // AutoLilvNode uriNode = lilv_new_uri(pWorld, presetUri.c_str());

                auto lilvState = lilv_state_new_from_world(pWorld, GetLv2UridMap(), preset);

                if (lilvState == nullptr)
                {
                    throw PiPedalStateException("Preset not found.");
                }

                StateCallbackData cbData;
                cbData.pHost = this;
                cbData.pResult = &result;
                auto n = lilv_state_get_num_properties(lilvState);
                lilv_state_emit_port_values(lilvState, fn_LilvSetPortValueFunc, &cbData);

                lilv_state_free(lilvState);
                break;

                /*********************************/
        }
    }
    lilv_nodes_free(presets);

    return result;
}

struct PresetCallbackState
{
    PluginHost *pHost;
    std::map<std::string, float> *values;
    bool failed = false;
};

void PluginHost::PortValueCallback(const char *symbol, void *user_data, const void *value, uint32_t size, uint32_t type)
{
    PresetCallbackState *pState = static_cast<PresetCallbackState *>(user_data);
    PluginHost *pHost = pState->pHost;

    if (type == pHost->urids->atom_Float)
    {
        (*pState->values)[symbol] = *static_cast<const float *>(value);
    }
    else
    {
        pState->failed = true;
    }
}
PluginPresets PluginHost::GetFactoryPluginPresets(const std::string &pluginUri)
{
    const LilvPlugins *plugins = lilv_world_get_all_plugins(this->pWorld);

    AutoLilvNode uriNode = lilv_new_uri(pWorld, pluginUri.c_str());

    lilv_world_load_resource(pWorld, uriNode);

    const LilvPlugin *plugin = lilv_plugins_get_by_uri(plugins, uriNode);
    if (plugin == nullptr)
    {
        throw PiPedalStateException("No such plugin.");
    }

    PluginPresets result;
    result.pluginUri_ = pluginUri;

    LilvNodes *presets = lilv_plugin_get_related(plugin, lilvUris.pset_Preset);
    LILV_FOREACH(nodes, i, presets)
    {
        const LilvNode *preset = lilv_nodes_get(presets, i);
        lilv_world_load_resource(pWorld, preset);

        LilvState *state = lilv_state_new_from_world(pWorld, this->mapFeature.GetMap(), preset);
        if (state != nullptr)
        {
                std::string label = lilv_state_get_label(state);
                std::map<std::string, float> controlValues;
                PresetCallbackState cbData{this, &controlValues, false};
                lilv_state_emit_port_values(state, PortValueCallback, (void *)&cbData);
                lilv_state_free(state);

                if (!cbData.failed)
                {
                    result.presets_.push_back(PluginPreset(result.nextInstanceId_++, std::move(label), std::move(controlValues)));
                }
        }
    }
    lilv_nodes_free(presets);
    return result;
}

IEffect *PluginHost::CreateEffect(PedalboardItem &pedalboardItem)
{
    if (pedalboardItem.uri().starts_with("vst3:"))
    {
#if ENABLE_VST3
        try
        {
                Vst3Effect::Ptr vst3Plugin = this->vst3Host->CreatePlugin(pedalboardItem, this);
                return vst3Plugin.release();
        }
        catch (const std::exception &e)
        {
                Lv2Log::error(std::string("Failed to create VST3 plugin: ") + e.what());
                throw;
        }
#else
        Lv2Log::error(std::string("VST3 support not enabled at compile time."));
        throw PiPedalException("VST3 support not enabled at compile time, SEE ENABLE_VST3 in CMakeList.txt.");

#endif
    }
    else
    {
        auto info = this->GetPluginInfo(pedalboardItem.uri());
        if (!info)
                return nullptr;

        return new Lv2Effect(this, info, pedalboardItem);
    }
}

Lv2PortGroup::Lv2PortGroup(PluginHost *lv2Host, const std::string &groupUri)
{
    LilvWorld *pWorld = lv2Host->pWorld;

    this->uri_ = groupUri;
    AutoLilvNode uri = lilv_new_uri(pWorld, groupUri.c_str());
    LilvNode *symbolNode = lilv_world_get(pWorld, uri, lv2Host->lilvUris.symbolUri, nullptr);
    symbol_ = nodeAsString(symbolNode);
    LilvNode *nameNode = lilv_world_get(pWorld, uri, lv2Host->lilvUris.nameUri, nullptr);
    name_ = nodeAsString(nameNode);
}

std::shared_ptr<HostWorkerThread> PluginHost::GetHostWorkerThread()
{
    return pHostWorkerThread;
}

// void PiPedalHostLogError(const std::string &error)
// {
//     Lv2Log::error("%s",error.c_str());
// }

#define MAP_REF(class, name) \
    json_map::reference(#name, &class ::name##_)

#define CONDITIONAL_MAP_REF(class, name, condition) \
    json_map::conditional_reference(#name, &class ::name##_, condition)

json_map::storage_type<Lv2PluginClasses> Lv2PluginClasses::jmap{{
    // json_map::reference("char_", &JsonTestTarget::char_),
    json_map::reference("classes", &Lv2PluginClasses::classes_),
}};

json_map::storage_type<Lv2ScalePoint> Lv2ScalePoint::jmap{{
    json_map::reference("value", &Lv2ScalePoint::value_),
    json_map::reference("label", &Lv2ScalePoint::label_),
}};

json_map::storage_type<Lv2PortInfo> Lv2PortInfo::jmap{
    {json_map::reference("index", &Lv2PortInfo::index_),

     json_map::reference("symbol", &Lv2PortInfo::symbol_),
     json_map::reference("index", &Lv2PortInfo::index_),
     json_map::reference("name", &Lv2PortInfo::name_),

     json_map::reference("min_value", &Lv2PortInfo::min_value_),
     json_map::reference("max_value", &Lv2PortInfo::max_value_),
     json_map::reference("default_value", &Lv2PortInfo::default_value_),
     json_map::reference("classes", &Lv2PortInfo::classes_),

     json_map::reference("scale_points", &Lv2PortInfo::scale_points_),

     json_map::reference("is_input", &Lv2PortInfo::is_input_),
     json_map::reference("is_output", &Lv2PortInfo::is_output_),

     json_map::reference("is_control_port", &Lv2PortInfo::is_control_port_),
     json_map::reference("is_audio_port", &Lv2PortInfo::is_audio_port_),
     json_map::reference("is_atom_port", &Lv2PortInfo::is_audio_port_),
     json_map::reference("is_cv_port", &Lv2PortInfo::is_cv_port_),

     json_map::reference("is_valid", &Lv2PortInfo::is_valid_),

     json_map::reference("supports_midi", &Lv2PortInfo::supports_midi_),
     json_map::reference("supports_time_position", &Lv2PortInfo::supports_time_position_),
     json_map::reference("port_group", &Lv2PortInfo::port_group_),
     json_map::reference("is_logarithmic", &Lv2PortInfo::is_logarithmic_),

     MAP_REF(Lv2PortInfo, is_logarithmic),
     MAP_REF(Lv2PortInfo, display_priority),
     MAP_REF(Lv2PortInfo, range_steps),
     MAP_REF(Lv2PortInfo, trigger),
     MAP_REF(Lv2PortInfo, integer_property),
     MAP_REF(Lv2PortInfo, enumeration_property),
     MAP_REF(Lv2PortInfo, toggled_property),
     MAP_REF(Lv2PortInfo, not_on_gui),
     MAP_REF(Lv2PortInfo, buffer_type),
     MAP_REF(Lv2PortInfo, port_group),
     json_map::enum_reference("units", &Lv2PortInfo::units_, get_units_enum_converter()),
     MAP_REF(Lv2PortInfo, comment)}};

json_map::storage_type<Lv2PortGroup> Lv2PortGroup::jmap{{
    MAP_REF(Lv2PortGroup, uri),

}};

json_map::storage_type<Lv2PluginInfo> Lv2PluginInfo::jmap{{
    json_map::reference("bundle_path", &Lv2PluginInfo::bundle_path_),
    json_map::reference("uri", &Lv2PluginInfo::uri_),
    json_map::reference("name", &Lv2PluginInfo::name_),
    json_map::reference("plugin_class", &Lv2PluginInfo::plugin_class_),
    json_map::reference("supported_features", &Lv2PluginInfo::supported_features_),
    json_map::reference("required_features", &Lv2PluginInfo::required_features_),
    json_map::reference("optional_features", &Lv2PluginInfo::optional_features_),
    json_map::reference("extensions", &Lv2PluginInfo::extensions_),

    json_map::reference("author_name", &Lv2PluginInfo::author_name_),
    json_map::reference("author_homepage", &Lv2PluginInfo::author_homepage_),

    json_map::reference("comment", &Lv2PluginInfo::comment_),
    json_map::reference("ports", &Lv2PluginInfo::ports_),
    json_map::reference("port_groups", &Lv2PluginInfo::port_groups_),
    json_map::reference("is_valid", &Lv2PluginInfo::is_valid_),
    json_map::reference("has_factory_presets", &Lv2PluginInfo::has_factory_presets_),
}};

json_map::storage_type<Lv2PluginClass> Lv2PluginClass::jmap{{
    MAP_REF(Lv2PluginClass, uri),
    MAP_REF(Lv2PluginClass, display_name),
    MAP_REF(Lv2PluginClass, parent_uri),
    json_map::enum_reference("plugin_type", &Lv2PluginClass::plugin_type_, get_plugin_type_enum_converter()),
    MAP_REF(Lv2PluginClass, children),
}};

json_map::storage_type<Lv2PluginUiPortGroup> Lv2PluginUiPortGroup::jmap{{
    MAP_REF(Lv2PluginUiPortGroup, symbol),
    MAP_REF(Lv2PluginUiPortGroup, name),
    MAP_REF(Lv2PluginUiPortGroup, parent_group),
    MAP_REF(Lv2PluginUiPortGroup, program_list_id),
}};

json_map::storage_type<Lv2PluginUiControlPort> Lv2PluginUiControlPort::jmap{{
    json_map::reference("symbol", &Lv2PluginUiControlPort::symbol_),
    json_map::reference("name", &Lv2PluginUiControlPort::name_),
    MAP_REF(Lv2PluginUiControlPort, index),
    MAP_REF(Lv2PluginUiControlPort, min_value),
    MAP_REF(Lv2PluginUiControlPort, max_value),
    MAP_REF(Lv2PluginUiControlPort, default_value),
    MAP_REF(Lv2PluginUiControlPort, is_logarithmic),
    MAP_REF(Lv2PluginUiControlPort, display_priority),

    MAP_REF(Lv2PluginUiControlPort, range_steps),
    MAP_REF(Lv2PluginUiControlPort, integer_property),
    MAP_REF(Lv2PluginUiControlPort, enumeration_property),
    MAP_REF(Lv2PluginUiControlPort, not_on_gui),
    MAP_REF(Lv2PluginUiControlPort, toggled_property),
    MAP_REF(Lv2PluginUiControlPort, scale_points),
    MAP_REF(Lv2PluginUiControlPort, port_group),

    json_map::enum_reference("units", &Lv2PluginUiControlPort::units_, get_units_enum_converter()),
    MAP_REF(Lv2PluginUiControlPort, comment),
    MAP_REF(Lv2PluginUiControlPort, is_bypass),
    MAP_REF(Lv2PluginUiControlPort, is_program_controller),
    MAP_REF(Lv2PluginUiControlPort, custom_units),

}};

json_map::storage_type<Lv2PluginUiInfo>
    Lv2PluginUiInfo::jmap{
        {json_map::reference("uri", &Lv2PluginUiInfo::uri_),
         json_map::reference("name", &Lv2PluginUiInfo::name_),
         json_map::enum_reference("plugin_type", &Lv2PluginUiInfo::plugin_type_, get_plugin_type_enum_converter()),
         json_map::reference("plugin_display_type", &Lv2PluginUiInfo::plugin_display_type_),
         json_map::reference("author_name", &Lv2PluginUiInfo::author_name_),
         json_map::reference("author_homepage", &Lv2PluginUiInfo::author_homepage_),
         json_map::reference("audio_inputs", &Lv2PluginUiInfo::audio_inputs_),
         json_map::reference("audio_outputs", &Lv2PluginUiInfo::audio_outputs_),
         json_map::reference("description", &Lv2PluginUiInfo::description_),
         json_map::reference("has_midi_input", &Lv2PluginUiInfo::has_midi_input_),
         json_map::reference("has_midi_output", &Lv2PluginUiInfo::has_midi_output_),
         json_map::reference("controls", &Lv2PluginUiInfo::controls_),
         json_map::reference("port_groups", &Lv2PluginUiInfo::port_groups_),
         json_map::reference("is_vst3", &Lv2PluginUiInfo::is_vst3_),
         json_map::reference("fileProperties", &Lv2PluginUiInfo::fileProperties_),
         json_map::reference("uiPortNotifications", &Lv2PluginUiInfo::uiPortNotifications_),
    }};
