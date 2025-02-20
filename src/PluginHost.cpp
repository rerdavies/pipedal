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
#include "AtomConverter.hpp"
#include "PluginHost.hpp"
#include <lilv/lilv.h>
#include <stdexcept>
#include "Locale.hpp"
#include <string_view>
#include "Lv2Log.hpp"
#include <functional>
#include "Pedalboard.hpp"
#include "Lv2Effect.hpp"
#include "Lv2Pedalboard.hpp"
#include "OptionsFeature.hpp"
#include "JackConfiguration.hpp"
#include "lv2/urid/urid.h"
#include "lv2/ui/ui.h"
// #include "lv2.h"
#include "lv2/atom/atom.h"
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
#include "util.hpp"
#include "ModFileTypes.hpp"

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

    // in ttl files, but not header files.

    static const char *LV2_MIDI_PLUGIN = "http://lv2plug.in/ns/lv2core#MIDIPlugin";

    class PluginHost::Urids
    {
    public:
        Urids(MapFeature &mapFeature)
        {
            atom__Float = mapFeature.GetUrid(LV2_ATOM__Float);
            atom__Double = mapFeature.GetUrid(LV2_ATOM__Double);
            atom_Int = mapFeature.GetUrid(LV2_ATOM__Int);
            ui__portNotification = mapFeature.GetUrid(LV2_UI__portNotification);
            ui__plugin = mapFeature.GetUrid(LV2_UI__plugin);
            ui__protocol = mapFeature.GetUrid(LV2_UI__protocol);
            ui__floatProtocol = mapFeature.GetUrid(LV2_UI__protocol);
            ui__peakProtocol = mapFeature.GetUrid(LV2_UI__peakProtocol);
        }
        LV2_URID atom__Float;
        LV2_URID atom__Double;
        LV2_URID atom_Int;
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
    isA = lilv_new_uri(pWorld, "http://www.w3.org/1999/02/22-rdf-syntax-ns#type");
    rdfs__Comment = lilv_new_uri(pWorld, PluginHost::RDFS__comment);
    rdfs__range = lilv_new_uri(pWorld, PluginHost::RDFS__range);
    port_logarithmic = lilv_new_uri(pWorld, LV2_PORT_PROPS__logarithmic);
    port__display_priority = lilv_new_uri(pWorld, LV2_PORT_PROPS__displayPriority);
    port_range_steps = lilv_new_uri(pWorld, LV2_PORT_PROPS__rangeSteps);
    integer_property_uri = lilv_new_uri(pWorld, LV2_CORE__integer);
    enumeration_property_uri = lilv_new_uri(pWorld, LV2_CORE__enumeration);
    core__toggled = lilv_new_uri(pWorld, LV2_CORE__toggled);
    core__connectionOptional = lilv_new_uri(pWorld, LV2_CORE__connectionOptional);
    portprops__not_on_gui_property_uri = lilv_new_uri(pWorld, LV2_PORT_PROPS__notOnGUI);
    portprops__trigger = lilv_new_uri(pWorld, LV2_PORT_PROPS__trigger);
    midi__event = lilv_new_uri(pWorld, LV2_MIDI__MidiEvent);
    core__designation = lilv_new_uri(pWorld, LV2_CORE__designation);
    portgroups__group = lilv_new_uri(pWorld, LV2_PORT_GROUPS__group);
    units__unit = lilv_new_uri(pWorld, LV2_UNITS__unit);

    invada_units__unit = lilv_new_uri(pWorld, "http://lv2plug.in/ns/extension/units#unit");                   // a typo in invada plugin ttl files.
    invada_portprops__logarithmic = lilv_new_uri(pWorld, "http://lv2plug.in/ns/dev/extportinfo#logarithmic"); // a typo in invada plugin ttl files.

    atom__bufferType = lilv_new_uri(pWorld, LV2_ATOM__bufferType);
    atom__Path = lilv_new_uri(pWorld, LV2_ATOM__Path);
    presets__preset = lilv_new_uri(pWorld, LV2_PRESETS__Preset);
    state__state = lilv_new_uri(pWorld, LV2_STATE__state);
    rdfs__label = lilv_new_uri(pWorld, LILV_NS_RDFS "label");

    lv2core__symbol = lilv_new_uri(pWorld, LV2_CORE__symbol);
    lv2core__name = lilv_new_uri(pWorld, LV2_CORE__name);
    lv2core__index = lilv_new_uri(pWorld, LV2_CORE__index);
    lv2core__Parameter = lilv_new_uri(pWorld, LV2_CORE_PREFIX "Parameter");

    pipedalUI__ui = lilv_new_uri(pWorld, PIPEDAL_UI__ui);
    pipedalUI__fileProperties = lilv_new_uri(pWorld, PIPEDAL_UI__fileProperties);
    pipedalUI__patchProperty = lilv_new_uri(pWorld, PIPEDAL_UI__patchProperty);
    pipedalUI__directory = lilv_new_uri(pWorld, PIPEDAL_UI__directory);
    pipedalUI__fileTypes = lilv_new_uri(pWorld, PIPEDAL_UI__fileTypes);
    pipedalUI__resourceDirectory = lilv_new_uri(pWorld, PIPEDAL_UI__resourceDirectory);
    pipedalUI__fileProperty = lilv_new_uri(pWorld, PIPEDAL_UI__fileProperty);
    pipedalUI__fileExtension = lilv_new_uri(pWorld, PIPEDAL_UI__fileExtension);
    pipedalUI__mimeType = lilv_new_uri(pWorld, PIPEDAL_UI__mimeType);
    pipedalUI__outputPorts = lilv_new_uri(pWorld, PIPEDAL_UI__outputPorts);
    pipedalUI__text = lilv_new_uri(pWorld, PIPEDAL_UI__text);
    pipedalUI__ledColor = lilv_new_uri(pWorld,PIPEDAL_UI__ledColor);

    pipedalUI__frequencyPlot = lilv_new_uri(pWorld, PIPEDAL_UI__frequencyPlot);
    pipedalUI__xLeft = lilv_new_uri(pWorld, PIPEDAL_UI__xLeft);
    pipedalUI__xRight = lilv_new_uri(pWorld, PIPEDAL_UI__xRight);
    pipedalUI__yTop = lilv_new_uri(pWorld, PIPEDAL_UI__yTop);
    pipedalUI__yBottom = lilv_new_uri(pWorld, PIPEDAL_UI__yBottom);
    pipedalUI__xLog = lilv_new_uri(pWorld, PIPEDAL_UI__xLog);
    pipedalUI__width = lilv_new_uri(pWorld, PIPEDAL_UI__width);

    ui__portNotification = lilv_new_uri(pWorld, LV2_UI__portNotification);
    ui__plugin = lilv_new_uri(pWorld, LV2_UI__plugin);
    ui__protocol = lilv_new_uri(pWorld, LV2_UI__protocol);
    ui__floatProtocol = lilv_new_uri(pWorld, LV2_UI__protocol);
    ui__peakProtocol = lilv_new_uri(pWorld, LV2_UI__peakProtocol);
    ui__portIndex = lilv_new_uri(pWorld, LV2_UI__portIndex);
    lv2__symbol = lilv_new_uri(pWorld, LV2_CORE__symbol);
    lv2__port = lilv_new_uri(pWorld, LV2_CORE__port);

#define MOD_PREFIX "http://moddevices.com/ns/mod#"
    mod__label = lilv_new_uri(pWorld, MOD_PREFIX "label");
    mod__brand = lilv_new_uri(pWorld, MOD_PREFIX "brand");
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

    patch__writable = lilv_new_uri(pWorld, LV2_PATCH__writable);
    patch__readable = lilv_new_uri(pWorld, LV2_PATCH__readable);

    dc__format = lilv_new_uri(pWorld, "http://purl.org/dc/terms/format");

    mod__fileTypes = lilv_new_uri(pWorld, "http://moddevices.com/ns/mod#fileTypes");
}

