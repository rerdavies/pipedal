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

#include "AtomConverter.hpp"
#include <cstddef>
#include <cstring>
#include <sstream>
#include "json.hpp"
#include "ss.hpp"
#include "lv2/atom/util.h"

using namespace pipedal;

const std::string AtomConverter::ID_TAG {"id_"};
const std::string AtomConverter::OTYPE_TAG {"otype_"};
const std::string AtomConverter::VTYPE_TAG {"vtype_"};

#define SHORT_ATOM__Bool "Bool"
#define SHORT_ATOM__Float "Float"
#define SHORT_ATOM__Int "Int"
#define SHORT_ATOM__Long "Long"
#define SHORT_ATOM__Double "Double"
#define SHORT_ATOM__Vector "Vector"
#define SHORT_ATOM__Object "Object"
#define SHORT_ATOM__Tuple "Tuple"
#define SHORT_ATOM__URI "URI"
#define SHORT_ATOM__URID "URID"
#define SHORT_ATOM__String "String"
#define SHORT_ATOM__Path "Path"



AtomConverter::AtomConverter(MapFeature &map)
    : map(map)
{
    InitUrids();
    if (prototype)
    {
        this->prototypeBuffer.resize(prototype->size+sizeof(LV2_Atom));
        std::memcpy(&(this->prototypeBuffer[0]), prototype, this->prototypeBuffer.size());
        this->prototype = (LV2_Atom*)&(prototype[0]);
    }
    lv2_atom_forge_init(&outputForge, map.GetMap());
}

json_variant AtomConverter::ToJson(const LV2_Atom *atom)
{
    json_variant variant = ToVariant(const_cast<LV2_Atom*>(atom));
    return std::move(variant);
}
LV2_Atom*AtomConverter::ToAtom(const json_variant&json)
{
    if (outputBuffer.size() == 0)
    {
        outputBuffer.resize(512);
    }

    while (true)
    {
        try {
            LV2_Atom *result = (LV2_Atom*)(&outputBuffer[0]);
            result->size = outputBuffer.size();
            lv2_atom_forge_set_buffer(&outputForge,(uint8_t*)&(outputBuffer[0]),outputBuffer.size());
            ToForge(json);
            return (LV2_Atom*)(&outputBuffer[0]);
        } catch (const BufferOverflowException&)
        {
        }

        if (outputBuffer.size() >= 1024*1024)
        {
            throw std::logic_error("Atom is too large.");
        }
        outputBuffer.resize(outputBuffer.size()*2);
    }
}


static inline void *AtomContent(LV2_Atom*atom,size_t offset = 0)
{
    // always % sizeof(LV2_Atom)
    return (void*)((char*)atom + sizeof(LV2_Atom)+offset);
}
static inline LV2_Atom*NextAtom(LV2_Atom*atom)
{
    size_t size = sizeof(LV2_Atom) + (atom->size +(sizeof(LV2_Atom)-1))/sizeof(LV2_Atom)*sizeof(LV2_Atom);
    return (LV2_Atom*)((char*)atom + size);
}

