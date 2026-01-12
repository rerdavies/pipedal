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

import React, { TouchEvent, PointerEvent, ReactNode, Component, SyntheticEvent } from 'react';
import { css } from '@emotion/react';
import Button from '@mui/material/Button';
import { Theme } from '@mui/material/styles';
import WithStyles, { withTheme } from './WithStyles';
import { createStyles } from './WithStyles';
import ControlTooltip from './ControlTooltip';

import { withStyles } from "tss-react/mui";
import { UiControl, ScalePoint } from './Lv2Plugin';
import Typography from '@mui/material/Typography';
import Input from '@mui/material/Input';
import Select from '@mui/material/Select';
import Switch from '@mui/material/Switch';
import Utility from './Utility';
import MenuItem from '@mui/material/MenuItem';
import { PiPedalModel, PiPedalModelFactory } from './PiPedalModel';
import DialIcon from './svg/fx_dial.svg?react';
import { isDarkMode } from './DarkMode';


import PlayArrowIcon from '@mui/icons-material/PlayArrow';
import StopIcon from '@mui/icons-material/Stop';
import FiberManualRecordIcon from '@mui/icons-material/FiberManualRecord';
import GraphicEqCtl, { UpdateGraphicEqPath } from './GraphicEqCtl';

const MIN_ANGLE = -135;
const MAX_ANGLE = 135;
const FONT_SIZE = "0.8em";


function isMobileDevice() {
    return (navigator.maxTouchPoints && navigator.maxTouchPoints > 1);
}


enum ButtonStyle { None, Trigger, Momentary, MomentaryOnByDefault }


const SELECTED_OPACITY = 0.8;
const DEFAULT_OPACITY = 0.6;
const RANGE_SCALE = 120; // 120 pixels to move from 0 to 1.
const FINE_RANGE_SCALE = RANGE_SCALE * 10; // 1200 pixels to move from 0 to 1.
const ULTRA_FINE_RANGE_SCALE = RANGE_SCALE * 50; // 12000 pixels to move from 0 to 1.


export const StandardItemSize = { width: 80, height: 140 }


function preventNextClickAfterDrag() {
    // the broswer fires a click event when the pointer goes up 
    // on ANY element under the mouse. Prevent this click event 
    // (and any other click event) from happening for 100ms
    // after the drag stops.
    let clickHandler = (e: MouseEvent) => {
        e.preventDefault();
        e.stopPropagation();
    };
    document.addEventListener('click', clickHandler, true);
    window.setTimeout(() => {
        document.removeEventListener('click', clickHandler, true);
    }, 100);
}

function androidEmoji(text: string) {
    // android chrome doesn't support these characters properly.
    if (text === "⏹") {
        return (<StopIcon fontSize={"medium"} />);
    }
    if (text === "⏺") {
        return (<FiberManualRecordIcon fontSize={"medium"} />)
    }
    if (text === "⏵") {
        return (<PlayArrowIcon fontSize={"medium"} />);
    }
    return text;
}

export const pluginControlStyles = (theme: Theme) => createStyles({
    frame: css({
        position: "relative",
        margin: "12px"
    }),
    switchTrack: css({
        height: '100%',
        width: '100%',
        borderRadius: 14 / 2,
        zIndex: -1,
        transition: theme.transitions.create(['opacity', 'background-color'], {
            duration: theme.transitions.duration.shortest
        }),
        backgroundColor: theme.palette.primary.main,
        opacity: theme.palette.mode === 'light' ? 0.38 : 0.3
    }),
    controlFrame: css({
        display: "flex", flexDirection: "column", alignItems: "center", justifyContent: "space-between", height: 116
    }),

    titleSection: css({
        flex: "0 0 auto", alignSelf: "stretch", marginBottom: 8, marginLeft: 0, marginRight: 0
    }),
    displayValue: css({
        position: "absolute",
        top: 0,
        left: 0,
        right: 0,
        bottom: 4,
        textAlign: "center",
        background: "transparent",
        color: theme.palette.text.secondary,
        // zIndex: -1,
    }),
    midSection: css({
        flex: "1 1 1", display: "flex", flexFlow: "column nowrap", alignContent: "center", justifyContent: "center"
    }),
    editSection: css({
        flex: "0 0 0", display: "flex", flexFlow: "column nowrap", justifyContent: "center", position: "relative", width: 60, height: 28, minHeight: 28
    })

});


