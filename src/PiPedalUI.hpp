/*
 * MIT License
 * 
 * Copyright (c) 2023 Robin E. R. Davies
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

#pragma once

#include <string>
#include <memory>
#include <lilv/lilv.h>
#include "json.hpp"
#include <filesystem>
#include <set>
#include "ModFileTypes.hpp"


#define PIPEDAL_PATCH "http://github.com/rerdavies/pipedal/patch"
#define PIPEDAL_PATCH_PREFIX PIPEDAL_PATCH "#"
#define PIPEDAL_PATCH__readable (PIPEDAL_PATCH_PREFIX "readable")

#define PIPEDAL_UI "http://github.com/rerdavies/pipedal/ui"
#define PIPEDAL_UI_PREFIX PIPEDAL_UI "#"

#define PIPEDAL_UI__ui PIPEDAL_UI_PREFIX "ui"

#define PIPEDAL_UI__fileProperties PIPEDAL_UI_PREFIX "fileProperties"

#define PIPEDAL_UI__fileProperty PIPEDAL_UI_PREFIX "fileProperty"
#define PIPEDAL_UI__patchProperty  PIPEDAL_UI_PREFIX "patchProperty"
#define PIPEDAL_UI__directory  PIPEDAL_UI_PREFIX "directory"
#define PIPEDAL_UI__fileTypes  PIPEDAL_UI_PREFIX "fileTypes"
#define PIPEDAL_UI__resourceDirectory PIPEDAL_UI_PREFIX "resourceDirectory"

#define PIPEDAL_UI__fileType  PIPEDAL_UI_PREFIX "fileType"
#define PIPEDAL_UI__fileExtension  PIPEDAL_UI_PREFIX "fileExtension"
#define PIPEDAL_UI__mimeType  PIPEDAL_UI_PREFIX "mimeType"

#define PIPEDAL_UI__outputPorts  PIPEDAL_UI_PREFIX "outputPorts"
#define PIPEDAL_UI__text  PIPEDAL_UI_PREFIX "text"

#define PIPEDAL_UI__frequencyPlot PIPEDAL_UI_PREFIX "frequencyPlot"
#define PIPEDAL_UI__xLeft PIPEDAL_UI_PREFIX "xLeft"
#define PIPEDAL_UI__xRight PIPEDAL_UI_PREFIX "xRight"
#define PIPEDAL_UI__xLog PIPEDAL_UI_PREFIX "xLog"
#define PIPEDAL_UI__yTop PIPEDAL_UI_PREFIX "yTop"
#define PIPEDAL_UI__yBottom PIPEDAL_UI_PREFIX "yBottom"
#define PIPEDAL_UI__width PIPEDAL_UI_PREFIX "width"

#define PIPEDAL_UI__ledColor  PIPEDAL_UI_PREFIX "ledColor"


namespace pipedal {

    class PluginHost;


    class UiFileType {
    private:
        std::string label_;
        std::string mimeType_;
        std::string fileExtension_;
    public:
        UiFileType() { }
        UiFileType(PluginHost*pHost, const LilvNode*node);
        UiFileType(const std::string&label, const std::string &fileType);


        static std::vector<UiFileType> GetArray(PluginHost*pHost, const LilvNode*node,const LilvNode*uri);

        const std::string& label() const { return label_;}
        const std::string &fileExtension() const { return fileExtension_; }
        const std::string &mimeType() const { return mimeType_; }
        bool IsValidExtension(const std::string&extension) const;
    public:
        DECLARE_JSON_MAP(UiFileType);

    };



    class UiPortNotification {
    private:
        int32_t portIndex_;
        std::string symbol_;
        std::string plugin_;
        std::string protocol_;
    public:
        using ptr = std::shared_ptr<UiPortNotification>;

        UiPortNotification() { }
        UiPortNotification(PluginHost*pHost, const LilvNode*node);
    
    public:
        DECLARE_JSON_MAP(UiPortNotification);

    };
    class UiFileProperty {
    private:
        std::string label_;
        std::int32_t index_ = -1;
        std::string directory_;
        std::vector<UiFileType> fileTypes_;
        std::string patchProperty_;
        std::string portGroup_;
        std::string resourceDirectory_;
        std::vector<std::string> modDirectories_;
        bool useLegacyModDirectory_= false;
        std::map<std::string,std::set<std::string>> fileExtensionsByModDirectory; // non-serialized.
        void PrecalculateFileExtensions();
    public:
        using ptr = std::shared_ptr<UiFileProperty>;
        UiFileProperty() { }
        UiFileProperty(PluginHost*pHost, const LilvNode*node, const std::filesystem::path&resourcePath);
        UiFileProperty(const std::string&label, const std::string&patchProperty,const std::string &directory);
        UiFileProperty(
            const std::string&label, 
            const std::string&patchProperty, 
            const ModFileTypes&modFileType);

        void setModFileTypes(const ModFileTypes&modFileType);

        std::vector<std::string>& modDirectories() { return modDirectories_; }
        const std::vector<std::string>& modDirectories() const { return modDirectories_; }
        
        bool useLegacyModDirectory() const { return useLegacyModDirectory_; }
        void useLegacyModDirectory(bool value) { useLegacyModDirectory_ = value; }
        const std::string &label() const { return label_; }

        int32_t index() const { return index_; }
        void index(int32_t value) { index_ = value; }
        
        const std::string &directory() const { return directory_; }
        void directory(const std::string &path) { directory_ = path; }

        const std::string&portGroup() const { return portGroup_; }

        const std::vector<UiFileType> &fileTypes() const { return fileTypes_; }
        std::vector<UiFileType> &fileTypes() { return fileTypes_; }

        const std::string &patchProperty() const { return patchProperty_; }

        bool IsValidExtension(const std::filesystem::path&relativePath) const;

        static  bool IsDirectoryNameValid(const std::string&value);

        static std::string GetFileExtension(const std::filesystem::path&path);
        
        const std::string&resourceDirectory() const { return resourceDirectory_; }


        const std::set<std::string>& GetPermittedFileExtensions(const std::string &modDirectory) const;

        std::string getParentModDirectory(const std::filesystem::path&path) const;
    public:
        DECLARE_JSON_MAP(UiFileProperty);
    };
    class UiFrequencyPlot {
    private:
        std::string patchProperty_;
        std::int32_t index_ = -1;
        std::string portGroup_;
        float xLeft_ = 100;
        float xRight_ = 22000;
        float yTop_ = 5;
        float yBottom_ = -30;
        bool xLog_ = true;
        float width_ = 60;
    public:
        using ptr = std::shared_ptr<UiFrequencyPlot>;
        UiFrequencyPlot() { }
        UiFrequencyPlot(PluginHost*pHost, const LilvNode*node,
          const std::filesystem::path&resourcePath);

        const std::string &patchProperty() const { return patchProperty_; }
        int32_t index() const { return index_; }
        const std::string&portGroup() const { return portGroup_; }
        float xLeft() const { return xLeft_; }
        float xRight() const { return xRight_; }
        bool xLog() const { return xLog_; }
        float yTop() const { return yTop_; }
        float yBottom() const { return yBottom_; }
        float width() const { return width_; }

    public:
        DECLARE_JSON_MAP(UiFrequencyPlot);
    };

    class PiPedalUI {
    public:
        using ptr = std::shared_ptr<PiPedalUI>;
        PiPedalUI(PluginHost*pHost, const LilvNode*uiNode, const std::filesystem::path&resourcePath);
        PiPedalUI(
            std::vector<UiFileProperty::ptr> &&fileProperties,
            std::vector<UiFrequencyPlot::ptr> &&frequencyPlots);
        PiPedalUI(
            std::vector<UiFileProperty::ptr> &&fileProperties);

        const std::vector<UiFileProperty::ptr>& fileProperties() const
        {
            return fileProperties_;
        }
        const std::vector<UiFrequencyPlot::ptr>& frequencyPlots() const
        {
            return frequencyPlots_;
        }



        const std::vector<UiPortNotification::ptr> &portNotifications() const { return portNotifications_; }

        const UiFileProperty*GetFileProperty(const std::string &propertyUri) const
        {
            for (const auto&fileProperty : fileProperties())
            {
                if (fileProperty->patchProperty() == propertyUri)
                {
                    return fileProperty.get();
                }
            }
            return nullptr;
        }
        bool unsupportedPatchProperty() const { return unsupportedPatchProperty_; }
        void  unsupportedPatchProperty(bool value) { unsupportedPatchProperty_ = value; }
    private:
        bool unsupportedPatchProperty_ = false; // not serialized.
        std::vector<UiFileProperty::ptr> fileProperties_;
        std::vector<UiFrequencyPlot::ptr> frequencyPlots_;
        std::vector<UiPortNotification::ptr> portNotifications_;
    };

    // utilities for validating file paths received via PiPedalFileProperty-related APIs.
    bool IsAlphaNumeric(const std::string&value);
  

};