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
    onDoubleClick?: (e: React.MouseEvent<HTMLButtonElement>) => void;
    onLongPressStart?: (currentTarget: HTMLButtonElement, e: React.PointerEvent<HTMLButtonElement> | React.MouseEvent<HTMLButtonElement>) => boolean;
    onLongPressMove?: (e: React.PointerEvent<HTMLButtonElement>) => void;
    onLongPressEnd?: (e: React.PointerEvent<HTMLButtonElement>| React.MouseEvent<HTMLButtonElement>) => void;
    instantMouseLongPress?: boolean; 
    longPressDelay?: number;
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
    const { onClick, onDoubleClick, onLongPressStart, onLongPressMove: 
        doLongPressMove,onLongPressEnd, longPressDelay,instantMouseLongPress, ...rest } = props;

    let [hTimeout, setHTimeout] = React.useState<number | null>(null);
    let [pointerId, setPointerId] = React.useState<number | null>(null);
    let [longPressed, setLongPressed] = React.useState<boolean>(false);
    let [clickSuppressed, setClickSuppressed] = React.useState<boolean>(false);
    let [pointerDownPoint, setPointerDownPoint] = React.useState<Point>({ x: 0, y: 0 });
    let [ longPressedElement, setLongPressedElement ] = React.useState<HTMLButtonElement | null>(null);
    let [lastClick, setLastClick] = React.useState<number | null>(null);


    function handleSuppressClick(e: MouseEvent) {
        e.stopPropagation();
        e.preventDefault();
        setSuppressClick(false);
    }

    function setSuppressClick(value: boolean) {
        setClickSuppressed(value);
    }
  function startLongPress(currentTarget: HTMLButtonElement, e: React.PointerEvent<HTMLButtonElement> | React.MouseEvent<HTMLButtonElement>)
    {
        if (hTimeout !== null) {
            window.clearTimeout(hTimeout);
            setHTimeout(null);
        }
        if (props.onLongPressStart) {
            if (!props.onLongPressStart(currentTarget, e)) {
                return;
            }
        }
        setLongPressedElement(currentTarget);
        setLongPressed(true);
        setSuppressClick(true);
        currentTarget.style.touchAction = "none"; // prevent scrolling.
        // console.log("DraggableButtonBase: long press started");

    }

    function cancelLongPress() {
        if (hTimeout !== null) {
            window.clearTimeout(hTimeout);
            setHTimeout(null);
        }
        setPointerId(null);
        setLongPressed(false);
        setSuppressClick(false);
        if (longPressedElement) {
            longPressedElement.style.touchAction = "";
            setLongPressedElement(null);
        }
        if (longPressed) {
            // console.log("DraggableButtonBase: long press canceled");
        }
    }

    function stopLongPress(e: React.PointerEvent<HTMLButtonElement> | React.MouseEvent<HTMLButtonElement>) {
        if (longPressed) {
            if (props.onLongPressEnd) {
                props.onLongPressEnd(e);
            }
        }
        cancelLongPress();
    }

    const handleTouchMove = (e: TouchEvent) => {
        if (longPressed) {
            e.preventDefault(); // Prevent scrolling during long press
            e.stopPropagation();
        }
    }
    const handleTouchstart = (e: TouchEvent) => {
        if (longPressed) {
            e.preventDefault(); // Prevent default touch behavior during long press
            e.stopPropagation();
        }
    }

    useEffect(() => {

        let hTouchMove = (e: TouchEvent) => {
            handleTouchMove(e);
        }
        addEventListener("touchmove", hTouchMove, { passive: false, capture: true });

        let hTouchStart = (e: TouchEvent) => {
            handleTouchstart(e);
        }
        addEventListener("touchstart", hTouchStart, { passive: false, capture: true });

        return () => {
            removeEventListener("touchmove", hTouchMove, { capture: true });
            removeEventListener("touchstart", hTouchStart, { capture: true });

            setSuppressClick(false);
            cancelLongPress();
        };
    }, []);

    useEffect(() => {
        if (clickSuppressed) {
            let hclick = (e: MouseEvent) => {
                handleSuppressClick(e);
            };
            window.addEventListener("click",hclick, { capture: true });
            let hTimeout: number | null = null;
            hTimeout = window.setTimeout(() => {
                setSuppressClick(false);
                hTimeout = null;
            }, 100);
            return () => {
                if (hTimeout !== null) {
                    window.clearTimeout(hTimeout);
                }
                window.removeEventListener("click", hclick, { capture: true });
            }
        } else {
            return ()=>{};
        }
    }, [clickSuppressed]);
    useEffect(() => {
        
        if (longPressed) {
            const suppressTouchMove = (e: TouchEvent) => {
                e.preventDefault();
            }
            window.addEventListener("touchmove", suppressTouchMove, { capture: true, passive: false } );
            return () => {
                window.removeEventListener("touchmove", suppressTouchMove, {capture: true})
            }
        }
        else {
            return () => {
            };
        }
    }, [longPressed]);

    return (
        <ButtonBase
            {...rest}
            style={{ touchAction: longPressed ? "none" : undefined, ...props.style }}
            onContextMenu={(e) => {
                e.preventDefault();
                e.stopPropagation();
                if (e.button === -1) { // touch long press
                    startLongPress(e.currentTarget as HTMLButtonElement,e);
                }
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
                if (longPressDelay === undefined || longPressDelay >= 0) {
                    let delay = longPressDelay ?? 1250;
                    if (instantMouseLongPress === true && e.pointerType === "mouse") {
                        delay = 0;
                    }
                    setHTimeout(window.setTimeout(() => {
                        startLongPress(currentTarget, e);
                    }, delay));
                }
                e.preventDefault();
                e.stopPropagation();
                return false;
            }}
            onPointerMove={(e) => {
                if (e.pointerId === pointerId) {
                    if (longPressed) {
                        if (doLongPressMove) {
                            doLongPressMove(e);
                        }
                    } else {
                        let clientPoint = screenToClient(e);

                        let dx = pointerDownPoint.x - clientPoint.x;
                        let dy = pointerDownPoint.y - clientPoint.y;
                        if (Math.abs(dx) > 5 || Math.abs(dy) > 5) {
                            stopLongPress(e);
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
                    stopLongPress(e);
                    if (e.isPropagationStopped() || e.isDefaultPrevented()) {
                        setSuppressClick(true); // only way to cancel the click
                        return;
                    }
                    e.preventDefault();
                    e.stopPropagation();

                }
            }}
            onTouchStart={(e)=> {
                //e.preventDefault();
            }}
            onClick={(e) => {
                if (clickSuppressed) {
                    e.stopPropagation();
                    e.preventDefault();
                    setSuppressClick(false);
                } else {
                    if (onClick) {
                        onClick(e);
                    }
                    // check to see whether this is a double-click
                    let time = Date.now();
                    if (lastClick && time-lastClick < 500) {
                        if (onDoubleClick) {
                            onDoubleClick(e);
                        }
                        setLastClick(null);
                    } else {
                        setLastClick(time);
                    }
                }
            }}
            onDoubleClick={(e) => {
                // chrome doesnt' do double-click for touch events
                // so we need to re-implement it on the onclick handler.
                e.stopPropagation();
                e.preventDefault();
                return false;

            }}
            onPointerCancelCapture={(e) => {
                if (pointerId !== null) {
                    e.currentTarget.releasePointerCapture(e.pointerId);
                    stopLongPress(e);
                }
            }}>
            {props.children}
        </ButtonBase>
    );
}