json_variant AtomConverter::ToVariant(LV2_Atom *atom)
{
    if (atom->type == urids.ATOM__Float)
    {
        LV2_Atom_Float*pVal = (LV2_Atom_Float*)atom;
        return json_variant((double)pVal->body);
    }
    if (atom->type == urids.ATOM__Bool)
    {
        LV2_Atom_Bool *pVal = (LV2_Atom_Bool*)atom;
        return json_variant(pVal->body != 0);
    }
    if (atom->type == urids.ATOM__Int)
    {
        LV2_Atom_Int *pVal = (LV2_Atom_Int*)atom;
        return TypedProperty(SHORT_ATOM__Int,(double)pVal->body);
    }
    if (atom->type == urids.ATOM__Long)
    {
        LV2_Atom_Long *pVal = (LV2_Atom_Long*)atom;
        return TypedProperty(SHORT_ATOM__Long,(double)pVal->body);
    }
    if (atom->type == urids.ATOM__Double)
    {
        LV2_Atom_Double*pVal = (LV2_Atom_Double*)atom;
        return TypedProperty(SHORT_ATOM__Double,(double)pVal->body);
    }
    if (atom->type == urids.ATOM__URID)
    {
        LV2_Atom_URID*pVal = (LV2_Atom_URID*)atom;
        const char*strValue = map.UridToString(pVal->body);
        return TypedProperty(SHORT_ATOM__URID,std::string(strValue));
    }
    if (atom->type == urids.ATOM__String)
    {
        const char*p = (const char*)atom +sizeof(LV2_Atom);
        size_t size = atom->size;
        while (size > 0 && p[size-1] == '\0')
        {
            --size;
        }
        return json_variant(std::string(p,size));
    } else  if (atom->type == urids.ATOM__Path)
    {
        const char*p = (const char*)atom +sizeof(LV2_Atom);
        size_t size = atom->size;
        while (size > 0 && p[size-1] == '\0')
        {
            --size;
        }
        return TypedProperty(SHORT_ATOM__Path,std::string(p,size));

    } else if (atom->type == urids.ATOM__URI)
    {
        const char*p = (const char*)atom +sizeof(LV2_Atom);
        size_t size = atom->size;
        while (size > 0 && p[size-1] == '\0')
        {
            --size;
        }
        return TypedProperty(SHORT_ATOM__URI,std::string(p,size));

    }
    else if (atom->type == urids.ATOM__Tuple)
    {
        json_array array;
        LV2_Atom*current = (LV2_Atom*)AtomContent(atom,0);
        LV2_Atom*end = (LV2_Atom*)AtomContent(atom,atom->size);
        while (current < end)
        {
            array.push_back(ToVariant(current));
            current = NextAtom(current);
        }
        json_variant vArray { std::move(array)};
        return TypedProperty(SHORT_ATOM__Tuple,std::move(vArray));
    }
    if (atom->type == urids.ATOM__URID)
    {
        LV2_Atom_URID *pVal = (LV2_Atom_URID*)atom;
        return TypedProperty(SHORT_ATOM__URID,std::string(map.UridToString(pVal->body)));
    }
    if (atom->type == urids.ATOM__Vector)
    {
        json_array array;
        LV2_Atom_Vector *pVal = (LV2_Atom_Vector*)atom;

        size_t n = (atom->size-sizeof(LV2_Atom_Vector_Body))/pVal->body.child_size;
        void *vectorData = AtomContent(atom,sizeof(LV2_Atom_Vector_Body));
        if (pVal->body.child_type == urids.ATOM__Float)
        {
            float*p = (float*)vectorData;
            for (size_t i = 0; i < n; ++i)
            {
                array.push_back(json_variant((double)p[i]));
            }
        }
        else if (pVal->body.child_type == urids.ATOM__Int)
        {
            auto p = (int32_t*)vectorData;
            for (size_t i = 0; i < n; ++i)
            {
                array.push_back(json_variant((double)p[i]));
            }
        }
        else if (pVal->body.child_type == urids.ATOM__Bool)
        {
            auto p = (int32_t*)vectorData;
            for (size_t i = 0; i < n; ++i)
            {
                array.push_back(json_variant(p[i] != 0));
            }
        }
        else if (pVal->body.child_type == urids.ATOM__Long)
        {
            auto p = (int64_t*)vectorData;
            for (size_t i = 0; i < n; ++i)
            {
                array.push_back(json_variant((double)(p[i])));
            }
        }
        else if (pVal->body.child_type == urids.ATOM__Double)
        {
            auto p = (double*)vectorData;
            for (size_t i = 0; i < n; ++i)
            {
                array.push_back(json_variant((double)p[i]));
            }
        } else {
            std::string dataType = map.UridToString(pVal->body.child_type);
            throw std::logic_error("AtomConverter: Vector dataype not supported. (" + dataType + ") Please contact support if you get this message.");
        }
        json_variant vArray {std::move(array)};
        json_variant object = json_variant::make_object();
        object[OTYPE_TAG] = json_variant(std::string(SHORT_ATOM__Vector));
        object[VTYPE_TAG] = json_variant(std::string(TypeUridToString(pVal->body.child_type)));
        object["value"] = std::move(vArray);

        return std::move(object);
    } else if (atom->type == urids.ATOM__Property)
    {
        throw std::logic_error("Not implemented.");
    } else if (atom->type == urids.ATOM__Object)
    {
        LV2_Atom_Object *pVal = (LV2_Atom_Object*)atom;

        json_variant result = json_variant::make_object();
        if (pVal->body.id != 0)
        {

            result[ ID_TAG] = map.UridToString(pVal->body.id);
        }
        if (pVal->body.otype != 0)
        {

            result[OTYPE_TAG] = map.UridToString(pVal->body.otype);
        }
        LV2_Atom_Property_Body *current = (LV2_Atom_Property_Body *)AtomContent(atom,sizeof(LV2_Atom_Object_Body));
        LV2_Atom_Property_Body *end = (LV2_Atom_Property_Body *)AtomContent(atom,atom->size);

        while (current < end)
        {
            std::string key = map.UridToString(current->key);
            json_variant value = ToVariant(&current->value);
            result[key] = std::move(value);
            current = (LV2_Atom_Property_Body*)(NextAtom(&(current->value)));
        }

        return result;
    }
    throw std::logic_error(
        SS("AtomConverter: Datatype not supported. ("
            << map.UridToString(atom->type) 
            << ") Please contact support if you get this message."));



}


