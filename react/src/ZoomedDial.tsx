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

import React, { TouchEvent, PointerEvent, SyntheticEvent } from 'react';import { Theme } from '@mui/material/styles';
import { WithStyles } from '@mui/styles';
import withStyles from '@mui/styles/withStyles';
import createStyles from '@mui/styles/createStyles';
import { PiPedalModel, PiPedalModelFactory, ZoomedControlInfo } from './PiPedalModel';
import {ReactComponent as DialIcon} from './svg/fx_dial.svg';

const SELECTED_OPACITY = 0.8;
const DEFAULT_OPACITY = 0.6;
const FINE_RANGE_SCALE = 0.1; //
const ULTRA_FINE_RANGE_SCALE = 0.02; // 12000 pixels to move from 0 to 1.

const MIN_ANGLE = -135;
const MAX_ANGLE = 135;
const DEAD_ZONE_SIZE = 5;


const styles = (theme: Theme) => createStyles({
    dialIcon: {
        overscrollBehavior: "none", touchAction: "none", 
        fill: theme.palette.text.primary,
        opacity: DEFAULT_OPACITY,

    }
});


interface ZoomedDialProps extends WithStyles<typeof styles> {
    theme: Theme,
    size: number,

    controlInfo: ZoomedControlInfo | undefined,

    onPreviewValue(value: number): void,
    onSetValue(value: number): void
}

interface ZoomedDialState {

}

