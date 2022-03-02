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
import React, { MouseEvent, PointerEvent, Component } from 'react';
import { Theme } from '@mui/material/styles';
import createStyles from '@mui/styles/createStyles';
import { Property } from "csstype";
import { nullCast} from './Utility'

const SELECT_SCALE = 1.0;

const AUTOSCROLL_TICK_DELAY = 30;
const AUTOSCROLL_THRESHOLD = 48;
const AUTOSCROLL_SCROLL_PER_SECOND = 300;
const AUTOSCROLL_SCROLL_RATE = AUTOSCROLL_SCROLL_PER_SECOND * AUTOSCROLL_TICK_DELAY / 1000;

const LONG_PRESS_TIME_MS = 250;


const ANIMATION_DURATION_MS = 100;



class AnimationData {
    constructor(element: HTMLElement, originalPosition: DOMRect) {
        this.element = element;
        this.originalPosition = originalPosition;
        this.fromY = this.toY = this.currentY = this.originalPosition.y;
        this.key = (element as any)["key"];
    }
    key: any;
    element: HTMLElement;
    originalPosition: DOMRect;
    fromY: number;
    toY: number;
    currentY: number;

    animating: boolean =  false;

    setPosition(y: number)
    {
        this.fromY = this.toY;
        this.toY = y;
        this.animating = true;
    }
    snapToPosition() {
        this.fromY = this.toY;
        this.currentY = this.toY;
        this.animating = true;
    }

}

// eslint-disable-next-line @typescript-eslint/no-unused-vars
const styles = (theme: Theme) => createStyles({
    frame: {
        position: "relative",
        margin: "12px",
        background: theme.palette.background.paper
    }
});

type Point = { x: number, y: number };

export enum ScrollDirection {
    None, X, Y
}
export interface DraggableGridProps
{
    fullWidth?: boolean; // defaults to true
    fullHeight?: boolean; // defaults to true.
    onDragStart?: (itemIndex: number, clientX: number, clientY: number) => void;
    onDragMove?: (currentItemIndex: number, clientX: number, clientY: number) => void;
    onDragEnd?: (fromIndex: number, toIndex: number, clientX: number, clientY: number) => void;
    onDragCancel?: (clientX: number, clientY: number) => void;

    defaultSelectedIndex?: number;

    onLongPress?: (itemIndex: number) => void;

    canDrag?: boolean;

    children: React.ReactNode[];

    moveElement: (from: number, to: number) => void;
    scroll: ScrollDirection;

}

type DraggableGridState = {
    indexToSelect?: number;
};



