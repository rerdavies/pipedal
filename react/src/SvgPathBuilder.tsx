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

import StringBuilder from './StringBuilder';

class SvgPathBuilder {
    _sb: StringBuilder = new StringBuilder();

    moveTo(x: number, y: number): SvgPathBuilder
    {
        this._sb.append('M').append(x).append(',').append(y).append(' ');
        return this;
    }
    lineTo(x: number, y: number): SvgPathBuilder
    {
        this._sb.append('L').append(x).append(',').append(y).append(' ');
        return this;
    }
    bezierTo(x1: number, y1: number, x2: number, y2: number, x: number, y: number)
    {
        this._sb.append('C').append(x1).append(',').append(y1)
            .append(' ').append(x2).append(',').append(y2)
            .append(' ').append(x).append(',').append(y).append(' ');

    }
    closePath(): SvgPathBuilder {
        this._sb.append("Z ");
        return this;

    }

    toString(): string { return this._sb.toString(); }
}

export default SvgPathBuilder;