#include "pch.h"
#include "Lv2EventBufferWriter.hpp"
#include "Lv2Host.hpp"


using namespace pipedal;


Lv2EventBufferUrids::Lv2EventBufferUrids(IHost *pHost)
{
    atom_Chunk = pHost->GetLv2Urid(LV2_ATOM__Chunk);
    atom_Sequence = pHost->GetLv2Urid(LV2_ATOM__Sequence);
    midi_Event = pHost->GetLv2Urid(LV2_MIDI__MidiEvent);
}

