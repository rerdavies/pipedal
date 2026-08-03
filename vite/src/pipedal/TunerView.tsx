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

const styles = (theme: Theme) => createStyles({
});

interface TunerProps extends WithStyles<typeof styles> {
    instanceId: number;
    item: PedalboardItem;
    isToobTuner: boolean;

}
interface TunerState {
    containerWidth: number;
    containerHeight: number;
}

const TunerView =
    withStyles(
        class extends React.Component<TunerProps, TunerState> 
        implements ControlViewCustomization
        {
            model: PiPedalModel;

            customizationId: number = 1; 

            containerRef: HTMLDivElement | null = null;
            resizeObserver: ResizeObserver | null = null;

            constructor(props: TunerProps) {
                super(props);
                this.model = PiPedalModelFactory.getInstance();
                this.state = {
                    containerWidth: 0,
                    containerHeight: 0
                }
                this.setContainerRef = this.setContainerRef.bind(this);
                this.handleConnectionStateChanged = this.handleConnectionStateChanged.bind(this);
            }

            private setContainerRef(el: HTMLDivElement | null) {
                if (this.resizeObserver) {
                    this.resizeObserver.disconnect();
                    this.resizeObserver = null;
                }
                this.containerRef = el;
                if (el) {
                    this.resizeObserver = new ResizeObserver((entries) => {
                        for (const entry of entries) {
                            const { width, height } = entry.contentRect;
                            this.setState({
                                containerWidth: width,
                                containerHeight: height
                            });
                        }
                    });
                    this.resizeObserver.observe(el);
                    // set initial size
                    const rect = el.getBoundingClientRect();
                    if (rect.width !== 0 || rect.height !== 0) {
                        this.setState({
                            containerWidth: rect.width,
                            containerHeight: rect.height
                        });
                    }
                }
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
                if (this.resizeObserver) {
                    this.resizeObserver.disconnect();
                    this.resizeObserver = null;
                }
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
                const { containerWidth, containerHeight } = this.state;
                if (containerWidth === 0 || containerHeight === 0) {
                    // fallback before first measurement
                    return false;
                }
                return containerWidth > containerHeight;
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

                let auxControls: React.ReactNode[] = [];
                auxControls.push(controls[refFreqIndex] as React.ReactNode);
                auxControls.push(controls[thresholdIndex] as React.ReactNode);
                if (muteIndex !== -1)
                {
                    auxControls.push(controls[muteIndex] as React.ReactNode);
                }

                let isLandscape = this.isLandscape();

                return [this.buildLayout(auxControls, isLandscape)];
            }

            private buildLayout(auxControls: React.ReactNode[], isLandscape: boolean): React.ReactNode {
                const isPortrait = !isLandscape;

                // Direction-specific styles
                const flexDirection = isLandscape ? "row" : "column";

                // Tuner: constrain width in portrait, height in landscape
                const tunerConstraint = isPortrait ? "width" : "height";
                const tunerPadding = isPortrait ? "16px 24px 0 24px" : "8px 0 8px 16px";

                // Controls: min opposite dimension
                const controlsPadding = isPortrait ? "8px 16px 44px 16px" : "12px 16px 36px 12px";
                const controlsMin = isPortrait ? { minHeight: 120 } : { minWidth: 100 };

                return (
                    <div key="tuner-root" ref={this.setContainerRef} style={{
                        width: "100%",
                        height: "100%",
                        display: "flex",
                        flexDirection: flexDirection,
                        overflow: "hidden"
                    }}>
                        {/* Tuner. sized by {tunerConstraint}, other dim from aspect ratio */}
                        <div style={{
                            flex: "0 1 auto",
                            [tunerConstraint]: "100%",
                            aspectRatio: "220 / 100",
                            padding: tunerPadding
                        }}>
                            <div style={{ width: "100%", height: "100%" }}>
                                <TunerControl instanceId={this.props.instanceId} valueIsMidi={this.props.isToobTuner} />
                            </div>
                        </div>

                        {/* Auxiliary controls. remaining space, wraps */}
                        <div style={{
                            flex: "1 1 auto",
                            display: "flex",
                            alignContent: "start",
                            flexWrap: "wrap",
                            gap: 12,
                            padding: controlsPadding,
                            overflow: "auto",
                            ...controlsMin
                        }}>
                            {auxControls}
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