bool AtomConverter::AreTypesTheSame(LV2_Atom*left, LV2_Atom *right) const
{
    if (left->type != right->type) return false;
    if (left->type == urids.ATOM__Object)
    {
        LV2_Atom_Object *l = (LV2_Atom_Object*)left;
        LV2_Atom_Object *r = (LV2_Atom_Object*)right;
            return l->body.id == r->body.id && l->body.otype == r->body.otype;
    }
    return true;
}

void AtomConverter::ObjectToForge(const json_variant&json)
{
    LV2_URID bodyId = 0;
    assert(json.is_object());
    std::string oType = json[OTYPE_TAG].as_string();
    if (oType == SHORT_ATOM__Int)
    {
        lv2_atom_forge_int(&this->outputForge,(int32_t)json["value"].as_number());
    }
    else if (oType == SHORT_ATOM__Long)
    {
        lv2_atom_forge_long(&this->outputForge,(int64_t)json["value"].as_number());
    } else if (oType == SHORT_ATOM__Double)
    {
        lv2_atom_forge_double(&this->outputForge,(double)json["value"].as_number());
    }
    else if (oType == SHORT_ATOM__Path)
    {
        std::string value = json["value"].as_string().c_str();

        lv2_atom_forge_path(&this->outputForge,value.c_str(),value.length()+1);
    }
    else if (oType == SHORT_ATOM__URI)
    {
        std::string value = json["value"].as_string().c_str();

        lv2_atom_forge_uri(&this->outputForge,value.c_str(),value.length()+1);
    }
    else if (oType == SHORT_ATOM__URID)
    {
        LV2_URID urid = map.GetUrid(json["value"].as_string().c_str());
        lv2_atom_forge_urid(&outputForge,urid);

    }
    else if (oType == SHORT_ATOM__Tuple)
    {
        TupleToForge(json);
    } else if (oType == SHORT_ATOM__Vector)
    {
        VectorToForge(json);
    } else 
    {
        LV2_URID id = 0;
        if (json.contains(ID_TAG))
        {
            id = map.GetUrid(json[ID_TAG].as_string().c_str());
        }
        LV2_URID oType = 0;
        if (json.contains(OTYPE_TAG))
        {
            oType = map.GetUrid(json[OTYPE_TAG].as_string().c_str());
        }
        LV2_Atom_Forge_Frame frame;
        lv2_atom_forge_object(&outputForge,&frame,id,oType);
        const auto& obj = json.as_object();
        for (auto member: *obj.get())
        {
            const std::string& property = member.first;
            if (property != OTYPE_TAG && property != ID_TAG)
            {
                LV2_URID property = map.GetUrid(member.first.c_str());
                lv2_atom_forge_key(&outputForge,property);
                ToForge(member.second);
            }
        }
        lv2_atom_forge_pop(&outputForge,&frame);
    } 
}
void AtomConverter::ToForge(const json_variant&json)
{
    if (json.is_number())
    {
        CheckResult(lv2_atom_forge_float(&outputForge,(float)json.as_number()));
    }
    else if (json.is_bool())
    {
        CheckResult(lv2_atom_forge_bool(&outputForge,json.as_bool()));
    }
    else if (json.is_string())
    {
        const std::string&str = json.as_string(); 
        CheckResult(lv2_atom_forge_string(&outputForge,str.c_str(),str.size()+1));
    } else if (json.is_object())
    {
        return ObjectToForge(json);
    } else {
        throw std::logic_error("Malformed json atom.");
    }
}

