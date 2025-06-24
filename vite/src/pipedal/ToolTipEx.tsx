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
    valueTooltip?: React.ReactNode; // For use with value prop, to show a tooltip on the value.
}

/* ToolTipEx: a reimplementation of the MUI Tooltip that works with pointer events and long presses. */

function ToolTipEx(props: ToolTipExProps) {
    let {title, valueTooltip, ...extras} = props;
    let [open,setOpen]= React.useState(false);
    let [isLongPress, setIsLongPress] = React.useState(false);
    let [longPressLeaving, setLongPressLeaving] = React.useState(false);
    let [timeout, setTimeout] = React.useState<number>(0);
    let [timeoutInstance, setTimeoutInstance] = React.useState<number>(0);
    let [pointerDownPoint, setPointerdownPoint] = React.useState<{x: number, y: number} | null>(null);

    const hoverTimeout = 1250;
    const longpressTimeout = 500;
    const longpressLeaveTimeout = 1500;

    function startTimeout(timeout: number) {
        setTimeout(timeout);
        setTimeoutInstance(timeoutInstance + 1); // make useeffect run.
    }
    function stopTimeout() {
        if (timeout > 0) {
            startTimeout(0);
        }
    }
    function handleMouseEnter(event: React.MouseEvent<HTMLDivElement>) {
        setOpen(false);
        startTimeout(hoverTimeout);
    }

    function handleMouseLeave(event: React.MouseEvent<HTMLDivElement>) {
        setOpen(false);
        stopTimeout();
    }   

    function handlePointerDownCapture(event: React.PointerEvent<HTMLDivElement>) {
        let pointerType = (event as any).pointerType || "n/a";
        setIsLongPress(false);
        if (pointerType === "mouse") {
            setOpen(false);
            stopTimeout(); // Reset hover timeout for mouse
        } else { // pen, or touch.
            setTimeout(longpressTimeout);
            setOpen(false);
            setPointerdownPoint({x: event.clientX, y: event.clientY});
        }
    }

    React.useEffect(()=> {
        let t = timeout;
        let handle: number | null = null;
        if (valueTooltip === undefined)  // no timeout if there's a value tooltip
        {
            if (t > 0) {
                // console.log("ToolTipEx: starting timeout for ", t);
                handle = window.setTimeout(() => {
                    setOpen(true);
                },t);
            }
        }
        return () => {
            if (handle !== null) {
                // console.log("ToolTipEx: clearing timeout for ", t);
                window.clearTimeout(handle);
            }
        };
    },[timeoutInstance]);

    React.useEffect(()=> {
        if (longPressLeaving) {
            let handle: number | null = null;
            handle = window.setTimeout(() => {
                setIsLongPress(false);
                setLongPressLeaving(false);
            }, longpressLeaveTimeout);
            return () => {
                if (handle !== null) {
                    window.clearTimeout(handle);
                }
            };
        } else {
            return () => { };
        }
    },[longPressLeaving]);

    function handlePointerCancel(event: React.PointerEvent<HTMLDivElement>) {
        setOpen(false);
        setIsLongPress(false);
        stopTimeout();
        setPointerdownPoint(null); // Reset the mouse down point
    }
    function handlePointerMoveCapture(event: React.PointerEvent<HTMLDivElement>) {
        let pointerType = (event as any).pointerType || "n/a";
        if (pointerType === "mouse") {
            setOpen(false);
            startTimeout(hoverTimeout); // Reset hover timeout for mouse
        } else { // pen, or touch.
            if (pointerDownPoint) {
                const dx = event.clientX - pointerDownPoint.x;
                const dy = event.clientY - pointerDownPoint.y;
                if (Math.abs(dx) > 5 || Math.abs(dy) > 5) {
                    // If the mouse has moved more than 5 pixels, re-enable the tooltip
                    setOpen(false);
                    setIsLongPress(false);
                    stopTimeout();
                    setPointerdownPoint(null); // Reset the mouse down point
                }
            }
        }
    }
    function handleConextMenu(event: React.MouseEvent<HTMLDivElement>) {
        event.preventDefault(); // Prevent the default context menu from appearing
        event.stopPropagation(); // Prevent the default context menu from appearing
        if (pointerDownPoint) // touch sequence. This is a long press.
        {
            setIsLongPress(true);
            setOpen(true);
            stopTimeout();
        }
    } // Reset hover timeout for context menu
    function handlePointerUpCapture(event: React.PointerEvent<HTMLDivElement>) {
        let pointerType = (event as any).pointerType || "n/a";
        if (pointerType === "mouse") {
            setOpen(false);
            stopTimeout(); // Reset hover timeout for mouse
        } else { // pen, or touch.
            stopTimeout();
            setOpen(false);
            if (isLongPress) {
                // If this is a long press, we want to leave the tooltip open for a while.
                setLongPressLeaving(true);
            } 
        }
    }
    let effectiveTitle: React.ReactNode | null = null;
    let placement: "top-start" | "right" = "top-start";
    if (valueTooltip !== undefined && !isLongPress) {
        effectiveTitle = valueTooltip; // Show value tooltip if available
        placement = "right";
    }
    else {
        if (open || isLongPress) {
            effectiveTitle = title; // Show regular title if no value tooltip
        }
    }
    function handleClickCapture(event: React.MouseEvent<HTMLDivElement>) {
        setLongPressLeaving(false);
        setIsLongPress(false);
        setOpen(false);
        stopTimeout(); // Reset hover timeout on click
    }
    return (
        <div 
            onClickCapture={(e) => {
                // console.log("ToolTipEx: onClickCapture");
                handleClickCapture(e);
            }}

            onMouseEnter={(e)=> {
                // console.log("ToolTipEx: onMouseEnter");

                handleMouseEnter(e);
            }}
            onMouseLeave={(e)=> {
                // console.log("ToolTipEx: onMouseLeave");
                handleMouseLeave(e);
            }}

            onPointerCancelCapture={(e) => {
                // console.log("ToolTipEx: onPointerCancelCapture");
                handlePointerCancel(e);
            }}
            onContextMenuCapture={(e) => {
                // console.log("ToolTipEx: onContextMenuCapture");
                handleConextMenu(e);
                return false;
            }}
            onPointerDownCapture={(e) => {
                // console.log("ToolTipEx: onPointerDownCapture");
                handlePointerDownCapture(e);
            }}
            onPointerMoveCapture={(e) => {
                // console.log("ToolTipEx: onPointerMoveCapture");
                handlePointerMoveCapture(e);
            }}
            onPointerUpCapture={(e) => {
                // console.log("ToolTipEx: onPointerUpCapture");
                handlePointerUpCapture(e);
            }}
        >
            <Tooltip {...extras}
                open={open || isLongPress || valueTooltip !== undefined}
                disableInteractive={true}
                disableTouchListener={true}
                disableHoverListener={true}
                disableFocusListener={true}

                title={effectiveTitle}
                placement={ placement}  
                arrow enterDelay={1500} enterNextDelay={1500} 
                slotProps={{
                    transition: {
                        timeout: 0 // Disable transition for the tooltip
                    }
                }}
            />
        </div>
    )
}

export default ToolTipEx;