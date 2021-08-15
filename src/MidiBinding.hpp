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
    int bindingType_;
    int note_ = 12*4+24;
    int control_ = 1;
    float minValue_ = 0;
    float maxValue_ = 1;
    float rotaryScale_ = 1;
    int linearControlType_ = 0;
    int switchControlType_ = 0;
public:
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