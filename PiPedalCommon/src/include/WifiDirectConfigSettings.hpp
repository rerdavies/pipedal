/*
 * MIT License
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

#include "json.hpp"

namespace pipedal {

    class WifiDirectConfigSettings {
    public:
        bool valid_ = false;
        bool rebootRequired_ = false;
        bool enable_ = false;
        std::string countryCode_ = ""; // iso 3661
        std::string hotspotName_ = "pipedal";
        bool pinChanged_ = false;
        std::string pin_;
        std::string channel_ = "1"; // "0" -> select automatically.
        std::string wlan_ = "wlan0";

        void ParseArguments(const std::vector<std::string> &arguments);
        void Save() const;
        void Load();

    public:
        DECLARE_JSON_MAP(WifiDirectConfigSettings);
    };
}