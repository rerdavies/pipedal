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


#define PIPEDAL_UI "http://github.com/rerdavies/pipedal/ui"
#define PIPEDAL_UI_PREFIX PIPEDAL_UI "#"

#define PIPEDAL_UI__ui PIPEDAL_UI_PREFIX "ui"

#define PIPEDAL_UI__fileProperties PIPEDAL_UI_PREFIX "fileProperties"

#define PIPEDAL_UI__fileProperty PIPEDAL_UI_PREFIX "fileProperty"
#define PIPEDAL_UI__defaultFile PIPEDAL_UI_PREFIX "defaultFile"
#define PIPEDAL_UI__patchProperty  PIPEDAL_UI_PREFIX "patchProperty"
#define PIPEDAL_UI__directory  PIPEDAL_UI_PREFIX "directory"
#define PIPEDAL_UI__fileTypes  PIPEDAL_UI_PREFIX "fileTypes"

#define PIPEDAL_UI__fileType  PIPEDAL_UI_PREFIX "fileType"
#define PIPEDAL_UI__fileExtension  PIPEDAL_UI_PREFIX "fileExtension"

#define PIPEDAL_UI__outputPorts  PIPEDAL_UI_PREFIX "outputPorts"
#define PIPEDAL_UI__text  PIPEDAL_UI_PREFIX "text"



namespace pipedal {

    class PiPedalHost;

    class PiPedalFileType {
    private:
        std::string name_;
        std::string fileExtension_;
    public:
        PiPedalFileType() { }
        PiPedalFileType(PiPedalHost*pHost, const LilvNode*node);

        static std::vector<PiPedalFileType> GetArray(PiPedalHost*pHost, const LilvNode*node,const LilvNode*uri);

        const std::string& name() const { return name_;}
        const std::string &fileExtension() const { return fileExtension_; }

    public:
        DECLARE_JSON_MAP(PiPedalFileType);

    };

    class PiPedalFileProperty {
    private:
        std::string name_;
        std::string directory_;
        std::vector<PiPedalFileType> fileTypes_;
        std::string patchProperty_;
        std::string defaultFile_;
    public:
        using ptr = std::shared_ptr<PiPedalFileProperty>;
        PiPedalFileProperty() { }
        PiPedalFileProperty(PiPedalHost*pHost, const LilvNode*node);


        const std::string &name() const { return name_; }
        const std::string &directory() const { return directory_; }
        const std::string &defaultFile() const { return defaultFile_; }

        const std::vector<PiPedalFileType> &fileTypes() const { return fileTypes_; }

        const std::string &patchProperty() const { return patchProperty_; }
        bool IsValidExtension(const std::string&extension) const;
        static  bool IsDirectoryNameValid(const std::string&value);

    public:
        DECLARE_JSON_MAP(PiPedalFileProperty);
    };

    class PiPedalUI {
    public:
        using ptr = std::shared_ptr<PiPedalUI>;
        PiPedalUI(PiPedalHost*pHost, const LilvNode*uiNode);
        const std::vector<PiPedalFileProperty::ptr>& fileProperties() const
        {
            return fileProperites_;
        }
    private:
        std::vector<PiPedalFileProperty::ptr> fileProperites_;
    };

    // Utiltities for validating file paths received via PiPedalFileProperty-related APIs.
    bool IsAlphaNumeric(const std::string&value);
  

};