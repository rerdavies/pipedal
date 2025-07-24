// Copyright (c) 2025 Robin E. R. Davies
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

interface AlsaSequencerPortSelectionJson {
    id: string;
    name: string;
    sortOrder: number;
}

export class AlsaSequencerPortSelection {
    deserialize(json: AlsaSequencerPortSelectionJson) {
        this.id = json.id;
        this.name = json.name;
        this.sortOrder = json.sortOrder;
        return this;
    };
    static deserialize_array(input: AlsaSequencerPortSelectionJson[]): AlsaSequencerPortSelection[] {
        const result: AlsaSequencerPortSelection[] = [];
        for (let i = 0; i < input.length; ++i) {
            result[i] = new AlsaSequencerPortSelection().deserialize(input[i]);
        }
        return result;
    }
    id: string = "";
    name: string = "";
    sortOrder: number = 0;
};

interface AlsaSequencerConfigurationJson {
    connections: AlsaSequencerPortSelectionJson[];
}

export class AlsaSequencerConfiguration {
    deserialize(input: AlsaSequencerConfigurationJson) {
        this.connections = AlsaSequencerPortSelection.deserialize_array(input.connections);
        return this;
    }
     deserialize_array(input: AlsaSequencerConfigurationJson[]): AlsaSequencerConfiguration[] {
        const result: AlsaSequencerConfiguration[] = [];
        for (let i = 0; i < input.length; ++i) {
            result[i] = new AlsaSequencerConfiguration().deserialize(input[i]);
        }
        return result;
    }

    connections: AlsaSequencerPortSelection[] = [];
};
