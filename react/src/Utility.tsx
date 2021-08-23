// Copyright (c) 2021 Robin Davies
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

import { PiPedalArgumentError } from './PiPedalError';

export function nullCast<T>(value: T | null | undefined): T {
    if (!value) throw new PiPedalArgumentError("Unexpected null value.");
    return value;
}


const FLAT = "\u266D";


const LONG_PRESS_TIME_MS = 500; // current android long-press.

export function getLongPressTimeMs(): number { return LONG_PRESS_TIME_MS; }

export interface ControlEntry {
    value: number;
    displayName: string;
};

const Utility = class {
    static isTouchDevice(): boolean {
        return navigator.maxTouchPoints > 0;
    }

    static hasIMEKeyboard(): boolean {
        if (/Mobi|Android/i.test(navigator.userAgent)) {
            return true;
        }
        return false;
    }

    static delay(ms: number): Promise<void> {
        return new Promise<void>(resolve => setTimeout(resolve, ms));
    }


    static NOTE_DEGREES: string[] =
        [
            "C", "C#", "D", "E" + FLAT, "E", "F", "F#", "G", "A" + FLAT, "A", "B" + FLAT, "B"
        ];
    static midiNoteName(noteNumber: number): string {
        let n = noteNumber - 24;
        let octave = Math.floor(n / 12);
        let iNote = n % 12;
        if (iNote < 0) iNote += 12;

        return Utility.NOTE_DEGREES[iNote] + octave;


    }

    static validMidiControllers: ControlEntry[] = [
        { value: 1, displayName: "CC-1 Mod Wheel" },
        { value: 2, displayName: "CC-2 Breath controller" },
        { value: 3, displayName: "CC-3" },
        { value: 4, displayName: "CC-4 Foot Pedal" },
        { value: 5, displayName: "CC-5 Portamento Time" },
        { value: 7, displayName: "CC-7 Volume" },
        { value: 8, displayName: "CC-8 Balance" },
        { value: 10, displayName: "CC-10 Pan position" },
        { value: 11, displayName: "CC-11 Expression" },
        { value: 12, displayName: "CC-12 Effect Control 1" },
        { value: 13, displayName: "CC-13 Effect Control 2" },
        { value: 14, displayName: "CC-14" },
        { value: 15, displayName: "CC-15" },
        { value: 16, displayName: "CC-16" },
        { value: 17, displayName: "CC-17" },
        { value: 18, displayName: "CC-18" },
        { value: 19, displayName: "CC-19" },
        { value: 20, displayName: "CC-20" },
        { value: 21, displayName: "CC-21" },
        { value: 22, displayName: "CC-22" },
        { value: 23, displayName: "CC-23" },
        { value: 24, displayName: "CC-24" },
        { value: 25, displayName: "CC-25" },
        { value: 26, displayName: "CC-26" },
        { value: 27, displayName: "CC-27" },
        { value: 28, displayName: "CC-28" },
        { value: 29, displayName: "CC-29" },
        { value: 30, displayName: "CC-30" },
        { value: 31, displayName: "CC-31" },
        // Control LSB.
        { value: 64, displayName: "CC-64 Hold Pedal" },
        { value: 65, displayName: "CC-65 Portamento" },
        { value: 66, displayName: "CC-66 Sustenuto Pedal" },
        { value: 67, displayName: "CC-67 Soft Pedal" },
        { value: 68, displayName: "CC-68 Legato Pedal" },
        { value: 69, displayName: "CC-69 Hold 2 Pedal" },
        { value: 70, displayName: "CC-70 Sound Variation" },
        { value: 71, displayName: "CC-71 Resonance" },
        { value: 72, displayName: "CC-72 Sound Release Time" },
        { value: 73, displayName: "CC-73 Sound Attack Time" },
        { value: 74, displayName: "CC-74 Frequency Cutoff" },
        { value: 75, displayName: "CC-75" },
        { value: 76, displayName: "CC-76" },
        { value: 77, displayName: "CC-77" },
        { value: 78, displayName: "CC-78" },
        { value: 79, displayName: "CC-79" },
        { value: 80, displayName: "CC-80 Decay" },
        { value: 81, displayName: "CC-81" },
        { value: 82, displayName: "CC-82" },
        { value: 83, displayName: "CC-83" },
        { value: 91, displayName: "CC-91 Reverb Level" },
        { value: 92, displayName: "CC-92 Tremolo Level" },
        { value: 93, displayName: "CC-93 Chorus Level" },
        { value: 94, displayName: "CC-94 Detune" },
        { value: 95, displayName: "CC-95 Phaser Level" },
        // RPN/DATA controls
        { value: 102, displayName: "CC-102" },
        { value: 103, displayName: "CC-103" },
        { value: 104, displayName: "CC-104" },
        { value: 105, displayName: "CC-105" },
        { value: 106, displayName: "CC-106" },
        { value: 107, displayName: "CC-107" },
        { value: 108, displayName: "CC-108" },
        { value: 109, displayName: "CC-109" },
        { value: 110, displayName: "CC-110" },
        { value: 111, displayName: "CC-111" },
        { value: 112, displayName: "CC-112" },
        { value: 113, displayName: "CC-113" },
        { value: 114, displayName: "CC-114" },
        { value: 115, displayName: "CC-115" },
        { value: 116, displayName: "CC-116" },
        { value: 117, displayName: "CC-117" },
        { value: 118, displayName: "CC-118" },
        { value: 119, displayName: "CC-119" },
        { value: 120, displayName: "CC-120 All Sound Off" },
        { value: 121, displayName: "CC-121 All Controllers Off" },
        { value: 122, displayName: "CC-122 Local Keyboard" },
        { value: 123, displayName: "CC-123 All Notes Off" },
        { value: 124, displayName: "CC-124 Omni Mode Off" },
        { value: 125, displayName: "CC-125 Omni Mode On" },
        { value: 126, displayName: "CC-126 Mono " },
        { value: 127, displayName: "CC-127 Poly" },
    ];

};

export default Utility;
