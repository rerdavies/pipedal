#include "pch.h"
#include "Lv2Host.hpp"
#include <lilv/lilv.h>
#include <stdexcept>
#include <locale>
#include <string_view>
#include "Lv2Log.hpp"
#include <functional>
#include "PedalBoard.hpp"
#include "Lv2Effect.hpp"
#include "Lv2PedalBoard.hpp"
#include "OptionsFeature.hpp"
#include "JackConfiguration.hpp"
#include "lv2/urid/urid.h"
#include "lv2/core/lv2.h"
#include "lv2/atom/atom.h"
#include "lv2/lv2plug.in/ns/ext/buf-size/buf-size.h"
#include "lv2/lv2plug.in/ns/ext/presets/presets.h"
#include "lv2/lv2plug.in/ns/ext/port-props/port-props.h"
#include  "lv2/lv2plug.in/ns/extensions/units/units.h"
#include "lv2/lv2plug.in/ns/ext/port-groups/port-groups.h"
#include <fstream>
#include "PiPedalException.hpp"
#include "Locale.hpp"

using namespace pipedal;

#define MAP_CHECK()                                                           \
    {                                                                         \
        if (GetPluginClass("http://lv2plug.in/ns/lv2core#Plugin") == nullptr) \
            throw PiPedalStateException("Map is broken");                      \
    }

