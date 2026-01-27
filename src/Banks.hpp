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
#include "Pedalboard.hpp"
#include "PiPedalException.hpp"

namespace pipedal
{

#define GETTER_SETTER_REF(name)                               \
    decltype(name##_) &name() { return name##_; }             \
    const decltype(name##_) &name() const { return name##_; } \
    void name(const decltype(name##_) &value) { name##_ = value; }

#define GETTER_SETTER_VEC(name)                   \
    decltype(name##_) &name() { return name##_; } \
    const decltype(name##_) &name() const { return name##_; }

#define GETTER_SETTER(name)                            \
    decltype(name##_) name() const { return name##_; } \
    void name(decltype(name##_) value) { name##_ = value; }

    class PresetIndexEntry
    {
        int64_t instanceId_ = 0;
        std::string name_;

    public:
        PresetIndexEntry() {}
        PresetIndexEntry(uint64_t instanceId, const std::string &name)
            : instanceId_(instanceId), name_(name)
        {
        }

        GETTER_SETTER(instanceId);
        GETTER_SETTER(name);
        DECLARE_JSON_MAP(PresetIndexEntry);
    };

    class PresetIndex
    {
        int64_t selectedInstanceId_ = -1;
        bool presetChanged_ = false;
        std::vector<PresetIndexEntry> presets_;

    public:
        bool GetPresetName(int64_t instanceId, std::string *pResult)
        {
            for (size_t i = 0; i < presets_.size(); ++i)
            {
                if (presets_[i].instanceId() == instanceId)
                {
                    *pResult = presets_[i].name();
                    return true;
                }
            }
            return false;
        }
        GETTER_SETTER(selectedInstanceId);
        GETTER_SETTER_VEC(presets);
        GETTER_SETTER(presetChanged);

        DECLARE_JSON_MAP(PresetIndex);
    };

    class BankFileEntry
    {
        int64_t instanceId_;
        Pedalboard preset_;

    public:
        GETTER_SETTER(instanceId);
        GETTER_SETTER_REF(preset);

        DECLARE_JSON_MAP(BankFileEntry);
    };
    class BankFile
    {
        std::string name_;
        int64_t nextInstanceId_ = 0;
        int64_t selectedPreset_ = -1;
        std::vector<std::unique_ptr<BankFileEntry>> presets_;

    public:
        GETTER_SETTER(name);
        GETTER_SETTER(nextInstanceId);
        GETTER_SETTER(selectedPreset);
        GETTER_SETTER_VEC(presets);

        void clear()
        {
            nextInstanceId_ = 0;
            presets_.clear();
            selectedPreset_ = -1;
        }

        void move(size_t from, size_t to)
        {
            if (from >= this->presets_.size())
            {
                throw std::invalid_argument("Argument out of range.");
            }
            if (to >= this->presets_.size())
            {
                throw std::invalid_argument("Argument out of range.");
            }
            std::unique_ptr<BankFileEntry> t = std::move(this->presets_[from]);
            presets_.erase(presets_.begin() + from);
            presets_.insert(presets_.begin() + to, std::move(t));
        }
        void updateNextIndex()
        {
            int64_t t = 0;
            for (size_t i = 0; i < this->presets_.size(); ++i)
            {
                int64_t instanceId = this->presets_[i]->instanceId();
                if (instanceId > t)
                    t = instanceId;
            }
            this->nextInstanceId_ = t;
        }
        int64_t addPreset(const Pedalboard &preset, int64_t afterItem = -1)
        {
            if (hasName(preset.name()))
            {
                throw PiPedalStateException("A preset by that name already exists.");
            }
            std::unique_ptr<BankFileEntry> entry = std::make_unique<BankFileEntry>();
            entry->instanceId(++nextInstanceId_);
            entry->preset(preset);
            int64_t instanceId = entry->instanceId();
            if (afterItem == -1)
            {
                this->presets_.push_back(std::move(entry));
            }
            else
            {
                for (auto it = this->presets_.begin(); it != this->presets_.end(); ++it)
                {
                    if ((*it)->instanceId() == afterItem)
                    {
                        ++it;
                        this->presets_.insert(it, std::move(entry));
                        break;
                    }
                }
            }
            return instanceId;
        }
        bool hasItem(int64_t instanceId)
        {
            for (size_t i = 0; i < presets_.size(); ++i)
            {
                if (presets_[i]->instanceId() == instanceId)
                {
                    return true;
                }
            }
            return false;
        }
        bool hasName(const std::string &name)
        {
            for (size_t i = 0; i < presets_.size(); ++i)
            {
                if (presets_[i]->preset().name() == name)
                {
                    return true;
                }
            }
            return false;
        }
        bool renamePreset(int64_t instanceId, const std::string &name)
        {
            if (hasName(name))
            {
                throw PiPedalStateException("A preset by that name already exists.");
            }
            for (size_t i = 0; i < presets_.size(); ++i)
            {
                if (presets_[i]->instanceId() == instanceId)
                {
                    presets_[i]->preset().name(name);
                    return true;
                }
            }
            return false;
        }

        BankFileEntry &getItem(int64_t instanceId)
        {
            for (size_t i = 0; i < presets_.size(); ++i)
            {
                if (presets_[i]->instanceId() == instanceId)
                {
                    return *(presets_[i].get());
                }
            }
            throw PiPedalArgumentException("Instance not found.");
        }
        int64_t deletePreset(int64_t instanceId)
        {
            for (size_t i = 0; i < presets_.size(); ++i)
            {
                if (presets_[i]->instanceId() == instanceId)
                {
                    presets_.erase(presets_.begin() + i);
                    int64_t newSelection;
                    if (i < presets_.size())
                    {
                        newSelection = presets_[i]->instanceId();
                    }
                    else if (i == presets_.size() && i != 0) {
                        newSelection = presets_[i-1]->instanceId();

                    }
                    else if (presets_.size() >= 1)
                    {
                        newSelection = presets_[0]->instanceId();
                    }
                    else
                    {
                        // zero length? We can never have a zero-length bank.
                        // Add a default preset and make it the selected preset.
                        Pedalboard pedalboard = Pedalboard::MakeDefault(Pedalboard::InstanceType::MainPedalboard);
                        this->addPreset(pedalboard);
                        newSelection = presets_[0]->instanceId();
                    }
                    if (instanceId == this->selectedPreset_)
                    {
                        this->selectedPreset_ = newSelection;
                    }
                    return newSelection;
                }
            }
            throw PiPedalStateException("Preset not found.");
        }

        DECLARE_JSON_MAP(BankFile);
    };

    class BankIndexEntry
    {
        int64_t instanceId_ = 0;
        std::string name_;

    public:
        GETTER_SETTER(instanceId);
        GETTER_SETTER_REF(name);

        DECLARE_JSON_MAP(BankIndexEntry);
    };

    class BankIndex
    {
        int64_t selectedBank_ = 0;
        int64_t nextInstanceId_ = 0;
        std::vector<BankIndexEntry> entries_;

    public:
        GETTER_SETTER(selectedBank);
        GETTER_SETTER_VEC(entries);

        DECLARE_JSON_MAP(BankIndex);

        bool hasName(const std::string &name) const
        {
            for (size_t i = 0; i < this->entries_.size(); ++i)
            {
                if (this->entries_[i].name() == name)
                    return true;
            }
            return false;
        }

        void move(size_t from, size_t to)
        {
            if (from >= this->entries_.size())
            {
                throw std::invalid_argument("Argument out of range.");
            }
            if (to >= this->entries_.size())
            {
                throw std::invalid_argument("Argument out of range.");
            }
            BankIndexEntry t = std::move(this->entries_[from]);
            entries_.erase(entries_.begin() + from);
            entries_.insert(entries_.begin() + to, std::move(t));
        }

        void clear()
        {
            entries_.clear();
            selectedBank_ = 0;
        }
        int64_t addBank(int64_t afterId, const std::string &name)
        {
            BankIndexEntry bank;
            bank.name(name);
            bank.instanceId(++nextInstanceId_);

            for (size_t i = 0; i < this->entries_.size(); ++i)
            {
                if (entries_[i].instanceId() == afterId)
                {
                    entries_.insert(entries_.begin() + (i + 1), bank);
                    return bank.instanceId();
                }
            }
            // else at the end.
            entries_.push_back(bank);
            return bank.instanceId();
        }
        BankIndexEntry *getEntryByName(const std::string &name)
        {
            for (size_t i = 0; i < entries_.size(); ++i)
            {
                if (entries_[i].name() == name)
                {
                    return &entries_[i];
                }
            }
            return nullptr;
        }
        BankIndexEntry &getBankIndexEntry(int64_t instanceId)
        {
            for (size_t i = 0; i < entries_.size(); ++i)
            {
                if (entries_[i].instanceId() == instanceId)
                {
                    return entries_[i];
                }
            }
            throw PiPedalArgumentException("Bank not found.");
        }
        const BankIndexEntry &getBankIndexEntry(int64_t instanceId) const
        {
            for (size_t i = 0; i < entries_.size(); ++i)
            {
                if (entries_[i].instanceId() == instanceId)
                {
                    return entries_[i];
                }
            }
            throw PiPedalArgumentException("Bank not found.");
        }
    };
#undef GETTER_SETTER
#undef GETTER_SETTER_VEC
#undef GETTER_SETTER_REF

}
