/*
 *   Copyright (c) 2025 Robin E. R. Davies
 *   All rights reserved.

 *   Permission is hereby granted, free of charge, to any person obtaining a copy
 *   of this software and associated documentation files (the "Software"), to deal
 *   in the Software without restriction, including without limitation the rights
 *   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *   copies of the Software, and to permit persons to whom the Software is
 *   furnished to do so, subject to the following conditions:
 
 *   The above copyright notice and this permission notice shall be included in all
 *   copies or substantial portions of the Software.
 
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *   SOFTWARE.
 */


 #include "ModGui.hpp"
 #include "MapFeature.hpp"
 #include "PluginHost.hpp"
 #include "Finally.hpp"
 #include "lv2/atom/atom.h"


#define MOD_GUI_PREFIX "http://moddevices.com/ns/modgui#"
#define MOD_GUI__gui (MOD_GUI_PREFIX "gui")
#define MOD_GUI__modgui (MOD_GUI_PREFIX "modgui")
#define MOD_GUI__resourcesDirectory (MOD_GUI_PREFIX "resourcesDirectory")
#define MOD_GUI__iconTemplate (MOD_GUI_PREFIX "iconTemplate")
#define MOD_GUI__settingsTemplate (MOD_GUI_PREFIX "settingsTemplate")
#define MOD_GUI__javascript (MOD_GUI_PREFIX "javascript")
#define MOD_GUI__stylesheet (MOD_GUI_PREFIX "stylesheet")
#define MOD_GUI__screenshot (MOD_GUI_PREFIX "screenshot")
#define MOD_GUI__thumbnail (MOD_GUI_PREFIX "thumbnail")
#define MOD_GUI__discussionURL (MOD_GUI_PREFIX "discussionURL")
#define MOD_GUI__documentation (MOD_GUI_PREFIX "documentation")
#define MOD_GUI__brand (MOD_GUI_PREFIX "brand")
#define MOD_GUI__label (MOD_GUI_PREFIX "label")
#define MOD_GUI__model (MOD_GUI_PREFIX "model")
#define MOD_GUI__panel (MOD_GUI_PREFIX "panel")
#define MOD_GUI__color (MOD_GUI_PREFIX "color")
#define MOD_GUI__knob (MOD_GUI_PREFIX "knob")
#define MOD_GUI__port (MOD_GUI_PREFIX "port")
#define MOD_GUI__monitoredOutputs (MOD_GUI_PREFIX "monitoredOutputs")



using namespace pipedal;

ModGuiUris::ModGuiUris(LilvWorld *pWorld, MapFeature &mapFeature)
{
    mod_gui__gui = lilv_new_uri(pWorld, MOD_GUI__gui);
    mod_gui__modgui = lilv_new_uri(pWorld, MOD_GUI__modgui);
    mod_gui__resourceDirectory = lilv_new_uri(pWorld, MOD_GUI__resourcesDirectory);
    mod_gui__iconTemplate = lilv_new_uri(pWorld, MOD_GUI__iconTemplate);
    mod_gui__settingsTemplate = lilv_new_uri(pWorld, MOD_GUI__settingsTemplate);
    mod_gui__javascript = lilv_new_uri(pWorld, MOD_GUI__javascript);
    mod_gui__stylesheet = lilv_new_uri(pWorld, MOD_GUI__stylesheet);      
    mod_gui__screenshot = lilv_new_uri(pWorld, MOD_GUI__screenshot);
    mod_gui__thumbnail = lilv_new_uri(pWorld, MOD_GUI__thumbnail);
    mod_gui__discussionURL = lilv_new_uri(pWorld, MOD_GUI__discussionURL);
    mod_gui__documentation = lilv_new_uri(pWorld, MOD_GUI__documentation);
    mod_gui__brand = lilv_new_uri(pWorld, MOD_GUI__brand);
    mod_gui__label = lilv_new_uri(pWorld, MOD_GUI__label);
    mod_gui__model = lilv_new_uri(pWorld, MOD_GUI__model);
    mod_gui__panel = lilv_new_uri(pWorld, MOD_GUI__panel);
    mod_gui__color = lilv_new_uri(pWorld, MOD_GUI__color);
    mod_gui__knob = lilv_new_uri(pWorld, MOD_GUI__knob);
    mod_gui__port = lilv_new_uri(pWorld, MOD_GUI__port);
    mod_gui__monitoredOutputs = lilv_new_uri(pWorld, MOD_GUI__monitoredOutputs);
}

