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

import React, { useEffect } from 'react';
import ButtonBase, { ButtonBaseProps } from "@mui/material/ButtonBase";

export interface DraggableButtonBaseProps extends ButtonBaseProps {
    onClick?: (e: React.MouseEvent<HTMLButtonElement>) => void;
    onLongPressStart?: (currentTarget: HTMLButtonElement, e: React.PointerEvent<HTMLButtonElement>) => boolean;
    onLongPressMove?: (e: React.PointerEvent<HTMLButtonElement>) => void;
    onLongPressEnd?: (e: React.PointerEvent<HTMLButtonElement>) => void;
    longPressDelay? : number;
}

interface Point {
    x: number;
    y: number;
};

function isValidPointer(e: React.PointerEvent): boolean {
    if (e.pointerType === "mouse") {
        return e.button === 0;
    } else if (e.pointerType === "pen") {
        return true;
    } else if (e.pointerType === "touch") {
        return true;
    }
    return false;
}

function screenToClient(e: React.PointerEvent): Point {
    let element = e.currentTarget;
    let rect = element.getBoundingClientRect();
    const x = e.clientX + rect.left;
    const y = e.clientY + rect.right;
    return { x: x, y: y };
}

export default function DraggableButtonBase(props: DraggableButtonBaseProps) {
    // Ensure that the props are spread correctly
    const { onClick, onLongPressStart, onLongPressEnd,longPressDelay, ...rest } = props;
    
    let [hTimeout, setHTimeout] = React.useState<number | null>(null);
    let [pointerId, setPointerId] = React.useState<number | null>(null);
    let [longPressed, setLongPressed] = React.useState<boolean>(false);
    let [clickSuppressed, setClickSuppressed] = React.useState<boolean>(false);
    let [pointerDownPoint, setPointerDownPoint] = React.useState<Point >({x: 0, y: 0});


    function handleSuppressClick(e: MouseEvent) {
            e.stopPropagation();
            e.preventDefault();
            setSuppressClick(false);
    }

    function setSuppressClick(value: boolean) {
        
        if (value !== clickSuppressed) {
            setClickSuppressed(value);
            if (value) {
                window.addEventListener(
                    "click",
                    handleSuppressClick, 
                    { 
                        once: true,
                        capture: true
                    });
            } else {
                window.removeEventListener("click", handleSuppressClick);
            }
        }
    }

    function cancelLongPress() {
        if (hTimeout !== null) {
            window.clearTimeout(hTimeout);
            setHTimeout(null);
        }
        setPointerId(null);
        setLongPressed(false);
        setSuppressClick(false);
    }


    useEffect(() => {
        return () => {
            setSuppressClick(false);
            cancelLongPress();
        };
    }, []);

    return (
        <ButtonBase
            {...rest}
            className="draggable-button-base"
            onContextMenu={(e) => {
                e.preventDefault();
                e.stopPropagation();
                return false;
            }}
            onPointerDown={(e) => {
                if (!isValidPointer(e)) {
                    return;
                }
                if (pointerId !== null) {
                    return; //tracking another pointer already.
                }
                e.currentTarget.setPointerCapture(e.pointerId);
                setPointerId(e.pointerId);
                setPointerDownPoint(screenToClient(e));

                let currentTarget = e.currentTarget as HTMLButtonElement;
                if (longPressDelay === undefined || longPressDelay >= 0)
                {
                    setHTimeout(window.setTimeout(() => {
                        setHTimeout(null);
                        if (props.onLongPressStart) {
                            if (!props.onLongPressStart(currentTarget,e))
                            {
                                return;
                            }
                        }
                        setLongPressed(true);
                        setSuppressClick(true);
                    }, longPressDelay??1250)); 
                }
                e.preventDefault();
                e.stopPropagation();
                return false;
            }}
            onPointerMove={(e) => {
                if (e.pointerId === pointerId) {
                    if(longPressed) {
                        if (props.onLongPressMove) {
                            props.onLongPressMove(e);
                        }
                    } else {
                        let clientPoint = screenToClient(e);

                        let dx = pointerDownPoint.x- clientPoint.x;
                        let dy = pointerDownPoint.y - clientPoint.y;
                        if (Math.abs(dx) > 5 || Math.abs(dy) > 5) {
                            cancelLongPress();
                        }
                    }
                }
                e.preventDefault();
                e.stopPropagation();
                return false;
            }}
            onPointerUp={(e) => {
                if (e.pointerId === pointerId) {
                    e.currentTarget.releasePointerCapture(e.pointerId);
                    setPointerId(null);
                    if (longPressed) {
                        if (props.onLongPressEnd) {
                            props.onLongPressEnd(e);
                        }
                    }
                    cancelLongPress();
                    if (e.isPropagationStopped()) {
                        setSuppressClick(true); // only way to cancel the click
                        return;
                    }
                    e.preventDefault();
                    e.stopPropagation();

                }
            }}
            // onTouchStart={(e)=> {
            //     //e.preventDefault();
            // }}
            // onTouchMove={(e)=> {
            //     //e.preventDefault();
            // }}
            onClick={(e) => {
                if (clickSuppressed) {
                    e.stopPropagation();
                    e.preventDefault();
                    setSuppressClick(false);
                } else {
                    if (props.onClick) {
                        props.onClick(e);
                    }
                }
            }}
            onPointerCancelCapture={(e) => {
                if (pointerId !== null) {
                    e.currentTarget.releasePointerCapture(e.pointerId);
                    if (longPressed && props.onLongPressEnd) {
                        props.onLongPressEnd(e)
                    }
                    cancelLongPress();
                }
            }}>
            {props.children}
        </ButtonBase>
    );
}