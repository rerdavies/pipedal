// Copyright (c) 2025 Robin E. R. Davies
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

#pragma once

#include <vector>
#include <memory>
#include "json.hpp"
#include "PluginType.hpp"
#include <lilv/lilv.h>
#include "MapFeature.hpp"
#include "FileMetadataFeature.hpp"
#include <filesystem>
#include <cmath>
#include <string>
#include "IHost.hpp"
#include <set>
#include "ModGui.hpp"

// #include "lv2.h"
#include "Units.hpp"
#include "PluginPreset.hpp"

#include "IEffect.hpp"
#include "PiPedalConfiguration.hpp"
#include "AutoLilvNode.hpp"
#include "PiPedalUI.hpp"
#include "MapPathFeature.hpp"
#include "ModGui.hpp"
#include "ChannelRouterSettings.hpp"

namespace pipedal
{

    // forward declarations
    class Lv2Effect;
    class Lv2Pedalboard;
    class Lv2PedalboardErrorList;
    class PluginHost;
    class JackConfiguration;
    class JackChannelSelection;
    class ChannelRouterSettings;

#ifndef LV2_PROPERTY_GETSET
#define LV2_PROPERTY_GETSET(name)             \
    const decltype(name##_) &name() const     \
    {                                         \
        return name##_;                       \
    };                                        \
    decltype(name##_) &name()                 \
    {                                         \
        return name##_;                       \
    };                                        \
    void name(const decltype(name##_) &value) \
    {                                         \
        name##_ = value;                      \
    };
#endif

#ifndef LV2_PROPERTY_GETSET_SCALAR
#define LV2_PROPERTY_GETSET_SCALAR(name) \
    decltype(name##_) name() const       \
    {                                    \
        return name##_;                  \
    };                                   \
    void name(decltype(name##_) value)   \
    {                                    \
        name##_ = value;                 \
    };
#endif

    class Lv2PluginClass
    {
    public:
        friend class PluginHost;

    private:
        Lv2PluginClass *parent_ = nullptr; // NOT SERIALIZED!
        std::string parent_uri_;
        std::string display_name_;
        std::string uri_;
        PluginType plugin_type_;
        std::vector<std::shared_ptr<Lv2PluginClass>> children_;

        friend class ::pipedal::PluginHost;
        // hide copy constructor.
        Lv2PluginClass(const Lv2PluginClass &other)
        {
        }
        void set_parent(std::shared_ptr<Lv2PluginClass> &parent)
        {
            this->parent_ = parent.get();
        }
        void add_child(std::shared_ptr<Lv2PluginClass> &child)
        {
            for (size_t i = 0; i < children_.size(); ++i)
            {
                if (children_[i]->uri_ == child->uri_)
                {
                    return;
                }
            }
            children_.push_back(child);
        }

    public:
        Lv2PluginClass(
            const char *display_name, const char *uri, const char *parent_uri)
            : parent_uri_(parent_uri), display_name_(display_name), uri_(uri), plugin_type_(uri_to_plugin_type(uri))
        {
        }
        Lv2PluginClass() {}
        LV2_PROPERTY_GETSET(uri)
        LV2_PROPERTY_GETSET(display_name)
        LV2_PROPERTY_GETSET(parent_uri)

        const Lv2PluginClass *parent() { return parent_; }

        bool operator==(const Lv2PluginClass &other)
        {
            return uri_ == other.uri_;
        }
        bool is_a(const std::string &classUri) const;

        static json_map::storage_type<Lv2PluginClass> jmap;
    };
    class Lv2PluginClasses
    {
    private:
        std::vector<std::string> classes_;

    public:
        Lv2PluginClasses()
        {
        }
        Lv2PluginClasses(std::vector<std::string> classes)
            : classes_(classes)
        {
        }
        const std::vector<std::string> &classes() const
        {
            return classes_;
        }
        bool is_a(PluginHost *lv2Plugins, const char *classUri) const;

        static json_map::storage_type<Lv2PluginClasses> jmap;
    };

    class Lv2ScalePoint
    {
    private:
        float value_;
        std::string label_;

    public:
        Lv2ScalePoint() {}
        Lv2ScalePoint(float value, std::string label)
            : value_(value), label_(label)
        {
        }

        LV2_PROPERTY_GETSET_SCALAR(value);
        LV2_PROPERTY_GETSET(label);

        static json_map::storage_type<Lv2ScalePoint> jmap;
    };

    enum class Lv2BufferType
    {
        None,
        Event,
        Sequence,
        Unknown
    };

    class Lv2PortInfo
    {
    public:
        Lv2PortInfo(PluginHost *lv2Host, const LilvPlugin *pPlugin, const LilvPort *pPort);

    private:
        friend class Lv2PluginInfo;

        uint32_t index_;
        std::string symbol_;
        std::string name_;
        float min_value_, max_value_, default_value_;
        Lv2PluginClasses classes_;
        std::vector<Lv2ScalePoint> scale_points_;
        bool is_input_ = false;
        bool is_output_ = false;

        bool is_control_port_ = false;
        bool is_audio_port_ = false;
        bool is_sidechain_ = false;
        bool is_atom_port_ = false;
        bool is_cv_port_ = false;
        bool connection_optional_ = false;

        bool is_valid_ = false;
        bool supports_midi_ = false;
        bool supports_time_position_ = false;
        bool is_logarithmic_ = false;
        int display_priority_ = -1;
        int range_steps_ = 0;
        bool trigger_property_ = false;
        bool integer_property_ = false;
        bool enumeration_property_ = false;
        bool toggled_property_ = false;

        bool mod_momentaryOffByDefault_ = false;
        bool mod_momentaryOnByDefault_ = false;
        bool pipedal_graphicEq_ = false;

        bool not_on_gui_ = false;
        std::string buffer_type_;
        std::string port_group_;

        std::string designation_;
        bool is_bypass_ = false;
        Units units_ = Units::none;
        std::string custom_units_;
        std::string comment_;
        PiPedalUI::ptr piPedalUI_;

        std::string pipedal_ledColor_;

    public:
        bool IsSwitch() const
        {
            return min_value_ == 0 && max_value_ == 1 && (integer_property_ || toggled_property_ || enumeration_property_);
        }
        const PiPedalUI::ptr &piPedalUI() const { return piPedalUI_; }
        float rangeToValue(float range) const
        {
            float value;
            if (is_logarithmic_)
            {
                value = std::pow(max_value_ / min_value_, range) * min_value_;
            }
            else
            {
                value = (max_value_ - min_value_) * range + min_value_;
            }
            if (integer_property_ || enumeration_property_)
            {
                value = std::round(value);
            }
            else if (range_steps_ >= 2)
            {
                value = std::round(value * (range_steps_ - 1)) / (range_steps_ - 1);
            }
            if (toggled_property_)
            {
                value = value == 0 ? 0 : max_value_;
            }
            if (value > max_value_)
                value = max_value_;
            if (value < min_value_)
                value = min_value_;
            return value;
        }
        LV2_PROPERTY_GETSET(symbol);
        LV2_PROPERTY_GETSET_SCALAR(index);
        LV2_PROPERTY_GETSET(name);
        LV2_PROPERTY_GETSET(classes);
        LV2_PROPERTY_GETSET(scale_points);
        LV2_PROPERTY_GETSET_SCALAR(min_value);
        LV2_PROPERTY_GETSET_SCALAR(max_value);
        LV2_PROPERTY_GETSET_SCALAR(default_value);
        LV2_PROPERTY_GETSET_SCALAR(is_bypass);

        LV2_PROPERTY_GETSET_SCALAR(is_input);
        LV2_PROPERTY_GETSET_SCALAR(is_output);
        LV2_PROPERTY_GETSET_SCALAR(is_control_port);
        LV2_PROPERTY_GETSET_SCALAR(is_audio_port);
        LV2_PROPERTY_GETSET_SCALAR(is_sidechain);
        LV2_PROPERTY_GETSET_SCALAR(is_atom_port);
        LV2_PROPERTY_GETSET_SCALAR(connection_optional);
        LV2_PROPERTY_GETSET_SCALAR(is_cv_port);
        LV2_PROPERTY_GETSET_SCALAR(is_valid);
        LV2_PROPERTY_GETSET_SCALAR(supports_midi);
        LV2_PROPERTY_GETSET_SCALAR(supports_time_position);
        LV2_PROPERTY_GETSET_SCALAR(is_logarithmic);
        LV2_PROPERTY_GETSET_SCALAR(display_priority);
        LV2_PROPERTY_GETSET_SCALAR(range_steps);
        LV2_PROPERTY_GETSET_SCALAR(trigger_property);
        LV2_PROPERTY_GETSET_SCALAR(integer_property);
        LV2_PROPERTY_GETSET_SCALAR(mod_momentaryOffByDefault);
        LV2_PROPERTY_GETSET_SCALAR(mod_momentaryOnByDefault);
        LV2_PROPERTY_GETSET_SCALAR(pipedal_graphicEq);
        LV2_PROPERTY_GETSET_SCALAR(enumeration_property);
        LV2_PROPERTY_GETSET_SCALAR(toggled_property);
        LV2_PROPERTY_GETSET_SCALAR(not_on_gui);
        LV2_PROPERTY_GETSET(port_group);
        LV2_PROPERTY_GETSET(comment);
        LV2_PROPERTY_GETSET_SCALAR(units);
        LV2_PROPERTY_GETSET(custom_units);
        LV2_PROPERTY_GETSET(pipedal_ledColor);

        LV2_PROPERTY_GETSET(buffer_type);

        Lv2BufferType GetBufferType()
        {
            if (buffer_type_ == "")
                return Lv2BufferType::None;
            if (buffer_type_ == LV2_ATOM__Sequence)
                return Lv2BufferType::Sequence;
            return Lv2BufferType::Unknown;
        }

    public:
        Lv2PortInfo() {}
        ~Lv2PortInfo() = default;
        bool is_a(PluginHost *lv2Plugins, const char *classUri);

        static json_map::storage_type<Lv2PortInfo> jmap;
    };

    class Lv2PatchPropertyInfo
    {

    private:
        std::string uri_;
        bool writable_ = false;
        bool readable_ = false;
        std::string label_;
        uint32_t index_ = -1;
        std::string type_;
        std::string comment_;
        std::string shortName_;
        std::vector<std::string> fileTypes_;
        std::vector<std::string> supportedExtensions_;

    public:
        Lv2PatchPropertyInfo() {}
        Lv2PatchPropertyInfo(PluginHost *pluginHost, const LilvNode *propertyUri);

        LV2_PROPERTY_GETSET(uri);
        LV2_PROPERTY_GETSET(writable);
        LV2_PROPERTY_GETSET(readable);
        LV2_PROPERTY_GETSET(label);
        LV2_PROPERTY_GETSET_SCALAR(index);
        LV2_PROPERTY_GETSET(type);
        LV2_PROPERTY_GETSET(comment);
        LV2_PROPERTY_GETSET(shortName);
        LV2_PROPERTY_GETSET(fileTypes);
        LV2_PROPERTY_GETSET(supportedExtensions);

        DECLARE_JSON_MAP(Lv2PatchPropertyInfo);
    };

    class Lv2PortGroup
    {
    private:
        std::vector<std::string> isA_;
        std::string uri_;
        std::string symbol_;
        std::string name_;
        std::string sideChainOf_;

    public:
        LV2_PROPERTY_GETSET(isA);
        LV2_PROPERTY_GETSET(uri);
        LV2_PROPERTY_GETSET(symbol);
        LV2_PROPERTY_GETSET(name);
        LV2_PROPERTY_GETSET(sideChainOf);
        
        Lv2PortGroup() {}
        Lv2PortGroup(PluginHost *lv2Host, const std::string &groupUri);

        static json_map::storage_type<Lv2PortGroup> jmap;
    };

    class Lv2PluginInfo
    {
    private:
        friend class PluginHost;

    public:
        using ptr = std::shared_ptr<Lv2PluginInfo>;
        Lv2PluginInfo(PluginHost *lv2Host, LilvWorld *pWorld, const LilvPlugin *);
        Lv2PluginInfo() {}
        bool isSplit() const;

    private:
        struct FindWritablePathPropertiesResult
        {
            std::shared_ptr<PiPedalUI> pipedalUi;
            bool hasUnsupportedPatchProperties = false;
        };
        FindWritablePathPropertiesResult FindWritablePathProperties(PluginHost *lv2Host, const LilvPlugin *pPlugin);

        bool HasFactoryPresets(PluginHost *lv2Host, const LilvPlugin *plugin);
        std::string bundle_path_;
        std::string uri_;
        uint32_t minorVersion_ = 0;
        uint32_t microVersion_ = 0;
        std::string name_;
        std::string plugin_class_;
        std::string brand_;
        std::string label_;
        std::vector<std::string> supported_features_;
        std::vector<std::string> required_features_;
        std::vector<std::string> optional_features_;
        std::vector<std::string> extensions_;
        bool has_factory_presets_ = false;

        std::string author_name_;
        std::string author_homepage_;

        std::string comment_;
        std::vector<std::shared_ptr<Lv2PortInfo>> ports_;
        std::vector<std::shared_ptr<Lv2PortGroup>> port_groups_;
        std::vector<Lv2PatchPropertyInfo> patchProperties_;
        std::string audio_sidechain_title_;
        bool hasDefaultState_;

        bool is_valid_ = false;
        PiPedalUI::ptr piPedalUI_;
        ModGui::ptr modGui_;

        bool hasUnsupportedPatchProperties_ = false;

        bool IsSupportedFeature(const std::string &feature) const;

        bool powerOf2BlockLength_ = false;
        float minBlockLength_ = -1;
        float maxBlockLength_ = -1;

    public:
        LV2_PROPERTY_GETSET(bundle_path)
        LV2_PROPERTY_GETSET(uri)
        LV2_PROPERTY_GETSET(name)
        LV2_PROPERTY_GETSET(minorVersion)
        LV2_PROPERTY_GETSET(microVersion)
        LV2_PROPERTY_GETSET(brand)
        LV2_PROPERTY_GETSET(label)
        LV2_PROPERTY_GETSET(plugin_class)
        LV2_PROPERTY_GETSET(supported_features)
        LV2_PROPERTY_GETSET(required_features)
        LV2_PROPERTY_GETSET(optional_features)
        LV2_PROPERTY_GETSET(author_name)
        LV2_PROPERTY_GETSET(author_homepage)
        LV2_PROPERTY_GETSET(comment)
        LV2_PROPERTY_GETSET(extensions)
        LV2_PROPERTY_GETSET(ports)
        LV2_PROPERTY_GETSET(is_valid)
        LV2_PROPERTY_GETSET(port_groups)
        LV2_PROPERTY_GETSET(has_factory_presets)
        LV2_PROPERTY_GETSET(piPedalUI)
        LV2_PROPERTY_GETSET(hasUnsupportedPatchProperties)
        LV2_PROPERTY_GETSET(modGui)
        LV2_PROPERTY_GETSET(patchProperties)
        LV2_PROPERTY_GETSET(hasDefaultState)
        LV2_PROPERTY_GETSET(minBlockLength)
        LV2_PROPERTY_GETSET(maxBlockLength)
        LV2_PROPERTY_GETSET(powerOf2BlockLength)
        LV2_PROPERTY_GETSET(audio_sidechain_title)

        bool WantsWorkerThread() const;

        const Lv2PortInfo &getPort(const std::string &symbol)
        {
            for (size_t i = 0; i < ports_.size(); ++i)
            {
                if (ports_[i]->symbol() == symbol)
                {
                    return *(ports_[i].get());
                }
            }
            throw PiPedalArgumentException("Port not found.");
        }
        bool hasExtension(const std::string &uri)
        {
            for (int i = 0; i < extensions_.size(); ++i)
            {
                if (extensions_[i] == uri)
                    return true;
            }
            return false;
        }
        bool hasCvPorts() const
        {
            for (size_t i = 0; i < ports_.size(); ++i)
            {
                if (ports_[i]->is_cv_port())
                {
                    return true;
                }
            }
            return false;
        }
        bool hasMidiInput() const
        {
            for (size_t i = 0; i < ports_.size(); ++i)
            {
                if (ports_[i]->is_atom_port() && ports_[i]->supports_midi() && ports_[i]->is_input())
                {
                    return true;
                }
            }
            return false;
        }
        bool hasMidiOutput() const
        {
            for (size_t i = 0; i < ports_.size(); ++i)
            {
                if (ports_[i]->is_atom_port() && ports_[i]->supports_midi() && ports_[i]->is_output())
                {
                    return true;
                }
            }
        }
        bool IsPathProperty(const std::string &uri) const;
        uint32_t getBypassPortIndex() const;

    public:
        virtual ~Lv2PluginInfo();

        static json_map::storage_type<Lv2PluginInfo> jmap;
    };

    class Lv2PluginUiPortGroup
    {
    private:
        std::vector<std::string> isA_;
        std::string uri_;
        std::string symbol_;
        std::string name_;
        std::string sideChainOf_;

        std::string parent_group_;
        int32_t program_list_id_ = -1; // used by VST3.
    public:
        LV2_PROPERTY_GETSET(uri)

        LV2_PROPERTY_GETSET(symbol)
        LV2_PROPERTY_GETSET(name)
        LV2_PROPERTY_GETSET(parent_group)
        LV2_PROPERTY_GETSET_SCALAR(program_list_id)

    public:
        Lv2PluginUiPortGroup() {}
        Lv2PluginUiPortGroup(Lv2PortGroup *pPortGroup)
            : isA_(pPortGroup->isA()), symbol_(pPortGroup->symbol()), name_(pPortGroup->name()), uri_(pPortGroup->uri()), sideChainOf_(pPortGroup->sideChainOf())
        {
        }
        Lv2PluginUiPortGroup(
            const std::string &symbol, const std::string &name,
            const std::string &parent_group, int32_t programListId)
            : symbol_(symbol), name_(name), parent_group_(parent_group), program_list_id_(programListId)
        {
        }

    public:
        static json_map::storage_type<Lv2PluginUiPortGroup> jmap;
    };

    class Lv2PluginUiPort
    {
    public:
        Lv2PluginUiPort()
        {
        }
        Lv2PluginUiPort(const Lv2PluginInfo *pPlugin, const Lv2PortInfo *pPort)
            : symbol_(pPort->symbol()), index_(pPort->index()),
              is_input_(pPort->is_input()),
              is_sidechain_(pPort->is_sidechain()),
              name_(pPort->name()),
              min_value_(pPort->min_value()),
              max_value_(pPort->max_value()),
              default_value_(pPort->default_value()), range_steps_(pPort->range_steps()), display_priority_(pPort->display_priority()),
              is_logarithmic_(pPort->is_logarithmic()),
              integer_property_(pPort->integer_property()),
              mod_momentaryOffByDefault_(pPort->mod_momentaryOffByDefault()),
              mod_momentaryOnByDefault_(pPort->mod_momentaryOnByDefault()),
              pipedal_graphicEq_(pPort->pipedal_graphicEq()),

              enumeration_property_(pPort->enumeration_property()),
              toggled_property_(pPort->toggled_property()), not_on_gui_(pPort->not_on_gui()), scale_points_(pPort->scale_points()),
              trigger_property_(pPort->trigger_property()),
              pipedal_ledColor_(pPort->pipedal_ledColor()),
              comment_(pPort->comment()), units_(pPort->units()),
              custom_units_(pPort->custom_units()),
              connection_optional_(pPort->connection_optional())
        {
            // Use symbols to index port groups, instead of uris.
            // symbols are guaranteed to be unique.
            const auto &portGroup = pPort->port_group();
            if (portGroup.length() != 0)
            {
                for (int i = 0; i < pPlugin->port_groups().size(); ++i)
                {

                    auto &p = pPlugin->port_groups()[i];
                    if (p->symbol().length() == 0)
                    {
                        // supposed to be mandatory; but make up a synthetic symbol.
                    }
                    if (p->uri() == portGroup)
                    {
                        this->port_group_ = p->symbol();
                        break;
                    }
                }
            }
            is_bypass_ =
                pPort->is_bypass() ||
                name_ == "bypass" || name_ == "Bypass" || symbol_ == "bypass" || symbol_ == "Bypass";
        }

    private:
        std::string symbol_;
        int index_;
        std::string name_;
        bool is_input_ = true;
        bool is_sidechain_ = false;
        float min_value_ = 0, max_value_ = 1, default_value_ = 0;
        bool is_logarithmic_ = false;
        int display_priority_ = -1;

        int range_steps_ = 0;
        bool integer_property_ = false;

        bool mod_momentaryOffByDefault_ = false;
        bool mod_momentaryOnByDefault_ = false;
        bool pipedal_graphicEq_ = false;

        bool enumeration_property_ = false;
        bool not_on_gui_ = false;
        bool toggled_property_ = false;
        bool trigger_property_ = false;
        std::vector<Lv2ScalePoint> scale_points_;
        std::string port_group_;
        std::string pipedal_ledColor_;

        Units units_ = Units::none;
        std::string custom_units_;
        std::string comment_;
        bool is_bypass_ = false;
        bool is_program_controller_ = false;
        bool connection_optional_ = false;

    public:
        LV2_PROPERTY_GETSET(symbol);
        LV2_PROPERTY_GETSET_SCALAR(index);
        LV2_PROPERTY_GETSET_SCALAR(is_input);
        LV2_PROPERTY_GETSET(name);
        LV2_PROPERTY_GETSET(port_group);
        LV2_PROPERTY_GETSET_SCALAR(min_value);
        LV2_PROPERTY_GETSET_SCALAR(max_value);
        LV2_PROPERTY_GETSET_SCALAR(default_value);
        LV2_PROPERTY_GETSET_SCALAR(range_steps);
        LV2_PROPERTY_GETSET_SCALAR(display_priority);
        LV2_PROPERTY_GETSET_SCALAR(is_logarithmic);
        LV2_PROPERTY_GETSET_SCALAR(integer_property);
        LV2_PROPERTY_GETSET_SCALAR(enumeration_property);
        LV2_PROPERTY_GETSET_SCALAR(toggled_property);
        LV2_PROPERTY_GETSET_SCALAR(trigger_property);
        LV2_PROPERTY_GETSET_SCALAR(not_on_gui);
        LV2_PROPERTY_GETSET(scale_points);
        LV2_PROPERTY_GETSET(units);
        LV2_PROPERTY_GETSET(comment);
        LV2_PROPERTY_GETSET_SCALAR(is_bypass);
        LV2_PROPERTY_GETSET_SCALAR(is_program_controller);
        LV2_PROPERTY_GETSET(custom_units);
        LV2_PROPERTY_GETSET(connection_optional);
        LV2_PROPERTY_GETSET(pipedal_ledColor);

    public:
        static json_map::storage_type<Lv2PluginUiPort> jmap;
    };

    class Lv2PluginUiInfo
    {
    public:
        Lv2PluginUiInfo() {}
        Lv2PluginUiInfo(PluginHost *pPlugins, const Lv2PluginInfo *plugin);

    private:
        std::string uri_;
        std::string name_;
        uint32_t minorVersion_ = 0;
        uint32_t microVersion_ = 0;
        ;
        std::string brand_;
        std::string label_;
        std::string author_name_;
        std::string author_homepage_;
        PluginType plugin_type_;
        std::string plugin_display_type_;
        int audio_inputs_ = 0;
        int audio_side_chain_inputs_ = 0;
        int audio_outputs_ = 0;
        std::string audio_side_chain_title_;
        int has_midi_input_ = false;
        int has_midi_output_ = false;
        std::string description_;
        bool is_vst3_ = false;

        int64_t side_chain_input_id = -1;


        std::vector<Lv2PluginUiPort> controls_;
        std::vector<Lv2PluginUiPortGroup> port_groups_;
        std::vector<UiFileProperty::ptr> fileProperties_;
        std::vector<UiFrequencyPlot::ptr> frequencyPlots_;
        std::vector<UiPortNotification::ptr> uiPortNotifications_;
        ModGui::ptr modGui_;
        std::vector<Lv2PatchPropertyInfo> patchProperties_;

    public:
        LV2_PROPERTY_GETSET(uri)
        LV2_PROPERTY_GETSET(name)
        LV2_PROPERTY_GETSET(minorVersion)
        LV2_PROPERTY_GETSET(microVersion)
        LV2_PROPERTY_GETSET(brand)
        LV2_PROPERTY_GETSET(label)
        LV2_PROPERTY_GETSET(author_name)
        LV2_PROPERTY_GETSET(author_homepage)
        LV2_PROPERTY_GETSET_SCALAR(plugin_type)
        LV2_PROPERTY_GETSET(plugin_display_type)
        LV2_PROPERTY_GETSET_SCALAR(audio_inputs)
        LV2_PROPERTY_GETSET_SCALAR(audio_outputs)
        LV2_PROPERTY_GETSET(audio_side_chain_title)
        LV2_PROPERTY_GETSET_SCALAR(has_midi_input)
        LV2_PROPERTY_GETSET_SCALAR(has_midi_output)
        LV2_PROPERTY_GETSET_SCALAR(description)
        LV2_PROPERTY_GETSET(controls)
        LV2_PROPERTY_GETSET(port_groups)
        LV2_PROPERTY_GETSET_SCALAR(is_vst3)
        LV2_PROPERTY_GETSET(fileProperties)
        LV2_PROPERTY_GETSET(frequencyPlots)
        LV2_PROPERTY_GETSET(uiPortNotifications)
        LV2_PROPERTY_GETSET(modGui)
        LV2_PROPERTY_GETSET(patchProperties)
        LV2_PROPERTY_GETSET_SCALAR(audio_side_chain_inputs)

        static json_map::storage_type<Lv2PluginUiInfo> jmap;
    };

}

#if ENABLE_VST3
#include "vst3/Vst3Host.hpp"
#endif

namespace pipedal
{

    class PluginHost : private IHost
    {
    private:
#if ENABLE_VST3
        Vst3Host::Ptr vst3Host;

#endif
        friend class pipedal::AutoLilvNode;
        friend class pipedal::PiPedalUI;
        friend class PluginHostTest;

        static const char *RDFS__comment;
        static const char *RDFS__range;

    public:
        class LilvUris
        {
        public:
            void Initialize(LilvWorld *pWorld);
            void Free();

            AutoLilvNode rdfs__Comment;
            AutoLilvNode rdfs__range;
            AutoLilvNode port_logarithmic;
            AutoLilvNode port__display_priority;
            AutoLilvNode port_range_steps;
            AutoLilvNode integer_property_uri;
            AutoLilvNode enumeration_property_uri;
            AutoLilvNode core__toggled;
            AutoLilvNode core__connectionOptional;
            AutoLilvNode core__isSideChain;
            AutoLilvNode portprops__not_on_gui_property_uri;
            AutoLilvNode portprops__trigger;
            AutoLilvNode midi__event;
            AutoLilvNode core__designation;
            AutoLilvNode portgroups__group;
            AutoLilvNode units__unit;
            AutoLilvNode units__render;
            AutoLilvNode invada_units__unit;            // typo in invada plugins.
            AutoLilvNode invada_portprops__logarithmic; // typo in invada plugins.

            AutoLilvNode atom__bufferType;
            AutoLilvNode atom__Path;
            AutoLilvNode atom__String;
            AutoLilvNode presets__preset;
            AutoLilvNode state__state;
            AutoLilvNode rdfs__label;
            AutoLilvNode lv2core__symbol;
            AutoLilvNode lv2core__name;
            AutoLilvNode lv2core__shortName;
            AutoLilvNode lv2core__index;
            AutoLilvNode lv2core__Parameter;
            AutoLilvNode lv2core__minorVersion;
            AutoLilvNode lv2core__microVersion;
            AutoLilvNode pipedalUI__ui;
            AutoLilvNode pipedalUI__fileProperties;
            AutoLilvNode pipedalUI__directory;
            AutoLilvNode pipedalUI__patchProperty;

            AutoLilvNode pipedalUI__fileProperty;
            AutoLilvNode pipedalUI__resourceDirectory;
            AutoLilvNode pipedalUI__fileTypes;
            AutoLilvNode pipedalUI__fileExtension;
            AutoLilvNode pipedalUI__mimeType;

            AutoLilvNode pipedalUI__frequencyPlot;
            AutoLilvNode pipedalUI__xLeft;
            AutoLilvNode pipedalUI__xRight;
            AutoLilvNode pipedalUI__xLog;
            AutoLilvNode pipedalUI__yDb;
            AutoLilvNode pipedalUI__yTop;
            AutoLilvNode pipedalUI__yBottom;
            AutoLilvNode pipedalUI__width;
            AutoLilvNode pipedalUI__graphicEq;

            AutoLilvNode pipedalUI__outputPorts;
            AutoLilvNode pipedalUI__text;

            AutoLilvNode pipedalUI__ledColor;

            AutoLilvNode time_Position;
            AutoLilvNode time_barBeat;
            AutoLilvNode time_beatsPerMinute;
            AutoLilvNode time_speed;

            AutoLilvNode appliesTo;
            AutoLilvNode isA;

            AutoLilvNode ui__portNotification;
            AutoLilvNode ui__plugin;
            AutoLilvNode ui__ledColor;
            AutoLilvNode ui__protocol;
            AutoLilvNode ui__floatProtocol;
            AutoLilvNode ui__peakProtocol;
            AutoLilvNode ui__portIndex;
            AutoLilvNode lv2__symbol;
            AutoLilvNode lv2__port;

            AutoLilvNode ui__FrequencyPlot;

            AutoLilvNode patch__writable;
            AutoLilvNode patch__readable;
            AutoLilvNode pipedal_patch__readable;

            AutoLilvNode mod__brand;
            AutoLilvNode mod__label;
            AutoLilvNode mod__preferMomentaryOffByDefault;
            AutoLilvNode mod__preferMomentaryOnByDefault;
            AutoLilvNode dc__format;

            AutoLilvNode mod__fileTypes;
            AutoLilvNode mod__supportedExtensions;
            AutoLilvNode pipedalui__fileTypes;

            AutoLilvNode buf_size__minBlockLength;
            AutoLilvNode buf_size__maxBlockLength;
            AutoLilvNode buf_size__fixedBlockLength;
            AutoLilvNode buf_size__coarseBlockLength;
            AutoLilvNode port_groups__sideChainOf;
        };
        LilvUris *lilvUris = nullptr;

    private:
        bool vst3Enabled = true;

        LilvNode *get_comment(const std::string &uri);

        size_t maxBufferSize = 1024;
        size_t maxAtomBufferSize = 16 * 1024;
        bool hasMidiInputChannel;
        ChannelSelection channelSelection;

        double sampleRate = 48000;

        std::string vst3CachePath;

        std::vector<const LV2_Feature *> lv2Features;
        MapFeature mapFeature;
        FileMetadataFeature fileMetadataFeature;
        std::string pluginStoragePath;

        static void fn_LilvSetPortValueFunc(const char *port_symbol,
                                            void *user_data,
                                            const void *value,
                                            uint32_t size,
                                            uint32_t type);

        LilvWorld *pWorld;
        void free_world();

        std::vector<std::shared_ptr<Lv2PluginInfo>> plugins_;
        std::map<std::string, std::shared_ptr<Lv2PluginInfo>> pluginsByUri;
        std::vector<Lv2PluginUiInfo> ui_plugins_;

        std::map<std::string, std::shared_ptr<Lv2PluginClass>> classesMap;

        friend class Lv2PluginInfo;
        friend class Lv2PortInfo;
        friend class Lv2PortGroup;
        friend class ModGui;

        std::shared_ptr<Lv2PluginClass> GetPluginClass(const LilvPluginClass *pClass);
        std::shared_ptr<Lv2PluginClass> MakePluginClass(const LilvPluginClass *pClass);
        Lv2PluginClasses GetPluginPortClass(const LilvPlugin *lilvPlugin, const LilvPort *lilvPort);

        bool classesLoaded = false;
        // IHost implementation

    public:
        void LogError(const std::string &message)
        {
            Lv2Log::error(message);
        }
        void LogWarning(const std::string &message)
        {
            Lv2Log::warning(message);
        }
        void LogNote(const std::string &message)
        {
            Lv2Log::info(message);
        }
        void LogTrace(const std::string &message)
        {
            Lv2Log::debug(message);
        }
        virtual LilvWorld *getWorld()
        {
            return pWorld;
        }

    private:
        // IHost implementation.
        virtual void SetMaxAudioBufferSize(size_t size) override { maxBufferSize = size; }
        virtual size_t GetMaxAudioBufferSize() const override { return maxBufferSize; }
        virtual size_t GetAtomBufferSize() const override { return maxAtomBufferSize; }
        virtual bool HasMidiInputChannel() const override  { return hasMidiInputChannel; }
        virtual LV2_Feature *const *GetLv2Features() const override { return (LV2_Feature *const *)&(this->lv2Features[0]); }
        virtual const ChannelSelection &GetChannelSelection() const override { return this->channelSelection; }

    public:
        virtual MapFeature &GetMapFeature() override { return this->mapFeature; }
        void CheckForResourceInitialization(const std::string &pluginUri, const std::filesystem::path &pluginUploadDirectory);

        std::string MapResourcePath(const std::string &uri, const std::string &relativePath);

        void ReloadPlugins();

        // equivalent to LV2 MapPath AbstractPath features.
        std::string MapPath(const std::string &abstractPath);
        std::string AbstractPath(const std::string &path);
        json_variant MapPath(const json_variant &json);
        json_variant AbstractPath(const json_variant &json);

    private:
        std::set<std::string> pluginsThatHaveBeenCheckedForResources;

        void CheckForResourceInitization(const std::string pluginUri);

        virtual LV2_URID_Map *GetLv2UridMap()
        {
            return this->mapFeature.GetMap();
        }
        static void PortValueCallback(const char *symbol, void *user_data, const void *value, uint32_t size, uint32_t type);
        static void StateRestoreCallback(
            const char *port_symbol,
            void *user_data,
            const void *value,
            uint32_t size,
            uint32_t type);

        virtual IEffect *CreateEffect(PedalboardItem &pedalboardItem);
        void LoadPluginClassesFromLilv();
        void AddJsonClassesToMap(std::shared_ptr<Lv2PluginClass> pluginClass);

    public:
        PluginHost();
        void SetPluginStoragePath(const std::filesystem::path &path);
        virtual std::string GetPluginStoragePath() const;

        void SetConfiguration(const PiPedalConfiguration &configuration);

        virtual ~PluginHost();

        IHost *asIHost() { return this; }

        virtual Lv2Pedalboard *CreateLv2Pedalboard(Pedalboard &pedalboard, Lv2PedalboardErrorList &errorList);

        virtual Lv2Pedalboard *UpdateLv2PedalboardStructure(Pedalboard &pedalboard, Lv2Pedalboard *existingPedalboard, Lv2PedalboardErrorList &errorList);

        void setSampleRate(double sampleRate)
        {
            this->sampleRate = sampleRate;
        }
        double GetSampleRate() const
        {
            return sampleRate;
        }

        class Urids;

        Urids *urids = nullptr;
        ModGuiUris *mod_gui_uris = nullptr;

        void OnConfigurationChanged(const JackConfiguration &configuration, const ChannelSelection &channelSelection);


        std::shared_ptr<Lv2PluginClass> GetPluginClass(const std::string &uri) const;
        bool is_a(const std::string &class_, const std::string &target_class);

        std::shared_ptr<Lv2PluginClass> GetLv2PluginClass() const;

        const std::vector<std::shared_ptr<Lv2PluginInfo>> &GetPlugins() const { return plugins_; }
        const std::vector<Lv2PluginUiInfo> &GetUiPlugins() const { return ui_plugins_; }

        virtual std::shared_ptr<Lv2PluginInfo> GetPluginInfo(const std::string &uri) const;

        static constexpr const char *DEFAULT_LV2_PATH = "/usr/lib/lv2";

        void LoadPluginClassesFromJson(std::filesystem::path jsonFile);
        void LoadLilv(const char *lv2Path = PluginHost::DEFAULT_LV2_PATH);

        virtual LV2_URID GetLv2Urid(const char *uri)
        {
            return this->mapFeature.GetUrid(uri);
        }
        virtual std::string Lv2UridToString(LV2_URID urid)
        {
            return this->mapFeature.UridToString(urid);
        }

        PluginPresets GetFactoryPluginPresets(const std::string &pluginUri);
        std::vector<ControlValue> LoadFactoryPluginPreset(PedalboardItem *pedalboardItem,
                                                          const std::string &presetUri);
    };

#undef LV2_PROPERTY_GETSET
#undef LV2_PROPERTY_GETSET_SCALAR
} // namespace pipedal.