static const char*NonNull(const char*string) {
    return string ? string : "";
}


ModGui::ModGui(
    PluginHost *lv2Host, 
    const LilvPlugin *lilvPlugin, 
    const std::string &resourceDirectory,
    const LilvNode * modGuiUrl)
    : pluginUri_(lilv_node_as_uri(lilv_plugin_get_uri(lilvPlugin))),
    resourceDirectory_(resourceDirectory)
{
    LilvWorld *pWorld = lv2Host->getWorld();
    ModGuiUris &modGuiUrids = *lv2Host->mod_gui_uris;
    PluginHost::LilvUris &lilvUris = *lv2Host->lilvUris;

    AutoLilvNode modguiIcon = lilv_world_get(pWorld, modGuiUrl, modGuiUrids.mod_gui__iconTemplate, nullptr);
    if (modguiIcon) {
        this->iconTemplate_ = NonNull(lilv_file_uri_parse(lilv_node_as_string(modguiIcon), nullptr));
    }

    AutoLilvNode modguiSettingsTemplate = lilv_world_get(pWorld, modGuiUrl, modGuiUrids.mod_gui__settingsTemplate, nullptr);
    if (modguiSettingsTemplate) {
        this->settingsTemplate_ = NonNull(lilv_file_uri_parse(lilv_node_as_string(modguiSettingsTemplate), nullptr));
    }   
    
    AutoLilvNode modguiJavascript = lilv_world_get(pWorld, modGuiUrl, modGuiUrids.mod_gui__javascript, nullptr);
    if (modguiJavascript) {
        this->javascript_ = NonNull(lilv_file_uri_parse(lilv_node_as_string(modguiJavascript), nullptr));
    }

    AutoLilvNode modguiStylesheet = lilv_world_get(pWorld, modGuiUrl, modGuiUrids.mod_gui__stylesheet, nullptr);
    if (modguiStylesheet) {
        this->stylesheet_ = NonNull(lilv_file_uri_parse(lilv_node_as_string(modguiStylesheet), nullptr));
    }

    AutoLilvNode modguiScreenshot = lilv_world_get(pWorld, modGuiUrl, modGuiUrids.mod_gui__screenshot, nullptr);
    if (modguiScreenshot) {     
        this->screenshot_ = NonNull(lilv_file_uri_parse(lilv_node_as_string(modguiScreenshot), nullptr));
    }   

    AutoLilvNode modguiThumbnail = lilv_world_get(pWorld, modGuiUrl, modGuiUrids.mod_gui__thumbnail, nullptr);
    if (modguiThumbnail) {
        this->thumbnail_ = NonNull(lilv_file_uri_parse(lilv_node_as_string(modguiThumbnail), nullptr));
    }
    AutoLilvNode discussionUrl = lilv_world_get(pWorld, modGuiUrl, modGuiUrids.mod_gui__discussionURL, nullptr);
    if (discussionUrl) {
        this->discussionUrl_ = NonNull(lilv_file_uri_parse(lilv_node_as_string(discussionUrl), nullptr));
    }

    AutoLilvNode documentationUrl = lilv_world_get(pWorld, modGuiUrl, modGuiUrids.mod_gui__documentation, nullptr);
    if (documentationUrl) {
        this->documentationUrl_ = NonNull(lilv_file_uri_parse(lilv_node_as_string(documentationUrl), nullptr));
    }

    AutoLilvNode brand = lilv_world_get(pWorld, modGuiUrl, modGuiUrids.mod_gui__brand, nullptr);
    if (brand) {
        this->brand_ = NonNull(lilv_node_as_string(brand));
    }

    AutoLilvNode label = lilv_world_get(pWorld, modGuiUrl, modGuiUrids.mod_gui__label, nullptr);
    if (label) {
        this->label_ = NonNull(lilv_node_as_string(label));
    }

    AutoLilvNode model = lilv_world_get(pWorld, modGuiUrl, modGuiUrids.mod_gui__model, nullptr);
    if (model) {
        this->model_ = NonNull(lilv_node_as_string(model));
    }

    AutoLilvNode panel = lilv_world_get(pWorld, modGuiUrl, modGuiUrids.mod_gui__panel, nullptr);
    if (panel) {
        this->panel_ = NonNull(lilv_node_as_string(panel));
    }

    AutoLilvNode color = lilv_world_get(pWorld, modGuiUrl, modGuiUrids.mod_gui__color, nullptr);
    if (color) {
        this->color_ = NonNull(lilv_node_as_string(color));
    }

    AutoLilvNode knob = lilv_world_get(pWorld, modGuiUrl, modGuiUrids.mod_gui__knob, nullptr);
    if (knob) {
        this->knob_ = NonNull(lilv_node_as_string(knob));
    }

    AutoLilvNodes ports = lilv_world_find_nodes(pWorld, modGuiUrl, modGuiUrids.mod_gui__port, nullptr);
    if (ports) {
        LILV_FOREACH(nodes, it, ports.Get()) {
            AutoLilvNode port_node = lilv_nodes_get(ports, it);
            this->ports_.push_back(ModGuiPort(lv2Host,port_node));
        }
    }
    std::sort(this->ports_.begin(), this->ports_.end(),
        [](const ModGuiPort &p1, const ModGuiPort &p2) {
            return p1.index() < p2.index();
        });
    AutoLilvNodes monitoredOutputs = lilv_world_find_nodes(pWorld, modGuiUrl, modGuiUrids.mod_gui__monitoredOutputs, nullptr);
    if (monitoredOutputs) {
        LILV_FOREACH(nodes, it, monitoredOutputs.Get()) {
            AutoLilvNode monitoredOutput = lilv_nodes_get(monitoredOutputs.Get(), it);
            AutoLilvNode symbol = lilv_world_get(pWorld, monitoredOutput, lilvUris.lv2core__symbol, nullptr);
            if (symbol) {
                this->monitoredOutputs_.push_back(NonNull(lilv_node_as_string(symbol)));
            }
        }
    }
}


