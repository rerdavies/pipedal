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

/*
  Based on lv2_evbuf.h/lv2_evbuf.c in https://raw.githubusercontent.com/drobilla/jalv/master/src/lv2_evbuf.h

Copyright 2008-2014 David Robillard <d@drobilla.net>

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THIS SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#pragma once

#include <stdint.h>
#include <cassert>
#include <lv2.h>
#include <lv2/atom/atom.h>
#include <lv2/midi/midi.h>
#include <lv2/urid/urid.h>
#include <cstring>

namespace pipedal
{
    class IHost;
    class Lv2EventBufferUrids
    {
    public:
        Lv2EventBufferUrids(IHost *pHost);
        
        LV2_URID atom_Chunk;
        LV2_URID atom_Sequence;
        LV2_URID midi_Event;
    };

    class Lv2EventBufferWriter
    {
        const Lv2EventBufferUrids &urids;
        LV2_Atom_Sequence *evbuf;

        size_t bufferSize;
        size_t offset;
        size_t padSize(size_t size) const
        {
            return (size + 7) & (~7);
        }

        size_t size() const {
            assert(evbuf->atom.type != urids.atom_Sequence
	       || evbuf->atom.size >= sizeof(LV2_Atom_Sequence_Body));
	    return evbuf->atom.type == urids.atom_Sequence
		    ? evbuf->atom.size - sizeof(LV2_Atom_Sequence_Body)
            : 0;
        }

    public:
        Lv2EventBufferWriter(const Lv2EventBufferUrids &urids)
            : urids(urids),
              evbuf(nullptr),
              bufferSize(0),
              offset(0)
        {
        }

        Lv2EventBufferWriter(const Lv2EventBufferUrids &urids, uint8_t *buffer, size_t size)
            : urids(urids),
              evbuf((LV2_Atom_Sequence *)buffer),
              bufferSize(size),
              offset(0)
        {
            this->evbuf->atom.size = sizeof(LV2_Atom_Sequence_Body);
            this->evbuf->atom.type = urids.atom_Sequence;
        }
        void Reset(uint8_t *buffer, size_t size)
        {
            this->evbuf = (LV2_Atom_Sequence*)buffer;

            this->bufferSize = size;
            this->offset = 0;
            this->evbuf->atom.size = sizeof(LV2_Atom_Sequence_Body);
            this->evbuf->atom.type = urids.atom_Sequence;
        }

        class LV2_EvBuf_Iterator
        {
        public:
            Lv2EventBufferWriter *writer;
            size_t offset;
        };

        LV2_EvBuf_Iterator begin() const
        {
            LV2_EvBuf_Iterator iter = {const_cast<Lv2EventBufferWriter*>(this), 0};
            return iter;
        }

        LV2_EvBuf_Iterator end() const
        {
            LV2_EvBuf_Iterator iter = {const_cast<Lv2EventBufferWriter*>(this), padSize(size())};
            return iter;
        }
        bool isValid(const LV2_EvBuf_Iterator &iterator) const
        {
            return iterator.offset < size();
        }

        bool write(
            LV2_EvBuf_Iterator &iterator,
            uint32_t frameOffset,
            uint32_t type,
            size_t size,
            const void*data
            )
        {
            LV2_Atom_Sequence *aseq = evbuf;
            if (bufferSize - sizeof(LV2_Atom) - evbuf->atom.size <
                sizeof(LV2_Atom_Event) + size)
            {
                return false;
            }

            LV2_Atom_Event *aev = (LV2_Atom_Event *)((char *)LV2_ATOM_CONTENTS(LV2_Atom_Sequence, aseq) + iterator.offset);

            aev->time.frames = frameOffset;
            aev->body.type = type;
            aev->body.size = size;
            memcpy(LV2_ATOM_BODY(&aev->body), data, size);

            size = padSize(sizeof(LV2_Atom_Event) + size);
            aseq->atom.size += size;
            iterator.offset += size;

            return true;
        }
        bool writeMidiEvent(
            LV2_EvBuf_Iterator &iterator,
            int32_t frameOffset,
            size_t midiSize,
            void *midiData
            )
        {
            
            write(iterator,frameOffset,urids.midi_Event,midiSize,midiData);
            return false;
        }
    };

} // namespace.