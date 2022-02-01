// Copyright (c) 2021 Robin Davies
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files (the "Software"), to deal in
// the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
// the Software, and to permit persons to whom the Software is furnished to do so,
// subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
// FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
// IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#pragma once

#include "PedalBoard.hpp"
#include "json.hpp"

namespace pipedal {

class PedalPreset {
private:
    int instanceId_ = -1;
    std::vector<ControlValue> values_; 
public:
    DECLARE_JSON_MAP(PedalPreset);
};

class PedalBoardPreset {
    uint64_t instanceId_;
    std::string displayName_;
    std::vector<std::unique_ptr<PedalPreset> > values_;
public:
    DECLARE_JSON_MAP(PedalBoardPreset);
};

class PedalBoardPresets {
    long nextInstanceId_ = 0;

    long currentPreset_ = 0;
    std::vector<std::unique_ptr<PedalBoardPreset> > presets_;
public:
    DECLARE_JSON_MAP(PedalBoardPresets);

};

} // namespace.