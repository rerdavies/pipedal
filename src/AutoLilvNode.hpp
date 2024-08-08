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

    public:
        AutoLilvNode(const AutoLilvNode&) = delete;
        AutoLilvNode&operator=(const AutoLilvNode&) = delete;
        AutoLilvNode(AutoLilvNode&&) = default;

        AutoLilvNode()
        {
        }
        AutoLilvNode(const LilvNode *node)
            : node(const_cast<LilvNode *>(node))
        {
            isConst = true;
        }
        AutoLilvNode(LilvNode *node)
            : node(node)
        {
            isConst = false;
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
        const LilvNode *Get() { return node; }

        AutoLilvNode &operator=(LilvNode *node)
        {
            Free();
            this->node = node;
            this->isConst = false;
            return *this;
        }
        AutoLilvNode &operator=(const LilvNode *node)
        {
            Free();
            this->node = const_cast<LilvNode*>(node);
            this->isConst = true;
            return *this;
        }

        operator LilvNode **()
        {
            Free();
            this->isConst = false;
            return &node;
        }
        operator const LilvNode **()
        {
            Free();
            this->isConst = true;
            return (const LilvNode **)&node;
        }
        AutoLilvNode &operator=(AutoLilvNode &&other)
        {
            std::swap(this->node, other.node);
            std::swap(this->isConst,other.isConst);
            return *this;
        }

        float AsFloat(float defaultValue = 0);
        int32_t AsInt(int defaultValue = 0);
        bool AsBool(bool defaultValue = false);
        std::string AsUri();
        std::string AsString();

        void Free()
        {
            if (node != nullptr && !isConst)
            {
                try {
                    lilv_node_free(node);
                } catch (const std::exception&)
                {

                }
            }
            node = nullptr;
        }

    private:
        LilvNode *node = nullptr;
        bool isConst = true;
    };

    class AutoLilvNodes
    {

    public:
        AutoLilvNodes()
        {
        }
        AutoLilvNodes(const LilvNodes *nodes)
            : nodes(const_cast<LilvNodes *>(nodes))
        {
            isConst = true;
        }
        AutoLilvNodes(LilvNodes *nodes)
            : nodes(nodes)
        {
            isConst = false;
        }

        ~AutoLilvNodes() { Free(); }

        operator const LilvNodes *()
        {
            return this->nodes;
        }
        operator bool()
        {
            return this->nodes != nullptr;
        }
        const LilvNodes *Get() { return nodes; }

        AutoLilvNodes &operator=(LilvNodes *node)
        {
            Free();
            this->nodes = nodes;
            return *this;
        }

        operator LilvNodes **()
        {
            Free();
            this->isConst = false;
            return &nodes;
        }
        operator const LilvNodes **()
        {
            Free();
            this->isConst = true;
            return (const LilvNodes **)&nodes;
        }
        AutoLilvNodes &operator=(AutoLilvNodes &&other)
        {
            std::swap(this->nodes, other.nodes);
            return *this;
        }

        void Free()
        {
            if (nodes != nullptr && !isConst)
                lilv_nodes_free(nodes);
            nodes = nullptr;
        }

    private:
        LilvNodes *nodes = nullptr;
        bool isConst = true;
    };
}
