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

#include "pch.h"
#include "catch.hpp"
#include <sstream>
#include <cstdint>
#include <string>
#include <iostream>

#include "json.hpp"
#include "json_variant.hpp"
#include "AtomConverter.hpp"
#include "lv2/atom.lv2/util.h"
#include "lv2/patch.lv2/patch.h"
#include "MapFeature.hpp"
#include <cstring>

using namespace pipedal;
using namespace std;

MapFeature mapFeature;

using AtomBuffer = std::vector<uint8_t>;

static void RoundTripTest(const AtomBuffer &buffer)
{
    LV2_Atom *prototype = (LV2_Atom *)&(buffer[0]);

    AtomConverter converter(mapFeature);

    json_variant variant = converter.ToJson(prototype);
    cout << "    " << variant.to_string() << endl;

    LV2_Atom *atomOut = converter.ToAtom(variant);
    REQUIRE(atomOut->size == prototype->size);
    size_t size = atomOut->size + sizeof(LV2_Atom);
    REQUIRE(std::memcmp(atomOut, prototype, size) == 0);
}

AtomBuffer MakeSimpleAtom()
{
    AtomBuffer atomBuffer(512);
    LV2_Atom_Forge t;
    LV2_Atom_Forge *forge = &t;
    lv2_atom_forge_init(forge, mapFeature.GetMap());
    lv2_atom_forge_set_buffer(forge, &(atomBuffer[0]), atomBuffer.size());
    lv2_atom_forge_string(forge, "abcde\0", 6);
    return atomBuffer;
}
AtomBuffer MakeTestTupleAtom()
{
    AtomBuffer atomBuffer(512);
    LV2_Atom_Forge t;
    LV2_Atom_Forge *forge = &t;
    lv2_atom_forge_init(forge, mapFeature.GetMap());
    lv2_atom_forge_set_buffer(forge, &(atomBuffer[0]), atomBuffer.size());

    LV2_Atom_Forge_Frame frame;
    lv2_atom_forge_tuple(forge, &frame);
    {
        lv2_atom_forge_bool(forge, true);
        lv2_atom_forge_int(forge, 1);
        lv2_atom_forge_float(forge, 2.1f);
        lv2_atom_forge_double(forge, 1.23456789);
        lv2_atom_forge_string(forge, "abcd\0", 5);
        float vectorValues[] = {1.1f, 2.2f, 3.3f, 4.4f};
        lv2_atom_forge_vector(forge, sizeof(float), mapFeature.GetUrid(LV2_ATOM__Float), 4, vectorValues);

        {
            LV2_Atom_Forge_Frame objFrame;
            lv2_atom_forge_object(forge, &objFrame, 0, mapFeature.GetUrid(LV2_PATCH__Set));
            lv2_atom_forge_key(forge,mapFeature.GetUrid(LV2_PATCH__property));
            lv2_atom_forge_urid(forge,mapFeature.GetUrid("http://something.com/custom#prop"));
            lv2_atom_forge_key(forge,mapFeature.GetUrid(LV2_PATCH__value));
            std::string path = "/var/pipdeal/test.wav";
            lv2_atom_forge_path(forge, path.c_str(),path.size()+1);

            lv2_atom_forge_pop(forge, &objFrame);
        }
    }
    lv2_atom_forge_pop(forge, &frame);

    lv2_atom_forge_string(forge, "abcde\0", 6);
    return atomBuffer;
}

TEST_CASE("AtomConverter", "[atom_converter][Build][Dev]")
{
    cout << "=== AtomConverter Test ====" << endl;

    cout << "  testTuple" << endl;
    RoundTripTest(MakeTestTupleAtom());

    cout << "  simpleAtom" << endl;
    RoundTripTest(MakeSimpleAtom());
}
