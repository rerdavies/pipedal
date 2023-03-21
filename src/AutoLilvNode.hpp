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

#include <lilv/lilv.h>
#include <cassert>

namespace pipedal
{
    class AutoLilvNode
    {

        // const LilvNode* returns must not be freed, by convention.
    private:
        LilvNode *node = nullptr;
        AutoLilvNode(const LilvNode *node) = delete;

    public:
        AutoLilvNode()
        {
        }
        AutoLilvNode(LilvNode *node)
            : node(node)
        {
        }


        ~AutoLilvNode() { Free(); }


        operator const LilvNode *()
        {
            return this->node;
        }
        operator bool()
        {
            return this->node != nullptr;
        }
        LilvNode*&Get() { return node; }

        AutoLilvNode&operator=(LilvNode*node) { 
            Free(); 
            this->node = node; 
            return *this;
        }
        AutoLilvNode&operator=(AutoLilvNode&&other) { 
            std::swap(this->node,other.node);
            return *this;
        }


        float AsFloat(float defaultValue = 0);
        int AsInt(int defaultValue = 0);
        bool AsBool(bool defaultValue = false);
        std::string AsUri();
        std::string AsString();

        void Free()
        {
            if (node != nullptr)
                lilv_node_free(node);
            node = nullptr;
        }


    };
};