ModGui::ptr ModGui::Create(PluginHost *lv2Host, const LilvPlugin *lilvPlugin)
{

    LilvWorld *pWorld = lv2Host->getWorld();
    ModGuiUris &modGuiUrids = *(lv2Host->mod_gui_uris);

    AutoLilvNode modGuiUri;
    std::string resourceDirectory;

    AutoLilvNodes modGuiNodes = lilv_plugin_get_value(lilvPlugin,modGuiUrids.mod_gui__gui);

    if (!modGuiNodes) {
        return nullptr; // no mod gui found.
    }
    LILV_FOREACH(nodes, it, modGuiNodes.Get()) {
        AutoLilvNode node  = lilv_nodes_get(modGuiNodes.Get(), it);

        AutoLilvNode resourceDir = lilv_world_get(pWorld, node, modGuiUrids.mod_gui__resourceDirectory,nullptr);
        if (resourceDir) {
            resourceDirectory = lilv_file_uri_parse(lilv_node_as_string(resourceDir),nullptr);
            modGuiUri = lilv_node_duplicate(node);
        } else {
            resourceDirectory.clear();
            modGuiUri.Free();
        }
    }
    if (!modGuiUri) {
        return nullptr; // no mod gui found.
    }


    return std::shared_ptr<ModGui>(new ModGui(lv2Host, lilvPlugin, resourceDirectory, modGuiUri.Get()));
}


