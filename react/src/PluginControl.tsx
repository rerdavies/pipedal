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
import { Theme } from '@mui/material/styles';
import { WithStyles } from '@mui/styles';
import createStyles from '@mui/styles/createStyles';
import withStyles from '@mui/styles/withStyles';
import { UiControl, ScalePoint } from './Lv2Plugin';
import Typography from '@mui/material/Typography';
import Input from '@mui/material/Input';
import Select from '@mui/material/Select';
import Switch from '@mui/material/Switch';
import Utility, {nullCast} from './Utility';
import MenuItem from '@mui/material/MenuItem';
import {PiPedalModel,PiPedalModelFactory} from './PiPedalModel';


const MIN_ANGLE = -135;
const MAX_ANGLE = 135;
const FONT_SIZE = "0.8em";



const SELECTED_OPACITY = 0.8;
const DEFAULT_OPACITY = 0.6;
const RANGE_SCALE = 120; // 120 pixels to move from 0 to 1.
const FINE_RANGE_SCALE = RANGE_SCALE * 10; // 1200 pixels to move from 0 to 1.
const ULTRA_FINE_RANGE_SCALE = RANGE_SCALE * 50; // 12000 pixels to move from 0 to 1.


export const StandardItemSize = { width: 80, height: 140 }



const styles = (theme: Theme) => createStyles({
    frame: {
        position: "relative",
        margin: "12px"
    },
    switchTrack: {
        height: '100%',
        width: '100%',
        borderRadius: 14 / 2,
        zIndex: -1,
        transition: theme.transitions.create(['opacity', 'background-color'], {
            duration: theme.transitions.duration.shortest
        }),
        backgroundColor: theme.palette.secondary.main,
        opacity: theme.palette.mode === 'light' ? 0.38 : 0.3
    },
    displayValue: {
        position: "absolute",
        top: 0,
        left: 0,
        right: 0,
        bottom: 4,
        textAlign: "center",
        background: "white",
        color: "#666",
        // zIndex: -1,
    }
});


export interface PluginControlProps extends WithStyles<typeof styles> {
    uiControl?: UiControl;
    instanceId: number;
    value: number;
    onPreviewChange?: (value: number) => void;
    onChange: (value: number) => void;
    theme: Theme;
    requestIMEEdit: (uiControl: UiControl, value: number) => void;
}
type PluginControlState = {
    error: boolean;
};

