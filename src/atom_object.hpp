// Copyright (c) 2022-2023 Robin Davies
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

#include "lv2/atom/atom.h"
#include <cstdint>
#include <vector>

namespace pipedal
{
    // memory-managed binary atom.
    class atom_object {
    public:
        atom_object();
        atom_object(const LV2_Atom*atom);
        atom_object(const atom_object&other);
        atom_object(atom_object&&other);

        atom_object&operator=(const atom_object&other);
        atom_object&operator=(atom_object&&other);
        const LV2_Atom*get() const { return (LV2_Atom*)&(atomData[0]); }
        bool operator==(const atom_object&other) const;
        bool operator!=(const atom_object&other) const;
    private:
        std::vector<uint8_t> atomData;
    };

    /*****/

    inline atom_object::atom_object() { }
    
    inline atom_object::atom_object(const atom_object&other) 
    : atomData(other.atomData)
    {

    }
    inline atom_object::atom_object(atom_object&&other) 
    : atomData(std::move(other.atomData))
    {

    }
    inline bool atom_object::operator==(const atom_object&other) const
    {
        return this->atomData == other.atomData;
    }
    inline bool atom_object::operator!=(const atom_object&other) const
    {
        return this->atomData != other.atomData;
    }

    inline atom_object&atom_object::operator=(const atom_object&other)
    {
        this->atomData = other.atomData;
        return *this;
    }
    inline atom_object&atom_object::operator=(atom_object&&other)
    {
        this->atomData = std::move(other.atomData);
        return *this;
    }


}