ModGuiPort::ModGuiPort(PluginHost *lv2Host, const LilvNode *portNode)
{
    LilvWorld *pWorld = lv2Host->getWorld();
    PluginHost::LilvUris &lilvUris = *lv2Host->lilvUris;

    this->index_ = lilv_node_as_int(lilv_world_get(pWorld, portNode, lilvUris.lv2core__index, nullptr));
    this->symbol_ = NonNull(lilv_node_as_string(lilv_world_get(pWorld, portNode, lilvUris.lv2core__symbol, nullptr)));
    this->name_ = NonNull(lilv_node_as_string(lilv_world_get(pWorld, portNode, lilvUris.lv2core__name, nullptr)));
}   

static json_variant UnitsObj(const std::string &label, const std::string &render, const std::string &symbol)
{
    json_variant obj = json_variant::make_object();
    obj["label"] = label;
    obj["render"] = render;
    obj["symbol"] = symbol;
    return obj;
}


static std::map<Units,json_variant> unitsMap
{
    {Units::none, UnitsObj("","%f","")},
    {Units::unknown, UnitsObj("","%f","")},

    {Units::s, UnitsObj("seconds", "%f s", "s")},
    {Units::ms, UnitsObj("milliseconds", "%f ms", "ms")},
    {Units::min, UnitsObj("minutes", "%f mins", "min")},
    {Units::bar, UnitsObj("bars", "%f bars", "bars")},
    {Units::beat, UnitsObj("beats", "%f beats", "beats")},
    {Units::frame, UnitsObj("audio frames", "%f frames", "frames")},
    {Units::m, UnitsObj("metres", "%f m", "m")},
    {Units::cm, UnitsObj("centimetres", "%f cm", "cm")},
    {Units::mm, UnitsObj("millimetres", "%f mm", "mm")},
    {Units::km, UnitsObj("kilometres", "%f km", "km")},
    {Units::inch, UnitsObj("inches", "\"%f\"", "in")},
    {Units::mile, UnitsObj("miles", "%f mi", "mi")},
    {Units::db, UnitsObj("decibels", "%f dB", "dB")},
    {Units::pc, UnitsObj("percent", "%f%%", "%")},
    {Units::coef, UnitsObj("coefficient", "* %f", "*")},
    {Units::hz, UnitsObj("hertz", "%f Hz", "Hz")},
    {Units::khz, UnitsObj("kilohertz", "%f kHz", "kHz")},
    {Units::mhz, UnitsObj("megahertz", "%f MHz", "MHz")},
    {Units::bpm, UnitsObj("beats per minute", "%f BPM", "BPM")},
    {Units::oct, UnitsObj("octaves", "%f octaves", "oct")},
    {Units::cent, UnitsObj("cents", "%f ct", "ct")},
    {Units::semitone12TET, UnitsObj("semitones", "%f semi", "semi")},
    {Units::degree, UnitsObj("degrees", "%f°", "°")},
    {Units::midiNote, UnitsObj("MIDI note", "MIDI note %d", "note")},
};

static json_variant UnitsToJsonVariant(Units units)
{
    if (unitsMap.find(units) != unitsMap.end())
    {
        return unitsMap[units];
    }
    return unitsMap[Units::none];
}

template<typename T>
static inline json_variant MakeJsonArray(const std::vector<T> &vec)
{
    json_variant array = json_variant::make_array();
    auto &arrayRef = *array.as_array();
    for (const auto &item : vec)
    {
        json_variant itemVariant = json_variant(item);
        arrayRef.push_back(itemVariant);
    }
    return array;
}

