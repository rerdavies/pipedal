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

import React, {ReactNode,  SyntheticEvent } from 'react';
import MenuItem from '@mui/material/MenuItem';
import Select from '@mui/material/Select';
import Switch from '@mui/material/Switch';
import { UiControl, ScalePoint } from './Lv2Plugin';
import { Theme } from '@mui/material/styles';
import WithStyles, {withTheme} from './WithStyles';
import { withStyles } from "tss-react/mui";

import {createStyles} from './WithStyles';

import { ZoomedControlInfo } from './PiPedalModel';
import DialogEx from './DialogEx';
import Typography from '@mui/material/Typography';
import { PiPedalModelFactory, PiPedalModel } from './PiPedalModel';
import ZoomedDial from './ZoomedDial';
import IconButton from '@mui/material/IconButton';
import NavigateBeforeIcon from '@mui/icons-material/NavigateBefore';
import NavigateNextIcon from '@mui/icons-material/NavigateNext';


const styles = (theme: Theme) => createStyles({
    switchTrack: {
        height: '100%',
        width: '100%',
        borderRadius: 14 / 2,
        zIndex: -1,
        transition: theme.transitions.create(['opacity', 'background-color'], {
            duration: theme.transitions.duration.shortest
        }),
        backgroundColor: theme.palette.primary.main,
        opacity: theme.palette.mode === 'light' ? 0.38 : 0.3
    }

});


interface ZoomedUiControlProps extends WithStyles<typeof styles> {
    theme: Theme;
    dialogOpen: boolean;
    onDialogClose: () => void;
    onDialogClosed: () => void;
    controlInfo: ZoomedControlInfo | undefined
}

interface ZoomedUiControlState {
    value: number
}

/*
const Transition = React.forwardRef(function Transition(
    props: TransitionProps & { children?: React.ReactElement },
    ref: React.Ref<unknown>,
) {
    return <Slide direction="up" ref={ref} {...props} />;
});
*/


