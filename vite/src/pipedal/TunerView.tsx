// Copyright (c) Robin E.R. Davies
// Copyright (c) Fulgencio Ruiz Rubio.
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
import { PiPedalModelFactory, PiPedalModel, State } from "./PiPedalModel";
import { PedalboardItem } from './Pedalboard';
import PluginControlView, {   ICustomizationHost,ControlGroup,ControlViewCustomization } from './PluginControlView';
import TunerControl from './TunerControl';

const GXTUNER_URI = "http://guitarix.sourceforge.net/plugins/gxtuner#tuner";
const TOOBTUNER_URI = "http://two-play.com/plugins/toob-tuner";

const getWindowSize = () => {
    return {
        width: document.documentElement.clientWidth,
        height: document.documentElement.clientHeight,
    };
};

const styles = (theme: Theme) => createStyles({
});

interface TunerProps extends WithStyles<typeof styles> {
    instanceId: number;
    item: PedalboardItem;
    isToobTuner: boolean;

}
interface TunerState {
    windowSize: { width: number, height: number };
}

const TunerView =
    withStyles(
        class extends React.Component<TunerProps, TunerState> 
        implements ControlViewCustomization
        {
            model: PiPedalModel;

            customizationId: number = 1; 

            constructor(props: TunerProps) {
                super(props);
                this.model = PiPedalModelFactory.getInstance();
                this.state = {
                    windowSize: getWindowSize()
                }
                this.handleResize = this.handleResize.bind(this);
                this.handleConnectionStateChanged = this.handleConnectionStateChanged.bind(this);
            }

            handleResize() {
                this.setState({ windowSize: getWindowSize() });
            }

            private subscribedId: number | null = null;
            
            private handleConnectionStateChanged(state: State) {
                if (state === State.Ready) {
                    this.unsubscribe();
                    this.subscribe();
                }
            }
            
            subscribe() {
                this.subscribedId = this.props.instanceId;
            }
            unsubscribe() {
            }

            componentDidMount() {
                this.subscribe();
                this.model.state.addOnChangedHandler(this.handleConnectionStateChanged);
                window.addEventListener("resize", this.handleResize);
            }
            componentDidUpdate(oldProps: TunerProps) {
                if (this.props.instanceId !== this.subscribedId) {
                    this.unsubscribe();
                    this.subscribe();
                }
            }
            componentWillUnmount() {
                this.unsubscribe();
                this.model.state.removeOnChangedHandler(this.handleConnectionStateChanged);
                window.removeEventListener("resize", this.handleResize);
            }

            getControlIndex(key: string): number {
                let item = this.props.item;
                for (let i = 0; i < item.controlValues.length; ++i)
                {
                    if (item.controlValues[i].key === key)
                    {
                        return i;
                    }
                }
                throw new Error("GxTuner: Control '" + key + "' not found.");
            }

            fullScreen() {
                return true;
            }

            isLandscape(): boolean {
                return this.state.windowSize.width > this.state.windowSize.height;
            }

            modifyControls(host: ICustomizationHost, controls: (React.ReactNode | ControlGroup)[]): (React.ReactNode | ControlGroup)[]
            {
                let refFreqIndex = this.getControlIndex("REFFREQ");
                let thresholdIndex = this.getControlIndex("THRESHOLD");
                let muteIndex = -1;
                if (this.props.isToobTuner)
                {
                    muteIndex = this.getControlIndex("MUTE")
                }

                let auxControls: (React.ReactNode)[] = [];
                auxControls.push(controls[refFreqIndex]);
                auxControls.push(controls[thresholdIndex]);
                if (muteIndex !== -1)
                {
                    auxControls.push(controls[muteIndex]);
                }

                let isLandscape = this.isLandscape();

                // Choose layout based on orientation
                if (isLandscape) {
                    return [this.buildLandscapeLayout(auxControls)];
                } else {
                    return [this.buildPortraitLayout(auxControls)];
                }
            }

            private buildPortraitLayout(auxControls: React.ReactNode[]): React.ReactNode {
                return (
                    <div style={{
                        width: "100%",
                        height: "100%",
                        display: "flex",
                        flexDirection: "column",
                        overflow: "hidden"
                    }}>
                        {/* Tuner dial area — takes all available space */}
                        <div style={{
                            flex: "1 1 auto",
                            minHeight: 0,
                            display: "flex",
                            alignItems: "center",
                            justifyContent: "center",
                            padding: "4% 6% 2% 6%"
                        }}>
                            <div style={{
                                width: "100%",
                                height: "100%",
                                maxWidth: "100%",
                                maxHeight: "100%",
                                aspectRatio: "220 / 100"
                            }}>
                                <TunerControl instanceId={this.props.instanceId} valueIsMidi={false} />
                            </div>
                        </div>

                        {/* Auxiliary controls strip (scrollable) */}
                        <div style={{
                            flex: "0 0 auto",
                            overflowX: "auto",
                            overflowY: "hidden",
                            width: "100%"
                        }}>
                            <div style={{
                                display: "flex",
                                flexDirection: "row",
                                justifyContent: "center",
                                alignItems: "center",
                                gap: 12,
                                padding: "8px 16px 12px 16px",
                                flexWrap: "nowrap",
                                width: "fit-content",
                                minWidth: "100%"
                            }}>
                                {auxControls.map((ctl, i) => (
                                    <div key={"aux" + i} style={{ flex: "0 0 auto" }}>
                                        {ctl}
                                    </div>
                                ))}
                            </div>
                        </div>
                    </div>
                );
            }

            private buildLandscapeLayout(auxControls: React.ReactNode[]): React.ReactNode {
                return (
                    <div style={{
                        width: "100%",
                        height: "100%",
                        display: "flex",
                        flexDirection: "row",
                        overflow: "hidden"
                    }}>
                        {/* Tuner dial area */}
                        <div style={{
                            flex: "1 1 auto",
                            minWidth: 0,
                            display: "flex",
                            alignItems: "center",
                            justifyContent: "center",
                            padding: "2% 2% 2% 4%"
                        }}>
                            <div style={{
                                width: "100%",
                                height: "100%",
                                maxWidth: "100%",
                                maxHeight: "100%",
                                aspectRatio: "220 / 100"
                            }}>
                                <TunerControl instanceId={this.props.instanceId} valueIsMidi={false} />
                            </div>
                        </div>

                        {/* Auxiliary controls column (scrollable) */}
                        <div style={{
                            flex: "0 0 auto",
                            overflowY: "auto",
                            overflowX: "hidden",
                            height: "100%"
                        }}>
                            <div style={{
                                display: "flex",
                                flexDirection: "column",
                                justifyContent: "center",
                                alignItems: "center",
                                gap: 8,
                                padding: "12px 16px 36px 8px",
                                minHeight: "100%"
                            }}>
                                {auxControls.map((ctl, i) => (
                                    <div key={"aux" + i} style={{ flex: "0 0 auto" }}>
                                        {ctl}
                                    </div>
                                ))}
                            </div>
                        </div>
                    </div>
                );
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

export class GxTunerViewFactory implements IControlViewFactory {
    uri: string = GXTUNER_URI;

    Create(model: PiPedalModel, pedalboardItem: PedalboardItem): React.ReactNode {
        return (<TunerView isToobTuner={false} instanceId={pedalboardItem.instanceId} item={pedalboardItem} />);
    }
}

// ToobTuner uses the same controls and control semantics.
export class ToobTunerViewFactory implements IControlViewFactory {
    uri: string = TOOBTUNER_URI;

    Create(model: PiPedalModel, pedalboardItem: PedalboardItem): React.ReactNode {
        return (<TunerView isToobTuner={true} instanceId={pedalboardItem.instanceId} item={pedalboardItem} />);
    }
}
