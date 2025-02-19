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

import { MouseEvent, PointerEvent, ReactNode, Component } from 'react';
import { Theme } from '@mui/material/styles';
import WithStyles from './WithStyles';
import {createStyles} from './WithStyles';

import { withStyles } from "tss-react/mui";
import { PiPedalStateError } from './PiPedalError';
import { css } from '@emotion/react';


const SELECT_SCALE = 1.5;

const AUTOSCROLL_TICK_DELAY = 30;
const AUTOSCROLL_THRESHOLD = 48;
const AUTOSCROLL_SCROLL_PER_SECOND = 100;
const AUTOSCROLL_SCROLL_RATE = AUTOSCROLL_SCROLL_PER_SECOND * AUTOSCROLL_TICK_DELAY / 1000;

const LONG_PRESS_TIME_MS = 250;

const styles = (theme: Theme) => createStyles({
    frame: css({
        width: "100%",
        height: "100%",
        display: "flex",
        alignItems: "center",
        justifyContent: "center",
        position: "relative"
    })
});

type Point = { x: number, y: number };

export interface DraggableProps extends WithStyles<typeof styles> {
    onDragStart?: (clientX: number, clientY: number) => void;
    onDragMove?: (clientX: number, clientY: number) => void;
    onDragEnd?: (clientX: number, clientY: number) => void;
    onDragCancel?: (clientX: number, clientY: number) => void;
    getScrollContainer: () => HTMLDivElement | null | undefined;

    children?: ReactNode | ReactNode[];
    draggable?: boolean;


}
type DraggableState = {
};