#define PLUGIN_MAP_CHECK()                                                           \
    {                                                                                \
        if (pHost->GetPluginClass("http://lv2plug.in/ns/lv2core#Plugin") == nullptr) \
            throw PiPedalStateException("Map is broken");                             \
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


    class Lv2Host::Urids {
    public:
        Urids(MapFeature &mapFeature)
        {
            atom_Float           = mapFeature.GetUrid(LV2_ATOM__Float);
        }
        LV2_URID atom_Float;
    };
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

class NodeAutoFree
{
    LilvNode *node = nullptr;

    // const LilvNode* returns must not be freed, by convention.
    NodeAutoFree(const LilvNode *node)
    {
    }

public:
    NodeAutoFree()
    {
        this->node = nullptr;
    }
    NodeAutoFree(LilvNode *node)
    {
        this->node = node;
    }
    operator const LilvNode *()
    {
        return this->node;
    }

    LilvNode **operator&()
    {
        return &(this->node);
    }

    operator bool()
    {
        return this->node != nullptr;
    }

    float as_float(float defaultValue = 0)
    {
        return nodeAsFloat(this->node, defaultValue);
    }
    int as_int()
    {
        if (node == nullptr)
            return 0;
        if (lilv_node_is_int(node))
            return lilv_node_as_int(node);
        return 0;
    }
    bool as_bool()
    {
        if (node == nullptr)
            return false;
        if (lilv_node_is_int(node))
            return lilv_node_as_bool(node);
        return false;
    }
    std::string as_string()
    {
        return nodeAsString(this->node);
    }

    ~NodeAutoFree()
    {
        if (this->node != nullptr)
        {
            lilv_free(this->node);
            this->node = nullptr;
        }
    }
};

Lv2Host::Lv2Host()
{
    pWorld = nullptr;

    LV2_Feature **features = new LV2_Feature *[10];
    features[0] = const_cast<LV2_Feature *>(mapFeature.GetMapFeature());
    features[1] = const_cast<LV2_Feature *>(mapFeature.GetUnmapFeature());

    logFeature.Prepare(&mapFeature);
    features[2] = const_cast<LV2_Feature *>(logFeature.GetFeature());
    optionsFeature.Prepare(mapFeature, 44100, this->GetMaxAudioBufferSize(), this->GetAtomBufferSize());
    features[3] = const_cast<LV2_Feature *>(optionsFeature.GetFeature());
    features[4] = nullptr;

    this->lv2Features = features;
    this->urids = new Urids(mapFeature);
}

void Lv2Host::OnConfigurationChanged(const JackConfiguration &configuration, const JackChannelSelection &settings)
{
    this->sampleRate = configuration.GetSampleRate();
    if (configuration.isValid())
    {
        this->numberOfAudioInputChannels = settings.GetInputAudioPorts().size();
        this->numberOfAudioOutputChannels = settings.getOutputAudioPorts().size();
        this->maxBufferSize = configuration.GetBlockLength();
        optionsFeature.Prepare(this->mapFeature, configuration.GetSampleRate(), configuration.GetBlockLength(), GetAtomBufferSize());
    }
}

Lv2Host::~Lv2Host()
{
    delete[] lv2Features;
    free_world();
    delete urids;
}

void Lv2Host::free_world()
{
    if (pWorld)
    {
        lilv_world_free(pWorld);
        pWorld = nullptr;
    }
}
std::shared_ptr<Lv2PluginClass> Lv2Host::GetPluginClass(const std::string &uri) const
{
    auto it = this->classesMap.find(uri);
    if (it == this->classesMap.end())
        return nullptr;
    return (*it).second;
}

void Lv2Host::AddJsonClassesToMap(std::shared_ptr<Lv2PluginClass> pluginClass)
{
    this->classesMap[pluginClass->uri()] = pluginClass;
    for (int i = 0; i < pluginClass->children_.size(); ++i)
    {
        std::shared_ptr<Lv2PluginClass> child = pluginClass->children_[i];
        AddJsonClassesToMap(pluginClass->children_[i]);
        child->parent_ = pluginClass.get();
    }
}
void Lv2Host::LoadPluginClassesFromJson(std::filesystem::path jsonFile)
{
    std::ifstream f(jsonFile);
    json_reader reader(f);

    std::shared_ptr<Lv2PluginClass> classes;
    reader.read(&classes);

    AddJsonClassesToMap(classes);
    this->classesLoaded = true;

    MAP_CHECK();
}

std::shared_ptr<Lv2PluginClass> Lv2Host::GetPluginClass(const LilvPluginClass *pClass)
{
    MAP_CHECK();

    std::string uri = nodeAsString(lilv_plugin_class_get_uri(pClass));
    std::shared_ptr<Lv2PluginClass> pResult = this->classesMap[uri];
    return pResult;
}
std::shared_ptr<Lv2PluginClass> Lv2Host::MakePluginClass(const LilvPluginClass *pClass)
{
    std::string uri = nodeAsString(lilv_plugin_class_get_uri(pClass));

    std::shared_ptr<Lv2PluginClass> pResult = this->classesMap[uri];
    if (!pResult)
    {
        const LilvNode *parentNode = lilv_plugin_class_get_parent_uri(pClass);
        std::string parent_uri = nodeAsString(parentNode);

        std::string name = nodeAsString(lilv_plugin_class_get_label(pClass));

        std::shared_ptr<Lv2PluginClass> result = std::make_shared<Lv2PluginClass>(
            name.c_str(), uri.c_str(), parent_uri.c_str());

        classesMap[uri] = std::move(result);
    }
    return pResult;
}

void Lv2Host::LoadPluginClassesFromLilv()
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

void Lv2Host::Load(const char *lv2Path)
{
    this->plugins_.clear();
    this->ui_plugins_.clear();
    if (!classesLoaded)
    {
        this->classesMap.clear();
    }

    free_world();

    setenv("LV2_PATH", lv2Path, true);
    pWorld = lilv_world_new();
    lilv_world_load_all(pWorld);

    if (!classesLoaded)
    {
        LoadPluginClassesFromLilv();
    }

    const LilvPlugins *plugins = lilv_world_get_all_plugins(pWorld);

    LILV_FOREACH(plugins, iPlugin, plugins)
    {
        const LilvPlugin *lilvPlugin = lilv_plugins_get(plugins, iPlugin);

        std::shared_ptr<Lv2PluginInfo> pluginInfo = std::make_shared<Lv2PluginInfo>(this, lilvPlugin);

        Lv2Log::debug("Plugin: " + pluginInfo->name());

        if (pluginInfo->hasCvPorts())
        {
            Lv2Log::debug("Plugin %s (%s) skipped. (Has CV ports).", pluginInfo->name().c_str(), pluginInfo->uri().c_str());
        }
        else if (pluginInfo->plugin_class() == LV2_MIDI_PLUGIN)
        {
            Lv2Log::debug("Plugin %s (%s) skipped. (MIDI Plugin).", pluginInfo->name().c_str(), pluginInfo->uri().c_str());
        }
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

    auto compare = [](
                       const std::shared_ptr<Lv2PluginInfo> &left,
                       const std::shared_ptr<Lv2PluginInfo> &right)
    {
        const char *pb1 = left->name().c_str();
        const char *pb2 = right->name().c_str();
        return Locale::collation.compare(
                   pb1, pb1 + left->name().size(),
                   pb2, pb2 + right->name().size()) < 0;
    };
    std::sort(this->plugins_.begin(), this->plugins_.end(), compare);

    for (auto plugin : this->plugins_)
    {
        Lv2PluginUiInfo info(this, plugin.get());
        if (plugin->is_valid())
        {
            if (info.audio_inputs() == 0)
            {
                Lv2Log::debug("Plugin %s (%s) skipped. No audio inputs.", plugin->name().c_str(), plugin->uri().c_str());
            }
            else if (info.audio_outputs() == 0)
            {
                Lv2Log::debug("Plugin %s (%s) skipped. No audio outputs.", plugin->name().c_str(), plugin->uri().c_str());
            }
            else
            {
                ui_plugins_.push_back(std::move(info));
            }
        }
    }
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

static const char *RDFS_COMMENT_URI = "http://www.w3.org/2000/01/rdf-schema#"
                                      "comment";

static LilvNode *get_comment(LilvWorld *pWorld, const std::string &uri)
{
    NodeAutoFree uriNode = lilv_new_uri(pWorld, uri.c_str());
    NodeAutoFree rdfsComment = lilv_new_uri(pWorld, RDFS_COMMENT_URI);
    LilvNode *result = lilv_world_get(pWorld, uriNode, rdfsComment, nullptr);
    return result;
}

static bool ports_sort_compare(std::shared_ptr<Lv2PortInfo> &p1, const std::shared_ptr<Lv2PortInfo> &p2)
{
    return p1->index() < p2->index();
}

Lv2PluginInfo::Lv2PluginInfo(Lv2Host *lv2Host, const LilvPlugin *pPlugin)
{
    this->uri_ = nodeAsString(lilv_plugin_get_uri(pPlugin));

    NodeAutoFree name = (lilv_plugin_get_name(pPlugin));
    this->name_ = nodeAsString(name);

    NodeAutoFree author_name = (lilv_plugin_get_author_name(pPlugin));
    this->author_name_ = nodeAsString(author_name);

    NodeAutoFree author_homepage = (lilv_plugin_get_author_homepage(pPlugin));
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

    NodeAutoFree comment = get_comment(lv2Host->pWorld, this->uri_);
    this->comment_ = nodeAsString(comment);

    uint32_t ports = lilv_plugin_get_num_ports(pPlugin);

    bool isValid = true;
    std::vector<std::string> portGroups;

    for (uint32_t i = 0; i < ports; ++i)
    {
        const LilvPort *pPort = lilv_plugin_get_port_by_index(pPlugin, i);

        std::shared_ptr<Lv2PortInfo> portInfo{new Lv2PortInfo(lv2Host, pPlugin, pPort)};
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

bool Lv2PortInfo::is_a(Lv2Host *lv2Plugins, const char *classUri)
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
Lv2PortInfo::Lv2PortInfo(Lv2Host *plugins, const LilvPlugin *plugin, const LilvPort *pPort)
{
    auto pWorld = plugins->pWorld;
    index_ = lilv_port_get_index(plugin, pPort);
    symbol_ = nodeAsString(lilv_port_get_symbol(plugin, pPort));

    NodeAutoFree name = lilv_port_get_name(plugin, pPort);
    name_ = nodeAsString(name);

    classes_ = plugins->GetPluginPortClass(plugin, pPort);

    NodeAutoFree minNode, maxNode, defaultNode;
    min_value_ = 0;
    max_value_ = 1;
    default_value_ = 0;
    lilv_port_get_range(plugin, pPort, &defaultNode, &minNode, &maxNode);
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

    NodeAutoFree logarithic_uri = lilv_new_uri(pWorld, LV2_PORT_LOGARITHMIC);
    this->is_logarithmic_ = lilv_port_has_property(plugin, pPort, logarithic_uri);

    NodeAutoFree display_priority_uri = lilv_new_uri(pWorld, LV2_PORT_DISPLAY_PRIORITY);
    NodesAutoFree priority_nodes = lilv_port_get_value(plugin, pPort, display_priority_uri);

    this->display_priority_ = -1;
    if (priority_nodes)
    {
        auto priority_node = lilv_nodes_get_first(priority_nodes);
        if (priority_node)
        {
            this->display_priority_ = lilv_node_as_int(priority_node);
        }
    }

    NodeAutoFree range_steps_uri = lilv_new_uri(pWorld, LV2_PORT_RANGE_STEPS);
    NodesAutoFree range_steps_nodes = lilv_port_get_value(plugin, pPort, range_steps_uri);
    this->range_steps_ = 0;
    if (range_steps_nodes)
    {
        auto range_steps_node = lilv_nodes_get_first(range_steps_nodes);
        if (range_steps_node)
        {
            this->range_steps_ = lilv_node_as_int(range_steps_node);
        }
    }
    NodeAutoFree integer_property_uri = lilv_new_uri(pWorld, LV2_INTEGER);
    this->integer_property_ = lilv_port_has_property(plugin, pPort, integer_property_uri);

    NodeAutoFree enumeration_property_uri = lilv_new_uri(pWorld, LV2_ENUMERATION);
    this->enumeration_property_ = lilv_port_has_property(plugin, pPort, enumeration_property_uri);

    NodeAutoFree toggle_property_uri = lilv_new_uri(pWorld, LV2_CORE__toggled);
    this->toggled_property_ = lilv_port_has_property(plugin, pPort, toggle_property_uri);

    NodeAutoFree not_on_gui_property_uri = lilv_new_uri(pWorld, LV2_PORT_PROPS__notOnGUI);
    this->not_on_gui_ = lilv_port_has_property(plugin, pPort, not_on_gui_property_uri);

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

    is_input_ = is_a(plugins, LV2_INPUT_PORT);
    is_output_ = is_a(plugins, LV2_OUTPUT_PORT);

    is_control_port_ = is_a(plugins, LV2_CORE__ControlPort);
    is_audio_port_ = is_a(plugins, LV2_CORE__AudioPort);
    is_atom_port_ = is_a(plugins, LV2_ATOM_PORT);
    is_cv_port_ = is_a(plugins, LV2_CORE__CVPort);

    NodeAutoFree midiEventNode = lilv_new_uri(plugins->pWorld, LV2_MIDI_EVENT);
    supports_midi_ = lilv_port_supports_event(plugin, pPort, midiEventNode);

    NodeAutoFree designationNode = lilv_new_uri(plugins->pWorld, LV2_DESIGNATION);
    NodeAutoFree designationValue = lilv_port_get(plugin, pPort, designationNode);
    designation_ = nodeAsString(designationValue);

    NodeAutoFree portGroupUri = lilv_new_uri(plugins->pWorld, LV2_PORT_GROUPS__group);
    NodeAutoFree portGroup_value = lilv_port_get(plugin, pPort, portGroupUri);
    port_group_ = nodeAsString(portGroup_value);

    NodeAutoFree unitsUri = lilv_new_uri(plugins->pWorld, LV2_UNITS__unit);
    NodeAutoFree unitsValueUri = lilv_port_get(plugin, pPort, unitsUri);
    this->units_ = UriToUnits(nodeAsString(unitsValueUri));


    NodeAutoFree bufferType_uri = lilv_new_uri(pWorld, LV2_ATOM__bufferType);
    NodeAutoFree bufferType = lilv_port_get(plugin, pPort, bufferType_uri);

    this->buffer_type_ = "";
    if (bufferType)
    {
        this->buffer_type_ = nodeAsString(bufferType);
    }

    is_valid_ = is_control_port_ ||
                ((is_input_ || is_output_) && (is_audio_port_ || is_atom_port_ || is_cv_port_));
}

Lv2PluginClasses Lv2Host::GetPluginPortClass(const LilvPlugin *lilvPlugin, const LilvPort *lilvPort)
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
bool Lv2PluginClasses::is_a(Lv2Host *lv2Plugins, const char *classUri) const
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

bool Lv2Host::is_a(const std::string &class_, const std::string &target_class)
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

Lv2PluginUiInfo::Lv2PluginUiInfo(Lv2Host *pHost, const Lv2PluginInfo *plugin)
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
        Lv2Log::error("%s: Unknown plugin type: %s", plugin->uri().c_str(), plugin->plugin_class().c_str());
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
}

std::shared_ptr<Lv2PluginClass> Lv2Host::GetLv2PluginClass() const
{
    return this->GetPluginClass(LV2_PLUGIN);
}

std::shared_ptr<Lv2PluginInfo> Lv2Host::GetPluginInfo(const std::string &uri) const
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


Lv2PedalBoard *Lv2Host::CreateLv2PedalBoard(const PedalBoard &pedalBoard)
{
    Lv2PedalBoard *pPedalBoard = new Lv2PedalBoard();
    try
    {
        pPedalBoard->Prepare(this, pedalBoard);
        return pPedalBoard;
    }
    catch (const std::exception &)
    {
        delete pPedalBoard;
        throw;
    }
}


struct StateCallbackData {
    Lv2Host*pHost;
    std::vector<ControlValue> *pResult;
};

void Lv2Host::fn_LilvSetPortValueFunc(const char* port_symbol,
                                     void*       user_data,
                                     const void* value,
                                     uint32_t    size,
                                     uint32_t    type)
{
    StateCallbackData *pData = (StateCallbackData*)user_data;
    Lv2Host *pHost = pData->pHost;
    std::vector<ControlValue> *pResult = pData->pResult;
    LV2_URID valueType = (LV2_URID)type;
    if (valueType != pHost->urids->atom_Float)
    {
        throw PiPedalStateException("Preset format not supported.");
    }
    pResult->push_back(ControlValue(port_symbol,*(float*)value));

}



std::vector<ControlValue> Lv2Host::LoadPluginPreset(PedalBoardItem*pedalBoardItem, const std::string&presetUri)
{

    std::vector<ControlValue> result;

    const LilvPlugins*plugins = lilv_world_get_all_plugins(this->pWorld);

    NodeAutoFree uriNode = lilv_new_uri(pWorld, pedalBoardItem->uri().c_str());

    lilv_world_load_resource(pWorld, uriNode);

    const LilvPlugin * plugin = lilv_plugins_get_by_uri (plugins, uriNode);
    if (plugin == nullptr)
    {
        throw PiPedalStateException("No such plugin.");
    }

    NodeAutoFree pset_Preset = lilv_new_uri(pWorld, LV2_PRESETS__Preset);
    NodeAutoFree rdfs_label = lilv_new_uri(pWorld, LILV_NS_RDFS "label");
    

    LilvNodes* presets = lilv_plugin_get_related(plugin, pset_Preset);
	LILV_FOREACH(nodes, i, presets) {
		const LilvNode* preset = lilv_nodes_get(presets, i);
		lilv_world_load_resource(pWorld, preset);

        std::string thisPresetUri = nodeAsString(preset);
        if (presetUri == thisPresetUri)
        {

            /*********************************/


                // NodeAutoFree uriNode = lilv_new_uri(pWorld, presetUri.c_str());

                auto lilvState = lilv_state_new_from_world(pWorld, GetLv2UridMap(), preset);

                if (lilvState == nullptr)
                {
                    throw PiPedalStateException("Preset not found.");
                }


                StateCallbackData cbData;
                cbData.pHost = this;
                cbData.pResult = &result;
                auto n = lilv_state_get_num_properties(lilvState);
                lilv_state_emit_port_values (lilvState, fn_LilvSetPortValueFunc,&cbData);

                lilv_state_free(lilvState);
                break;

            /*********************************/
        }

	}
	lilv_nodes_free(presets);

    return result;
}

std::vector<Lv2PluginPreset> Lv2Host::GetPluginPresets(const std::string &pluginUri)
{
    std::vector<Lv2PluginPreset> result;

    const LilvPlugins*plugins = lilv_world_get_all_plugins(this->pWorld);

    NodeAutoFree uriNode = lilv_new_uri(pWorld, pluginUri.c_str());

    lilv_world_load_resource(pWorld, uriNode);

    const LilvPlugin * plugin = lilv_plugins_get_by_uri (plugins, uriNode);
    if (plugin == nullptr)
    {
        throw PiPedalStateException("No such plugin.");
    }

    NodeAutoFree pset_Preset = lilv_new_uri(pWorld, LV2_PRESETS__Preset);
    NodeAutoFree rdfs_label = lilv_new_uri(pWorld, LILV_NS_RDFS "label");
    

    LilvNodes* presets = lilv_plugin_get_related(plugin, pset_Preset);
	LILV_FOREACH(nodes, i, presets) {
		const LilvNode* preset = lilv_nodes_get(presets, i);
		lilv_world_load_resource(pWorld, preset);

		LilvNodes* labels = lilv_world_find_nodes(
			pWorld, preset, rdfs_label, NULL);
		if (labels) {
			const LilvNode* label = lilv_nodes_get_first(labels);
            std::string presetName = nodeAsString(label);
            result.push_back(Lv2PluginPreset(nodeAsString(preset),presetName));
			lilv_nodes_free(labels);
		} else {
            std::stringstream s;
            s << "Preset <" << nodeAsString(lilv_nodes_get(presets,i)) << "> has no rdfs:label.";

			Lv2Log::warning(s.str());
		}
	}
	lilv_nodes_free(presets);

    auto compare = [] (const Lv2PluginPreset&left, const Lv2PluginPreset&right)
    {
        return Locale::collation.compare(
            left.name_.c_str(),
            left.name_.c_str()+left.name_.size(),
            right.name_.c_str(),
            right.name_.c_str()+right.name_.size()) < 0;
    };

    std::sort(result.begin(),result.end(),compare);
	return result;

}


Lv2Effect *Lv2Host::CreateEffect(const PedalBoardItem &pedalBoardItem)
{

    auto info = this->GetPluginInfo(pedalBoardItem.uri());
    if (!info) return nullptr;

    return new Lv2Effect(this, info, pedalBoardItem);
}

Lv2PortGroup::Lv2PortGroup(Lv2Host *lv2Host, const std::string &groupUri)
{
    LilvWorld *pWorld = lv2Host->pWorld;

    NodeAutoFree symbolUri = lilv_new_uri(pWorld, LV2_CORE__symbol);
    NodeAutoFree nameUri = lilv_new_uri(pWorld, LV2_CORE__name);
    this->uri_ = groupUri;
    NodeAutoFree uri = lilv_new_uri(pWorld, groupUri.c_str());
    LilvNode *symbolNode = lilv_world_get(pWorld, uri, symbolUri, nullptr);
    symbol_ = nodeAsString(symbolNode);
    LilvNode *nameNode = lilv_world_get(pWorld, uri, nameUri, nullptr);
    name_ = nodeAsString(nameNode);
}



#define MAP_REF(class, name) \
    json_map::reference(#name, &class ::name##_)

#define CONDITIONAL_MAP_REF(class, name, condition) \
    json_map::conditional_reference(#name, &class ::name##_, condition)

json_map::storage_type<Lv2PluginClasses> Lv2PluginClasses::jmap{{
    //json_map::reference("char_", &JsonTestTarget::char_),
    json_map::reference("classes", &Lv2PluginClasses::classes_),
}};

json_map::storage_type<Lv2ScalePoint> Lv2ScalePoint::jmap{{
    json_map::reference("value", &Lv2ScalePoint::value_),
    json_map::reference("label", &Lv2ScalePoint::label_),
}};

json_map::storage_type<Lv2PortInfo> Lv2PortInfo::jmap{{
    json_map::reference("index", &Lv2PortInfo::index_),
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
}};

json_map::storage_type<Lv2PortGroup> Lv2PortGroup::jmap{{
    MAP_REF(Lv2PortGroup, uri),

}};

json_map::storage_type<Lv2PluginInfo> Lv2PluginInfo::jmap{{
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

}};

json_map::storage_type<Lv2PluginUiInfo> Lv2PluginUiInfo::jmap{{
    json_map::reference("uri", &Lv2PluginUiInfo::uri_),
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
}};

JSON_MAP_BEGIN(Lv2PluginPreset)
    JSON_MAP_REFERENCE(Lv2PluginPreset,presetUri)
    JSON_MAP_REFERENCE(Lv2PluginPreset,name)
JSON_MAP_END()