const DraggableGrid = 
    (
        class extends Component<DraggableGridProps, DraggableGridState>
        {
            refScrollContainer: React.RefObject<HTMLDivElement>;
            refGrid: React.RefObject<HTMLDivElement>;

            constructor(props: DraggableGridProps) {
                super(props);
                this.refScrollContainer = React.createRef();
                this.refGrid = React.createRef();

                this.state = {
                    indexToSelect: props.defaultSelectedIndex
                };

                this.handlePointerDownCapture = this.handlePointerDownCapture.bind(this);
                this.handlePointerDown = this.handlePointerDown.bind(this);
                this.handlePointerCancel = this.handlePointerCancel.bind(this);
                this.handlePointerMove = this.handlePointerMove.bind(this);
                this.handlePointerUp = this.handlePointerUp.bind(this);
                this.onClick = this.onClick.bind(this);
                this.handleAutoScrollTick = this.handleAutoScrollTick.bind(this);
                this.animationTick = this.animationTick.bind(this);
                this.longPressTimerTick = this.longPressTimerTick.bind(this);
                this.handleTouchMove = this.handleTouchMove.bind(this);
                this.handleTouchEnd = this.handleTouchEnd.bind(this);
            }

            animationData: AnimationData[] = [];


            handleTouchMove(e: any)
            {
                if (this.dragStarted)
                {
                    e.preventDefault();
                }
            }
            handleTouchEnd(e: any)
            {
                if (this.dragStarted)
                {
                    e.preventDefault();
                }

            }

            animationStartTime: number = 0;
            timerHandle: number| null = null;

            animationTick() {
                let t = (new Date().getTime() - this.animationStartTime) / ANIMATION_DURATION_MS;
                if (t > 1) t = 1;

                // easing function.
                //t = 1-(1-t)*(1-t);
                let didAnimate: boolean = false;
                for (let i = 0; i < this.animationData.length; ++i) {
                    let animationData = this.animationData[i];
                    if (animationData.animating)
                    {
                        if (i !== this.startIndex) {
                            //if ((animationData.xFrom !== animationData.xTo) || (animationData.yFrom !== animationData.yTo))
                            let y = t * (animationData.toY - animationData.fromY) + animationData.fromY;
                            let style = animationData.element.style;
                            style.left = "0px";
                            style.top = y-animationData.originalPosition.y + "px";
                            animationData.currentY = y;
                            animationData.animating = animationData.toY !== animationData.fromY;
                            if (animationData.animating) didAnimate = true;
                        }
                    }
                }
                if (t < 1 && didAnimate) {
                    this.timerHandle = requestAnimationFrame(this.animationTick)
                } else {
                    this.timerHandle = null;
                }

            }
            startAnimation() {
                this.animationStartTime = new Date().getTime();

                if (this.timerHandle === null) {
                    
                    this.timerHandle = requestAnimationFrame(this.animationTick);
                    this.animationTick();
                }
            }
            stopAnimation() {
                if (this.timerHandle !== null) {
                    cancelAnimationFrame(this.timerHandle);
                }
                this.timerHandle = null;
            }

            isValidPointer(e: PointerEvent<HTMLDivElement>): boolean 
            {
                if (e.pointerType === "mouse") {
                    return e.button === 0;
                } else if (e.pointerType === "pen") {
                    return true;
                } else if (e.pointerType === "touch") {
                    return true;
                }
                return false;
            }

            bringSelectedItemIntoView() {
                if (this.state.indexToSelect && this.state.indexToSelect >= 0)
                {
                    let grid = this.refGrid.current;
                    if (grid)
                    {
                        if (this.state.indexToSelect >= 0 && this.state.indexToSelect < grid.childNodes.length)
                        {
                            let element = grid.childNodes[this.state.indexToSelect] as HTMLElement;
                            element.scrollIntoView({
                                block: "nearest",
                                inline: "nearest"
                            });
                        }
                    }
                }
            }
            componentDidMount() {
                this.bringSelectedItemIntoView();
                // this.refGrid.current!.addEventListener("touchstart",(e)=> this.handleTouchStart(e),{passive: false});
            }
            componentWillUnmount()
            {
                //this.refGrid.current!.removeEventListener("touchstart",(e) => this.handleTouchStart(e));
                this.stopAnimation();

            }

            componentDidUpdate() {
                if (!this.mouseDown)
                {
                    this.bringSelectedItemIntoView();
                }
                if (this.mouseDown)
                {
                    this.reprepareChildren();
                    this.reflowChildren();
                    this.stopAnimation();
                    this.animationTick();
                }
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
            hasClass(element: HTMLElement, className: string) {
                let classes = element.classList;
                for (let i = 0; i < classes.length; ++i) {
                    if (classes[i] === className) return true;
                }
                return false;
            }
            findGridItem(element: any | null): HTMLDivElement | null {
                let grid = this.refGrid.current;
                if(!grid) return null;
                while (element !== null && element !== grid) {
                    if (element.parentElement === grid) {
                        return element as HTMLDivElement;
                    }
                    element = element.parentElement;
                }
                return null;
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
            savedBackground: string = "";
            lastPointerDown: number = 0;

            ptItemStart: DOMPoint = new DOMPoint(0, 0);
            captureElement?: HTMLDivElement;

            handlePointerDownCapture(e: any) {
                // a new pointer down of any type cancels capture
                this.cancelDrag();
                e.preventDefault();
                e.stopPropagation();
            }

            

            cancelDrag() {
                //window.removeEventListener("touchstart",this.handleTouchStart);
                window.removeEventListener("touchmove",this.handleTouchMove);
                window.removeEventListener("touchend",this.handleTouchEnd);

                if (this.originalOverscrollBehaviourY)
                {
                    this.getScrollContainer()!.style.setProperty("overscroll-behaviour-y",this.originalOverscrollBehaviourY);
                    this.originalOverscrollBehaviourY = undefined;
                }

                if (this.longPressTimer) {
                    clearTimeout(this.longPressTimer);
                    this.longPressTimer = undefined;
                }
                this.stopAnimation();
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
                if (this.dragTarget) {
                    this.clearDragTransform(this.dragTarget);
                    this.dragTarget = null;
                }
                this.captureElement = undefined;
                this.mouseDown = false;
                this.unprepareChildren();

                document.body.removeEventListener("pointerdown", this.handlePointerDownCapture, true);
            }


            prepareChildren() {
                if (this.refGrid.current === null) return;
                let grid = this.refGrid.current;
                let nodes = grid.childNodes;
                let childRects = [];

                let gridClientRect = grid.getBoundingClientRect();
                for (let i = 0; i < nodes.length; ++i) {
                    let child = nodes[i] as HTMLDivElement;
                    let rect = child.getBoundingClientRect();
                    rect.x -= gridClientRect.left;
                    rect.y -= gridClientRect.top;
                    childRects[i] = rect;
                }
                for (let i = 0; i < nodes.length; ++i) {
                    let child = nodes[i] as HTMLDivElement;
                    child.style.position = "relative";
                    child.style.left = "0px";
                    child.style.top = "0px";
                }
                this.animationData = [];
                for (let i = 0; i < nodes.length; ++i) {
                    this.animationData[i] = new AnimationData(nodes[i] as HTMLElement, childRects[i]);
                }
                grid.style.height = this.gridBounds.height + "px";
                grid.style.width = this.gridBounds.width + "px";
                
            }
            reprepareChildren()
            {
                if (this.refGrid.current === null) return;
                let grid = this.refGrid.current;
                let nodes = grid.childNodes;

                for (let i = 0; i < nodes.length; ++i) {
                    let child = nodes[i] as HTMLDivElement;
                    child.style.position = "relative";
                    child.style.left = "0px";
                    child.style.top = "0px";
                }
                if (nodes.length !== this.animationData.length)
                {
                    this.cancelDrag();
                    return;
                }
                for (let i = 0; i < nodes.length; ++i) {
                    let key: any = (nodes[i] as any).key;
                    if (nodes[i] as HTMLElement !== this.animationData[i].element) {
                        console.log("Animation failed: Element " + i + " changed.");
                    }
                    if (key !== this.animationData[i].key) {
                        console.log("Animation failed: Element " + i + " key has changed.");
                    }
                    this.animationData[i].element = nodes[i] as HTMLElement;
                    if (i !== this.startIndex)
                    {
                        this.animationData[i].animating = true;
                    }
                }
                grid.style.height = this.gridBounds.height + "px";
                grid.style.width = this.gridBounds.width + "px";
                this.animationTick();


            }

            getIndexFromPosition(clientX: number, clientY: number)
            {
                let grid = nullCast(this.refGrid.current);
                let gridBounds = grid.getBoundingClientRect();
                clientX -= gridBounds.left;
                clientY -= gridBounds.top;

                if (clientY < 0) return -1;
                for (let i = 0; i < this.animationData.length; ++i)
                {
                    let item = this.animationData[i];
                    if (clientY >= item.originalPosition.x && clientY < item.originalPosition.bottom)
                    {
                        return i;
                    }
                }
                return -1;

            
            }
            getDragIndexFromPosition(clientX: number, clientY: number): number {

                let grid = nullCast(this.refGrid.current);

                let originalBounds = this.animationData[this.startIndex].originalPosition;
                // Base test on the CENTER point of the currently dragged item.
                clientX += originalBounds.width / 2 - this.ptItemStart.x;
                clientY += originalBounds.height / 2 - this.ptItemStart.y;

                let gridBounds = grid.getBoundingClientRect();

                clientX -= gridBounds.x;
                clientY -= gridBounds.top;

                // clientX/Y is now the position of the center of the currently-dragged item, in the coordinates of the contenst of the scroll view (scroll applied)


                if (clientY < 0) return 0;
                for (let i = 0; i < this.animationData.length; ++i) {
                    let rect = this.animationData[i].originalPosition;

                    if (clientY >= rect.top && clientY < rect.bottom)
                    {
                        return i;
                    }
                }
                return this.animationData.length-1;
            }


            unprepareChildren() {
                if (this.refGrid.current === null) return;
                let grid = this.refGrid.current;
                let nodes = grid.childNodes;

                for (let i = 0; i < nodes.length; ++i) {
                    let child = nodes[i] as HTMLDivElement;
                    child.style.position = "";
                    child.style.top = ""
                    child.style.left = "";
                    child.style.width = "";
                    child.style.height = "";
                }

                grid.style.height = "";
                grid.style.width = "100%";

                // releast refs and storage.
                this.dragTarget = null;
                this.animationData = [];

            }

            dragTarget: HTMLDivElement | null = null;

            elementKeys: any[] = [];



            clearDragTransform(element: HTMLDivElement): void {
                element.style.transform = "";
                element.style.zIndex = this.savedIndex;
                element.style.background = this.savedBackground;
                element.style.opacity = "1";

            }

            handlePointerCancel(e: PointerEvent<HTMLDivElement>) {
                if (this.isCapturedPointer(e)) {
                    this.cancelDrag();
                    if (this.props.onDragCancel) {
                        this.props.onDragCancel(e.clientX, e.clientY);
                    }
                }
            }

            handlePointerUp(e: PointerEvent<HTMLDivElement>) {
                if (this.isCapturedPointer(e)) {

                    this.assertKeysNotChanged(this.elementKeys); // make sure that a late render hasn't swapped elements while we're dragging.

                    this.handlePointerMove(e);
                    e.preventDefault();
                    e.stopPropagation();
                    if (this.dragStarted) {
                        this.lastOnDragTime = new Date().getTime();
                        if (this.props.onDragEnd) {
                            this.props.onDragEnd(
                                this.startIndex, this.currentIndex,
                                e.clientX, e.clientY);
                        }
                        if (this.startIndex !== this.currentIndex) {
                            this.props.moveElement(this.startIndex, this.currentIndex);
                        }
                    }
                    this.cancelDrag();
                }
            }

            dragThresholdExceeded(e: PointerEvent<HTMLDivElement>) {
                if (this.pointerType === "touch")
                {
                    let timeMs = new Date().getTime();
                    let dt = timeMs -this.lastPointerDown;
                    if (dt < LONG_PRESS_TIME_MS) return false;
                }
    
                let dx = e.clientX - this.startX;
                let dy = e.clientY - this.startY;
                let d2 = dx * dx + dy * dy;

                let DRAG_THRESHOLD = 5;
                if (this.pointerType === "touch")
                {
                    DRAG_THRESHOLD = 15;
                }
                return  (d2 > DRAG_THRESHOLD * DRAG_THRESHOLD);
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

            makeTransform(targetElement: HTMLElement, clientX: number, clientY: number): string {
                
                // the bounds of the translated element must COMPLETELY fit within the scroll range of the container.
                let originalBounds = this.animationData[this.startIndex].originalPosition;

                let newLocation = new DOMRect(
                    originalBounds.x + clientX - this.startX,
                    originalBounds.y + clientY - this.startY,
                    originalBounds.width,
                    originalBounds.height);


                if (newLocation.x < 0) {
                    newLocation.x = 0;
                } else if (newLocation.right >= this.gridBounds.width) {
                    newLocation.x -= newLocation.right - this.gridBounds.width;
                }

                // chrome: inconsistent scroll height calculations.
                let maxY = this.gridBounds.height;
                if (newLocation.top < 0) {
                    newLocation.y = 0;
                } else if (newLocation.bottom > maxY) {
                    newLocation.y -= newLocation.bottom - maxY;
                }

                return "translate(" + (newLocation.x - originalBounds.x) + "px,"
                    + (newLocation.y - originalBounds.y) + "px) scale(" + SELECT_SCALE + ")";
            }

            createOriginalBounds(): DOMRect {
                // bounds of the target element, in ScrollContainer coordinates.
                let rcTarget = nullCast(this.dragTarget).getBoundingClientRect();

                let rcScroll = nullCast(this.getScrollContainer()).getBoundingClientRect();
                rcTarget.x -= rcScroll.x;
                rcTarget.y -= rcScroll.y;
                return rcTarget;
            }

            startIndex: number = 0;
            currentIndex: number = 0;


            reflowChildren(): void {
            
                let y = 0;
                for (let i = 0; i < this.animationData.length; ++i) {
                    if (i === this.startIndex) {
                        if (i === this.currentIndex) {
                            y += this.animationData[this.startIndex].originalPosition.height;
                        }

                    } else {
                        if (i === this.currentIndex && this.currentIndex < this.startIndex)
                        {
                            y += this.animationData[this.startIndex].originalPosition.height;
                        }
                        let animationItem = this.animationData[i];
                        animationItem.setPosition(y);
                        y += animationItem.originalPosition.height;
                        if (i === this.currentIndex && this.currentIndex > this.startIndex)
                        {
                            y += this.animationData[this.startIndex].originalPosition.height;
                        }
                    }
                }
                this.stopAnimation();
                this.animationStartTime = new Date().getTime();
                this.animationTick();
            }

            itemOffsetX: number = 0;
            itemOffsetY: number = 0;
            longPressTimer?: NodeJS.Timeout = undefined;


            longPressTimerTick() {
                if (navigator.vibrate)
                {
                    navigator.vibrate([5]);
                }
                if (!this.dragStarted)
                {
                    this.startDrag();
                }
            }


            handlePointerDown(e: PointerEvent<HTMLDivElement>): void {

                if (this.props.canDrag !== undefined && !this.props.canDrag) return;

                // any new pointer down cancels a drag in progress.
                this.cancelDrag();

                // avoid 2nd click of a double click (which is difficult to cancel)
                let timeMs = new Date().getTime();
                if (timeMs - this.lastPointerDown < 300) {
                    this.lastPointerDown = timeMs;
                    return;
                }
                this.lastPointerDown = timeMs;


                let gridElement = this.findGridItem(e.target);
                if (gridElement === null) return;



                if (!this.mouseDown && this.isValidPointer(e)) {

                    this.dragTarget = gridElement;


                    document.body.addEventListener("pointerdown", this.handlePointerDownCapture, true);
                    this.captureElement = e.currentTarget;

                    this.elementKeys = this.getElementKeys();
                    this.longPressTimer = setTimeout(this.longPressTimerTick,LONG_PRESS_TIME_MS);
                    this.mouseDown = true;

                    ///window.addEventListener("touchstart",this.handleTouchStart, {passive: false});

                    this.dragStarted = false;
                    this.startX = e.clientX;
                    this.startY = e.clientY;
                    this.startClientX = e.clientX;
                    this.startClientY = e.clientY;

                    this.pointerId = e.pointerId;
                    this.pointerType = e.pointerType;

                    this.savedIndex = gridElement.style.zIndex;
                    gridElement.style.zIndex = "3";
                    this.savedBackground = gridElement.style.background;
                    gridElement.style.background = "#EEE";
                    gridElement.style.opacity = "0.8";
                }
            }

            originalOverscrollBehaviourY?: string;

            gridBounds: DOMRect = new DOMRect();

            startDrag() {
                this.dragStarted = true;
                let grid = this.refGrid.current!;
                this.gridBounds = grid.getBoundingClientRect();

                window.addEventListener("touchmove",this.handleTouchMove, {passive: false});
                window.addEventListener("touchend",this.handleTouchEnd, {passive: false});

                this.prepareChildren();
                
                this.startIndex = this.getIndexFromPosition(this.startX, this.startY);
                if (this.startIndex === -1)
                {
                    this.cancelDrag();
                    return;
                }
                let t = grid.childNodes[this.startIndex];
                if (t !== this.dragTarget) {
                    console.count("dragTarget mismatch!");
                    this.startIndex = this.getIndexFromPosition(this.startX,this.startY);
                    this.dragTarget = grid.childNodes[this.startIndex] as HTMLDivElement;
                }
                this.currentIndex = this.startIndex;
                if (this.props.onDragStart) {
                    this.props.onDragStart(this.startIndex, this.startX, this.startY);
                }
                this.dragTarget!.setPointerCapture(this.pointerId);

                let rcItem = this.dragTarget!.getBoundingClientRect();
                this.ptItemStart = new DOMPoint(this.startX - rcItem.x, this.startY - rcItem.y);

                this.originalOverscrollBehaviourY = this.getScrollContainer()!.style.getPropertyValue("overscroll-behaviour-y");
                this.getScrollContainer()!.style.setProperty("overscroll-behaviour-y","none");

                if (this.pointerType === "touch")
                {
                    this.startY -= 4; // causes the selected item to "pop"
                }
            }

            handlePointerMove(e: PointerEvent<HTMLDivElement>): void {
                if (this.isCapturedPointer(e)) {
                    if (!this.dragStarted && this.dragThresholdExceeded(e)) {
                        this.startDrag();
                    }
                    if (this.dragStarted) {
                        this.lastX = e.clientX;
                        this.lastY = e.clientY;
                        if (this.dragTarget !== null) {
                            this.dragTarget.style.transform = this.makeTransform(this.dragTarget, e.clientX, e.clientY);

                            let index = this.getDragIndexFromPosition(e.clientX, e.clientY);
                            if (index !== this.currentIndex) {
                                this.currentIndex = index;
                                this.reflowChildren();
                            }
                        }
                        e.stopPropagation();
                        e.preventDefault();

                        if (this.props.onDragMove) {
                            this.props.onDragMove(this.currentIndex, e.clientX, e.clientY);
                        }

                        this.checkForAutoScroll(e.currentTarget);
                    }
                }
            }
            autoScrollTimer?: NodeJS.Timeout;


            getElementKeys(): any[] {
                let grid = nullCast(this.refGrid.current);
                let children = grid.childNodes;
                let result: any[] = [];
                for (let i = 0; i < children.length; ++i)
                {
                    result[i] = (children[i] as any).key;
                }
                return result;
            }

            assertKeysNotChanged(oldKeys: any[])
            {
                let newKeys = this.getElementKeys();
                for (let i = 0; i < newKeys.length; ++i)
                {
                    if (newKeys[i] !== oldKeys[i])
                    {
                        throw new Error("Keys have changed.");
                    }
                }
            }

            handleAutoScrollTick(currentTarget: HTMLElement, dx: number, dy: number) {
                if (!this.mouseDown) return;
                let scrollContainer = this.getScrollContainer();
                if (scrollContainer) {
                    let tx = scrollContainer.scrollLeft;
                    let ty = scrollContainer.scrollTop;

                    let x = scrollContainer.scrollLeft + dx;
                    let y = scrollContainer.scrollTop + dy;
                    x = Math.min(x,scrollContainer.scrollWidth-scrollContainer.clientWidth);
                    y = Math.min(y,scrollContainer.scrollHeight-scrollContainer.clientHeight);
                    if (x < 0) x = 0;
                    if (y < 0) y = 0;

                    scrollContainer.scrollTo(x, y);
                    this.startX -= (scrollContainer.scrollLeft - tx);
                    this.startY -= (scrollContainer.scrollTop - ty);

                    if (this.dragTarget) {
                        this.dragTarget.style.transform = this.makeTransform(this.dragTarget, this.lastX, this.lastY);
                    }

                    this.checkForAutoScroll(currentTarget);
                }
            }
            stopAutoScroll() {
                if (this.autoScrollTimer !== undefined) {
                    clearTimeout(this.autoScrollTimer);
                    this.autoScrollTimer = undefined;
                }
            }
            startAutoScroll(scrollContainer: HTMLElement, dx: number, dy: number) {
                this.stopAutoScroll();

                this.autoScrollTimer = setTimeout(
                    () => { this.handleAutoScrollTick(scrollContainer, dx, dy) }, AUTOSCROLL_TICK_DELAY);
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
                } else if (this.lastY >= scrollContainerRect.bottom - AUTOSCROLL_THRESHOLD) {
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
                return this.refScrollContainer.current;
            }

            overflowX(): Property.OverflowX {
                return this.props.scroll === ScrollDirection.X ? "auto" : "hidden";
            }
            overflowY(): Property.OverflowX {
                return this.props.scroll === ScrollDirection.Y ? "auto" : "hidden";
            }

            render() {
                return (
                    <div  
                       style={{
                        transform: "",
                        overflowX: this.overflowX(),
                        overflowY: this.overflowY(),
                        width: this.props.fullWidth??true? "100%": "auto",
                        height: this.props.fullHeight??true? "100%": "auto",
                    }}
                        onPointerDown={this.handlePointerDown}
                        onPointerMove={this.handlePointerMove}
                        onPointerCancel={this.handlePointerCancel}
                        onPointerCancelCapture={this.handlePointerCancel}
                        onPointerUp={this.handlePointerUp}
                        onDragStart={(e) => { e.preventDefault(); e.stopPropagation(); }}
                        onClick={this.onClick}
                        ref={this.refScrollContainer}
                    >
                        <div  style={{ display: "flex", flexDirection: "column", flexWrap: "nowrap",  position: "relative" }}
                            ref={this.refGrid}
                        >
                            {this.props.children}
                        </div>
                    </div>
                )
            }

        }
    );

export default DraggableGrid;