const PluginControl =
    withStyles(styles, { withTheme: true })(
        class extends Component<PluginControlProps, PluginControlState>
        {

            frameRef: React.RefObject<HTMLDivElement>;
            imgRef: React.RefObject<HTMLImageElement>;
            inputRef: React.RefObject<HTMLInputElement>;
            selectRef: React.RefObject<HTMLSelectElement>;

            displayValueRef: React.RefObject<HTMLDivElement>;
            model: PiPedalModel;


            constructor(props: PluginControlProps) {
                super(props);

                this.state = {
                    error: false
                };
                this.model = PiPedalModelFactory.getInstance();
                this.imgRef = React.createRef();
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
                return Utility.isTouchDevice();
            }

            showZoomedControl()
            {
                if (this.props.uiControl && this.frameRef.current)
                {
                    this.model.zoomUiControl(this.frameRef.current,this.props.instanceId,this.props.uiControl);
                }
            }
            hideZoomedControl()
            {
                if (this.frameRef.current && this.model.zoomedUiControl.get()?.source === this.frameRef.current)
                {
                    this.model.clearZoomedControl();
                }
            }

            componentWillUnmount()
            {
                this.hideZoomedControl();
            }
            inputChanged: boolean = false;

            onInputLostFocus(event: any): void {
                if (this.inputChanged) // validation requried?
                {
                    this.inputChanged = false;
                    this.validateInput(event, true);

                }
                this.displayValueRef.current!.style.display = "block";
            }
            onInputFocus(event: SyntheticEvent): void {
                this.displayValueRef.current!.style.display = "none";
                if (Utility.hasIMEKeyboard())
                {
                    event.preventDefault();
                    event.stopPropagation();
                    this.inputRef.current?.blur();
                    this.props.requestIMEEdit(nullCast(this.props.uiControl),this.props.value)
                }
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
                    let displayVal = this.props.uiControl?.formatShortValue(result)??"";
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
            }

            isValidPointer(e: PointerEvent<HTMLImageElement>): boolean {
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

            onTouchStart(e: TouchEvent<HTMLImageElement>) {
            }
            onTouchMove(e: TouchEvent<HTMLImageElement>) {
                // e.preventDefault();
                e.stopPropagation(); // cancels scroll!!!

            }

            capturedPointers: number[] = [];

            onBodyPointerDownCapture(e_: any): any
            {
                let e = e_ as PointerEvent;
                if (this.isExtraTouch(e))
                {
                    this.captureElement!.setPointerCapture(e.pointerId);
                    this.capturedPointers.push(e.pointerId);
                    ++this.pointersDown;
                }

            }

            pointerId: number = 0;
            pointerType: string = "";

            isCapturedPointer(e: PointerEvent<HTMLImageElement>): boolean {
                return this.mouseDown
                    && e.pointerId === this.pointerId
                    && e.pointerType === this.pointerType;

            }

            lastX: number = 0;
            lastY: number = 0;
            dRange: number = 0;

            pointersDown: number = 0;
            captureElement?: HTMLElement = undefined;

            onPointerDown(e: PointerEvent<HTMLImageElement>): void {
                if (!this.mouseDown && this.isValidPointer(e)) {
                    e.preventDefault();
                    e.stopPropagation();

                    if (this.isTouchDevice())
                    {
                        if (this.props.uiControl?.isDial()??false)
                        {
                            this.showZoomedControl();
                            return;
                        }
                    }

                    ++this.pointersDown;


                    this.mouseDown = true;

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
                            this.onBodyPointerDownCapture,true
                            );

                        img.setPointerCapture(e.pointerId);
                        if (img.style) {
                            img.style.opacity = "" + SELECTED_OPACITY;
                        }
                    }

                } else {
                    if (this.isExtraTouch(e))
                    {
                        ++this.pointersDown;
                        
                    }
                }
            }
            isExtraTouch(e: PointerEvent): boolean {
                return (this.mouseDown && this.pointerType === "touch" && e.pointerType === "touch" && e.pointerId !== this.pointerId);

            }

            onPointerLostCapture(e: PointerEvent<HTMLImageElement>) {
                if (this.isCapturedPointer(e)) {
                    --this.pointersDown;
                    

                    this.releaseCapture(e);
                }

            }

            updateRange(e: PointerEvent<HTMLImageElement>): number {

                let ultraHigh = false;
                let high = false;
                if (e.ctrlKey)
                {
                    ultraHigh = true;
                }
                if (e.shiftKey)
                {
                    high = true;
                }
                if (e.pointerType === "touch")
                {
                    if (this.pointersDown >= 3)
                    {
                        ultraHigh = true;
                    } else if (this.pointersDown === 2)
                    {
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
            onPointerUp(e: PointerEvent<HTMLImageElement>) {

                if (this.isCapturedPointer(e)) {
                    --this.pointersDown;
                    

                    e.preventDefault();
                    let dRange = this.updateRange(e)
                    this.previewRange(dRange, true);

                    this.releaseCapture(e);

                } else {
                    --this.pointersDown;
                    
                }
            }

            releaseCapture(e: PointerEvent<HTMLImageElement>)
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

            onPointerMove(e: PointerEvent<HTMLImageElement>): void {
                if (this.isCapturedPointer(e)) {
                    e.preventDefault();
                    let dRange = this.updateRange(e)
                    this.previewRange(dRange, false);
                }
            }

            previewInputValue(value: number, commitValue: boolean) {
                let range = this.valueToRange(value);
                value = this.rangeToValue(range);

                let transform = this.rangeToRotationTransform(range);
                if (this.mouseDown && !commitValue) {
                    transform += " scale(1.5, 1.5)";
                }
                let imgElement = this.imgRef.current
                if (imgElement) {
                    if (imgElement.style) {
                        imgElement.style.transform = transform;
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

                let imgElement = this.imgRef.current
                if (imgElement) {
                    if (imgElement.style) {
                        imgElement.style.transform = this.rangeToRotationTransform(range);
                    }
                }
                let inputElement = this.inputRef.current;
                if (inputElement) {
                    let v = this.props.uiControl?.formatShortValue(value)??"";
                    inputElement.value = v;
                }
                let displayValue = this.displayValueRef.current;
                if (displayValue)
                {
                    let v = this.formatDisplayValue(this.props.uiControl,value);
                    displayValue.childNodes[0].textContent = v;
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
                setTimeout(()=> {
                    this.props.onChange(target.value);
                },0);


            }

            makeSelect(control: UiControl, value: number): ReactNode {
                if (control.isOnOffSwitch()) {
                    // normal gray unchecked state.
                    return (
                        <Switch checked={value !== 0} color="secondary"
                            onChange={(event) => {
                                this.onCheckChanged(event.target.checked);
                            }}
                        />
                    );
                }
                if (control.isAbToggle()) {
                    // unchecked color is not gray.
                    return (
                        <Switch checked={value !== 0} color="secondary"
                            onChange={(event) => {
                                this.onCheckChanged(event.target.checked);
                            }}
                            classes={{
                                track: this.props.classes.switchTrack
                            }}
                            style={{ color: this.props.theme.palette.secondary.main }}
                        />
                    );
                } else {
                    return (
                        <Select variant="standard"
                            ref={this.selectRef}
                            value={control.clampSelectValue(value)}
                            onChange={this.onSelectChanged}
                            inputProps={{
                                name: control.name,
                                id: 'id' + control.symbol,
                                style: { fontSize: FONT_SIZE }
                            }}
                            style={{ marginLeft: 4, marginRight: 4, width: 140 }}
                        >
                            {control.scale_points.map((scale_point: ScalePoint) => (
                                <MenuItem key={scale_point.value} value={scale_point.value}>{scale_point.label}</MenuItem>

                            ))}
                        </Select>
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
                    if (uiControl.is_logarithmic)
                    {
                        range = Math.log(value/uiControl.min_value)/Math.log(uiControl.max_value/uiControl.min_value);
                        if  (uiControl.range_steps > 1) {
                            range = Math.round(range*(uiControl.range_steps-1))/(uiControl.range_steps-1);
                        }
                    } else {
                        range = (value - uiControl.min_value) / (uiControl.max_value - uiControl.min_value);
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
                        range = Math.round(range * uiControl.range_steps-1) / (uiControl.range_steps-1);
                    }
                    let value: number;
                    if (uiControl.is_logarithmic)
                    {
                        value = uiControl.min_value*Math.pow(uiControl.max_value/uiControl.min_value,range);
                    } else {
                        value = range * (uiControl.max_value - uiControl.min_value) + uiControl.min_value;
                    }
                    if (uiControl.integer_property) {
                        value = Math.round(value);
                    }
                    return value;
                }
                return 0;
            }

            rangeToRotationTransform(range: number): string {
                let angle = range * (MAX_ANGLE - MIN_ANGLE) + MIN_ANGLE;
                return "rotate(" + angle + "deg)";
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
                let classes = this.props.classes;
                let t = this.props.uiControl;
                if (!t) {
                    return (<div>#Error</div>);

                }
                let control: UiControl = t;
                this.uiControl = control;
                let value = this.props.value;
                this.currentValue = value;
                let switchText = "";

                let isSelect = control.isSelect();
                let isAbSwitch = control.isAbToggle();
                let isOnOffSwitch = control.isOnOffSwitch();

                if (isAbSwitch) {
                    switchText = control.scale_points[0].value === value ? control.scale_points[0].label : control.scale_points[1].label;
                }

                let item_width = isSelect ? 160 : 80;


                return (
                    <div ref={this.frameRef} style={{ display: "flex", flexDirection: "column", alignItems: "center", justifyContent: "flex-start", width: item_width, margin: 8 }}>
                        {/* TITLE SECTION */}
                        <div style={{ flex: "0 0 auto", width: "100%", marginBottom: 8,marginLeft: isSelect? 16: 0, marginRight: 0 }}>
                            <Typography  variant="caption" display="block" noWrap style={{
                                width: "100%",
                                textAlign: isSelect ? "left" : "center"
                            }}> {control.name}</Typography>
                        </div>
                        {/* CONTROL SECTION */}

                        <div style={{ flex: "0 0 auto" }}>
                            {(isSelect || isAbSwitch || isOnOffSwitch) ? (
                                this.makeSelect(control, value)
                            ) : (
                                <div style={{ flex: "0 1 auto" }}>
                                    <img ref={this.imgRef} src="img/fx_dial.svg"
                                        style={{ overscrollBehavior: "none", touchAction: "none",
                                         width: 36, height: 36, opacity: DEFAULT_OPACITY, transform: this.getRotationTransform() }}
                                        draggable="true"
                                        onTouchStart={this.onTouchStart} onTouchMove={this.onTouchMove}
                                        onPointerDown={this.onPointerDown} onPointerUp={this.onPointerUp} onPointerMoveCapture={this.onPointerMove} onDrag={this.onDrag}
                                        alt={control.name}
                                    />
                                </div>
                            )
                            }
                        </div>

                        {/* LABEL/EDIT SECTION*/}
                        <div style={{ flex: "0 0 auto", position: "relative", width: 60 }}>
                            {(!(isSelect || isOnOffSwitch)) &&
                                (
                                    (isAbSwitch) ? (
                                        <Typography variant="caption" display="block" noWrap style={{
                                            width: "100%", textAlign: "center"
                                        }}> {switchText} </Typography>
                                    ) : (
                                        <div>
                                            <Input key={value} 
                                                type="number"
                                                defaultValue={control.formatShortValue(value)}
                                                error={this.state.error}
                                                inputProps={{
                                                    'aria-label':
                                                        control.symbol + " value",
                                                    style: { textAlign: "center", fontSize: FONT_SIZE },
                                                }}
                                                inputRef={this.inputRef} onChange={this.onInputChange}
                                                onBlur={this.onInputLostFocus}
                                                onFocus={this.onInputFocus}

                                                onKeyPress={this.onInputKeyPress} />
                                            <div className={classes.displayValue} ref={this.displayValueRef} onClick={(e) => { this.inputRef.current!.focus(); }} >
                                                <Typography noWrap color="inherit" style={{ fontSize: "12.8px", paddingTop:4, paddingBottom: 6 }}
                                                
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
            isSamePointer(PointerEvent<HTMLImageElement> e): boolean
            {
                return e.pointerId === this.pointerId
                    && e.pointerType === this.pointerType;
            }
            */
            touchPointerId: number = 0;


        }
    );

export default PluginControl;