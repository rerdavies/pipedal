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

export class BankIndexEntry {
    deserialize(input: any) : BankIndexEntry {
        this.instanceId = input.instanceId;
        this.name = input.name;
        return this; 

    }

    areEqual(other: BankIndexEntry) : boolean {
        return (this.instanceId === other.instanceId)
            && (this.name === other.name);
    }
    static deserialize_array(input: any): BankIndexEntry[] {
        let result: BankIndexEntry[] = [];
        for (let i = 0; i < input.length; ++i)
        {
            result.push(new BankIndexEntry().deserialize(input[i]));
        }
        return result;

    }
    instanceId: number = -1;
    name: string = "";
}

export class BankIndex {
    deserialize(input: any) : BankIndex {
        this.selectedBank = input.selectedBank;
        this.entries = BankIndexEntry.deserialize_array(input.entries);
        return this;
    }   
    selectedBank: number = -1;
    entries: BankIndexEntry[] = [];

    areEqual(other: BankIndex, includeSelection: boolean): boolean {
        if (includeSelection && this.selectedBank !== other.selectedBank) return false;

        if (this.entries.length !== other.entries.length) return false;

        for (let i = 0; i < this.entries.length; ++i)
        {
            if (!this.entries[i].areEqual(other.entries[i])) return false;
        }

        
        return true;
        
    }

    moveBank(from: number, to: number)
    {
        let t = this.entries[from];
        this.entries.splice(from,1);
        this.entries.splice(to,0,t);

    }

    clone(): BankIndex {
        return new BankIndex().deserialize(this);
    }

    hasName(name: string): boolean {
        for (let i = 0; i < this.entries.length; ++i)
        {
            let entry = this.entries[i];
            if (entry.name === name) {
                return true;
            }
        }
        return false;
    }
    getEntry(instanceId: number): BankIndexEntry | null {
        for (let i = 0; i < this.entries.length; ++i)
        {
            let entry = this.entries[i];
            if (entry.instanceId === instanceId) return entry;
        }
        return null;
    }
    getSelectedEntryName(): string {
        let entry = this.getEntry(this.selectedBank);
        if (!entry) return "";
        return entry.name;
    }
    nameExists(name: string): boolean {
        for (let i = 0; i < this.entries.length; ++i)
        {
            let entry = this.entries[i];
            if (entry.name === name) return true;
        }
        return false;

    }
}