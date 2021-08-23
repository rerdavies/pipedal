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

const LARGE_NUMBER = 0x1FFFFFFF;
const EMPTY_WIDTH = -LARGE_NUMBER * 2;

class Rect {
    x: number;
    y: number;
    width: number;
    height: number;

    constructor(x: number = LARGE_NUMBER, y: number = LARGE_NUMBER, width: number = EMPTY_WIDTH, height: number = EMPTY_WIDTH)
    {
        this.x = x; this.y = y; this.width = width; this.height = height;
    }

    toString() { return '{' + this.x + "," + this.y + " " + this.width + "," + this.height +"}"; }
    copy(): Rect {
        let result = new Rect();
        result.x = this.x;
        result.y = this.y;
        result.width = this.width;
        result.height = this.height;
        return result;

    }
    contains(x: number, y: number)
    {
        return (x >= this.x)
        && x < this.x+this.width
        && y >= this.y 
        && y < this.y + this.height;
    }
    get right(): number {
        return this.x + this.width;
    }
    get bottom(): number {
        return this.y + this.height;
    }
    isEmpty(): boolean {
        return this.width === 0 || this.height === 0;
    }
    clear(): void {
        this.x = LARGE_NUMBER;
        this.y = LARGE_NUMBER;
        this.width = EMPTY_WIDTH;
        this.height = EMPTY_WIDTH;

    }
    accumulate(rect: Rect): void {
        let right = Math.max(rect.x + rect.width, this.x + this.width);
        let bottom = Math.max(rect.y + rect.height, this.y + this.height);
        this.x = Math.min(rect.x, this.x);
        this.y = Math.min(rect.y, this.y);
        this.width = right - this.x;
        this.height = bottom - this.y;
    }
    union(rect: Rect): Rect {
        let result = new Rect();
        result.x = Math.min(this.x, rect.x);
        result.y = Math.min(this.y, rect.y);
        result.width = Math.max(this.x + this.width, rect.x + rect.width) - result.x;
        result.height = Math.max(this.y + this.height, rect.x + rect.height) - result.y;
        return result;
    }
    offset(xOffset: number, yOffset: number) {
        this.x += xOffset;
        this.y += yOffset;
    }
}

export default Rect;