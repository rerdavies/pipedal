/*
 * MIT License
 * 
 * Copyright (c) 2024 Robin E. R. Davies
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

const darkcolors: string[] = 
[
        "#b71C1C","#880E4F","#4A148c","#311B92","#1A237E","#0D47A1","#01579B","#006064","#004D40"
];

const lighcolors: string[] = [
        "#FFcDD2","#F8BBd0","#E1BEE7","#D1c4E9","#C5CAE9","#BBDEFB","#B3E5FC","#B2EbF2","B2DFDB"
];
export function getLightSnapshotcolors() {
    let result:string[]  = [];
    for (let h = 0; h < 360;  h += 30)
    {
        result.push("hsl(" + h + ",100%,79%)");
    }
    return result;
}

export function getDarkSnapshotColors() {
    let result:string[]  = [];
    for (let h = 0; h < 360;  h += 30)
    {
        result.push("hsl(" + h + ",90%,20%)");
    }
    return result;
}

export function getSnapshotColors() {
    return getDarkSnapshotColors();
}

