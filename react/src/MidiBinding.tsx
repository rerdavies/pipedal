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



export default class MidiBinding {
    deserialize(input: any) : MidiBinding {
        this.symbol = input.symbol;
        this.bindingType = input.bindingType;
        this.note = input.note;
        this.control = input.control;
        this.minValue = input.minValue;
        this.maxValue = input.maxValue;
        this.linearControlType = input.linearControlType;
        this.switchControlType = input.switchControlType;
        return this;
    }
    static systemBinding(symbol: string): MidiBinding {
        let result = new MidiBinding();
        result.symbol = symbol;
        return result;
    }
    static deserialize_array(input: any): MidiBinding[] {
        let result: MidiBinding[] = [];
        for (let i = 0; i < input.length; ++i)
        {
            result.push(new MidiBinding().deserialize(input[i]));
        }
        return result;

    }
    clone(): MidiBinding { return new MidiBinding().deserialize(this);}
    equals(other: MidiBinding) : boolean
    {
        return (this.symbol === other.symbol)
            && (this.bindingType === other.bindingType)
            && (this.note === other.note)
            && (this.control === other.control)
            && (this.minValue === other.minValue)
            && (this.maxValue === other.maxValue)
            && (this.linearControlType === other.linearControlType)
            && (this.switchControlType === other.switchControlType)
    }

    static  BINDING_TYPE_NONE: number = 0;
    static  BINDING_TYPE_NOTE: number = 1;
    static  BINDING_TYPE_CONTROL: number = 2;

    symbol: string = "";

    bindingType: number = MidiBinding.BINDING_TYPE_NONE;
    note: number = 12*4+24; // C4.
    control: number = 1;
    minValue: number = 0;
    maxValue: number = 1;
    rotaryScale: number = 1;

    static LINEAR_CONTROL_TYPE:number = 0;
    static CIRCULAR_CONTROL_TYPE: number = 1;

    linearControlType: number = MidiBinding.LINEAR_CONTROL_TYPE;

    static LATCH_CONTROL_TYPE: number = 0;
    static MOMENTARY_CONTROL_TYPE: number = 1;

    switchControlType: number = MidiBinding.LATCH_CONTROL_TYPE;

};