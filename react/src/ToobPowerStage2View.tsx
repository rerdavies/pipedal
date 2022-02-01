// Copyright (c) 2021 Robin Davies
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
import { createStyles, withStyles, WithStyles, Theme } from '@material-ui/core/styles';

import IControlViewFactory from './IControlViewFactory';
import { PiPedalModelFactory, PiPedalModel, State,ListenHandle } from "./PiPedalModel";
import { PedalBoardItem } from './PedalBoard';
import PluginControlView, { ControlGroup,ControlViewCustomization } from './PluginControlView';
import ToobWaveShapeView, {BidirectionalVuPeak} from './ToobWaveShapeView';



const styles = (theme: Theme) => createStyles({
});

interface ToobPowerstage2Props extends WithStyles<typeof styles> {
    instanceId: number;
    item: PedalBoardItem;

};
interface ToobPowerstage2State {
    uiState: number[];
};

const ToobPowerstage2View =
    withStyles(styles, { withTheme: true })(
        class extends React.Component<ToobPowerstage2Props, ToobPowerstage2State> 
        implements ControlViewCustomization
        {
            model: PiPedalModel;

            customizationId: number = 5; 

            constructor(props: ToobPowerstage2Props) {
                super(props);
                this.model = PiPedalModelFactory.getInstance();
                this.state = {
                    uiState: [0,0,0,0,0,0,0,0]
                }
                this.onStateChanged = this.onStateChanged.bind(this);
                this.onAtomOutput = this.onAtomOutput.bind(this);
            }

            listeningForOutput: boolean = false;
            atomOutputHandle?: ListenHandle;

            onAtomOutput(instanceId: number, atomOutput: any): void {
                if (atomOutput.lv2Type  === "http://two-play.com/plugins/toob-power-stage-2#uiState" )
                {
                    this.setState({uiState: atomOutput.data as number[]});
                }
            }
            maybeListenForAtomOutput(): void {
                let listenForOutput = this.isReady && this.isControlMounted;

                if (listenForOutput !== this.listeningForOutput)
                {
                    this.listeningForOutput = listenForOutput;
                    if (listenForOutput)
                    {
                        this.atomOutputHandle = this.model.listenForAtomOutput(this.props.instanceId,this.onAtomOutput);
                    } else {
                        if (this.atomOutputHandle)
                        {
                            this.model.cancelListenForAtomOutput(this.atomOutputHandle);
                            this.atomOutputHandle = undefined;
                        }

                    }
                }

            }
            isReady: boolean = false;

            gain1Peak: BidirectionalVuPeak = new BidirectionalVuPeak();
            gain2Peak: BidirectionalVuPeak = new BidirectionalVuPeak();
            gain3Peak: BidirectionalVuPeak = new BidirectionalVuPeak();
            gain1OutPeak: BidirectionalVuPeak = new BidirectionalVuPeak();
            gain2OutPeak: BidirectionalVuPeak = new BidirectionalVuPeak();
            gain3OutPeak: BidirectionalVuPeak = new BidirectionalVuPeak();
 
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

            ModifyControls(controls: (React.ReactNode| ControlGroup)[]): (React.ReactNode| ControlGroup)[]
            {
                let time = Date.now()*0.001;

                this.gain1Peak.update(time,this.state.uiState[0],this.state.uiState[1]);
                this.gain1OutPeak.update(time,this.state.uiState[2],this.state.uiState[3]);

                this.gain2Peak.update(time,this.state.uiState[4],this.state.uiState[5]);
                this.gain2OutPeak.update(time,this.state.uiState[6],this.state.uiState[7]);

                this.gain3Peak.update(time,this.state.uiState[8],this.state.uiState[9]);
                this.gain3OutPeak.update(time,this.state.uiState[10],this.state.uiState[11]);

                var gain1 = (
                    <ToobWaveShapeView instanceId={this.props.instanceId} controlNumber={1} controlKeys={["gain1","shape1","bias1"]} 
                        vuMin={this.gain1Peak.minValue} vuMax={this.gain1Peak.maxValue}
                        vuMinPeak={this.gain1Peak.minPeak} vuMaxPeak={this.gain1Peak.maxPeak}
                        vuOutMin={this.gain1OutPeak.minValue} vuOutMax={this.gain1OutPeak.maxValue}
                        vuOutMinPeak={this.gain1OutPeak.minPeak} vuOutMaxPeak={this.gain1OutPeak.maxPeak}
                    />
                );
                var gain2 = (
                    <ToobWaveShapeView instanceId={this.props.instanceId} controlNumber={2} controlKeys={["gain2","shape2","bias2"]} 
                        vuMin={this.gain2Peak.minValue} vuMax={this.gain2Peak.maxValue}
                        vuMinPeak={this.gain2Peak.minPeak} vuMaxPeak={this.gain2Peak.maxPeak}
                        vuOutMin={this.gain2OutPeak.minValue} vuOutMax={this.gain2OutPeak.maxValue}
                        vuOutMinPeak={this.gain2OutPeak.minPeak} vuOutMaxPeak={this.gain2OutPeak.maxPeak}
                    />
                );
                var gain3 = (
                    <ToobWaveShapeView instanceId={this.props.instanceId} controlNumber={3} controlKeys={["gain3","shape3","bias3"]} 
                        vuMin={this.gain3Peak.minValue} vuMax={this.gain3Peak.maxValue}
                        vuMinPeak={this.gain3Peak.minPeak} vuMaxPeak={this.gain3Peak.maxPeak}
                        vuOutMin={this.gain3OutPeak.minValue} vuOutMax={this.gain3OutPeak.maxValue}
                        vuOutMinPeak={this.gain3OutPeak.minPeak} vuOutMaxPeak={this.gain3OutPeak.maxPeak}
                    />
                );
                ((controls[0]) as ControlGroup).controls.splice(3,0,gain1);
                ((controls[1]) as ControlGroup).controls.splice(3,0,gain2);
                ((controls[2]) as ControlGroup).controls.splice(3,0,gain3);

                return controls;
            }
            render() {
                return (<PluginControlView
                    instanceId={this.props.instanceId}
                    item={this.props.item}
                    customization={this}
                    customizationId={this.customizationId}
                />);
            }
        }
    );



class ToobPowerstage2ViewFactory implements IControlViewFactory {
    uri: string = "http://two-play.com/plugins/toob-power-stage-2";

    Create(model: PiPedalModel, pedalBoardItem: PedalBoardItem): React.ReactNode {
        return (<ToobPowerstage2View instanceId={pedalBoardItem.instanceId} item={pedalBoardItem} />);
    }


};
export default ToobPowerstage2ViewFactory;