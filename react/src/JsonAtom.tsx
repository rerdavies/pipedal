// Copyright (c) 2023 Robin Davies
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


// Utility class for constructing Json atoms.
class JsonAtom {
    static Property(value: string): any
    {
        return {
            "otype_": "Property",
            "value": value
        };
    }
    static Bool(value: boolean): any
    {
        return {
            "otype_": "Bool",
            "value": value
        };
    }
    static Int(value: number): any
    {
        return {
            "otype_": "Int",
            "value": value
        };
    }
    static Long(value: number): any
    {
        return {
            "otype_": "Long",
            "value": value
        };
    }
    static Float(value: number): any
    {
        return value;
    }
    static Double(value: number): any
    {
        return {
            "otype_": "Double",
            "value": value
        };
    }
    static String(value: string): any
    {
        return value;
    }
    static Path(value: string): any
    {
        return {
            "otype_": "Path",
            "value": value
        };
    }
    static Uri(value: string): any
    {
        return {
            "otype_": "URI",
            "value": value
        };
    }
    static Urid(value: string): any
    {
        return {
            "otype_": "URID",
            "value": value
        };
    }
    static Object(type: string): any
    {
        return {
            "otype_": type
        };
    }
    static Tuple(values: any[]): any
    {
        return {
            "otype_": "Tuple",
            "value": values
        };
    }
    static IntVector(values: []): any
    {
        return {
            "otype_": "Vector",
            "vtype_": "Int",
            "value": values
        };
    }
    static LongVector(values: []): any
    {
        return {
            "otype_": "Vector",
            "vtype_": "Long",
            "value": values
        };
    }

    static FloatVector(values: []): any
    {
        return {
            "otype_": "Vector",
            "vtype_": "Float",
            "value": values
        };
    }
    static DoubleVector(values: []): any
    {
        return {
            "otype_": "Vector",
            "vtype_": "Float",
            "value": values
        };
    }


};

export default JsonAtom;
