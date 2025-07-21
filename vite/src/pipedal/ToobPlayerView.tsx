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

import React from 'react';
import { Theme } from '@mui/material/styles';

import WithStyles from './WithStyles';
import { createStyles } from './WithStyles';

import { withStyles } from "tss-react/mui";

import IControlViewFactory from './IControlViewFactory';
import { PiPedalModelFactory, PiPedalModel, MonitorPortHandle } from "./PiPedalModel";
import { PedalboardItem } from './Pedalboard';
import PluginControlView, { ControlGroup, ControlViewCustomization } from './PluginControlView';
// import ToobFrequencyResponseView from './ToobFrequencyResponseView';
import ToobPlayerControl from './ToobPlayerControl';

const styles = (theme: Theme) => createStyles({
});

interface ToobPlayerProps extends WithStyles<typeof styles> {
    instanceId: number;
    item: PedalboardItem;

}
interface ToobPlayerState {
    playPosition: number;
    duration: number;
}

const ToobPlayerView =
    withStyles(
        class extends React.Component<ToobPlayerProps, ToobPlayerState>
            implements ControlViewCustomization {
            model: PiPedalModel;
            gainRef: React.RefObject<HTMLDivElement | null>;

            customizationId: number = 1;

            constructor(props: ToobPlayerProps) {
                super(props);
                this.model = PiPedalModelFactory.getInstance();
                this.gainRef = React.createRef();
                this.state = {
                    playPosition: 0.0,
                    duration: 0.0
                }
            }

            subscribedId?: number = undefined;
            monitorPlayPositionHandle?: MonitorPortHandle = undefined;
            monitorDurationHandle?: MonitorPortHandle = undefined;
            removePortSubscriptions() {

                if (this.monitorPlayPositionHandle) {
                    this.model.unmonitorPort(this.monitorPlayPositionHandle);
                    this.monitorPlayPositionHandle = undefined;
                }
            }
            addPortSubscriptions(instanceId: number) {
                this.removePortSubscriptions();
                this.subscribedId = instanceId;
                this.monitorPlayPositionHandle = this.model.monitorPort(instanceId, "position", 1 / 15.0,
                    (value: number) => {
                        this.setState({ playPosition: value });
                    });
                this.monitorDurationHandle = this.model.monitorPort(instanceId, "duration", 1 / 15.0,
                    (value: number) => {
                        this.setState({ duration: value });
                    });
            }

            // componentDidUpdate() {
            //     if (this.props.instanceId !== this.subscribedId) {
            //         this.removeGainEnabledSubscription();
            //         this.addGainEnabledSubscription(this.props.instanceId);

            //     }
            // }
            componentDidMount() {
                this.addPortSubscriptions(this.props.instanceId);
            }
            componentWillUnmount() {
                this.removePortSubscriptions();
            }
            fullScreen() {
                return true;
            }
            modifyControls(controls: (React.ReactNode | ControlGroup)[]): (React.ReactNode | ControlGroup)[] {
                let extraControls: React.ReactElement[] = [];
                let mixPanel = controls[3] as ControlGroup;
                let iKey = 0;
                for (let mixControl of mixPanel.controls) {
                    extraControls.push(
                        (
                            <div key={"kExtra" + iKey++} style={{ flex: "0 0 auto", position: "relative", height: "100%" }}>
                                {mixControl}
                            </div>
                        ));
                }

                let panel = (
                    <ToobPlayerControl key={"MusicPlayer" + this.props.instanceId} instanceId={this.props.instanceId} extraControls={extraControls} />
                );

                let result: (React.ReactNode | ControlGroup)[] = [];
                result.push(panel);
                return result;
            }

            render() {
                return (
                    <PluginControlView
                        key={"ToobPlayerView" + this.props.instanceId}
                        instanceId={this.props.instanceId}
                        item={this.props.item}
                        customization={this}
                        customizationId={this.customizationId}
                        showModGui={false}
                        onSetShowModGui={(instanceId: number, showModGui: boolean) => { }}

                    />
                );
            }
        },
        styles
    );



class ToobPlayerViewFactory implements IControlViewFactory {
    uri: string = "http://two-play.com/plugins/toob-player";

    Create(model: PiPedalModel, pedalboardItem: PedalboardItem): React.ReactNode {
        return (<ToobPlayerView key={"ppmv" + pedalboardItem.instanceId} instanceId={pedalboardItem.instanceId} item={pedalboardItem} />);
    }


}
export default ToobPlayerViewFactory;