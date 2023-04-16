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

#include "AutoLilvNode.hpp"
#include "PluginHost.hpp"

using namespace pipedal;

float AutoLilvNode::AsFloat(float defaultValue)
{
    if (node == nullptr)
    {
        return defaultValue;
    }
    if (lilv_node_is_float(node))
    {
        return lilv_node_as_float(node);
    }
    if (lilv_node_is_int(node))
    {
        return lilv_node_as_int(node);
    }
    return defaultValue;
}
int32_t AutoLilvNode::AsInt(int defaultValue)
{
    if (node == nullptr)
        return defaultValue;
    if (lilv_node_is_int(node))
        return lilv_node_as_int(node);
    if (lilv_node_is_float(node))
    {
        return (int32_t)lilv_node_as_float(node);
    }

    return defaultValue;
}
bool AutoLilvNode::AsBool(bool defaultValue)
{
    if (node == nullptr)
        return defaultValue;
    if (lilv_node_is_int(node))
        return lilv_node_as_bool(node);
    return defaultValue;
}
std::string AutoLilvNode::AsUri()
{
    if (node == nullptr)
    {
        return "";
    }
    if (lilv_node_is_uri(node))
    {
        return lilv_node_as_uri(node);
    }
    return "";
}
std::string AutoLilvNode::AsString()
{
    if (node == nullptr)
    {
        return "";
    }
    if (lilv_node_is_string(node))
    {
        return lilv_node_as_string(node);
    }
    return "";
}
