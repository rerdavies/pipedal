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

#include "ModTemplateGenerator.hpp"
#include <fstream>
#include <sstream>
#include "util.hpp"

namespace fs = std::filesystem;

namespace pipedal
{
    namespace impl
    {

        static std::string trim(const std::string &str)
        {
            size_t first = str.find_first_not_of(" \t\n\r");
            if (first == std::string::npos)
                return "";
            size_t last = str.find_last_not_of(" \t\n\r");
            return str.substr(first, (last - first + 1));
        }

        static bool variableEndsWithNumber(const std::string &variableName)
        {
            size_t pos = variableName.find_last_of(".");
            if (pos == std::string::npos)
            {
                return false;
            }
            for (size_t i = pos + 1; i < variableName.length(); ++i)
            {
                if (!std::isdigit(variableName[i]))
                {
                    return false;
                }
            }
            // Check if the last character is a digit
            return true;
        }
        void splitIndexedArrayVariable(
            const std::string &variableName,
            std::string &arrayName,
            int64_t &arrayIndex)
        {
            size_t pos = variableName.find_last_of(".");
            if (pos == std::string::npos)
            {
                throw std::logic_error("Variable name does not contain an index: " + variableName);
            }
            else
            {
                arrayName = variableName.substr(0, pos);
                arrayIndex = std::stoll(variableName.substr(pos + 1));
            }
        }

        class VariableContext
        {
        public:
            VariableContext(
                const json_variant &context,
                VariableContext *parent = nullptr)
                : context(context), parent(parent)
            {
            }
            json_variant getVariable(const std::string &name)
            {
                std::vector<std::string> parts = split(trim(name), '.');
                json_variant *current = &context;
                for (const auto &part : parts)
                {
                    if (current->is_object())
                    {
                        auto ff = current->as_object()->find(part);
                        if (ff != current->as_object()->end())
                        {
                            current = &ff->second;
                            continue;
                        }
                    }
                    if (parent)
                    {
                        return parent->getVariable(name);
                    }
                    else
                    {
                        return json_variant(json_null());
                    }
                }
                return *current;
            }

        private:
            json_variant context;
            VariableContext *parent;
        };

        static std::string GenerateTemplateFromString(
            const std::string &content,
            VariableContext &context)
        {

            std::stringstream ss;

            size_t ix = 0;
            size_t end = content.length();
            while (ix < end)
            {
                // copy content until we find a '{{'
                char c = content[ix++];
                if (c != '{')
                {
                    ss << c;
                    continue;
                }
                if (ix >= end)
                {
                    break;
                }
                c = content[ix++];
                if (c != '{')
                {
                    ss << '{';
                    ss << c;
                    continue;
                }
                if (ix < end && content[ix] == '{')
                {
                    // this is a '{{{', so we just copy it.
                    ++ix; // skip the third '{'
                    size_t endTagPos = content.find("}}}", ix);
                    if (endTagPos == std::string::npos)
                    {
                        throw std::runtime_error("Unmatched opening tag '{{{' in template.");
                    }
                    std::string variableString = content.substr(ix, endTagPos - ix);
                    ix = endTagPos + 3; // Move past the closing '}}}'
                    auto result = context.getVariable("_" + variableString);
                    if (result.is_null())
                    {
                        result = ""; // Default to empty string if variable not found
                    }
                    if (!result.is_string())
                    {
                        throw std::runtime_error("Variable '" + variableString + "' is not a string.");
                    }
                    ss << result.as_string();
                    continue;
                }
                else
                {
                    // Now we have a '{{' at position ix-2.
                    size_t endTagPos = content.find("}}", ix);
                    if (endTagPos == std::string::npos)
                    {
                        throw std::runtime_error("Unmatched opening tag '{{' in template.");
                    }
                    std::string variableString = content.substr(ix, endTagPos - ix);
                    ix = endTagPos + 2; // Move past the closing '}}'
                    if (!variableString.starts_with("#"))
                    {
                        // a straightforward variable substitution.
                        json_variant value = context.getVariable(variableString);
                        if (value.is_null())
                        {
                            value = ""; // Default to empty string if variable not found
                        }
                        if (!value.is_string())
                        {
                            throw std::runtime_error("Variable '" + variableString + "' is not a string.");
                        }
                        ss << value.as_string();
                    }
                    else
                    {
                        // this is a looping construct.
                        std::string variableName = variableString.substr(1);
                        std::string endTag = SS("{{/" << variableName << "}}");
                        size_t endTagPos = content.find(endTag, ix);
                        if (endTagPos == std::string::npos)
                        {
                            throw std::runtime_error("Unmatched opening tag for  '" + variableString + "' in template.");
                        }

                        std::string arrayContent = content.substr(ix, endTagPos - ix);

                        if (variableEndsWithNumber(variableName))
                        {
                            std::string arrayName;
                            int64_t arrayIndex = 0;
                            splitIndexedArrayVariable(variableName, arrayName, arrayIndex);

                            json_variant array = context.getVariable(arrayName);
                            auto &arrayValue = *array.as_array();
                            if (arrayIndex >= 0 && arrayIndex < static_cast<int64_t>(arrayValue.size()))
                            {
                                json_variant contextValue = arrayValue[(size_t)arrayIndex];
                                // variable frame.
                                VariableContext itemContext{contextValue, &context};

                                ss << GenerateTemplateFromString(
                                    arrayContent,
                                    itemContext);
                            }
                        }
                        else
                        {
                            json_variant variable = context.getVariable(variableName);
                            if (variable.is_array())
                            {
                                auto &arrayValue = *variable.as_array();

                                for (auto iter = arrayValue.begin(); iter != arrayValue.end(); ++iter)
                                {
                                    // variable frame.
                                    VariableContext itemContext{*iter, &context};

                                    ss << GenerateTemplateFromString(
                                        arrayContent,
                                        itemContext);
                                }
                            } else if (variable.is_object()) 
                            {
                                VariableContext itemContext(variable,&context);
                                ss << GenerateTemplateFromString(
                                    arrayContent,
                                    itemContext);
                            }
                        }

                        ix = endTagPos + endTag.length(); // Move past the closing tag
                    }
                }
            }
            return ss.str();
        }
    }
    using namespace impl;

    std::string GenerateFromTemplateString(
        const std::string &templateString,
        const json_variant &data)
    {
        VariableContext context{data, nullptr};
        return impl::GenerateTemplateFromString(
            templateString,
            context);
    }

    std::string GenerateFromTemplateFile(
        const std::filesystem::path &templateFilePath,
        const json_variant &data)
    {
        std::string templateContent;
        if (!fs::exists(templateFilePath))
        {
            throw std::runtime_error("Template file not found: " + templateFilePath.string());
        }
        std::ifstream file(templateFilePath);
        if (!file.is_open())
        {
            throw std::runtime_error("Failed to open template file: " + templateFilePath.string());
        }
        templateContent.assign((std::istreambuf_iterator<char>(file)),
                               std::istreambuf_iterator<char>());
        file.close();

        return GenerateFromTemplateString(
            templateContent,
            data);
    }

}