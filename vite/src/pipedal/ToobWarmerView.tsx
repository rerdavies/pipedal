// Copyright (c) Robin E.R. Davies
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

import React from 'react';
import { Theme } from '@mui/material/styles';

import WithStyles from './WithStyles';
import {createStyles} from './WithStyles';

import { withStyles } from "tss-react/mui";

import IControlViewFactory from './IControlViewFactory';
import { PiPedalModelFactory, PiPedalModel, State,ListenHandle } from "./PiPedalModel";
import { PedalboardItem } from './Pedalboard';
import PluginControlView, { ICustomizationHost,  ControlGroup,ControlViewCustomization } from './PluginControlView';
import ToobWaveShapeView, {BidirectionalVuPeak} from './ToobWaveShapeView';


let WARMER_UI_URI = "http://two-play.com/plugins/toob-warmer#uiState";

const styles = (theme: Theme) => createStyles({
});

interface ToobWarmerProps extends WithStyles<typeof styles> {
    instanceId: number;
    item: PedalboardItem;

}
interface ToobWarmerState {
    uiState: number[];
}

const ToobWarmerView =
    withStyles(
        class extends React.Component<ToobWarmerProps, ToobWarmerState> 
        implements ControlViewCustomization
        {
            model: PiPedalModel;

            customizationId: number = 5; 

            constructor(props: ToobWarmerProps) {
                super(props);
                this.model = PiPedalModelFactory.getInstance();
                this.state = {
                    uiState: [0,0,0,0,0,0,0,0]
                }
                this.onStateChanged = this.onStateChanged.bind(this);
            }

            listeningForOutput: boolean = false;
            atomOutputHandle?: ListenHandle;

            maybeListenForAtomOutput(): void {
                let listenForOutput = this.isReady && this.isControlMounted;

                if (listenForOutput !== this.listeningForOutput)
                {
                    this.listeningForOutput = listenForOutput;
                    if (listenForOutput)
                    {
                        this.atomOutputHandle = this.model.monitorPatchProperty(
                            this.props.instanceId,
                            WARMER_UI_URI,
                            (instanceId: number, propertyUri: string, json: any) => {
                                if (json.otype_ === "Vector")
                                {
                                    this.setState({uiState: json.value as number[]});
                                }
                            });

                        // discard the output as we will be getting it through monitorPatchProperty as well.
                         this.model.getPatchProperty(this.props.instanceId,WARMER_UI_URI);
                    } else {
                        if (this.atomOutputHandle)
                        {
                            this.model.cancelMonitorPatchProperty(this.atomOutputHandle);
                            this.atomOutputHandle = undefined;
                        }

                    }
                }

            }
            isReady: boolean = false;

            gain1Peak: BidirectionalVuPeak = new BidirectionalVuPeak();
            gain1OutPeak: BidirectionalVuPeak = new BidirectionalVuPeak();
 
            onStateChanged() {
                this.isReady = this.model.state.get() === State.Ready;
                this.maybeListenForAtomOutput();
            }

            isControlMounted: boolean = false;

            componentDidMount() {
                this.model.state.addOnChangedHandler(
                    this.onStateChanged);
                this.isControlMounted = true;
                this.isReady = this.model.state.get() === State.Ready;
                this.maybeListenForAtomOutput();
            }
            componentWillUnmount()
            {
                this.isControlMounted = false;
                this.maybeListenForAtomOutput();
            }
            fullScreen() {
                return false;
            }

            modifyControls(host: ICustomizationHost, controls: (React.ReactNode| ControlGroup)[]): (React.ReactNode| ControlGroup)[]
            {
                let time = Date.now()*0.001;

                this.gain1Peak.update(time,this.state.uiState[0],this.state.uiState[1]);
                this.gain1OutPeak.update(time,this.state.uiState[2],this.state.uiState[3]);

                var gain1 = (
                    <ToobWaveShapeView instanceId={this.props.instanceId} controlNumber={1} controlKeys={["gain1","shape1","bias1"]} 
                        vuMin={this.gain1Peak.minValue} vuMax={this.gain1Peak.maxValue}
                        vuMinPeak={this.gain1Peak.minPeak} vuMaxPeak={this.gain1Peak.maxPeak}
                        vuOutMin={this.gain1OutPeak.minValue} vuOutMax={this.gain1OutPeak.maxValue}
                        vuOutMinPeak={this.gain1OutPeak.minPeak} vuOutMaxPeak={this.gain1OutPeak.maxPeak}
                    />
                );
                ((controls[0]) as ControlGroup).controls.splice(3,0,gain1);

                return controls;
            }
            render() {
                return (<PluginControlView
                    instanceId={this.props.instanceId}
                    item={this.props.item}
                    customization={this}
                    customizationId={this.customizationId}
                    showModGui={false} 
                    onSetShowModGui= {(instanceId: number, showModGui: boolean) => {}}
                />);
            }
        },
        styles
    );



class ToobWarmerViewFactory implements IControlViewFactory {
    uri: string = "http://two-play.com/plugins/toob-warmer";

    Create(model: PiPedalModel, pedalboardItem: PedalboardItem): React.ReactNode {
        return (<ToobWarmerView instanceId={pedalboardItem.instanceId} item={pedalboardItem} />);
    }


}
export default ToobWarmerViewFactory;