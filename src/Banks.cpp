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
#include "Banks.hpp"

using namespace pipedal;

int64_t BankFile::selectedPreset(PedalboardType pedalboardType) const
{
    int64_t result = -1;

    switch (pedalboardType)
    {
    case PedalboardType::MainPedalboard:
        result = selectedPreset_;
        break;
    case PedalboardType::MainInserts:
        result = this->selectedMainInsertPreset_;
        break;
    case PedalboardType::AuxInserts:
        result = selectedAuxInsertPreset_;
        break;
    }
    return result;
}

void BankFile::selectedPreset(PedalboardType pedalboardType, int64_t selectedPreset)
{
    switch (pedalboardType)
    {
    case PedalboardType::MainPedalboard:
        this->selectedPreset_ = selectedPreset;
        break;
    case PedalboardType::MainInserts:
        this->selectedMainInsertPreset_ = selectedPreset;
        break;
    case PedalboardType::AuxInserts:
        this->selectedAuxInsertPreset_ = selectedPreset;
        break;
    }
}

int64_t BankIndex::selectedBank(PedalboardType pedalboardType) const
{
    int64_t result;
    switch (pedalboardType)
    {
    case PedalboardType::MainPedalboard:
        result =  this->selectedBank_;
        break;
    case PedalboardType::MainInserts:
        result = this->selectedMainInsertBank_;
        break;
    case PedalboardType::AuxInserts:
        result = this->selectedAuxInsertBank_;
        break;
    default:
        throw std::runtime_error("Invalid argument.");
    }
    if (result == -1) 
    {
        result = this->selectedBank_;
    }
    if (!isValidInstanceId(result))
    {
        if (this->entries_.size() == 0)
        {
            throw std::runtime_error("BankIndex::SelectedBank : No entries found.");
        }
        result = this->entries_[0].instanceId();
    }
    return result;
}
void BankIndex::selectedBank(PedalboardType pedalboardType, int64_t selectedIndex)
{
    switch (pedalboardType)
    {
    case PedalboardType::MainPedalboard:
        this->selectedBank_ = selectedIndex;
        break;
    case PedalboardType::MainInserts:
        this->selectedMainInsertBank_ = selectedIndex;
        break;
    case PedalboardType::AuxInserts:
        this->selectedAuxInsertBank_ = selectedIndex;
        break;
    default:
        throw std::runtime_error("Invalid argument.");
    }
    
}

JSON_MAP_BEGIN(PresetIndexEntry)
JSON_MAP_REFERENCE(PresetIndexEntry, instanceId)
JSON_MAP_REFERENCE(PresetIndexEntry, name)
JSON_MAP_END()

JSON_MAP_BEGIN(PresetIndex)
JSON_MAP_REFERENCE(PresetIndex, selectedInstanceId)
JSON_MAP_REFERENCE(PresetIndex, presetChanged)
JSON_MAP_REFERENCE(PresetIndex, presets)
JSON_MAP_END()

JSON_MAP_BEGIN(BankIndex)
JSON_MAP_REFERENCE(BankIndex, selectedBank)
JSON_MAP_REFERENCE(BankIndex, selectedMainInsertBank)
JSON_MAP_REFERENCE(BankIndex, selectedAuxInsertBank)
JSON_MAP_REFERENCE(BankIndex, nextInstanceId)
JSON_MAP_REFERENCE(BankIndex, entries)
JSON_MAP_END()

JSON_MAP_BEGIN(BankIndexEntry)
JSON_MAP_REFERENCE(BankIndexEntry, instanceId)
JSON_MAP_REFERENCE(BankIndexEntry, name)
JSON_MAP_END()

JSON_MAP_BEGIN(BankFile)
JSON_MAP_REFERENCE(BankFile, name)
JSON_MAP_REFERENCE(BankFile, nextInstanceId)
JSON_MAP_REFERENCE(BankFile, selectedPreset)
JSON_MAP_REFERENCE(BankFile, presets)
JSON_MAP_END()

JSON_MAP_BEGIN(BankFileEntry)
JSON_MAP_REFERENCE(BankFileEntry, instanceId)
JSON_MAP_REFERENCE(BankFileEntry, preset)
JSON_MAP_END()
