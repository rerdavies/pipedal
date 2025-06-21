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
import Tooltip from '@mui/material/Tooltip';

export interface ToolTipExProps extends React.ComponentProps<typeof Tooltip> {
}

function ToolTipEx(props: ToolTipExProps) {
    let [disableToolTip, setDisableToolTip] = React.useState(false);
    let {title, ...extras} = props;
    function handleMouseDownCapture(event: React.PointerEvent<HTMLDivElement>) {
        if ((event as any).pointerType === "mouse") {
            setDisableToolTip(true);
        }
    }

    function handlePointerMoveCapture(event: React.PointerEvent<HTMLDivElement>) {
        if ((event as any).pointerType !== "mouse") {
            setDisableToolTip(false);
        }
    }
    return (
        <div 
            onPointerDownCapture={(e) => {
                handleMouseDownCapture(e);
            }}
            onPointerMoveCapture={(e) => {
                handlePointerMoveCapture(e);
            }}
            onMouseLeave={() => {
                setDisableToolTip(false);
            }
            }
        >
            <Tooltip {...extras}
                title={disableToolTip? undefined: title}
                placement="top-start" arrow enterDelay={1500} enterNextDelay={1500}

            />
        </div>
    )
}

export default ToolTipEx;