static std::string AtomTypeToRangesType(const std::string& atomType)
{
    if (atomType == LV2_ATOM__Path ||
        atomType == LV2_ATOM__String ||
        atomType == LV2_ATOM__URI ||
        atomType == LV2_ATOM__URID)
    {
        return "s";
    }
    if (atomType == LV2_ATOM__Float)
    {
        return "f";
    }
    if (atomType == LV2_ATOM__Double)
    {
        return "d";
    }
    if (atomType == LV2_ATOM__Long)
    {
        return "l";
    }
    if (atomType == LV2_ATOM__Vector)
    {
        return "v";
    }
    return "?";
}
json_variant pipedal::MakeModGuiTemplateData(
    std::shared_ptr<Lv2PluginInfo> pluginInfo)
{
    json_variant context = json_variant::make_object();
    auto &contextObj = *(context.as_object());

    auto modGui = pluginInfo->modGui();

    contextObj["brand"] = modGui->brand();
    contextObj["label"] = modGui->label();
    contextObj["model"] = modGui->model();
    contextObj["panel"] = modGui->panel();
    contextObj["color"] = modGui->color();
    contextObj["knob"] = modGui->knob();

    json_variant controls = json_variant::make_array();
    auto &controlArray = *controls.as_array();
    size_t ix = 0;
    for (const auto& modGuiPort : modGui->ports())
    {
        const Lv2PortInfo &pluginPort = pluginInfo->getPort(modGuiPort.symbol());
        if (!pluginPort.is_control_port() || !pluginPort.is_input() || pluginPort.not_on_gui())
        {
            continue; // only include control ports
        }
        json_variant control = json_variant::make_object();
        auto &portObj = *control.as_object();
        portObj["index"] = double(pluginPort.index());
        portObj["symbol"] = pluginPort.symbol();
        portObj["name"] = modGuiPort.name();
        portObj["comment"] = pluginPort.comment();

        portObj["units"] = UnitsToJsonVariant(pluginPort.units());

        {
            json_variant ranges = json_variant::make_object();
            ranges["minimum"] = SS(pluginPort.min_value());
            ranges["maximum"] = SS(pluginPort.max_value());
            ranges["default"] = SS(pluginPort.default_value());
            portObj["ranges"] = json_variant::make_array();
        }
        json_variant scalePoints = json_variant::make_array();
        auto &scalePointsArray = *scalePoints.as_array();
        for (const auto &scalePoint: pluginPort.scale_points())
        {
            json_variant scalePointObj = json_variant::make_object();
            scalePointObj["valid"] = true;
            scalePointObj["label"] = scalePoint.label();
            scalePointObj["value"] = SS(scalePoint.value());
            scalePointsArray.push_back(scalePointObj);
        }
        portObj["scalePoints"] = scalePoints;

        controlArray.push_back(control);
    }
    contextObj["controls"] = controls;

    json_variant effect = json_variant::make_object();
    contextObj["effect"] = effect;

    json_variant ports = json_variant::make_object();
    (*effect.as_object())["ports"] = ports;

    json_variant audio = json_variant::make_object();
    (*ports.as_object())["audio"] = audio;

    json_variant audio_input = json_variant::make_array();
    (*audio.as_object())["input"] = audio_input;

    json_variant audio_output = json_variant::make_array();
    (*audio.as_object())["output"] = audio_output;
    json_variant midi = json_variant::make_object();
    (*ports.as_object())["midi"] = midi;

    json_variant midi_input = json_variant::make_array();
    (*midi.as_object())["input"] = midi_input;
    json_variant midi_output = json_variant::make_array();
    (*midi.as_object())["output"] = midi_output;

    json_variant cv = json_variant::make_object();
    (*ports.as_object())["cv"] = cv;

    json_variant cv_input = json_variant::make_array();
    (*cv.as_object())["input"] = cv_input;
    json_variant cv_output = json_variant::make_array();
    (*cv.as_object())["output"] = cv_output;

    {
        json_variant parametersObj = json_variant::make_object();
        json_variant pathObj = json_variant::make_array();
        parametersObj["path"] = pathObj;

        for (const auto &patchProperty: pluginInfo->patchProperties())
        {
            json_variant parameterObj = json_variant::make_object();
            parameterObj["valid"] = true;
            parameterObj["readable"] = patchProperty.readable();
            parameterObj["writable"] = patchProperty.writable();
            parameterObj["uri"] = patchProperty.uri();
            parameterObj["label"] = patchProperty.label();
            parameterObj["type"] = patchProperty.type();

            json_variant rangesObj = json_variant::make_object();
            rangesObj["type"] = AtomTypeToRangesType(patchProperty.type());
            rangesObj["atomType"] = patchProperty.type();
            parameterObj["ranges"] = rangesObj;

            parameterObj["comment"] = patchProperty.comment();
            parameterObj["shortName"] = patchProperty.shortName();

            parameterObj["fileTypes"] = MakeJsonArray(patchProperty.fileTypes());
            parameterObj["supportedExtensions"] = MakeJsonArray(patchProperty.supportedExtensions());

            if (patchProperty.type() == LV2_ATOM__Path && patchProperty.writable()) {
                // Feed them no files; they will be added later.
                json_variant filesObj = json_variant::make_array();
                json_variant fileObj = json_variant::make_object();
                fileObj["fileType"] = "{{filetype}}"; // magic tags that we will use to generate files at runtime.
                fileObj["fullname"] = "{{fullname}}";
                fileObj["basename"] = "{{basename}}";
                filesObj.as_array()->push_back(fileObj);
                parameterObj["files"] = filesObj;

                pathObj.as_array()->push_back(parameterObj);
            }

        }

        effect["parameters"] = parametersObj;


    }

    for (const auto &pluginPort : pluginInfo->ports())
    {
        if (pluginPort->is_control_port())
        {
            continue; // only include control ports
        }
        if (pluginPort->is_audio_port())
        {
            json_variant audioPort = json_variant::make_object();
            auto &audioPortObj = *audioPort.as_object();
            audioPortObj["index"] = double(pluginPort->index());
            audioPortObj["symbol"] = pluginPort->symbol();
            audioPortObj["name"] = pluginPort->name();
            if (pluginPort->is_input())
            {
                audio_input.as_array()->push_back(audioPort);
            }
            else
            {
                audio_output.as_array()->push_back(audioPort);
            }
        }
        else if (pluginPort->is_atom_port())
        {
            if (pluginPort->supports_midi())
            {
                json_variant midiPort = json_variant::make_object();
                auto &midiPortObj = *midiPort.as_object();
                midiPortObj["index"] = double(pluginPort->index());
                midiPortObj["symbol"] = pluginPort->symbol();
                midiPortObj["name"] = pluginPort->name();
                if (pluginPort->is_input())
                {
                    midi_input.as_array()->push_back(midiPort);
                }
                else
                {
                    midi_output.as_array()->push_back(midiPort);
                }
            }
        }
        /* else if plugin_port->is_cv_port()... */
    }
    return context;
}


