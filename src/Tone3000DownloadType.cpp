/*
 *   Copyright (c) Robin E.R. Davies
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

 #include "Tone3000DownloadType.hpp"
 #include <stdexcept>
 #include <unordered_map>

namespace pipedal {

    // Must agree with Tone3000DownloadType.tsx

    static std::unordered_map<std::string,Tone3000DownloadType> downloadTypeEnums =
    {
        {"nam", Tone3000DownloadType::Nam},
        {"cabir", Tone3000DownloadType::CabIr},
    };
    Tone3000DownloadType StringToTone3000DownloadType(const std::string &value)
    {
        auto ff = downloadTypeEnums.find(value);
        if (ff == downloadTypeEnums.end()) 
        {
            throw std::runtime_error("Invalid argument.");
        }
        return ff->second;
    }
    std::string Tone3000DownloadTypeToString(Tone3000DownloadType value)
    {
        switch (value)
        {
            case Tone3000DownloadType::Nam:
                return "nam";
            case Tone3000DownloadType::CabIr:
                return "cabir";
            default:
                throw std::runtime_error("Inalid argument.");
        }
    }

}