void AtomConverter::VectorToForge(const json_variant&json)
{
    const  json_array& array = *(json["value"].as_array().get());
    size_t size = array.size();


    LV2_Atom_Forge_Frame frame;
    LV2_URID childType = GetTypeUrid(json[VTYPE_TAG].as_string().c_str());

    if (childType == urids.ATOM__Float)
    {
        using T = float;
        CheckResult(lv2_atom_forge_vector_head(&outputForge,&frame,sizeof(T),childType));

        for (size_t i = 0; i < size; ++i)
        {
            T value = (T)(array[i].as_number());
            CheckResult(lv2_atom_forge_raw(&outputForge,&value,sizeof(value)));
        }
        // does this pad the frame?
        lv2_atom_forge_pop(&outputForge,&frame);
        lv2_atom_forge_pad(&outputForge,sizeof(LV2_Atom_Vector_Body)+size*sizeof(float));

    }
    else if (childType == urids.ATOM__Int)
    {
        using T = int32_t;
        CheckResult(lv2_atom_forge_vector_head(&outputForge,&frame,sizeof(T),childType));

        for (size_t i = 0; i < size; ++i)
        {
            T value = (T)(array[i].as_number());
            CheckResult(lv2_atom_forge_raw(&outputForge,&value,sizeof(value)));
        }
        // does this pad the frame?
        lv2_atom_forge_pop(&outputForge,&frame);
        lv2_atom_forge_pad(&outputForge,sizeof(LV2_Atom_Vector_Body)+size*sizeof(float));
    }
    else if (childType == urids.ATOM__Bool)
    {
        using T = int32_t;
        CheckResult(lv2_atom_forge_vector_head(&outputForge,&frame,sizeof(T),childType));

        for (size_t i = 0; i < size; ++i)
        {
            T value = (T)(array[i].as_bool());
            CheckResult(lv2_atom_forge_raw(&outputForge,&value,sizeof(value)));
        }
        // does this pad the frame?
        lv2_atom_forge_pop(&outputForge,&frame);
        lv2_atom_forge_pad(&outputForge,sizeof(LV2_Atom_Vector_Body)+size*sizeof(float));
    }
    else if (childType == urids.ATOM__Long)
    {
        using T = int64_t;
        CheckResult(lv2_atom_forge_vector_head(&outputForge,&frame,sizeof(T),childType));

        for (size_t i = 0; i < size; ++i)
        {
            T value = (T)(array[i].as_number());
            CheckResult(lv2_atom_forge_raw(&outputForge,&value,sizeof(value)));
        }
        // does this pad the frame?
        lv2_atom_forge_pop(&outputForge,&frame);
        lv2_atom_forge_pad(&outputForge,sizeof(LV2_Atom_Vector_Body)+size*sizeof(float));
    }
    else if (childType == urids.ATOM__Double)
    {
        using T = double;
        CheckResult(lv2_atom_forge_vector_head(&outputForge,&frame,sizeof(T),childType));

        for (size_t i = 0; i < size; ++i)
        {
            T value = (T)(array[i].as_number());
            CheckResult(lv2_atom_forge_raw(&outputForge,&value,sizeof(value)));
        }
        // does this pad the frame?
        lv2_atom_forge_pop(&outputForge,&frame);
        lv2_atom_forge_pad(&outputForge,sizeof(LV2_Atom_Vector_Body)+size*sizeof(float));
    } else {
        std::string dataType = map.UridToString(childType);
        throw std::logic_error("AtomConverter: Vector dataype not supported. (" + dataType + ") Please contact support if you get this message.");
    }
}

