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

#include <cstddef>
#include <vector>
#include "MapFeature.hpp"
#include "lv2/atom/atom.h"
#include "lv2/atom/forge.h"
#include "json_variant.hpp"

namespace pipedal
{
    /** @brief Roud-trippable Conversion of atoms to json, and json to atoms.
        @remarks
        This class implements round-trippable conversion of atoms to json and 
        vice-versa.

        Json objects are used for atom objects with non-trivial types; and 
        the "otype_" of a json object resolves the type of the corresponding
        atom object. For the sake of density and readability, the otype_ of 
        basic LV2_ATOM__xxx types are represented using only portion of the 
        ATOM uri following the '#'.

        Augmented semantics are required to preserve typed atoms in json.  For 
        example, an ATOM_Patch__Get atom would be reprsented as follows. 


               {
                "otype_": "http://lv2plug.in/ns/ext/patch#Set",
                "http://lv2plug.in/ns/ext/patch#property": {
                    "otype_": "URID",
                    "value": "http://something.com/custom#prop"
                },
                "http://lv2plug.in/ns/ext/patch#value": {
                    "otype_": "Path",
                    "value": "/var/pipdeal/test.wav"
                }
 
        LV2_ATOM__String's are represented as strings in the corresponding json.

        LV2_ATOM__Float's are represented as json numbers. 

        LV2_ATOM__Bool's are represented as unadorned boolean values in json.

        Remaining Atom types require augmented json in order to preserve type information.

        LV2_ATOM__Int:

            {"otype_": "Int","value": 1}
        
        LV2_ATOM__Long

            {"otype_": "Int","value": 1}

        LV2_ATOM__Double

            {"otype_": "Double","value": 1.234}                

        LV2_ATOM__Path

            {"otype_": "Path", "value": "/var/pipdeal/test.wav"}

        LV2_ATOM__URID

            {"otype_": "URID", "value": "http://something.com/custom#value"}

            The value is transmitted as an LV2_URID when translated to an atom, and un-mapped 
            when transmitted in json.

        LV2_ATOM__URI

            {"otype_": "URI", "value": "http://something.com/custom#prop"}

            The value is a string in both json and in atoms.

        LV2_ATOM_TUPLE

            {"otype_": "Tuple","value": [ ... arbitrary atoms ... ]}

        LV2_ATOM_VECTOR

            { "otype_": "Vector", "vtype_": "Int", "value": [ 1, 2, 3, 4 ] }

            Json values are co-erced to the type specified in "vtype_" and do not require
            type annotation. vtype_ can be "Bool", "Int", "Long", "Float", or "Double".

        LV2_ATOM_OBJECT 

            { "otype_": "http://lv2plug.in/ns/ext/patch#Set",
                "http://lv2plug.in/ns/ext/patch#property": {
                    "otype_": "URID",
                    "value": "http://something.com/custom#prop"
                },
                "http://two-play.com/ConvolutionReverb#volume": 3.5
            }

            An object has an "otype_" that specifies the type of the object, and contains
            a list of propertys of the form "propertyname": <atom_value>. Property names 
            are strings, but are converted to LV2_URID's in the object body. By usual Atom
            convention, the are URIs, but they could be simple strings as well.
    **/

    class AtomConverter
    {
    public:
        /// @brief Constructor
        /// @brief map  used to map LV2_URIDs. Must have a lifetime longer than the AtomConverter instance.
        
        AtomConverter(MapFeature &map);

        /// @brief Convert the supplied atom to json.
        /// @param atom The atom to convert.
        /// @return The atom data in json format.
        json_variant ToJson(const LV2_Atom *atom);

        /// @brief Convert a json variant to an atom.
        /// @param json Json variant that matches the structure of the prototype.
        /// @return An atom.
        /// @remarks
        /// The json variant must match the structure of the LV2_Atom prototype supplied to 
        /// the constructor. 
        LV2_Atom*ToAtom(const json_variant& json);
    private:
        static const std::string OTYPE_TAG;
        static const std::string VTYPE_TAG;
        static const std::string ID_TAG;

        std::map<std::string, LV2_URID> stringToTypeUrid;
        std::map<LV2_URID,std::string> typeUridToString;

        LV2_URID GetTypeUrid(const std::string uri);
        std::string TypeUridToString(LV2_URID urid);


        void InitUrids();
        LV2_URID InitUrid(const char*uri, const char*shortUri);

        template <typename U>
        json_variant TypedProperty(const std::string type,U value)
        {
            json_variant result = json_variant::make_object();
            result[OTYPE_TAG] = type;
            result["value"] = json_variant(value);
            return result;
        }
        json_variant TypedProperty(const std::string type,const json_variant&& value)
        {
            json_variant result = json_variant::make_object();
            result[OTYPE_TAG] = type;
            result["value"] = std::move(value);
            return result;
        }
        bool AreTypesTheSame(LV2_Atom*left, LV2_Atom *right) const;

        json_variant ToVariant(LV2_Atom *atom);
        void ToForge(const json_variant&json);
        void VectorToForge(const json_variant&json);
        void TupleToForge(const json_variant&json);
        void ObjectToForge(const json_variant&json);
        

        class BufferOverflowException: public std::exception {
        public:
            BufferOverflowException() { }
            virtual const char*what() const noexcept override {
                return "Buffer overflow";
            }
        };
        inline static void CheckResult(LV2_Atom_Forge_Ref ref)
        {
            if (ref == 0)
            {
                throw BufferOverflowException();
            }
        }

    private:
        class Urids
        {
        public:
            LV2_URID ATOM__Bool;
            LV2_URID ATOM__Chunk;
            LV2_URID ATOM__Double;
            LV2_URID ATOM__Event;
            LV2_URID ATOM__Float;
            LV2_URID ATOM__Int;
            LV2_URID ATOM__Literal;
            LV2_URID ATOM__Long;
            LV2_URID ATOM__Number;
            LV2_URID ATOM__Object;
            LV2_URID ATOM__Path;
            LV2_URID ATOM__Property;
            LV2_URID ATOM__Resource;
            LV2_URID ATOM__Sequence;
            LV2_URID ATOM__Sound;
            LV2_URID ATOM__String;
            LV2_URID ATOM__Tuple;
            LV2_URID ATOM__Vector;
            LV2_URID ATOM__URI;
            LV2_URID ATOM__URID;
            LV2_URID ATOM__atomTransfer;
            LV2_URID ATOM__beatTime;
            LV2_URID ATOM__frameTime;
            LV2_URID ATOM__supports;
            LV2_URID ATOM__timeUnit;
        };
        Urids urids;
        MapFeature &map;
        std::vector<uint8_t> prototypeBuffer;
        std::vector<uint8_t> outputBuffer;
        LV2_Atom_Forge outputForge;

        LV2_Atom * prototype = nullptr;
    };

}