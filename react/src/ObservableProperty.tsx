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



export type OnChangedHandler<VALUE_TYPE> = (newValue: VALUE_TYPE) => void;


export class ObservableProperty<VALUE_TYPE> {

    _value_type: VALUE_TYPE;
    _on_changed_handlers: OnChangedHandler<VALUE_TYPE>[] = [];

    constructor(default_value: VALUE_TYPE)
    {
        this._value_type = default_value;
    }

    addOnChangedHandler(handler: OnChangedHandler<VALUE_TYPE> ) : void
    {
        this._on_changed_handlers.push(handler);
        handler(this._value_type);
    }
    removeOnChangedHandler(handler: OnChangedHandler<VALUE_TYPE> ) : void
    {
        let oldArray = this._on_changed_handlers;

        let newArray: OnChangedHandler<VALUE_TYPE>[] = [];

        let outIx = 0;
        for (let i = 0; i < oldArray.length; ++i)
        {
            if (oldArray[i] !== handler)
            {
                newArray[outIx++]  = oldArray[i];
            }
        }
        this._on_changed_handlers = newArray;
    }

    get() : VALUE_TYPE {
        return this._value_type;
    }

    set(value: VALUE_TYPE) : void
    {
        this._value_type = value;

        let t = this._on_changed_handlers; // take an copy in case removes happen while iteratiing.
        t.forEach(c => c(value));
    }

};