// Copyright (c) 2022 Robin Davies
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

#include "json.hpp"


namespace pipedal {


#define GETTER_SETTER_REF(name) \
    const decltype(name##_)& name() const { return name##_;} \
    void name(const decltype(name##_) &value) { name##_ = value; }

#define GETTER_SETTER_VEC(name) \
    decltype(name##_)& name()  { return name##_;}  \
    const decltype(name##_)& name() const  { return name##_;} 



#define GETTER_SETTER(name) \
    decltype(name##_) name() const { return name##_;} \
    void name(decltype(name##_) value) { name##_ = value; }



    const int BINDING_TYPE_NONE = 0;
    const int BINDING_TYPE_NOTE = 1;
    const int BINDING_TYPE_CONTROL = 2;

    const int LINEAR_CONTROL_TYPE = 0;
    const int  CIRCULAR_CONTROL_TYPE = 1;

    const int LATCH_CONTROL_TYPE = 0;
    const int MOMENTARY_CONTROL_TYPE = 1;



class MidiBinding {
public: 
private:
    std::string symbol_;
    int channel_ = -1;
    int bindingType_ = BINDING_TYPE_NONE;
    int note_ = 12*4+24;
    int control_ = 1;
    float minValue_ = 0;
    float maxValue_ = 1;
    float rotaryScale_ = 1;
    int linearControlType_ = LINEAR_CONTROL_TYPE;
    int switchControlType_ = LATCH_CONTROL_TYPE;
public:
    static MidiBinding SystemBinding(const std::string&symbol)
    {
        MidiBinding result;
        result.symbol_ = symbol;
        return result;
    }
    GETTER_SETTER(channel);
    GETTER_SETTER_REF(symbol);
    GETTER_SETTER(bindingType);
    GETTER_SETTER(note);
    GETTER_SETTER(control);
    GETTER_SETTER(minValue);
    GETTER_SETTER(maxValue);
    GETTER_SETTER(rotaryScale);
    GETTER_SETTER(linearControlType);
    GETTER_SETTER(switchControlType);

    DECLARE_JSON_MAP(MidiBinding);

};

#undef GETTER_SETTER_REF
#undef GETTER_SETTER_VEC
#undef GETTER_SETTER



} // namespace