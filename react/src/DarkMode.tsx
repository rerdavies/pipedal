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

export enum ColorTheme {
    Light,
    Dark,
    System
};



export function getColorScheme(): ColorTheme {
    const androidHosted = !!((window as any).AndroidHost);

    switch (localStorage.getItem('colorScheme')) {
        case null:
        default:
            if (androidHosted) {
                return ColorTheme.System;
            } else {
                return ColorTheme.Light;
            }
        case "Light":
            return ColorTheme.Light;
        case "Dark":
            return ColorTheme.Dark;
        case "System":
            return ColorTheme.System;
    }
}
export function setColorScheme(value: ColorTheme): void {
    var storageValue;
    switch (value) {
        default:
        case ColorTheme.Light:
            storageValue = "Light";
            break;
        case ColorTheme.Dark:
            storageValue = "Dark";
            break;

        case ColorTheme.System:
            storageValue = "System";
            break;
    }
    localStorage.setItem("colorScheme", storageValue);
}

export function isDarkMode(): boolean {
    var colorTheme = getColorScheme();
    if (colorTheme === ColorTheme.System) {
        if (window.matchMedia && window.matchMedia('(prefers-color-scheme: dark)').matches) {
            return true;
        }
    }
    return colorTheme === ColorTheme.Dark;
}