void PluginHost::LilvUris::Free()
{
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
    pluginStoragePath = path;
}

std::string PluginHost::GetPluginStoragePath() const
{
    return pluginStoragePath;
}

PluginHost::PluginHost()
{
    pWorld = nullptr;

    lv2Features.push_back(mapFeature.GetMapFeature());
    lv2Features.push_back(mapFeature.GetUnmapFeature());

    optionsFeature.Prepare(mapFeature, 44100, this->GetMaxAudioBufferSize(), this->GetAtomBufferSize());
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
    delete lilvUris;
    lilvUris = nullptr;
    delete urids;
    urids = nullptr;
    free_world();
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
        // Get it to pre-populate the map.
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
    // LilvNode*lv2_path = lilv_new_file_uri(pWorld,NULL,lv2Path);
    // lilv_world_set_option(world,LILV_OPTION_LV2_PATH,lv)

    lilvUris = new LilvUris();
    lilvUris->Initialize(pWorld);

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
        } else if (pluginInfo->hasUnsupportedPatchProperties())
        {
            Lv2Log::debug("Plugin %s (%s) skipped. (Has unsupported patch parameters).", pluginInfo->name().c_str(), pluginInfo->uri().c_str());
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
        if (s.length() != 0)
        {
            Lv2Log::info("lilv: " + s);
        }
    }

    auto collator = Locale::GetInstance()->GetCollator();
    auto compare = [&collator](
                       const std::shared_ptr<Lv2PluginInfo> &left,
                       const std::shared_ptr<Lv2PluginInfo> &right)
    {
        const char *pb1 = left->name().c_str();
        const char *pb2 = right->name().c_str();
        return collator->Compare(
                   left->name(), right->name()) < 0;
    };
    std::sort(this->plugins_.begin(), this->plugins_.end(), compare);

    for (auto plugin : this->plugins_)
    {
        pluginsByUri[plugin->uri()] = plugin;

        Lv2PluginUiInfo info(this, plugin.get());
        if (plugin->is_valid())
        {
#if 1
        // no plugins with more than 2 inputs or outputs.
        // no zero-input or zero-output plugins (temporarily disables midi plugins)
        // no zero output devices (permanent, I think)
            if (info.audio_inputs() > 2 || info.audio_outputs() > 2) {
                Lv2Log::debug(
                    "Plugin %s (%s) skipped. %d inputs, %d outputs.", plugin->name().c_str(), plugin->uri().c_str(),
                    (int)info.audio_inputs(),(int)info.audio_outputs());

            }
            else if (info.audio_inputs() == 0  && info.audio_outputs() == 0 )
            {
                Lv2Log::debug("Plugin %s (%s) skipped. No audio i/o.", plugin->name().c_str(), plugin->uri().c_str());
                
            } else if (info.audio_inputs() == 0) 
            {
                // temporarily disable this feature.
                Lv2Log::debug("Plugin %s (%s) skipped. No inputs.", plugin->name().c_str(), plugin->uri().c_str());
            }
#elif SUPPORT_MIDI
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
                if (info.audio_inputs() == 0) {
                    Lv2Log::debug("************* ZERO INPUTS: %s (%s) skipped. No audio outputs.", plugin->name().c_str(), plugin->uri().c_str());
                }
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

static std::vector<std::string> nodeAsStringArray(const LilvNodes *nodes)
{
    std::vector<std::string> result;
    LILV_FOREACH(nodes, iNode, nodes)
    {
        result.push_back(nodeAsString(lilv_nodes_get(nodes, iNode)));
    }
    return result;
}

const char *PluginHost::RDFS__comment = "http://www.w3.org/2000/01/rdf-schema#"
                                        "comment";

const char *PluginHost::RDFS__range = "http://www.w3.org/2000/01/rdf-schema#"
                                      "range";

LilvNode *PluginHost::get_comment(const std::string &uri)
{
    AutoLilvNode uriNode = lilv_new_uri(pWorld, uri.c_str());
    LilvNode *result = lilv_world_get(pWorld, uriNode, lilvUris->rdfs__Comment, nullptr);
    return result;
}

static bool ports_sort_compare(std::shared_ptr<Lv2PortInfo> &p1, const std::shared_ptr<Lv2PortInfo> &p2)
{
    return p1->index() < p2->index();
}

bool Lv2PluginInfo::HasFactoryPresets(PluginHost *lv2Host, const LilvPlugin *plugin)
{
    AutoLilvNodes nodes = lilv_plugin_get_related(plugin, lv2Host->lilvUris->presets__preset);
    bool result = false;
    LILV_FOREACH(nodes, iNode, nodes)
    {
        result = true;
        break;
    }
    return result;
}

Lv2PluginInfo::FindWritablePathPropertiesResult
Lv2PluginInfo::FindWritablePathProperties(PluginHost *lv2Host, const LilvPlugin *pPlugin)
{
    // example:

    // <http://github.com/mikeoliphant/neural-amp-modeler-lv2#model>
    //     a lv2:Parameter;
    //     rdfs:label "Model";
    //     rdfs:range atom:Path.
    // ...
    //          patch:writable <http://github.com/mikeoliphant/neural-amp-modeler-lv2#model>;

    LilvWorld *pWorld = lv2Host->getWorld();
    AutoLilvNode pluginUri = lilv_plugin_get_uri(pPlugin);
    AutoLilvNodes patchWritables = lilv_world_find_nodes(pWorld, pluginUri, lv2Host->lilvUris->patch__writable, nullptr);

    std::vector<UiFileProperty::ptr> fileProperties;

    bool unsupportedPatchProperty = false;
    LILV_FOREACH(nodes, iNode, patchWritables)
    {
        AutoLilvNode propertyUri = lilv_nodes_get(patchWritables, iNode);
        if (propertyUri)
        {
            // a lv2:Parameter?
            if (lilv_world_ask(pWorld, propertyUri, lv2Host->lilvUris->isA, lv2Host->lilvUris->lv2core__Parameter))
            {
                //  rfs:range atom:Path?
                if (lilv_world_ask(pWorld, propertyUri, lv2Host->lilvUris->rdfs__range, lv2Host->lilvUris->atom__Path))
                {


                    AutoLilvNode label = lilv_world_get(pWorld, propertyUri, lv2Host->lilvUris->rdfs__label, nullptr);
                    std::string strLabel = label.AsString();

                    if (strLabel.length() == 0)
                    {
                        strLabel = "File";
                    }

                    std::filesystem::path path = this->bundle_path();
                    path = path.parent_path();
                    std::string lv2DirectoryName = path.filename().string();
                    // we have a valid path property!

                    auto fileProperty =
                        std::make_shared<UiFileProperty>(
                            strLabel, propertyUri.AsUri(), lv2DirectoryName);


                    AutoLilvNode indexNode = lilv_world_get(pWorld, propertyUri, lv2Host->lilvUris->lv2core__index, nullptr);
                    int32_t index = indexNode.AsInt(-1);
                    fileProperty->index(index);


                    // if there's a pipedalui_fileTypes node, use that instead.


                    AutoLilvNode mod__fileTypes = lilv_world_get(pWorld, propertyUri, lv2Host->lilvUris->mod__fileTypes, nullptr);

                    // default: everything.
                    std::string fileTypes = ModFileTypes::DEFAULT_FILE_TYPES; 
                    if (mod__fileTypes)
                    {
                        // "nam,nammodel"
                        fileTypes = mod__fileTypes.AsString();
                    }
                    ModFileTypes modFileTypes(fileTypes);



                    // Legacy case: audio_uploads/<plugin_directory> exists.

                    std::filesystem::path bundleDirectoryName = std::filesystem::path(bundle_path()).parent_path().filename();
                    std::filesystem::path legacyUploadPath = lv2Host->MapPath(bundleDirectoryName.string());

                    fileProperty->setModFileTypes(modFileTypes);

                    fileProperty->directory(bundleDirectoryName);

                    if (std::filesystem::exists(legacyUploadPath) 
                        && !std::filesystem::exists(legacyUploadPath / ".migrated"))
                    {
                        fileProperty->useLegacyModDirectory(true);
                        fileProperty->directory(bundleDirectoryName);
                        modFileTypes.rootDirectories().push_back(bundleDirectoryName.filename()); // push the private directory!
                    } else {
                        if (modFileTypes.rootDirectories().size() == 1)
                        {
                            std::string modName = modFileTypes.rootDirectories()[0];
                            auto modDirectory = modFileTypes.GetModDirectory(modName);
                            fileProperty->directory(modDirectory->pipedalPath);
                        }
                    }


                    fileProperties.push_back(fileProperty);
                } else {
                    unsupportedPatchProperty = true;
                }
            }
        }
    }
    FindWritablePathPropertiesResult result;



    if (fileProperties.size() != 0)
    {
        std::sort(fileProperties.begin(),fileProperties.end(),[](const UiFileProperty::ptr& left,const UiFileProperty::ptr&right) {
            // properies with indexes first.
            int32_t indexL = left->index();
            if (indexL < 0) indexL = std::numeric_limits<int32_t>::max();
            int32_t indexR = right->index();
            if (indexR < 0) indexR = std::numeric_limits<int32_t>::max();
            if (indexL < indexR) return true;
            if (indexL > indexR) return false;

            // there is no natural order. TTL indexing means that the order we see them in is random.
            // We can't order them sensibly. Let's at least order them consistently.
            return left->label() < right->label();

        });
        result.pipedalUi = std::make_shared<PiPedalUI>(std::move(fileProperties));
    }
    result.hasUnsupportedPatchProperties = unsupportedPatchProperty;
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

    AutoLilvNode plugUri = lilv_plugin_get_uri(pPlugin);
    this->uri_ = nodeAsString(lilv_plugin_get_uri(pPlugin));

    AutoLilvNode name = (lilv_plugin_get_name(pPlugin));
    this->name_ = nodeAsString(name);

    AutoLilvNode brand = lilv_world_get(pWorld, plugUri, lv2Host->lilvUris->mod__brand, nullptr);
    this->brand_ = nodeAsString(brand);

    AutoLilvNode label = lilv_world_get(pWorld, plugUri, lv2Host->lilvUris->mod__label, nullptr);
    this->label_ = nodeAsString(label);
    if (label_.length() == 0)
    {
        this->label_ = this->name_;
    }

    AutoLilvNode author_name = (lilv_plugin_get_author_name(pPlugin));
    this->author_name_ = nodeAsString(author_name);

    AutoLilvNode author_homepage = (lilv_plugin_get_author_homepage(pPlugin));
    this->author_homepage_ = nodeAsString(author_homepage);

    const LilvPluginClass *pClass = lilv_plugin_get_class(pPlugin);
    this->plugin_class_ = nodeAsString(lilv_plugin_class_get_uri(pClass));

    AutoLilvNodes required_features = lilv_plugin_get_required_features(pPlugin);
    this->required_features_ = nodeAsStringArray(required_features);

    AutoLilvNodes supported_features = lilv_plugin_get_supported_features(pPlugin);
    this->supported_features_ = nodeAsStringArray(supported_features);

    AutoLilvNodes optional_features = lilv_plugin_get_optional_features(pPlugin);
    this->optional_features_ = nodeAsStringArray(optional_features);

    AutoLilvNodes extensions = lilv_plugin_get_extension_data(pPlugin);
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
        lv2Host->lilvUris->pipedalUI__ui,
        nullptr);
    if (pipedalUINode)
    {
        this->piPedalUI_ = std::make_shared<PiPedalUI>(lv2Host, pipedalUINode, std::filesystem::path(bundlePath));
    }
    else
    {
        // look for
        auto result = FindWritablePathProperties(lv2Host, pPlugin);
        this->piPedalUI_ = result.pipedalUi;
        this->hasUnsupportedPatchProperties_ = result.hasUnsupportedPatchProperties;
    }

    int nInputs = 0;
    for (size_t i = 0; i < ports_.size(); ++i)
    {
        auto port = ports_[i];
        if (port->is_audio_port() && port->is_input())
        {
            if (nInputs >= 2 && !port->connection_optional())
            {
                isValid = false;
                break;
            }
            ++nInputs;
        }
    }
    int nOutputs = 0;
    for (size_t i = 0; i < ports_.size(); ++i)
    {
        auto port = ports_[i];
        if (port->is_audio_port() && port->is_output())
        {
            if (nOutputs >= 2 && !port->connection_optional())
            {
                isValid = false;
                break;
            }
            ++nOutputs;
        }
    }

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
    LV2_STATE__freePath,
    LV2_CORE__inPlaceBroken,

    // UI features that we can ignore, since we won't load their ui.
    "http://lv2plug.in/ns/extensions/ui#makeResident",
    "http://lv2plug.in/ns/ext/port-props#supportsStrictBounds"

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

    this->is_logarithmic_ = lilv_port_has_property(plugin, pPort, host->lilvUris->port_logarithmic);

    // typo in invada plugins.
    this->is_logarithmic_ |= lilv_port_has_property(plugin, pPort, host->lilvUris->invada_portprops__logarithmic);

    AutoLilvNodes priority_nodes = lilv_port_get_value(plugin, pPort, host->lilvUris->port__display_priority);

    this->display_priority_ = -1;
    if (priority_nodes)
    {
        auto priority_node = lilv_nodes_get_first(priority_nodes);
        if (priority_node)
        {
            this->display_priority_ = lilv_node_as_int(priority_node);
        }
    }

    AutoLilvNodes range_steps_nodes = lilv_port_get_value(plugin, pPort, host->lilvUris->port_range_steps);
    this->range_steps_ = 0;
    if (range_steps_nodes)
    {
        auto range_steps_node = lilv_nodes_get_first(range_steps_nodes);
        if (range_steps_node)
        {
            this->range_steps_ = lilv_node_as_int(range_steps_node);
        }
    }
    this->integer_property_ = lilv_port_has_property(plugin, pPort, host->lilvUris->integer_property_uri);

    this->enumeration_property_ = lilv_port_has_property(plugin, pPort, host->lilvUris->enumeration_property_uri);

    this->toggled_property_ = lilv_port_has_property(plugin, pPort, host->lilvUris->core__toggled);
    this->not_on_gui_ = lilv_port_has_property(plugin, pPort, host->lilvUris->portprops__not_on_gui_property_uri);
    this->connection_optional_ = lilv_port_has_property(plugin, pPort, host->lilvUris->core__connectionOptional);
    this->trigger_property_ = lilv_port_has_property(plugin, pPort, host->lilvUris->portprops__trigger);

    AutoLilvNode port_ledColor = lilv_port_get(plugin,pPort,host->lilvUris->pipedalUI__ledColor);
    if (port_ledColor)
    {
        auto value = lilv_node_as_string(port_ledColor);
        if (value) {
            this->pipedal_ledColor_ = value;
        }
    }

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

    is_input_ = is_a(host, LV2_CORE__InputPort);
    is_output_ = is_a(host, LV2_CORE__OutputPort);

    is_control_port_ = is_a(host, LV2_CORE__ControlPort);
    is_audio_port_ = is_a(host, LV2_CORE__AudioPort);
    is_atom_port_ = is_a(host, LV2_ATOM__AtomPort);
    is_cv_port_ = is_a(host, LV2_CORE__CVPort);

    supports_midi_ = lilv_port_supports_event(plugin, pPort, host->lilvUris->midi__event);
    supports_time_position_ = lilv_port_supports_event(plugin, pPort, host->lilvUris->time_Position);

    AutoLilvNode designationValue = lilv_port_get(plugin, pPort, host->lilvUris->core__designation);
    designation_ = nodeAsString(designationValue);

    AutoLilvNode portGroup_value = lilv_port_get(plugin, pPort, host->lilvUris->portgroups__group);
    port_group_ = nodeAsString(portGroup_value);

    AutoLilvNode unitsValueUri = lilv_port_get(plugin, pPort, host->lilvUris->units__unit);
    if (unitsValueUri)
    {
        this->units_ = UriToUnits(nodeAsString(unitsValueUri));
    }
    else
    {
        // invada plugins use the wrong URI.
        AutoLilvNode invadaUnitsValueUri = lilv_port_get(plugin, pPort, host->lilvUris->invada_units__unit);
        if (invadaUnitsValueUri)
        {
            std::string uri = nodeAsString(invadaUnitsValueUri);
            static const char *INCORRECT_URI = "http://lv2plug.in/ns/extension/units#";
            static const char *CORRECT_URI = "http://lv2plug.in/ns/extensions/units#";
            if (uri.starts_with(INCORRECT_URI))
            {
                uri = uri.replace(0, strlen(INCORRECT_URI), CORRECT_URI);
            }
            this->units_ = UriToUnits(uri);
        }
        else
        {
            this->units(Units::none);
        }
    }

    AutoLilvNode commentNode = lilv_port_get(plugin, pPort, host->lilvUris->rdfs__Comment);
    this->comment_ = nodeAsString(commentNode);

    AutoLilvNode bufferType = lilv_port_get(plugin, pPort, host->lilvUris->atom__bufferType);

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
      brand_(plugin->brand()),
      label_(plugin->label()),
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
            if (port->is_control_port())
            {
                controls_.push_back(Lv2PluginUiPort(plugin, port.get()));
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
            if (port->is_control_port())
            {
                controls_.push_back(Lv2PluginUiPort(plugin, port.get()));
            }
            else if (port->is_atom_port() && port->supports_midi())
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
        this->frequencyPlots_ = piPedalUI->frequencyPlots();
        this->uiPortNotifications_ = piPedalUI->portNotifications();
    }
}

std::shared_ptr<Lv2PluginClass> PluginHost::GetLv2PluginClass() const
{
    return this->GetPluginClass(LV2_CORE__Plugin);
}

std::shared_ptr<Lv2PluginInfo> PluginHost::GetPluginInfo(const std::string &uri) const
{
    auto ff = pluginsByUri.find(uri);
    if (ff == pluginsByUri.end())
    {
        return nullptr;
    }
    return ff->second;
}

Lv2Pedalboard *PluginHost::CreateLv2Pedalboard(Pedalboard &pedalboard, Lv2PedalboardErrorList &errorMessages)
{
    Lv2Pedalboard *pPedalboard = new Lv2Pedalboard();
    try
    {
        pPedalboard->Prepare(this, pedalboard, errorMessages);
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
    if (valueType != pHost->urids->atom__Float)
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

    LilvNodes *presets = lilv_plugin_get_related(plugin, lilvUris->presets__preset);
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

    if (type == pHost->urids->atom__Double)
    {
        (*pState->values)[symbol] = (float)(*static_cast<const double *>(value));
    }
    else if (type == pHost->urids->atom__Float)
    {
        (*pState->values)[symbol] = *static_cast<const float *>(value);
    }
    else if (type == pHost->urids->atom_Int)
    {
        (*pState->values)[symbol] = *static_cast<const int32_t *>(value);
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

    LilvNodes *presets = lilv_plugin_get_related(plugin, lilvUris->presets__preset);
    LILV_FOREACH(nodes, i, presets)
    {
        const LilvNode *preset = lilv_nodes_get(presets, i);
        lilv_world_load_resource(pWorld, preset);

        LilvState *state = lilv_state_new_from_world(pWorld, this->mapFeature.GetMap(), preset);
        if (state != nullptr)
        {
            const char *t = this->mapFeature.UridToString(14);
            const char *tLabel = lilv_state_get_label(state);
            if (tLabel != nullptr)
            {
                std::string strLabel = tLabel;
                std::map<std::string, float> controlValues;
                PresetCallbackState cbData{this, &controlValues, false};
                lilv_state_emit_port_values(state, PortValueCallback, (void *)&cbData);

                int numProperties = lilv_state_get_num_properties(state);
                // can't handle std:state part of preset.
                if (numProperties == 0)
                {
                    if (!cbData.failed)
                    {
                        result.presets_.push_back(PluginPreset(result.nextInstanceId_++, strLabel, controlValues, Lv2PluginState()));
                    }
                }
                else
                {
                    result.presets_.push_back(PluginPreset::MakeLilvPreset(result.nextInstanceId_++, strLabel, controlValues, lilv_node_as_uri(preset)));
                }
                lilv_state_free(state);
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
    LilvNode *symbolNode = lilv_world_get(pWorld, uri, lv2Host->lilvUris->lv2core__symbol, nullptr);
    symbol_ = nodeAsString(symbolNode);
    LilvNode *nameNode = lilv_world_get(pWorld, uri, lv2Host->lilvUris->lv2core__name, nullptr);
    name_ = nodeAsString(nameNode);
}

bool Lv2PluginInfo::isSplit() const
{
    return uri_ == SPLIT_PEDALBOARD_ITEM_URI;
}
std::shared_ptr<HostWorkerThread> PluginHost::GetHostWorkerThread()
{
    return pHostWorkerThread;
}

class ResourceInfo
{
public:
    ResourceInfo(const std::string &filePropertyDirectory, const std::string &resourceDirectory)
        : filePropertyDirectory(filePropertyDirectory), resourceDirectory(resourceDirectory) {}

    std::string filePropertyDirectory;
    std::string resourceDirectory;
    bool operator==(const ResourceInfo &other) const
    {
        return this->filePropertyDirectory == other.filePropertyDirectory && this->resourceDirectory == other.resourceDirectory;
    }
    bool operator<(const ResourceInfo &other) const
    {
        if (this->filePropertyDirectory < other.filePropertyDirectory)
            return true;
        if (this->filePropertyDirectory > other.filePropertyDirectory)
            return false;
        return this->resourceDirectory < other.resourceDirectory;
    }
};

static bool anyTargetFilesExist(const std::filesystem::path &sourceDirectory, const std::filesystem::path &targetDirectory)
{
    namespace fs = std::filesystem;
    try
    {
        if (!fs::exists(targetDirectory))
            return false;
        for (auto &directoryEntry : fs::directory_iterator(sourceDirectory))
        {
            fs::path thisPath = directoryEntry.path();
            if (directoryEntry.is_directory())
            {
                auto name = thisPath.filename();
                fs::path childSource = sourceDirectory / name;
                fs::path childTarget = targetDirectory / name;
                if (anyTargetFilesExist(childSource, childTarget))
                {
                    return true;
                }
            }
            else
            {
                fs::path targetPath = targetDirectory / thisPath.filename();
                if (fs::exists(targetPath))
                {
                    return true;
                }
            }
        }
    }
    catch (const std::exception &)
    {
    }
    return false;
}

static void createTargetLinks(const std::filesystem::path &sourceDirectory, const std::filesystem::path &targetDirectory)
{
    namespace fs = std::filesystem;

    fs::create_directories(targetDirectory);

    for (auto &dirEntry : fs::directory_iterator(sourceDirectory))
    {
        fs::path childSource = dirEntry.path();
        fs::path childTarget = targetDirectory / childSource.filename();
        if (dirEntry.is_directory())
        {
            createTargetLinks(childSource, childTarget);
        }
        else
        {
            fs::create_symlink(childSource, childTarget);
        }
    }
}
void PluginHost::CheckForResourceInitialization(const std::string &pluginUri, const std::filesystem::path &pluginUploadDirectory)
{

    auto plugin = GetPluginInfo(pluginUri);
    if (plugin)
    {
        std::filesystem::path bundlePath = plugin->bundle_path();
        if (!plugin->piPedalUI())
            return;
        const auto &fileProperties = plugin->piPedalUI()->fileProperties();
        if (fileProperties.size() != 0 && !pluginsThatHaveBeenCheckedForResources.contains(pluginUri))
        {
            pluginsThatHaveBeenCheckedForResources.insert(pluginUri);

            // eliminate duplicates.
            std::set<ResourceInfo> resourceInfoSet;
            for (const auto &fileProperty : fileProperties)
            {
                if (!fileProperty->resourceDirectory().empty() && !fileProperty->directory().empty())
                {
                    resourceInfoSet.insert(ResourceInfo(fileProperty->directory(), fileProperty->resourceDirectory()));
                }
            }
            try
            {
                for (const ResourceInfo &resourceInfo : resourceInfoSet)
                {
                    std::filesystem::path sourcePath = bundlePath / resourceInfo.resourceDirectory;
                    std::filesystem::path targetPath = pluginUploadDirectory / resourceInfo.filePropertyDirectory;
                    if (!anyTargetFilesExist(sourcePath, targetPath))
                    {
                        createTargetLinks(sourcePath, targetPath);
                    }
                }
            }
            catch (const std::exception &e)
            {
                Lv2Log::error(SS("CheckForResourceInitialization: " << e.what()));
            }
        }
    }
}

json_variant PluginHost::MapPath(const json_variant &json)
{
    AtomConverter converter(GetMapFeature());
    return converter.MapPath(json, GetPluginStoragePath());
}
json_variant PluginHost::AbstractPath(const json_variant &json)
{
    AtomConverter converter(GetMapFeature());
    return converter.AbstractPath(json, GetPluginStoragePath());
}

std::string PluginHost::MapPath(const std::string &abstractPath)
{
    auto storagePath = GetPluginStoragePath();
    if (abstractPath.length() == 0)
    {
        return "";
    }
    return SS(storagePath << '/' << abstractPath);
}
std::string PluginHost::AbstractPath(const std::string &path)
{
    auto storagePath = GetPluginStoragePath();
    if (path.starts_with(storagePath))
    {
        auto start = storagePath.length();
        if (start < path.length() && path[start] == '/')
        {
            ++start;
        }
        return path.substr(start);
    }
    return path;
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
     json_map::reference("connection_optional", &Lv2PortInfo::connection_optional_),

     json_map::reference("is_valid", &Lv2PortInfo::is_valid_),

     json_map::reference("supports_midi", &Lv2PortInfo::supports_midi_),
     json_map::reference("supports_time_position", &Lv2PortInfo::supports_time_position_),
     json_map::reference("port_group", &Lv2PortInfo::port_group_),
     json_map::reference("is_logarithmic", &Lv2PortInfo::is_logarithmic_),

     MAP_REF(Lv2PortInfo, is_logarithmic),
     MAP_REF(Lv2PortInfo, display_priority),
     MAP_REF(Lv2PortInfo, range_steps),
     MAP_REF(Lv2PortInfo, trigger_property),
     MAP_REF(Lv2PortInfo, integer_property),
     MAP_REF(Lv2PortInfo, enumeration_property),
     MAP_REF(Lv2PortInfo, toggled_property),
     MAP_REF(Lv2PortInfo, not_on_gui),
     MAP_REF(Lv2PortInfo, buffer_type),
     MAP_REF(Lv2PortInfo, port_group),
     MAP_REF(Lv2PortInfo, pipedal_ledColor),

     json_map::enum_reference("units", &Lv2PortInfo::units_, get_units_enum_converter()),
     MAP_REF(Lv2PortInfo, comment)}};

json_map::storage_type<Lv2PortGroup> Lv2PortGroup::jmap{{
    MAP_REF(Lv2PortGroup, uri),

}};

json_map::storage_type<Lv2PluginInfo> Lv2PluginInfo::jmap{{
    json_map::reference("bundle_path", &Lv2PluginInfo::bundle_path_),
    json_map::reference("uri", &Lv2PluginInfo::uri_),
    json_map::reference("name", &Lv2PluginInfo::name_),
    json_map::reference("brand", &Lv2PluginInfo::brand_),
    json_map::reference("label", &Lv2PluginInfo::label_),
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
    MAP_REF(Lv2PluginUiPortGroup, uri),
    MAP_REF(Lv2PluginUiPortGroup, symbol),
    MAP_REF(Lv2PluginUiPortGroup, name),
    MAP_REF(Lv2PluginUiPortGroup, parent_group),
    MAP_REF(Lv2PluginUiPortGroup, program_list_id),
}};

json_map::storage_type<Lv2PluginUiPort> Lv2PluginUiPort::jmap{{
    json_map::reference("symbol", &Lv2PluginUiPort::symbol_),
    json_map::reference("name", &Lv2PluginUiPort::name_),
    MAP_REF(Lv2PluginUiPort, index),
    MAP_REF(Lv2PluginUiPort, is_input),
    MAP_REF(Lv2PluginUiPort, min_value),
    MAP_REF(Lv2PluginUiPort, max_value),
    MAP_REF(Lv2PluginUiPort, default_value),
    MAP_REF(Lv2PluginUiPort, is_logarithmic),
    MAP_REF(Lv2PluginUiPort, display_priority),

    MAP_REF(Lv2PluginUiPort, range_steps),
    MAP_REF(Lv2PluginUiPort, integer_property),
    MAP_REF(Lv2PluginUiPort, enumeration_property),
    MAP_REF(Lv2PluginUiPort, not_on_gui),
    MAP_REF(Lv2PluginUiPort, toggled_property),
    MAP_REF(Lv2PluginUiPort, trigger_property),
    MAP_REF(Lv2PluginUiPort, scale_points),
    MAP_REF(Lv2PluginUiPort, port_group),
    MAP_REF(Lv2PluginUiPort, pipedal_ledColor),

    json_map::enum_reference("units", &Lv2PluginUiPort::units_, get_units_enum_converter()),
    MAP_REF(Lv2PluginUiPort, comment),
    MAP_REF(Lv2PluginUiPort, is_bypass),
    MAP_REF(Lv2PluginUiPort, is_program_controller),
    MAP_REF(Lv2PluginUiPort, custom_units),
    MAP_REF(Lv2PluginUiPort, connection_optional),

}};

json_map::storage_type<Lv2PluginUiInfo>
    Lv2PluginUiInfo::jmap{
        {
            json_map::reference("uri", &Lv2PluginUiInfo::uri_),
            json_map::reference("name", &Lv2PluginUiInfo::name_),
            json_map::reference("brand", &Lv2PluginUiInfo::brand_),
            json_map::reference("label", &Lv2PluginUiInfo::label_),
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
            json_map::reference("frequencyPlots", &Lv2PluginUiInfo::frequencyPlots_),
            json_map::reference("uiPortNotifications", &Lv2PluginUiInfo::uiPortNotifications_),
        }};