const ZoomedUiControl = withTheme(withStyles(

    class extends React.Component<ZoomedUiControlProps, ZoomedUiControlState> {

        model: PiPedalModel;

        constructor(props: ZoomedUiControlProps) {
            super(props);
            this.model = PiPedalModelFactory.getInstance();
            this.state = {
                value: this.getCurrentValue()
            };
        }

        getCurrentValue(): number {
            if (!this.props.controlInfo) {
                return 0;
            } else {
                let uiControl = this.props.controlInfo.uiControl;
                let instanceId = this.props.controlInfo.instanceId;
                if (instanceId === -1) return 0;
                let pedalboardItem = this.model.pedalboard.get()?.getItem(instanceId);
                let value: number = pedalboardItem?.getControlValue(uiControl.symbol) ?? 0;
                return value;
            }
        }


        onSelectChanged(val: string | number) {
            let v = Number.parseFloat(val.toString());
            this.setState({ value: v });
            if (this.props.controlInfo) {
                this.model.setPedalboardControl(
                    this.props.controlInfo.instanceId,
                    this.props.controlInfo.uiControl.symbol,
                    v);
            }
        }

        onCheckChanged(checked: boolean): void {
            let v = checked ? 1: 0;
            this.setState({ value: v });
            if (this.props.controlInfo) {
                this.model.setPedalboardControl(
                    this.props.controlInfo.instanceId,
                    this.props.controlInfo.uiControl.symbol,
                    v);
            }
        }

        hasControlChanged(oldProps: ZoomedUiControlProps, newProps: ZoomedUiControlProps) {
            if (oldProps.controlInfo === null) {
                return newProps.controlInfo !== null;
            } else if (newProps.controlInfo === null) {
                return oldProps.controlInfo !== null;
            } else {
                if (newProps.controlInfo?.instanceId !== oldProps.controlInfo?.instanceId) return true;
                if (newProps.controlInfo?.uiControl.symbol !== oldProps.controlInfo?.uiControl.symbol) return true;
            }
            return false;

        }

        componentDidUpdate(oldProps: ZoomedUiControlProps, oldState: ZoomedUiControlState) {
            if (this.hasControlChanged(oldProps, this.props)) {
                let currentValue = this.getCurrentValue();
                if (this.state.value !== currentValue) {
                    this.setState({ value: this.getCurrentValue() });
                }
            }
        }

        onDrag(e: SyntheticEvent) {
            e.preventDefault();
            e.stopPropagation();
            return false;
        }

        makeSelect(control: UiControl, value: number): ReactNode {
            const classes = withStyles.getClasses(this.props);

            if (control.isOnOffSwitch()) {
                // normal gray unchecked state.
                return (
                    <Switch checked={value !== 0} color="primary"
                        onChange={(event) => {
                            this.onCheckChanged(event.target.checked);
                        }}
                    />
                );
            }
            if (control.isAbToggle()) {
                // unchecked color is not gray.
                return (
                    <Switch checked={value !== 0} color="primary"
                        onChange={(event) => {
                            this.onCheckChanged(event.target.checked);
                        }}
                        classes={{
                            track: classes.switchTrack
                        }}
                        style={{ color: this.props.theme.palette.primary.main }}
                    />
                );
            } else {
                return (
                    <Select variant="standard"
                        value={control.clampSelectValue(value)}
                        onChange={(e)=> this.onSelectChanged(e.target.value)}
                        inputProps={{
                            name: control.name,
                            id: 'id' + control.symbol,
                            style: { fontSize: "1.0em" }
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
        onDoubleTap() {
            let controlInfo = this.props.controlInfo;
            if (!controlInfo) return;
            let instanceId = controlInfo?.instanceId;
            let uiControl = controlInfo?.uiControl
            
            this.model.setPedalboardControl(instanceId,uiControl.symbol,uiControl.default_value);
        }

        render() {
            if (!this.props.controlInfo) {
                return false;
            }
            let uiControl = this.props.controlInfo.uiControl;

            let displayValue = uiControl.formatDisplayValue(this.state.value) ?? "";
            if (uiControl.isOnOffSwitch())
            {
                displayValue = this.state.value !== 0 ? "On": "Off";
            } else if (uiControl.isSelect())
            {
                displayValue = "\u00A0";
            }
            return (
                <DialogEx tag="zoomedControl" open={this.props.dialogOpen}
                    onClose={() => { this.props.onDialogClose() }}
                    onAbort={() => { this.props.onDialogClose() }}
                    onEnterKey={()=>{/*nothing*/}}
                >
                    <div  style={{
                        width: 380, height: 300,
                        display: "flex", flexFlow: "row", alignItems: "center", alignContent: " center", justifyContent: "center"
                    }}>
                        <IconButton sx={{ height: "100%", width: 48, borderRadius: "0% 50% 50% 0%" }} onClick={
                            () => {
                                this.model.onNextZoomedControl();
                            }
                        } >
                            <NavigateBeforeIcon />
                        </IconButton>
                        <div style={{
                            width: 200, flexGrow: 1, height: 300, 
                            display: "flex", flexFlow: "column", alignItems: "center", alignContent: "center", justifyContent: "center"

                        }}
                            onDrag={this.onDrag}
                        >
                            <Typography
                                display="block" variant="h6" color="textPrimary" style={{ textAlign: "center" }}
                            >
                                {this.props.controlInfo.name}
                            </Typography>
                            {uiControl.isDial() ? (

                                <ZoomedDial size={200} controlInfo={this.props.controlInfo}
                                    onDoubleTap={()=>{this.onDoubleTap();}}
                                    onPreviewValue={(v) => {
                                        this.setState({ value: v });
                                    }}
                                    onSetValue={(v) => {
                                        this.setState({ value: v });
                                        if (this.props.controlInfo) {
                                            this.model.setPedalboardControl(
                                                this.props.controlInfo.instanceId,
                                                this.props.controlInfo.uiControl.symbol,
                                                v);
                                        }
                                    }}
                                />
                            ) : 
                                    <div style={{height: 200, paddingTop: 80}}>
                                        {this.makeSelect(uiControl,this.state.value)}
                                    </div>
                            }

                            <Typography
                                display="block" variant="h6" color="textPrimary" style={{ textAlign: "center" }}
                            >
                                {displayValue}
                            </Typography>
                        </div>
                        <IconButton sx={{ height: "100%", width: 48, borderRadius: "50% 0% 0% 50%" }}
                            onClick={() => {
                                this.model.onPreviousZoomedControl();
                            }
                            }
                        >
                            <NavigateNextIcon />
                        </IconButton>
                    </div>

                </DialogEx>
            );
        }
    },
    styles));

export default ZoomedUiControl;
