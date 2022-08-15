/*
 * MIT License
 * 
 * Copyright (c) 2022 Robin E. R. Davies
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

#include "vst3/Vst3PresetFile.hpp"
#include "public.sdk/source/vst/vstpresetfile.h"
using namespace pipedal;
using namespace Steinberg;
using namespace Steinberg::Vst;


namespace pipedal {

class Vst3PresetFileImpl : public Vst3PresetFile {
public:
       virtual ~Vst3PresetFileImpl() { }


        virtual void Load(const std::string&path);

        virtual std::vector<Vst3PresetInfo> GetPresets();

        virtual std::vector<uint8_t> GetPresetChunk(uint32_t index);
private:
    std::unique_ptr<PresetFile> presetFile;
 
}; 


void Vst3PresetFileImpl::Load(const std::string&path)
{
    OPtr<IBStream> stream = FileStream::open(path.c_str(),"r");
    presetFile = std::make_unique<PresetFile>(stream);

}

std::vector<Vst3PresetInfo> Vst3PresetFileImpl::GetPresets()
{
    std::vector<Vst3PresetInfo> result;
    auto count = presetFile->getEntryCount();
    result.reserve(count);

    for (int i = 0; i < count; ++i)
    {
        const PresetFile::Entry& entry = presetFile->at(i);
    }

    return result;
}

std::vector<uint8_t> Vst3PresetFileImpl::GetPresetChunk(uint32_t index)
{
    return std::vector<uint8_t>(); // STUB
}



std::unique_ptr<Vst3PresetFile> Vst3PresetFile::CreateInstance()
{
    return std::make_unique<Vst3PresetFileImpl>();
}




} // namespace pipedal