void AtomConverter::TupleToForge(const json_variant&json)
{
    LV2_Atom_Forge_Frame frame;
    const json_array&array = *(json["value"].as_array().get());

    lv2_atom_forge_tuple(&outputForge,&frame);
    {
        for (const json_variant&v: array)
        {
            ToForge(v);
        }
    }
    lv2_atom_forge_pop(&outputForge,&frame);
}


LV2_URID AtomConverter::GetTypeUrid(const std::string uri)
{
    if (stringToTypeUrid.find(uri) != stringToTypeUrid.end())
    {
        return stringToTypeUrid[uri];
    }
    return map.GetUrid(uri.c_str());
}

LV2_URID AtomConverter::InitUrid(const char*uri, const char*shortUri )
{
    LV2_URID urid =  map.GetUrid(uri);
    stringToTypeUrid[uri] = urid;
    stringToTypeUrid[shortUri] = urid;
    typeUridToString[urid] = shortUri;
    return urid;
}

std::string AtomConverter::TypeUridToString(LV2_URID urid)
{
    if (typeUridToString.find(urid) != typeUridToString.end())
    {
        return typeUridToString[urid];
    }
    return map.UridToString(urid);
}

void AtomConverter::InitUrids()
{
#define ATOM_INIT(name,shortName) urids.name = InitUrid(LV2_##name,#shortName)
    ATOM_INIT(ATOM__Bool,Bool);
    ATOM_INIT(ATOM__Chunk,Chunk);
    ATOM_INIT(ATOM__Double,Double);
    ATOM_INIT(ATOM__Event,Event);
    ATOM_INIT(ATOM__Float,Float);
    ATOM_INIT(ATOM__Int,Int);
    ATOM_INIT(ATOM__Literal,Literal);
    ATOM_INIT(ATOM__Long,Long);
    ATOM_INIT(ATOM__Number,Number);
    ATOM_INIT(ATOM__Object,Object);
    ATOM_INIT(ATOM__Path,Path);
    ATOM_INIT(ATOM__Property,Property);
    //ATOM_INIT(ATOM__Resource,Resource);
    //ATOM_INIT(ATOM__Sequence,Sequence);
    //ATOM_INIT(ATOM__Sound,Sound);
    ATOM_INIT(ATOM__String,String);
    ATOM_INIT(ATOM__Tuple,Tuple);
    ATOM_INIT(ATOM__Vector,Vector);
    ATOM_INIT(ATOM__URI,URI);
    ATOM_INIT(ATOM__URID,URID);
    ATOM_INIT(ATOM__Vector,Vector);
    //ATOM_INIT(ATOM__beatTime,beatTime);
    //ATOM_INIT(ATOM__frameTime,frameTime);
    //ATOM_INIT(ATOM__timeUnit,timeUnit);
#undef ATOM_INIT
}