JSON_MAP_BEGIN(ModGuiPort)
    JSON_MAP_REFERENCE(ModGuiPort,index)
    JSON_MAP_REFERENCE(ModGuiPort,symbol)
    JSON_MAP_REFERENCE(ModGuiPort,name)
JSON_MAP_END()

JSON_MAP_BEGIN(ModGui)
    JSON_MAP_REFERENCE(ModGui,pluginUri)
    JSON_MAP_REFERENCE(ModGui,resourceDirectory)
    JSON_MAP_REFERENCE(ModGui,iconTemplate)
    JSON_MAP_REFERENCE(ModGui,settingsTemplate)
    JSON_MAP_REFERENCE(ModGui,screenshot)
    JSON_MAP_REFERENCE(ModGui,javascript)
    JSON_MAP_REFERENCE(ModGui,stylesheet)
    JSON_MAP_REFERENCE(ModGui,thumbnail)
    JSON_MAP_REFERENCE(ModGui,discussionUrl)
    JSON_MAP_REFERENCE(ModGui,documentationUrl)
    JSON_MAP_REFERENCE(ModGui,brand)
    JSON_MAP_REFERENCE(ModGui,label)
    JSON_MAP_REFERENCE(ModGui,model)
    JSON_MAP_REFERENCE(ModGui,panel)
    JSON_MAP_REFERENCE(ModGui,color)
    JSON_MAP_REFERENCE(ModGui,knob)
    JSON_MAP_REFERENCE(ModGui,ports)
    JSON_MAP_REFERENCE(ModGui,monitoredOutputs)
JSON_MAP_END()

