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
// FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO `EVENT` SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
// IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.



export type ObservableEventHandler<VALUE_TYPE> = (newValue: VALUE_TYPE) => void;


export default class ObservableEvent<VALUE_TYPE> {

    private _on_event_handlers: ObservableEventHandler<VALUE_TYPE>[] = [];


    addEventHandler(handler: ObservableEventHandler<VALUE_TYPE> ) : void
    {
        this._on_event_handlers.push(handler);
    }
    removeEventHandler(handler: ObservableEventHandler<VALUE_TYPE> ) : void
    {
        let newArray: ObservableEventHandler<VALUE_TYPE>[] = [];

        for (let myHandler of  this._on_event_handlers)
        {
            if (myHandler !== handler)
            {
                newArray.push(myHandler);
            }
        }
        this._on_event_handlers = newArray;
    }
    fire(value: VALUE_TYPE)
    {
        for (let handler of this._on_event_handlers)
        {
            handler(value);
        }
    }
};