const ZoomedDial = withStyles(styles, { withTheme: true })(

    class extends React.Component<ZoomedDialProps, ZoomedDialState> {

        model: PiPedalModel = PiPedalModelFactory.getInstance();
        imgRef: React.RefObject<SVGSVGElement> = React.createRef();

        constructor(props: ZoomedDialProps) {
            super(props);
            this.state = {

            };

            this.onTouchStart = this.onTouchStart.bind(this);
            this.onTouchMove = this.onTouchMove.bind(this);
            this.onBodyPointerDownCapture = this.onBodyPointerDownCapture.bind(this);

            this.onPointerDown = this.onPointerDown.bind(this);
            this.onPointerUp = this.onPointerUp.bind(this);
            this.onPointerMove = this.onPointerMove.bind(this);
            this.onPointerLostCapture = this.onPointerLostCapture.bind(this);

        }

        onPointerUp(e: PointerEvent<SVGSVGElement>) {

            if (this.isCapturedPointer(e)) {
                --this.pointersDown;
                

                e.preventDefault();
                this.updateAngle(e);
                this.commitAngle();

                this.releaseCapture(e);

            } else {
                --this.pointersDown;
                
            }
        }


        releaseCapture(e: PointerEvent<SVGSVGElement>)
        {
            let img = this.imgRef.current;
            
            if (img && img.style) {
                img.releasePointerCapture(e.pointerId);
                img.style.opacity = "" + DEFAULT_OPACITY;
                // they get automaticlly released.
                // for (let i = 0; i < this.capturedPointers.length; ++i)
                // {
                //     img.releasePointerCapture (this.capturedPointers[i]);
                // }
                this.capturedPointers = [];
            }
            document.body.removeEventListener(
                "pointerdown",
                this.onBodyPointerDownCapture,true
                );

            this.mouseDown = false;

        }


        onPointerLostCapture(e: PointerEvent<SVGSVGElement>) {
            if (this.isCapturedPointer(e)) {
                --this.pointersDown;
                

                this.releaseCapture(e);
            }

        }

        onTouchStart(e: TouchEvent<SVGSVGElement>) {
            //must be defined to get onTouchMove
        }
        onTouchMove(e: TouchEvent<SVGSVGElement>) {
            // e.preventDefault();
            e.stopPropagation(); // cancels scroll!!!

        }
        capturedPointers: number[] = [];

        isExtraTouch(e: PointerEvent): boolean {
            return (this.mouseDown && this.pointerType === "touch" && e.pointerType === "touch" && e.pointerId !== this.pointerId);
        }

        mouseDown: boolean = false;
        pointerId: number = -1;
        pointerType: string = "";
        startX: number = 0;
        startY: number = 0;
        currentAngle: number = 0;
        lastPointerAngle: number = 0;

        isValidPointer(e: PointerEvent<SVGSVGElement>): boolean {
            if (e.pointerType === "mouse") {
                return e.button === 0;
            } else if (e.pointerType === "pen") {
                return true;
            } else if (e.pointerType === "touch") {
                return true;
            }
            return false;
        }

        defaultValue: number = 0;

        getCurrentValue(): number {
            if (this.props.controlInfo == null) {
                return this.defaultValue;
            } else {
                let uiControl = this.props.controlInfo.uiControl;
                let instanceId = this.props.controlInfo.instanceId;
                if (instanceId === -1) return 0;
                let pedalboardItem = this.model.pedalboard.get()?.getItem(instanceId);
                let value: number = pedalboardItem?.getControlValue(uiControl.symbol) ?? 0;
                this.defaultValue = value;
                return value;
            }
        }

        angleToValue(angle: number): number {
            if (this.props.controlInfo) {
                if (angle > MAX_ANGLE) angle = MAX_ANGLE;
                if (angle < MIN_ANGLE) angle = MIN_ANGLE;

                let r = (angle - MIN_ANGLE) / (MAX_ANGLE - MIN_ANGLE);

                let uiControl = this.props.controlInfo.uiControl;

                return r * (uiControl.max_value-uiControl.min_value)+uiControl.min_value;
            }
            return 0;
        }

        valueToAngle(value: number): number {
            if (this.props.controlInfo) {
                let uiControl = this.props.controlInfo.uiControl;
                let range = (value - uiControl.min_value) / (uiControl.max_value - uiControl.min_value);
                if (range < 0) range = 0;
                if (range > 1) range = 1;
                return (MAX_ANGLE - MIN_ANGLE) * range + MIN_ANGLE;
            }
            return 0;
        }

        pointerToAngle(e: PointerEvent<SVGSVGElement>): number {

            if (this.imgRef.current) {
                let img = this.imgRef.current;
                let rc = img.getBoundingClientRect();
                let x = e.clientX - (rc.left+ rc.width / 2);
                let y = e.clientY - (rc.top + rc.height / 2);

                // dead zone in center of image. 

                if (x*x+y*y < DEAD_ZONE_SIZE*DEAD_ZONE_SIZE) return NaN;

                let angle = -Math.atan2(-y, x) * 180 / Math.PI + 90;

                return angle;
            }
            return NaN;
        }
        captureElement?: SVGSVGElement;
        pointersDown: number = 0;

        onPointerDown(e: PointerEvent<SVGSVGElement>): void {
            if (!this.mouseDown && this.isValidPointer(e)) {
                e.preventDefault();
                e.stopPropagation();



                this.mouseDown = true;
                this.pointersDown = 1;
                this.pointerId = e.pointerId;
                this.pointerType = e.pointerType;
                this.startX = e.clientX;
                this.startY = e.clientY;
                this.currentAngle = this.valueToAngle(this.getCurrentValue());
                this.lastPointerAngle = this.pointerToAngle(e);

                let img = this.imgRef.current;

                if (img) {
                    this.captureElement = img;
                    document.body.addEventListener(
                        "pointerdown",
                        this.onBodyPointerDownCapture, true
                    );

                    img.setPointerCapture(e.pointerId);
                    if (img.style) {
                        img.style.opacity = "" + SELECTED_OPACITY;
                    }
                }

            } else {
                if (this.isExtraTouch(e)) {
                    ++this.pointersDown;

                }
            }
        }

        isCapturedPointer(e: PointerEvent<SVGSVGElement>): boolean {
            return this.mouseDown
                && e.pointerId === this.pointerId
                && e.pointerType === this.pointerType;

        }



        onPointerMove(e: PointerEvent<SVGSVGElement>): void {
            if (this.isCapturedPointer(e)) {
                e.preventDefault();
                this.updateAngle(e)
            }
        }


        commitAngle() {
            let previewValue = this.angleToValue(this.currentAngle);
            this.props.onSetValue(previewValue);
            this.defaultValue = previewValue;


        }
        updateAngle(e: PointerEvent<SVGSVGElement>): void {
            let angle: number = this.pointerToAngle(e);
            if (!isNaN(angle)) {
                if (!isNaN(this.lastPointerAngle)) {
                    let dAngle = angle-this.lastPointerAngle;
                    while (dAngle > 180) dAngle -= 360;
                    while (dAngle < -180) dAngle += 360;

                    let scale = 1.0;
                    if (this.pointersDown === 2)
                    {
                        scale = FINE_RANGE_SCALE;
                    } else if (this.pointersDown >= 3)
                    {
                        scale = ULTRA_FINE_RANGE_SCALE;
                    }

                    this.currentAngle += dAngle*scale;
                    let previewValue = this.angleToValue(this.currentAngle);
                    this.previewValue(previewValue);
                    this.props.onPreviewValue(previewValue);
                }
                this.lastPointerAngle = angle;
            }
        }
        makeRotationTransform(angle: number): string
        {
            return "rotate(" + angle + "deg)";
        }

        getDefaultRotationTransform(): string
        {
            let v = this.getCurrentValue();
            let a = this.valueToAngle(v);
            return this.makeRotationTransform(a);
        }
        previewValue(value: number): void
        {
            if (this.imgRef.current)
            {
                let img = this.imgRef.current;
                let angle = this.valueToAngle(value);
                img.style.transform = this.makeRotationTransform(angle);
            }

        }


        onBodyPointerDownCapture(e_: any): any {
            let e = e_ as PointerEvent;
            if (this.isExtraTouch(e)) {
                this.captureElement!.setPointerCapture(e.pointerId);
                this.capturedPointers.push(e.pointerId);
                ++this.pointersDown;
            }

        }
        onDrag(e: SyntheticEvent) {
            e.preventDefault();
            e.stopPropagation();
            return false;
        }



        render() {
            let classes = this.props.classes;
            return (
                <DialIcon ref={this.imgRef} className={classes.dialIcon}
                    style={{
                         transform: this.getDefaultRotationTransform(),
                         width: this.props.size, height: this.props.size, 
                    }}
                    onTouchStart={this.onTouchStart} onTouchMove={this.onTouchMove}
                    onPointerDown={this.onPointerDown} onPointerUp={this.onPointerUp}
                    onPointerMoveCapture={this.onPointerMove} onDrag={this.onDrag}
                />
            );
        }
    }
);

export default ZoomedDial;
