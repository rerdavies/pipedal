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

    static _Bool(value: boolean): any
    {
        return new JsonAtom({
            "otype_": "Bool",
            "value": value
        });
    }
    isBool(): boolean
    {
        return this.isObject() && this.json.otype_ === "Bool";
    }
    asBool(): boolean {
        if (this.isBool()) {
            return this.json.value as boolean;
        }
        throw new Error("JsonAtom is not a Bool");
    }
    static _Int(value: number): JsonAtom
    {
        return new JsonAtom({
            "otype_": "Int",
            "value": value
        });
    }
    isInt(): boolean
    {
        return this.isObject() && this.json.otype_ === "Int";
    }
    asInt(): number {
        if (this.isInt()) {
            return this.json.value as number;;
        }
        throw new Error("JsonAtom is not an Int");
    }
    static _Long(value: number): JsonAtom
    {
        return new JsonAtom({
            "otype_": "Long",
            "value": value
        });
    }

    isLong(): boolean
    {
        return this.isObject() && this.json.otype_ === "Long";
    }
    asLong(): number {
        if (this.isLong()) {
            return this.json.value as number;
        }
        throw new Error("JsonAtom is not a Long");
    }
    static _Float(value: number): JsonAtom
    {
        return new JsonAtom(value);
    }
    static _Double(value: number): JsonAtom
    {
        return new JsonAtom({
            "otype_": "Double",
            "value": value
        });
    }
    static _String(value: string): JsonAtom
    {
        return new JsonAtom(value);
    }
    isString() : boolean
    {
        return this.json && typeof this.json === 'string';
    }
    asString(): string {
        if (this.isString()) {
            return this.json as string;
        }
        throw new Error("JsonAtom is not a String");
    }

    static _Path(value: string): JsonAtom
    {
        return new JsonAtom({
            "otype_": "Path",
            "value": value
        });
    }
    isPath(): boolean
    {
        return this.isObject() && this.json.otype_ === "Path";
    }
    asPath(): string {
        if (this.isPath()) {
            return this.json.value as string;
        }
        throw new Error("JsonAtom is not a Path");
    }
    static _Uri(value: string): JsonAtom
    {
        return new JsonAtom({
            "otype_": "URI",
            "value": value
        });
    }
    isUri(): boolean
    {
        return this.isObject() && this.json.otype_ === "URI";
    }
    asUri(): string {
        if (this.isUri()) {
            return this.json.value as string;
        }
        throw new Error("JsonAtom is not a URI");
    }   
    static _Urid(value: string): JsonAtom
    {
        return new JsonAtom({
            "otype_": "URID",
            "value": value
        });
    }
    isUrid(): boolean
    {
        return this.isObject() && this.json.otype_ === "URID";
    }
    asUrid(): string {
        if (this.isUrid()) {
            return this.json.value as string;
        }
        throw new Error("JsonAtom is not a URID");
    }
    static _Object(type: string): JsonAtom
    {
        return new JsonAtom({
            "otype_": type
        });
    }
    isObject(type?: string): boolean
    {
        if (this.json && typeof this.json === 'object' && !Array.isArray(this.json)) {
            if (!type) return true;
            return type === this.json.otype_;
        }
        return false;
    }
    asObject(type?: string): Object {
        if (this.isObject(type)) {
            return this.json as Object;
        }
        throw new Error(`JsonAtom is not an Object of type ${type}`);
    }   
    static _Tuple(values: any[]): JsonAtom
    {
        return new JsonAtom({
            "otype_": "Tuple",
            "value": values
        });
    }
    static _IntVector(values: []): JsonAtom
    {
        return new JsonAtom({
            "otype_": "Vector",
            "vtype_": "Int",
            "value": values
        });
    }
    static _LongVector(values: []): JsonAtom
    {
        return new JsonAtom({
            "otype_": "Vector",
            "vtype_": "Long",
            "value": values
        });
    }

    static _FloatVector(values: []): JsonAtom
    {
        return new JsonAtom({
            "otype_": "Vector",
            "vtype_": "Float",
            "value": values
        });
    }
    static _DoubleVector(values: []): JsonAtom
    {
        return new JsonAtom({
            "otype_": "Vector",
            "vtype_": "Float",
            "value": values
        });
    }

    constructor(json: any) {
        this.json = json;
    }
    isArray(): boolean {
        return this.json && Array.isArray(this.json);
    }
    isFloat(): boolean {
        return this.json && typeof this.json === 'number';
    }
    isNull(): boolean {
        return this.json === null;
    }
    asAny(): any {
        return this.json;
    }

    private json: any;
};

export default JsonAtom;
