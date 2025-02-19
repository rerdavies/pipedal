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



export interface ModDirectory {
    modType: string;
    pipedalPath: string;
    displayName: string;
    defaultFileExtensions: string[];
};


export const modDirectories: ModDirectory[] = [
        {modType: "audioloop", pipedalPath: "shared/audio/Loops",displayName:  "Loops", defaultFileExtensions: ["audio/*"]}, // Audio Loops, meant to be used for looper-style plugins
        //"audiorecording","shared/audio/Audio Recordings", defaultFileExtensions: ["audio/*"]}, : Audio Recordings, triggered by plugins and stored in the unit
        {modType: "audiosample", pipedalPath: "shared/audio/Samples", displayName: "Samples", defaultFileExtensions: ["audio/*"]}, // One-shot Audio Samples, meant to be used for sampler-style plugins
        {modType: "audiotrack", pipedalPath: "shared/audio/Tracks",displayName:  "Tracks", defaultFileExtensions: ["audio/*"]},    // Audio Tracks, meant to be used as full-performance/song or backtrack
        {modType: "cabsim", pipedalPath: "CabIR", displayName: "Cab IRs", defaultFileExtensions: ["audio/*"]},                     // Speaker Cabinets, meant as small IR audio files

        /// - h2drumkit: Hydrogen Drumkits, must use h2drumkit file extension
        {modType: "ir", pipedalPath: "ReverbImpulseFiles", displayName: "Impulse Responses", defaultFileExtensions: ["audio/*"]},    // Impulse Responses
        {modType: "midiclip", pipedalPath: "shared/midiClips",displayName:  "MIDI Clips", defaultFileExtensions: [".mid", ".midi"]}, // MIDI Clips, to be used in sync with host tempo, must have mid or midi file extension
        {modType: "midisong", pipedalPath: "shared/midiSongs", displayName: "MIDI Songs", defaultFileExtensions: [".mid", ".midi"]}, // MIDI Songs, meant to be used as full-performance/song or backtrack
        {modType: "sf2", pipedalPath: "shared/sf2",displayName:  "Sound Fonts", defaultFileExtensions: [".sf2", ".sf3"]},              // SF2 Instruments, must have sf2 or sf3 file extension
        {modType: "sfz", pipedalPath: "shared/sfz",displayName:  "Sfz Files", defaultFileExtensions: [".sfz"]},                       // SFZ Instruments, must have sfz file extension
        // extensions observed in the field.
        {modType: "audio", pipedalPath: "shared/audio", displayName: "Audio", defaultFileExtensions: ["audio/*"]},                              // all audio files (Ratatoille)
        {modType: "nammodel", pipedalPath: "NeuralAmpModels", displayName: "Neural Amp Models", defaultFileExtensions: [".nam"]},              // Ratatoille, Mike's NAM.
        {modType: "aidadspmodel", pipedalPath: "shared/aidaaix", displayName: "AIDA IAX Models", defaultFileExtensions: [".json", ".aidaiax"]}, // Ratatoille
        {modType: "mlmodel", pipedalPath: "ToobMlModels", displayName: "ML Models", defaultFileExtensions: [".json"]},                         //
  
];

export function getModDirectory(modFileType: string): ModDirectory | undefined {
    for (let i = 0; i < modDirectories.length; ++i)
    {
        if (modDirectories[i].modType === modFileType)
        {
            return modDirectories[i];
        }
    }
    return undefined;
}