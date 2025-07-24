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

import React from 'react';

import { Theme, useTheme } from '@mui/material/styles';

export function createStyles<T>(style: T): T {
    return style;
}

export default
    interface WithStyles<T extends (...args: unknown[]) => unknown> {
    className?: string;
    classes?: Partial<Record<keyof ReturnType<T>, string>>;

}

export interface WithTheme {
    theme: Theme;
}


// export function XXwithTheme(Component: React.ComponentType<WithTheme<any>>) {
//     return function <T extends { theme: Theme }>(
//         Component: React.ComponentType<T>
//     ) {
//         return function (props: Omit<T, "theme">): React.JSX.Element {

//             const theme = useTheme();
//             const newProps = { ...props, theme: theme } as T;
//             return <Component { ...newProps} />
//         }
//     };
// };




export function withTheme<PROPS extends { theme: Theme }>(Component: React.ComponentType<PROPS>) {
    return function (props: Omit<PROPS, "theme">): React.JSX.Element {

        const theme = useTheme();
        const newProps = {theme: theme, ...props} as PROPS;
        return React.createElement<PROPS>(Component,newProps);
    }
};


    