export interface PluginControlProps extends WithStyles<typeof pluginControlStyles> {
    uiControl?: UiControl;
    instanceId: number;
    value: number;
    onPreviewChange?: (value: number) => void;
    onChange: (value: number) => void;
    theme: Theme;
    requestIMEEdit: (uiControl: UiControl, value: number) => void;
}
export type PluginControlState = {
    error: boolean;
    editFocused: boolean;
    previewValue?: string;
};

const PluginControl =
    withTheme(withStyles(
        class extends Component<PluginControlProps, PluginControlState> {

            frameRef: React.RefObject<HTMLDivElement | null>;
            imgRef: React.RefObject<SVGSVGElement | null>;
            graphicEqControlRef: React.RefObject<SVGSVGElement | null>;
            inputRef: React.RefObject<HTMLInputElement | null>;
            selectRef: React.RefObject<HTMLSelectElement | null>;

            displayValueRef: React.RefObject<HTMLDivElement | null>;
            model: PiPedalModel;


            constructor(props: PluginControlProps) {
                super(props);

                this.state = {
                    error: false,
                    editFocused: false,
                    previewValue: undefined

                };
                this.model = PiPedalModelFactory.getInstance();
                this.imgRef = React.createRef();
                this.graphicEqControlRef = React.createRef();
                this.inputRef = React.createRef();
                this.selectRef = React.createRef();
                this.displayValueRef = React.createRef();
                this.frameRef = React.createRef();


                this.onPointerDown = this.onPointerDown.bind(this);
                this.onPointerUp = this.onPointerUp.bind(this);
                this.onPointerMove = this.onPointerMove.bind(this);
                this.onPointerLostCapture = this.onPointerLostCapture.bind(this);
                this.onBodyPointerDownCapture = this.onBodyPointerDownCapture.bind(this);

                this.onTouchStart = this.onTouchStart.bind(this);
                this.onTouchMove = this.onTouchMove.bind(this);
                //this.onTouchCancel = this.onTouchCancel.bind(this);

                this.onSelectChanged = this.onSelectChanged.bind(this);
                this.onInputChange = this.onInputChange.bind(this);
                this.onInputLostFocus = this.onInputLostFocus.bind(this);
                this.onInputFocus = this.onInputFocus.bind(this);
                this.onInputKeyPress = this.onInputKeyPress.bind(this);
            }
            isTouchDevice(): boolean {
                return Utility.needsZoomedControls();
            }

            showZoomedControl() {
                if (this.props.uiControl && this.frameRef.current) {
                    this.model.zoomUiControl(this.frameRef.current, this.props.instanceId, this.props.uiControl);
                }
            }
            hideZoomedControl() {
                if (this.frameRef.current && this.model.zoomedUiControl.get()?.source === this.frameRef.current) {
                    this.model.clearZoomedControl();
                }
            }

            componentWillUnmount() {
                this.hideZoomedControl();
            }
            inputChanged: boolean = false;

            onInputLostFocus(event: any): void {
                this.setState({ editFocused: false });
                if (this.inputChanged) // validation requried?
                {
                    this.inputChanged = false;
                    this.validateInput(event, true);

                }

                //this.displayValueRef.current!.style.display = "block";
            }
            onInputFocus(event: SyntheticEvent): void {
                //this.displayValueRef.current!.style.display = "none";
                this.setState({ editFocused: true });
                // if (Utility.hasIMEKeyboard()) {
                //     event.preventDefault();
                //     event.stopPropagation();
                //     this.inputRef.current?.blur();
                //     this.props.requestIMEEdit(nullCast(this.props.uiControl), this.props.value)
                // }
            }
            onInputKeyPress(e: any): void {
                if (e.charCode === 13 && this.inputChanged) {
                    this.inputChanged = false;
                    this.validateInput(e, true);
                }
            }

            onInputChange(event: any): void {
                this.inputChanged = true;
                this.validateInput(event, false);
            }
            validateInput(event: any, commitValue: boolean) {
                if (!this.inputRef.current) return;

                let text = this.inputRef.current.value;
                let valid = false;
                let result: number = this.currentValue;
                try {
                    if (text.length === 0) {
                        valid = false;
                    } else {
                        let v = Number(text);
                        if (isNaN(v)) {
                            valid = false;
                        } else {
                            valid = true;
                            result = v;
                        }
                    }
                } catch (error) {
                    valid = false;
                }

                if (commitValue) {
                    this.setState({ error: false });
                    if (!valid) {
                        result = this.currentValue; // reset the value!
                    }
                    // clamp and quantize.
                    let range = this.valueToRange(result);
                    result = this.rangeToValue(range);
                    let displayVal = this.props.uiControl?.formatShortValue(result) ?? "";
                    if (event.currentTarget) {
                        event.currentTarget.value = displayVal;
                    }
                    this.previewInputValue(result, true);
                    this.inputRef.current.value = displayVal; // no rerender because the value won't change.
                } else {
                    this.setState({ error: !valid });
                    if (valid) {
                        this.previewInputValue(result, false);
                    }
                }
            }

            startX: number = 0;
            startY: number = 0;
            mouseDown: boolean = false;

            onDrag(e: SyntheticEvent) {
                e.preventDefault();
                e.stopPropagation();
            }

            isValidPointer(e: PointerEvent): boolean {
                if (e.pointerType === "mouse") {
                    return e.button === 0;
                } else if (e.pointerType === "pen") {
                    return true;
                } else if (e.pointerType === "touch") {
                    return true;
                }
                return false;
            }

            touchDown: boolean = false;
            touchIdentifier: number = -1;

            private isTap: boolean = false;

            onTouchStart(e: TouchEvent<SVGSVGElement>) {
            }
            onTouchMove(e: TouchEvent<SVGSVGElement>) {
                // e.preventDefault();
                e.stopPropagation(); // cancels scroll!!!

            }

            capturedPointers: number[] = [];

            onBodyPointerDownCapture(e_: any): any {
                let e = e_ as PointerEvent;
                if (this.isExtraTouch(e)) {
                    this.isTap = false;
                    this.captureElement!.setPointerCapture(e.pointerId);
                    this.capturedPointers.push(e.pointerId);
                    ++this.pointersDown;
                }

            }

            pointerId: number = 0;
            pointerType: string = "";

            isCapturedPointer(e: PointerEvent): boolean {
                return this.mouseDown
                    && e.pointerId === this.pointerId
                    && e.pointerType === this.pointerType;

            }

            lastX: number = 0;
            lastY: number = 0;
            dRange: number = 0;

            pointersDown: number = 0;
            captureElement?: SVGSVGElement = undefined;

            onPointerDown(e: PointerEvent): void {
                if (!this.mouseDown && this.isValidPointer(e)) {
                    e.preventDefault();
                    e.stopPropagation();
                    if (document.activeElement) {
                        let e = document.activeElement as any;
                        if (e.blur) {
                            e.blur();
                        }
                    }

                    // if (this.isTouchDevice()) {
                    //     if (this.props.uiControl
                    //         && (this.props.uiControl.isDial() || this.props.uiControl.isGraphicEq())
                    //     ) {
                    //         this.isTap = false;
                    //         this.showZoomedControl();
                    //         return;
                    //     }
                    // }

                    ++this.pointersDown;

                    this.mouseDown = true;
                    if (this.pointersDown === 1) {
                        this.isTap = true;
                        this.tapStartMs = e.timeStamp;
                    } else {
                        this.isTap = false;
                    }

                    this.pointerId = e.pointerId;
                    this.pointerType = e.pointerType;
                    this.startX = e.clientX;
                    this.startY = e.clientY;
                    this.lastX = e.clientX;
                    this.lastY = e.clientY;
                    this.dRange = 0;

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
                    this.setState({ previewValue: undefined });

                } else {
                    if (this.isExtraTouch(e)) {
                        ++this.pointersDown;
                        this.isTap = false;

                    }
                }
            }
            isExtraTouch(e: PointerEvent): boolean {
                return (this.mouseDown && this.pointerType === "touch" && e.pointerType === "touch" && e.pointerId !== this.pointerId);

            }

            onPointerLostCapture(e: PointerEvent) {
                if (this.isCapturedPointer(e)) {
                    if (this.pointersDown !== 0) {
                        --this.pointersDown;
                    }
                    this.isTap = false;


                    this.releaseCapture(e);
                }

            }

            updateRange(e: PointerEvent): number {

                let ultraHigh = false;
                let high = false;
                if (e.ctrlKey) {
                    ultraHigh = true;
                }
                if (e.shiftKey) {
                    high = true;
                }
                if (e.pointerType === "touch") {
                    if (this.pointersDown >= 3) {
                        ultraHigh = true;
                    } else if (this.pointersDown === 2) {
                        high = true;
                    }
                }
                if (ultraHigh) {
                    this.dRange += (this.lastY - e.clientY) / ULTRA_FINE_RANGE_SCALE;

                } else if (high) {
                    this.dRange += (this.lastY - e.clientY) / FINE_RANGE_SCALE;

                } else {
                    this.dRange += (this.lastY - e.clientY) / RANGE_SCALE;
                }
                this.lastY = e.clientY;
                this.lastX = e.clientX;
                return this.dRange;
            }
            private lastTapMs = 0;

            resetToDefaultValue(uiControl: UiControl): void {
                let value = uiControl.default_value;
                this.model.setPedalboardControl(this.props.instanceId, uiControl.symbol, value);
            }
            onPointerDoubleTap() {
                let uiControl = this.props.uiControl;
                if (uiControl) {
                    if (uiControl.isDial() || uiControl.isGraphicEq()) {
                        this.resetToDefaultValue(uiControl);
                    }
                }
            }
            onPointerTap(e: PointerEvent) {
                let tapTime = e.timeStamp;
                let dT = tapTime - this.lastTapMs;

                
                this.lastTapMs = tapTime;

                if (dT < 500) {
                    this.onPointerDoubleTap();
                }
            }
            private tapStartMs: number = 0;



            onPointerUp(e: PointerEvent) {

                if (this.isCapturedPointer(e)) {
                    if (this.pointersDown !== 0) {
                        --this.pointersDown;
                    }
                    if (this.pointersDown === 0) {
                        this.setState({ previewValue: undefined });
                    }
                    e.preventDefault();
                    e.stopPropagation();

                    let dRange = this.updateRange(e)
                    this.previewRange(dRange, true);

                    this.releaseCapture(e);
                    if (this.isTap) {
                        let ms = e.timeStamp - this.tapStartMs;
                        if (ms < 200) {
                            this.onPointerTap(e);
                        } 
                    }
                    // prevent click from firing on other elements 
                    // when the pointer goes up0.
                    preventNextClickAfterDrag();

                } else {
                    if (this.pointersDown !== 0) {
                        --this.pointersDown;
                    }

                }
            }

            releaseCapture(e: PointerEvent) {
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
                    this.onBodyPointerDownCapture, true
                );

                this.mouseDown = false;

            }

            clickSlop() {
                return 5;  // maybe larger on touch devices.
            }
            onPointerMove(e: PointerEvent): void {
                if (this.isCapturedPointer(e)) {
                    e.preventDefault();
                    let dRange = this.updateRange(e)
                    this.previewRange(dRange, false);

                    let x = e.clientX;
                    let y = e.clientY;
                    let dx = x - this.startX;
                    let dy = y - this.startY;
                    let distance = Math.sqrt(dx * dx + dy * dy);
                    if (distance >= this.clickSlop()) {
                        this.isTap = false;
                    }
                }

            }
            handleButtonMouseLeave(buttonStyle: ButtonStyle) {
            }
            handleButtonMouseDown(buttonStyle: ButtonStyle) {
                let uiControl = this.props.uiControl;
                if (uiControl) {
                    switch (buttonStyle) {
                        case ButtonStyle.Momentary:
                            this.model.setPedalboardControl(this.props.instanceId, uiControl.symbol, uiControl.max_value);
                            break;
                        case ButtonStyle.MomentaryOnByDefault:
                            this.model.setPedalboardControl(this.props.instanceId, uiControl.symbol, uiControl.min_value);
                            break;
                        case ButtonStyle.Trigger:
                            {
                                let value = uiControl.max_value;
                                if (uiControl.max_value === uiControl.default_value) {
                                    value = uiControl.min_value;
                                }
                                this.model.sendPedalboardControlTrigger(this.props.instanceId, uiControl.symbol, value);

                            }
                            break;
                    }
                }
            }
            handleButtonMouseUp(buttonStyle: ButtonStyle) {
                let uiControl = this.props.uiControl;
                if (uiControl) {
                    switch (buttonStyle) {
                        case ButtonStyle.Momentary:
                            this.model.setPedalboardControl(this.props.instanceId, uiControl.symbol, uiControl.min_value);
                            break;
                        case ButtonStyle.MomentaryOnByDefault:
                            this.model.setPedalboardControl(this.props.instanceId, uiControl.symbol, uiControl.max_value);
                            break;
                        case ButtonStyle.Trigger:
                            {
                                // trigger values are reset automatically.
                            }
                            break;
                    }
                }
                this.setState({ previewValue: undefined});
            }
            previewInputValue(value: number, commitValue: boolean) {
                let range = this.valueToRange(value);
                value = this.rangeToValue(range);

                let imgElement = this.imgRef.current
                if (imgElement) {
                    if (this.props.uiControl?.isGraphicEq()) {
                        let imgElement = this.imgRef.current
                        if (imgElement) {
                            UpdateGraphicEqPath(imgElement, range);
                        }
                    } else {

                        let transform = this.rangeToRotationTransform(range);
                        if (this.mouseDown && !commitValue) {
                            transform += " scale(1.5, 1.5)";
                        }
                        if (imgElement.style) {
                            imgElement.style.transform = transform;
                        }
                    }
                }
                if (commitValue) {
                    this.props.onChange(value);
                }
                if (this.props.onPreviewChange) {
                    this.props.onPreviewChange(value);
                }

            }
            previewRange(dRange: number, commitValue: boolean): void {
                let range = this.valueToRange(this.currentValue) + dRange;
                if (range > 1) range = 1;
                if (range < 0) range = 0;
                let value = this.rangeToValue(range);

                // apply value quantization and clipping.
                range = this.valueToRange(value);

                if (this.props.uiControl?.isGraphicEq()) {
                    let imgElement = this.imgRef.current
                    if (imgElement) {
                        UpdateGraphicEqPath(imgElement, range);
                    }

                } else {
                    let imgElement = this.imgRef.current
                    if (imgElement) {
                        if (imgElement.style) {
                            imgElement.style.transform = this.rangeToRotationTransform(range);
                        }
                    }
                }
                let inputElement = this.inputRef.current;
                if (inputElement) {
                    let v = this.props.uiControl?.formatShortValue(value) ?? "";
                    inputElement.value = v;
                }
                let displayValue = this.displayValueRef.current;
                if (displayValue) {
                    let v = this.formatDisplayValue(this.props.uiControl, value);
                    displayValue.childNodes[0].textContent = v;

                    if (commitValue) {
                        this.setState({ previewValue: undefined });
                    } else {
                        this.setState({ previewValue: v });
                    }   
                }
                let selectElement = this.selectRef.current;
                if (selectElement) {
                    //let v = this.formatValue(this.props.uiControl,value);
                    selectElement.selectedIndex = value;

                }
                if (commitValue) {
                    this.currentValue = value;
                    this.props.onChange(value);
                } else {
                    if (this.props.onPreviewChange) {
                        this.props.onPreviewChange(value);
                    }
                }
            }

            onCheckChanged(checked: boolean): void {
                this.props.onChange(checked ? 1 : 0);
            }
            onSelectChanged(e: any, value: any) {
                let target = e.target;
                setTimeout(() => {
                    this.props.onChange(target.value);
                }, 0);


            }

            makeSelect(control: UiControl, value: number): ReactNode {
                if (control.isOnOffSwitch()) {
                    // normal gray unchecked state.
                    return (
                        <ControlTooltip uiControl={control}>
                            <Switch checked={value !== 0} color="primary"
                                onChange={(event) => {
                                    this.onCheckChanged(event.target.checked);
                                }}
                            />
                        </ControlTooltip>
                    );
                }
                if (control.isAbToggle()) {
                    let classes = withStyles.getClasses(this.props);
                    // unchecked color is not gray.
                    return (
                        <ControlTooltip uiControl={control}>
                            <Switch checked={value !== 0} color="primary"
                                onChange={(event) => {
                                    this.onCheckChanged(event.target.checked);
                                }}
                                classes={{
                                    track: classes.switchTrack
                                }}
                                style={{ color: this.props.theme.palette.primary.main }}
                            />
                        </ControlTooltip>
                    );
                } else {
                    return (
                        <ControlTooltip uiControl={control}>
                            <Select variant="standard"
                                ref={this.selectRef}
                                value={control.clampSelectValue(value)}
                                onChange={this.onSelectChanged}
                                inputProps={{
                                    name: control.name,
                                    id: 'id' + control.symbol,
                                    style: { fontSize: FONT_SIZE }
                                }}
                                style={{ marginLeft: 4, marginRight: 4, width: 140, fontSize: 14 }}
                            >
                                {control.scale_points.map((scale_point: ScalePoint) => (
                                    <MenuItem key={scale_point.value} value={scale_point.value}>{scale_point.label}</MenuItem>

                                ))}
                            </Select>
                        </ControlTooltip>
                    );
                }
            }

            formatDisplayValue(uiControl: UiControl | undefined, value: number): string {
                if (!uiControl) return "";

                return uiControl.formatDisplayValue(value);


            }

            valueToRange(value: number): number {
                let uiControl = this.props.uiControl;
                if (uiControl) {
                    if (uiControl.integer_property) {
                        value = Math.round(value);
                    }
                    let range: number;
                    if (uiControl.is_logarithmic) {
                        let minValue = uiControl.min_value;
                        if (minValue === 0) // LSP plugins do this.
                        {
                            minValue = 0.0001;
                        }
                        range = Math.log(value / minValue) / Math.log(uiControl.max_value / minValue);
                        if (!isFinite(range)) {
                            if (range < 0) {
                                range = 0;
                            } else {
                                range = 1.0;
                            }
                        } else if (isNaN(range)) {
                            range = 0;
                        }
                        if (uiControl.range_steps > 1) {
                            range = Math.round(range * (uiControl.range_steps - 1)) / (uiControl.range_steps - 1);
                        }
                    } else {
                        range = (value - uiControl.min_value) / (uiControl.max_value - uiControl.min_value);
                        if (!isFinite(range)) {
                            if (range < 0) {
                                range = 0;
                            } else {
                                range = 1.0;
                            }
                        } else if (isNaN(range)) {
                            range = 0;
                        }
                    }

                    if (range > 1) range = 1;
                    if (range < 0) range = 0;
                    return range;
                }
                return 0;
            }
            rangeToValue(range: number): number {
                if (range < 0) range = 0;
                if (range > 1) range = 1;
                let uiControl = this.props.uiControl;
                if (uiControl) {
                    if (uiControl.range_steps > 1) {
                        range = Math.round(range * (uiControl.range_steps - 1)) / (uiControl.range_steps - 1);
                    }
                    let value: number;
                    if (uiControl.min_value === uiControl.max_value) {
                        value = uiControl.min_value;
                    } else {
                        if (uiControl.is_logarithmic) {
                            let minValue = uiControl.min_value;
                            if (minValue === 0) // LSP controls.    
                            {
                                minValue = 0.0001;
                            }
                            value = minValue * Math.pow(uiControl.max_value / minValue, range);
                            if (!isFinite(value)) {
                                value = uiControl.max_value;
                            } else if (isNaN(value)) {
                                value = uiControl.min_value;
                            }
                        } else {
                            value = range * (uiControl.max_value - uiControl.min_value) + uiControl.min_value;
                        }
                        if (uiControl.integer_property) {
                            value = Math.round(value);
                        }
                    }
                    return value;
                }
                return 0;
            }

            rangeToRotationTransform(range: number): string {
                let angle = range * (MAX_ANGLE - MIN_ANGLE) + MIN_ANGLE;
                return "rotate(" + angle + "deg)";
            }

            getEqPosition(): number {
                let range = 0;
                let uiControl = this.props.uiControl;
                if (uiControl) {
                    let value = this.props.value;
                    range = this.valueToRange(value);
                }
                return range;

            }
            getRotationTransform(): string {
                let range = 0;
                let uiControl = this.props.uiControl;
                if (uiControl) {
                    let value = this.props.value;
                    range = this.valueToRange(value);
                }
                return this.rangeToRotationTransform(range);
            }

            currentValue: number = 0;
            uiControl?: UiControl = undefined;

            render() {
                const classes = withStyles.getClasses(this.props);
                let t = this.props.uiControl;
                if (!t) {
                    return (<div>#Error</div>);

                }
                let dialColor = this.props.theme.palette.text.primary;
                let control: UiControl = t;
                this.uiControl = control;
                let value = this.props.value;
                this.currentValue = value;
                let switchText = "";

                let isSelect = control.isSelect();
                let isAbSwitch = control.isAbToggle();
                let isOnOffSwitch = control.isOnOffSwitch();

                let isButton = false;
                let buttonStyle: ButtonStyle = ButtonStyle.None;
                let isGraphicEq = control.isGraphicEq();

                if (control.isTrigger()) {
                    isButton = true;
                    buttonStyle = ButtonStyle.Trigger;
                }
                if (control.isMomentary()) {
                    isButton = true;
                    buttonStyle = ButtonStyle.Momentary;
                }
                if (control.isMomentaryOnByDefault()) {
                    isButton = true;
                    buttonStyle = ButtonStyle.MomentaryOnByDefault;
                }

                if (isAbSwitch) {
                    switchText = control.scale_points[0].value === value ? control.scale_points[0].label : control.scale_points[1].label;
                }

                let item_width: number | undefined = isSelect ? 160 : 80;
                if (isButton) {
                    item_width = undefined;
                }
                if (isGraphicEq) {
                    item_width = 58;
                }


                return (
                    <div ref={this.frameRef}
                        className={classes.controlFrame}
                        style={{ width: item_width }}
                    >
                        {/* TITLE SECTION */}
                        <div className={classes.titleSection}
                            style={
                                {
                                    alignSelf: "stretch", marginBottom: 8, marginLeft: isSelect ? 8 : 0, marginRight: 0

                                }}>
                            <Typography variant="caption" display="block" noWrap style={{
                                width: "100%",
                                textAlign: isSelect ? "left" : "center"
                            }}> {isButton ? "\u00A0" : control.name}</Typography>
                        </div>
                        {/* CONTROL SECTION */}

                        <div className={classes.midSection}>

                            {isButton ?
                                (
                                    control.name.length !== 1 ? (
                                        <ControlTooltip uiControl={control}>

                                            <Button variant="contained" color="primary" size="small"
                                                onMouseDown={
                                                    (evt) => { this.handleButtonMouseDown(buttonStyle); }
                                                }
                                                onMouseUp={
                                                    (evt) => { this.handleButtonMouseUp(buttonStyle); }
                                                }
                                                onTouchStart={
                                                    (evt) => {
                                                        evt.preventDefault();
                                                        this.handleButtonMouseDown(buttonStyle);
                                                    }
                                                }
                                                onTouchEnd={
                                                    (evt) => {
                                                        evt.preventDefault();
                                                        this.handleButtonMouseUp(buttonStyle);
                                                    }
                                                }
                                                onMouseLeave={(
                                                    (evet) => { this.handleButtonMouseLeave(buttonStyle); }
                                                )}

                                                style={{
                                                    textTransform: "none",
                                                    background: (isDarkMode() ? "#6750A4" : undefined),
                                                    marginLeft: 8, marginRight: 8, minWidth: 60,
                                                    marginTop: 0

                                                }}

                                            >
                                                {control.name}
                                            </Button>
                                        </ControlTooltip>

                                    ) : (
                                        <ControlTooltip uiControl={control}>
                                            <Button variant="contained" color="primary" size="small"
                                                onMouseDown={
                                                    (evt) => { this.handleButtonMouseDown(buttonStyle); }
                                                }
                                                onMouseUp={
                                                    (evt) => { this.handleButtonMouseUp(buttonStyle); }
                                                }
                                                onTouchStart={
                                                    (evt) => {
                                                        evt.preventDefault();
                                                        this.handleButtonMouseDown(buttonStyle);
                                                    }
                                                }
                                                onTouchEnd={
                                                    (evt) => {
                                                        evt.preventDefault();
                                                        this.handleButtonMouseUp(buttonStyle);
                                                    }
                                                }
                                                onMouseLeave={(
                                                    (evet) => { this.handleButtonMouseLeave(buttonStyle); }
                                                )}

                                                style={{
                                                    textTransform: "none",
                                                    background: (isDarkMode() ? "#6750A4" : undefined),
                                                    marginLeft: 8, marginRight: 8,
                                                    paddingLeft: 0, paddingRight: 0,
                                                    width: 36, height: 36,
                                                    marginTop: 0,
                                                    borderRadius: 8,
                                                    minWidth: 0,
                                                    fontSize: "1.2em"

                                                }}

                                            >
                                                {androidEmoji(control.name)}
                                            </Button>
                                        </ControlTooltip>
                                    )
                                )

                                : (isSelect || isAbSwitch || isOnOffSwitch) ? (
                                    this.makeSelect(control, value)
                                )
                                    : (isGraphicEq) ? (
                                        <div style={{ flex: "0 1 auto" }}>
                                            
                                            <ControlTooltip uiControl={control} valueTooltip={this.state.previewValue}>
                                                <GraphicEqCtl
                                                    imgRef={this.imgRef}
                                                    position={this.getEqPosition()}
                                                    dialColor={dialColor}
                                                    opacity={DEFAULT_OPACITY}

                                                    onTouchStart={this.onTouchStart}
                                                    onTouchMove={this.onTouchMove}
                                                    onPointerDown={this.onPointerDown} onPointerUp={this.onPointerUp}
                                                    onPointerMoveCapture={this.onPointerMove}
                                                    onDrag={this.onDrag}


                                                />
                                            </ControlTooltip>
                                        </div>
                                    ) : (
                                        <div style={{ flex: "0 1 auto" }}>
                                            <ControlTooltip uiControl={control} 
                                                valueTooltip={this.state.previewValue}>
                                                <DialIcon ref={this.imgRef}
                                                    style={{
                                                        overscrollBehavior: "none", touchAction: "none", fill: dialColor,
                                                        width: 36, height: 36, opacity: DEFAULT_OPACITY, transform: this.getRotationTransform()
                                                    }}
                                                    onTouchStart={this.onTouchStart} onTouchMove={this.onTouchMove}
                                                    onPointerDown={this.onPointerDown} onPointerUp={this.onPointerUp}
                                                    onPointerMove={this.onPointerMove}
                                                    onDrag={this.onDrag}

                                                />
                                            </ControlTooltip>
                                        </div>
                                    )
                            }
                        </div>

                        {/* LABEL/EDIT SECTION*/}
                        <div className={classes.editSection} >
                            {(!(isSelect || isOnOffSwitch || isButton)) &&
                                (
                                    (isAbSwitch) ? (
                                        <Typography variant="caption" display="block" textAlign="center" noWrap style={{
                                            width: "100%"
                                        }}> {switchText} </Typography>
                                    ) : (
                                        <div>
                                            <Input key={value}
                                                defaultValue={control.formatShortValue(value)}
                                                error={this.state.error}
                                                inputProps={{
                                                    className: "scrollMod",
                                                    type: isMobileDevice()? "text": "number",
                                                    inputMode: "numeric",

                                                    min: this.props.uiControl?.min_value,
                                                    max: this.props.uiControl?.max_value,
                                                }}

                                                sx={{
                                                    // Style the input element
                                                    '& input': {
                                                        width: 60,
                                                        opacity: this.state.editFocused ? 1 : 0,
                                                        textAlign: "center", fontSize: FONT_SIZE,
                                                        borderBottom: "0px",
                                                    },
                                                }}

                                                inputRef={this.inputRef} onChange={this.onInputChange}
                                                onBlur={this.onInputLostFocus}
                                                onFocus={this.onInputFocus}

                                                onKeyPress={this.onInputKeyPress} />
                                            <div className={classes.displayValue}
                                                ref={this.displayValueRef} onClick={(e) => { this.inputRef.current!.focus(); }}
                                                style={{ display: this.state.editFocused ? "none" : "block" }}
                                            >
                                                <Typography noWrap color="inherit" style={{ fontSize: "12.8px", paddingTop: 4, paddingBottom: 6 }}

                                                >
                                                    {this.formatDisplayValue(control, value)}</Typography>
                                            </div>
                                        </div>
                                    )
                                )
                            }
                        </div>
                    </div >
                );
            }
            /*
            isSamePointer(PointerEvent e): boolean
            {
                return e.pointerId === this.pointerId
                    && e.pointerType === this.pointerType;
            }
            */
            touchPointerId: number = 0;


        },
        pluginControlStyles
    ));

export default PluginControl;