const Draggable =
    withStyles(
        class extends Component<DraggableProps, DraggableState>
        {

            constructor(props: DraggableProps) {
                super(props);
                this.onPointerDownCapture = this.onPointerDownCapture.bind(this);
                this.onPointerDown = this.onPointerDown.bind(this);
                this.onPointerCancel = this.onPointerCancel.bind(this);
                this.onPointerMove = this.onPointerMove.bind(this);
                this.onPointerUp = this.onPointerUp.bind(this);
                this.onClick = this.onClick.bind(this);
                this.autoScrollTick = this.autoScrollTick.bind(this);
                this.handleTouchMove = this.handleTouchMove.bind(this);
                this.handleTouchEnd = this.handleTouchEnd.bind(this);
                this.handleTouchStart = this.handleTouchStart.bind(this);

            }
            handleTouchStart(e: any) {
                if (this.dragStarted) {
                    //e.preventDefault();
                }
            }
            handleTouchMove(e: any) {
                if (this.dragStarted) {
                    e.preventDefault();
                }
            }
            handleTouchEnd(e: any) {
                if (this.dragStarted) {
                    e.preventDefault();
                }

            }


            isValidPointer(e: PointerEvent<HTMLDivElement>): boolean {
                if (e.pointerType === "mouse") {
                    return e.button === 0;
                } else if (e.pointerType === "pen") {
                    return true;
                } else if (e.pointerType === "touch") {
                    return true;
                }
                return false;
            }

            mouseDown: boolean = false;
            pointerId: number = 0;
            pointerType: string = "";

            lastOnDragTime: number = 0;

            onClick(e: MouseEvent<HTMLDivElement>) {
                // if the click event immediately follows drag end, suppress it.
                let dt = new Date().getTime() - this.lastOnDragTime;
                if (dt < 150) {
                    e.preventDefault();
                    e.stopPropagation();
                }

            }

            componentDidMount()
            {
            }
            componentWillUnmount()
            {

            }
            isCapturedPointer(e: PointerEvent<HTMLDivElement>): boolean {
                return this.mouseDown
                    && e.pointerId === this.pointerId
                    && e.pointerType === this.pointerType;

            }

            startX: number = 0;
            startY: number = 0;
            startClientX: number = 0;
            startClientY: number = 0;
            dragStarted: boolean = false;
            savedIndex: string = "";
            lastPointerDown: number = 0;
            originalBounds?: DOMRect;
            captureElement?: HTMLDivElement;
            pointerDownTime: number = 0;

            onPointerDownCapture(e: any) {
                // a new pointer down of any type cancels capture
                this.cancelDrag();
                e.preventDefault();
                e.stopPropagation();
            }
            longPressTimer?: number = undefined;

            longPressTimerTick() {
                if (this.pointerType === "touch") {
                    if (navigator.vibrate) {
                        navigator.vibrate([10]);
                    }
                    if (!this.dragStarted) {
                        this.dragTarget!.style.transform = "scale(" + SELECT_SCALE + ")";
                        this.dragTarget!.style.zIndex = "3";
                    }

                    this.startDrag();
                }
            }


            cancelDrag() {
                window.removeEventListener("touchmove", this.handleTouchMove);
                window.removeEventListener("touchend", this.handleTouchEnd);
                if (this.longPressTimer) {
                    clearTimeout(this.longPressTimer);
                    this.longPressTimer = undefined;
                }

                this.stopAutoScroll();
                if (this.dragStarted && this.captureElement) {
                    try {
                        this.captureElement.releasePointerCapture(this.pointerId);
                    } catch (error)
                    {
                        // throws if we've already lost it.
                    }

                }
                this.dragStarted = false;
                if (this.captureElement) {
                    this.clearDragTransform(this.captureElement);
                    this.captureElement = undefined;
                }
                this.mouseDown = false;

                document.body.removeEventListener("pointerdown", this.onPointerDownCapture, true);
                this.dragTarget = undefined;
            }


            dragTarget?: HTMLDivElement;

            onPointerDown(e: PointerEvent<HTMLDivElement>): void {

                if (this.props.draggable !== undefined && !this.props.draggable) return;

                // any new pointer down cancles a drag in progress.
                this.cancelDrag();

                // avoid 2nd click of a double click (which is difficult to cancel)
                let timeMs = new Date().getTime();
                if (timeMs - this.lastPointerDown < 300) {
                    this.lastPointerDown = timeMs;
                    return;
                }
                this.lastPointerDown = timeMs;


                if (!this.mouseDown && this.isValidPointer(e)) {

                    this.longPressTimer = setTimeout(() => this.longPressTimerTick(), LONG_PRESS_TIME_MS);

                    this.dragTarget = e.currentTarget as HTMLDivElement;
                    document.body.addEventListener("pointerdown", this.onPointerDownCapture, true);
                    this.captureElement = e.currentTarget;
                    // e.currentTarget.setPointerCapture(e.pointerId);

                    this.mouseDown = true;
                    this.dragStarted = false;
                    this.startX = e.clientX;
                    this.startY = e.clientY;
                    this.startClientX = e.clientX;
                    this.startClientY = e.clientY;

                    this.pointerId = e.pointerId;
                    this.pointerType = e.pointerType;
                    this.savedIndex = e.currentTarget.style.zIndex;
                    if (this.pointerType !== "touch") {
                        this.dragTarget.style.transform = "scale(" + SELECT_SCALE + ")";
                        this.dragTarget.style.zIndex = "3";
                    }
                }
            }

            clearDragTransform(element: HTMLDivElement): void {
                element.style.transform = "";
                element.style.zIndex = this.savedIndex;
            }

            onPointerCancel(e: PointerEvent<HTMLDivElement>) {
                if (this.isCapturedPointer(e)) {
                    this.cancelDrag();
                    if (this.props.onDragCancel) {
                        this.props.onDragCancel(e.clientX, e.clientY);
                    }
                }
            }

            onPointerUp(e: PointerEvent<HTMLDivElement>) {
                if (this.isCapturedPointer(e)) {
                    e.preventDefault();
                    e.stopPropagation();
                    if (this.dragStarted) {
                        this.lastOnDragTime = new Date().getTime();
                        if (this.props.onDragEnd) {
                            this.props.onDragEnd(e.clientX, e.clientY);
                        }
                    }
                    this.cancelDrag();


                }
            }

            dragThresholdExceeded(e: PointerEvent<HTMLDivElement>) {
                let dx = e.clientX - this.startX;
                let dy = e.clientY - this.startY;
                let d2 = dx * dx + dy * dy;
                if (this.pointerType === "touch") {
                    return false; // wait for long press.
                }

                let DRAG_THRESHOLD = 5;
                if (this.pointerType === "touch") {
                    DRAG_THRESHOLD = 15;
                }
                let result = (d2 > DRAG_THRESHOLD * DRAG_THRESHOLD);
                return result;
            }

            lastX: number = 0;
            lastY: number = 0;

            translateViewPortPoint(fromElement: HTMLElement, toElement: HTMLElement, pt: Point): Point {
                let rcFrom = fromElement.getBoundingClientRect();
                let rcTo = toElement.getBoundingClientRect();

                return {
                    x: pt.x + rcFrom.left + fromElement.scrollLeft - rcTo.left - toElement.scrollLeft,
                    y: pt.y + rcFrom.top + fromElement.scrollTop - rcTo.top - toElement.scrollTop

                };
            }
            translateViewPortRect(fromElement: HTMLElement, toElement: HTMLElement, rect: DOMRect): DOMRect {
                let rcFrom = fromElement.getBoundingClientRect();
                let rcTo = toElement.getBoundingClientRect();

                return new DOMRect(
                    rect.x + rcFrom.left - fromElement.scrollLeft - rcTo.left + toElement.scrollLeft,
                    rect.y + rcFrom.top - fromElement.scrollTop - rcTo.top + toElement.scrollTop,
                    rect.width,
                    rect.height);
            }


            makeTransform(targetElement: HTMLElement, clientX: number, clientY: number): string {
                let scrollContainer = this.getScrollContainer();
                if (scrollContainer && this.originalBounds) {
                    // the bounds of the translated element must COMPLETELY fit within the scroll range of the container.
                    let newLocation = new DOMRect(
                        this.originalBounds.x + clientX - this.startX,
                        this.originalBounds.y + clientY - this.startY,
                        this.originalBounds.width,
                        this.originalBounds.height);


                    if (newLocation.x < 0) {
                        clientX -= newLocation.x;
                    } else if (newLocation.right >= scrollContainer.scrollWidth) {
                        clientX -= newLocation.right - scrollContainer.scrollWidth;
                    }
                    if (newLocation.top < 0) {
                        clientY -= newLocation.top;
                    } else if (newLocation.bottom > scrollContainer.scrollHeight) {
                        clientY -= newLocation.bottom - scrollContainer.scrollHeight;
                    }

                }
                return "translate(" + (clientX - this.startX) + "px," + (clientY - this.startY) + "px) scale(" + SELECT_SCALE + ")";

            }

            createOriginalBounds() {
                // bounds of the target element, in ScrollContainer coordinates.
                let rcTarget = this.dragTarget!.getBoundingClientRect();
                rcTarget.x = 0;
                rcTarget.y = 0;

                let scrollContainer = this.props.getScrollContainer();
                if (!scrollContainer) {
                    throw new PiPedalStateError("ScrollContainer reference not valid.");
                }
                return this.translateViewPortRect(this.dragTarget!, scrollContainer, rcTarget);
            }

            startDrag() {
                window.addEventListener("touchmove", this.handleTouchMove, { passive: false });
                window.addEventListener("touchend", this.handleTouchEnd, { passive: false });


                this.dragStarted = true;
                if (this.props.onDragStart) {
                    this.props.onDragStart(this.startX, this.startY);
                }
                this.dragTarget!.setPointerCapture(this.pointerId);
                this.originalBounds = this.createOriginalBounds();

            }

            onPointerMove(e: PointerEvent<HTMLDivElement>): void {
                if (this.isCapturedPointer(e)) {
                    if (!this.dragStarted && this.dragThresholdExceeded(e)) {
                        this.startDrag();


                    }
                    if (this.dragStarted) {
                        this.lastX = e.clientX;
                        this.lastY = e.clientY;
                        e.currentTarget.style.transform = this.makeTransform(e.currentTarget, e.clientX, e.clientY);
                        e.stopPropagation();
                        e.preventDefault();

                        if (this.props.onDragMove) {
                            this.props.onDragMove(e.clientX, e.clientY);
                        }

                        this.checkForAutoScroll(e.currentTarget);
                    }
                }
            }
            autoScrollTimer?: number;

            autoScrollTick(currentTarget: HTMLElement, dx: number, dy: number) {
                if (!this.mouseDown) return;
                let scrollContainer = this.getScrollContainer();
                if (scrollContainer) {
                    let tx = scrollContainer.scrollLeft;
                    let ty = scrollContainer.scrollTop;
                    scrollContainer.scrollBy(dx, dy);
                    this.startX -= (scrollContainer.scrollLeft - tx);
                    this.startY -= (scrollContainer.scrollTop - ty);

                    currentTarget.style.transform = this.makeTransform(currentTarget, this.lastX, this.lastY);

                    this.checkForAutoScroll(currentTarget);
                }
            }
            stopAutoScroll() {
                if (this.autoScrollTimer) {
                    cancelAnimationFrame(this.autoScrollTimer);
                    this.autoScrollTimer = undefined;
                }
            }
            startAutoScroll(scrollContainer: HTMLElement, dx: number, dy: number) {
                this.stopAutoScroll();

                this.autoScrollTimer = requestAnimationFrame(
                    () => { this.autoScrollTick(scrollContainer, dx, dy) });
            }

            checkForAutoScroll(currentTarget: HTMLElement) {
                let scrollContainer = this.getScrollContainer();
                if (!scrollContainer) {
                    this.stopAutoScroll();
                    return;
                }


                let scrollContainerRect = scrollContainer.getBoundingClientRect();

                let dy: number = 0;
                let dx: number = 0;

                if (this.lastX < scrollContainerRect.x + AUTOSCROLL_THRESHOLD) {
                    dx = -AUTOSCROLL_SCROLL_RATE;
                    if (dx < -scrollContainer.scrollLeft) {
                        dx = -scrollContainer.scrollLeft;
                    }
                } else if (this.lastX >= scrollContainerRect.right - AUTOSCROLL_THRESHOLD) {
                    dx = AUTOSCROLL_SCROLL_RATE;
                    let maxScroll = Math.max(scrollContainer.scrollWidth - scrollContainer.clientWidth, 0);
                    if (dx > maxScroll) dx = maxScroll;
                }
                if (this.lastY < scrollContainerRect.top) {
                    dy = -AUTOSCROLL_SCROLL_RATE;
                    if (dy < -scrollContainer.scrollTop) dy = -scrollContainer.scrollTop;
                } else if (this.lastY >= scrollContainerRect.top - AUTOSCROLL_THRESHOLD) {
                    dy = AUTOSCROLL_SCROLL_RATE;
                    let maxScroll = Math.max(scrollContainer.scrollHeight - scrollContainer.clientHeight);
                    if (dy > maxScroll) dy = maxScroll;
                }

                if (dx === 0 && dy === 0) {
                    this.stopAutoScroll();
                    return;
                } else {
                    this.startAutoScroll(currentTarget, dx, dy);
                }



            }

            getScrollContainer(): HTMLDivElement | null {
                if (this.props.getScrollContainer) {
                    let t = this.props.getScrollContainer();
                    if (!t) return null;
                    return t;
                }
                return null;
            }




            render() {
                const classes = withStyles.getClasses(this.props);
                return (
                    <div className={classes.frame} style={{ transform: "" }}
                        onPointerDown={this.onPointerDown}
                        onPointerMove={this.onPointerMove}
                        onPointerCancel={this.onPointerCancel}
                        onPointerCancelCapture={this.onPointerCancel}
                        onPointerUp={this.onPointerUp}
                        onDragStart={(e) => { e.preventDefault(); e.stopPropagation(); }}
                        onClick={this.onClick}
                    >
                        {
                            this.props.children
                        }
                    </div>
                )
            }

        },
        styles        
    );

export default Draggable;
