/*
 /* MIT License
 * 
 * Copyright (c) 2022 Robin E. R. Davies
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
#include <vector> 
#include <stdint.h>
#include <string>
#include <exception>
#include <stdexcept>

#include "public.sdk/source/vst/utility/uid.h"

#include "vst3/Vst3Effect.hpp"
#include "PedalBoard.hpp"


 namespace pipedal {


    class Vst3Exception: public std::logic_error {
    public:
        Vst3Exception(const std::string&error)
        :logic_error(error)
        {
        }
    };

    class Vst3Host {
    
    public:
        using Ptr = std::unique_ptr<Vst3Host>;
		using PluginList = std::vector<std::unique_ptr<Vst3PluginInfo>>;

        virtual ~Vst3Host() {}
        virtual const PluginList &RescanPlugins() = 0;
        virtual const PluginList &RefreshPlugins() = 0;
        virtual const PluginList &getPluginList() = 0;


        virtual std::unique_ptr<Vst3Effect> CreatePlugin(long instanceId,const std::string&url, IHost*pHost) = 0;
        virtual std::unique_ptr<Vst3Effect> CreatePlugin(PedalBoardItem&pedalBoardItem, IHost*pHost) = 0;


        static Ptr CreateInstance(const std::string &cacheFilePath = "");

    };


 };