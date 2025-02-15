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


import * as React from 'react';
import { Theme } from '@mui/material/styles';
import { WithStyles } from '@mui/styles';
import createStyles from '@mui/styles/createStyles';
import withStyles from '@mui/styles/withStyles';


const styles = (theme: Theme) => createStyles({
    frame: {
        position: "relative",
        margin: "12px"
    }
});


export interface MiniSpinnerProps extends WithStyles<typeof styles> {
        theme: Theme
}

export interface MiniSpinnerProps {
    size?: number;
};


const MiniSpinner = withStyles(styles)((props: MiniSpinnerProps) => {
    const { classes } = props;

    let size: number = 24;
    if (props.size)
    {
        size = props.size;
    }
    let thickness = 3;
    let r = size-thickness/2;
    return (
        <div style={{width:size, height: size}}>
            <svg viewBox={ (size/2) + " " + (size/2) + " " + size + " " + size}> width={size} height={size}
                <circle
                    cx="0" cy="0" r={r} fill="none" stroke={props.theme.palette.primary.main} strokeWidth={thickness}
                 />
            </svg>
        </div>
    );
});
