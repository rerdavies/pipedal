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

#pragma once

#include "lv2/urid/urid.h"
#include <string>
#include <lilv/lilv.h>
#include <memory>
#include "AutoLilvNode.hpp"
#include "json.hpp"
#include "json_variant.hpp"

namespace pipedal
{

    class MapFeature;
    class Lv2PluginInfo;

    class ModGuiUris
    {
    public:
        ModGuiUris(LilvWorld *pWorld, MapFeature &mapFeature);

        AutoLilvNode mod_gui__gui;
        AutoLilvNode mod_gui__modgui;
        AutoLilvNode mod_gui__resourceDirectory;
        AutoLilvNode mod_gui__iconTemplate;
        AutoLilvNode mod_gui__settingsTemplate;
        AutoLilvNode mod_gui__javascript;
        AutoLilvNode mod_gui__stylesheet;
        AutoLilvNode mod_gui__screenshot;
        AutoLilvNode mod_gui__thumbnail;
        AutoLilvNode mod_gui__discussionURL;
        AutoLilvNode mod_gui__documentation;
        AutoLilvNode mod_gui__brand;
        AutoLilvNode mod_gui__label;
        AutoLilvNode mod_gui__model;
        AutoLilvNode mod_gui__panel;
        AutoLilvNode mod_gui__color;
        AutoLilvNode mod_gui__knob;
        AutoLilvNode mod_gui__port;
        AutoLilvNode mod_gui__monitoredOutputs;
    };

    class PluginHost;

    class ModGuiPort {
    public:
        ModGuiPort() = default;
        ModGuiPort(PluginHost *lv2Host, const LilvNode *portNode);
    private: 
        uint32_t index_ = 0;
        std::string symbol_;
        std::string name_;
    public:
        uint32_t index() const { return index_; }
        void index(uint32_t value) { index_ = value; }
        const std::string &symbol() const { return symbol_; }
        void symbol(const std::string &value) { symbol_ = value; }
        const std::string &name() const { return name_; }
        void name(const std::string &value) { name_ = value; }

        DECLARE_JSON_MAP(ModGuiPort);
    };

    class ModGui
    {
    private:
        ModGui(PluginHost *lv2Host, const LilvPlugin *lilvPlugin, const std::string &resourceDirectory, const LilvNode *modGuiNode);

    public:
        using self = ModGui;
        using ptr = std::shared_ptr<self>;
        static ptr Create(PluginHost *lv2Host, const LilvPlugin *lilvPlugin);

        ModGui() = default;
        virtual ~ModGui() = default;

        ModGui(const ModGui &) = default;
        ModGui &operator=(const ModGui &) = default;
        ModGui(ModGui &&) = default;
        ModGui &operator=(ModGui &&) = default;

    private:
        std::string pluginUri_;
        
        std::string resourceDirectory_;
        std::string iconTemplate_;
        std::string settingsTemplate_;
        std::string javascript_;
        std::string stylesheet_;
        std::string screenshot_;
        std::string thumbnail_;
        std::string discussionUrl_;
        std::string documentationUrl_;
        std::string brand_;
        std::string label_;
        std::string model_;
        std::string panel_;
        std::string color_;
        std::string knob_;
        std::vector<ModGuiPort> ports_;

        std::vector<std::string> monitoredOutputs_;

    public:
        const std::string &pluginUri() const { return pluginUri_; }
        void pluginUri(const std::string &value) { pluginUri_ = value; }

        const std::string &resourceDirectory() const { return resourceDirectory_; }
        void resourceDirectory(const std::string &value) { resourceDirectory_ = value; }
        const std::string &iconTemplate() const { return iconTemplate_; }
        void iconTemplate(const std::string &value) { iconTemplate_ = value; }
        const std::string &settingsTemplate() const { return settingsTemplate_; }
        void settingsTemplate(const std::string &value) { settingsTemplate_ = value; }
        const std::string &screenshot() const { return screenshot_; }
        const std::string &javascript() const { return javascript_; }
        void javascript(const std::string &value) { javascript_ = value; }
        const std::string &stylesheet() const { return stylesheet_; }
        void stylesheet(const std::string &value) { stylesheet_ = value; }
        void screenshot(const std::string &value) { screenshot_ = value; }
        const std::string &thumbnail() const { return thumbnail_; }
        void thumbnail(const std::string &value) { thumbnail_ = value; }
        const std::string &discussionUrl() const { return discussionUrl_; }
        void discussionUrl(const std::string &value) { discussionUrl_ = value; }
        const std::string &documentationUrl() const { return documentationUrl_; }
        void documentationUrl(const std::string &value) { documentationUrl_ = value; }
        const std::string &brand() const { return brand_; }
        void brand(const std::string &value) { brand_ = value; }
        const std::string &label() const { return label_; }
        void label(const std::string &value) { label_ = value; }
        const std::string &model() const { return model_; }
        void model(const std::string &value) { model_ = value; }
        const std::string &panel() const { return panel_; }
        void panel(const std::string &value) { panel_ = value; }

        const std::string &color() const { return color_; }
        void color(const std::string &value) { color_ = value; }
        const std::string &knob() const { return knob_; }
        void knob(const std::string &value) { knob_ = value; }

        const std::vector<ModGuiPort> &ports() const { return ports_; }
        std::vector<ModGuiPort> &ports() { return ports_; }
        void ports(const std::vector<ModGuiPort> &value) { ports_ = value; }


        const std::vector<std::string> &monitoredOutputs() const { return monitoredOutputs_; }
        std::vector<std::string> &monitoredOutputs() { return monitoredOutputs_; }
        void monitoredOutputs(const std::vector<std::string> &value) { monitoredOutputs_ = value; }

        DECLARE_JSON_MAP(ModGui);

    };

    json_variant MakeModGuiTemplateData(
        std::shared_ptr<Lv2PluginInfo> pluginInfo
    );

}