/*
 *   Copyright (c) 2025 Robin E. R. Davies
 *   All rights reserved.

 *   Permission is hereby granted, free of charge, to any person obtaining a copy
 *   of this software and associated documentation files (the "Software"), to deal
 *   in the Software without restriction, including without limitation the rights
 *   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *   copies of the Software, and to permit persons to whom the Software is
 *   furnished to do so, subject to the following conditions:
 
 *   The above copyright notice and this permission notice shall be included in all
 *   copies or substantial portions of the Software.
 
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *   SOFTWARE.
 */


export function pathExtension(path: string) {
    let dotPos = path.lastIndexOf('.');
    if (dotPos === -1) return "";

    let slashPos = path.lastIndexOf('/');
    if (slashPos !== -1) {
        if (dotPos <= slashPos + 1) return "";
    }
    return path.substring(dotPos); // include the '.'.

}
export function pathParentDirectory(path: string) {
    let npos = path.lastIndexOf('/');
    if (npos === -1) return "";
    return path.substring(0, npos);
}
export function pathConcat(left: string, right: string) {
    if (left === "") return right;
    if (right === "") return left;
    if (left.endsWith('/')) {
        left = left.substring(0, left.length - 1);
    }
    if (right.startsWith("/")) {
        right = right.substring(1);
    }
    return left + "/" + right;
}
export function pathFileNameOnly(path: string): string {
    if (path === "..") return path;
    let slashPos = path.lastIndexOf('/');
    if (slashPos < 0) {
        slashPos = 0;
    } else {
        ++slashPos;
    }
    let extPos = path.lastIndexOf('.');
    if (extPos < 0 || extPos < slashPos) {
        extPos = path.length;
    }

    return path.substring(slashPos, extPos);
}

export function pathFileName(path: string): string {
    if (path === "..") return path;
    let slashPos = path.lastIndexOf('/');
    if (slashPos < 0) {
        slashPos = 0;
    } else {
        ++slashPos;
    }
    return path.substring(slashPos);
}



