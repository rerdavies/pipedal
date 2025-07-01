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

import React, {useEffect} from "react";
import Slider,{ SliderProps } from "@mui/material/Slider";
import Typography from "@mui/material/Typography";
import { PiPedalModelFactory,State } from "./PiPedalModel";

    function formatDuration(value_: number) {
        let value = Math.ceil(value_);
        const minute = Math.floor(value / 60);
        const secondLeft = value - minute * 60;
        return `${minute}:${secondLeft < 10 ? `0${secondLeft}` : secondLeft}`;
    }

export interface ControlSliderProps extends SliderProps {
    instanceId: number;
    controlKey: string;
    duration: number;
    onPreviewValue: (value: number) => void;
    onValueChanged: (value: number) => void; // Callback when the value changes
    style?: React.CSSProperties;
}


function ControlSlider(props: ControlSliderProps) {
    const { style,instanceId, controlKey, duration, 
        onPreviewValue, onValueChanged, ...extras} = props;

    const model = PiPedalModelFactory.getInstance();

    let [sliderValue, setSliderValue] = React.useState(0);
    let [effectiveValue, setEffectiveValue] = React.useState(0);
    let [dragging, setDragging] = React.useState(false);
    let [serverConnected,setServerConnected] = React.useState(model.state.get() === State.Ready);
    const handleStateChanged = (state: State) => {
        setServerConnected(state === State.Ready);
    };
    useEffect(() => {
        model.state.addOnChangedHandler(handleStateChanged);
        if (model.state.get() !== State.Ready) {
            return () => {
                model.state.removeOnChangedHandler(handleStateChanged);
            };
        }
        let handle = model.monitorPort(instanceId, controlKey, 1.0/15.0,(value: number) => {
            setSliderValue(value);
        });
        return () => {
            model.state.removeOnChangedHandler(handleStateChanged);
            model.unmonitorPort(handle);
        };
    }, [instanceId,controlKey,serverConnected]);

    return (
        <div style={{ display: "flex", flexFlow: "column nowrap" }}>

        <Slider
            {...extras}
            value={dragging ? effectiveValue : sliderValue}
            min={0}
            step={1}
            max={duration}
            onChange={(_, val: any) => {
                let v = parseFloat(val);
                setDragging(true);
                setEffectiveValue(v);
                onPreviewValue(v);
            }}
            onChangeCommitted={(_, val: any) => {
                let v = parseFloat(val);
                setDragging(false);
                setSliderValue(v);
                onValueChanged(v);
            }}
            onPointerDown={(e) => {
                setDragging(true);
            }}
            onPointerUp={(e) => {
                // setDragging(false);
            }}
            disabled={duration == 0}
            sx={(t) => ({
                width: "100%",
                color: 'rgba(0,0,0,0.87)',
                height: 4,
                '& .MuiSlider-thumb': {
                    width: 8,
                    height: 8,
                    transition: '0.0s',
                    transitionProperty: 'none',
                    '&::before': {
                        boxShadow: '0 2px 12px 0 rgba(0,0,0,0.4)',
                    },
                    '&:hover, &.Mui-focusVisible': {
                        boxShadow: `0px 0px 0px 8px ${'rgb(0 0 0 / 16%)'}`,
                        ...t.applyStyles('dark', {
                            boxShadow: `0px 0px 0px 8px ${'rgb(255 255 255 / 16%)'}`,
                        }),
                    },
                    '&.Mui-active': {
                        width: 20,
                        height: 20,
                    },
                },
                '& .MuiSlider-rail': {
                    opacity: 0.28,
                },
                '& .MuiSlider-track': {
                    transition: '0.0s',
                    transitionProperty: 'none',
                },
                ...t.applyStyles('dark', {
                    color: '#fff',
                }),
            })}
        />
                <div
                    style={{
                        flex: "0 0 auto",
                        width: "100%",
                        display: 'flex',
                        alignItems: 'top',
                        justifyContent: 'space-between',
                        marginTop: -4,
                    }}
                >
                    <Typography variant="caption">{formatDuration(dragging ? effectiveValue : sliderValue)}</Typography>
                    <Typography variant="caption">{formatDuration(duration)}</Typography>
                </div>
            </div>

    );
}

export default ControlSlider;