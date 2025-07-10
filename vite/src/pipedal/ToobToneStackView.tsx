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
import { PiPedalModelFactory, PiPedalModel, ControlValueChangedHandle } from "./PiPedalModel";
import { PedalboardItem } from './Pedalboard';
import PluginControlView, { ControlGroup, ControlViewCustomization } from './PluginControlView';
import ToobFrequencyResponseView from './ToobFrequencyResponseView';



const styles = (theme: Theme) => createStyles({
});

interface ToobToneStackProps extends WithStyles<typeof styles> {
    instanceId: number;
    item: PedalboardItem;

}
interface ToobToneStackState {
    isBaxandall: boolean;
}

const ToobToneStackView =
    withStyles(
        class extends React.Component<ToobToneStackProps, ToobToneStackState>
            implements ControlViewCustomization {
            model: PiPedalModel;

            customizationId: number = 1;

            constructor(props: ToobToneStackProps) {
                super(props);
                this.model = PiPedalModelFactory.getInstance();
                this.state = {
                    isBaxandall: this.IsBaxandall()
                }
            }
            IsBaxandall(): boolean {
                return this.props.item.getControl("ampmodel").value === 2.0;
            }


            controlValueChangedHandle?: ControlValueChangedHandle;
            componentDidMount() {
                this.controlValueChangedHandle = this.model.addControlValueChangeListener(
                    this.props.instanceId,
                    (key, value) => {
                        if (key === "ampmodel") {
                            this.setState({ isBaxandall: value === 2.0 });
                        }
                    }
                );
            }
            componentWillUnmount() {
                if (this.controlValueChangedHandle) {
                    this.model.removeControlValueChangeListener(this.controlValueChangedHandle);
                    this.controlValueChangedHandle = undefined;
                }
            }
            fullScreen() {
                return false;
            }

            modifyControls(controls: (React.ReactNode | ControlGroup)[]): (React.ReactNode | ControlGroup)[] {
                if (this.state.isBaxandall) {
                    controls.splice(0, 0,
                        (<ToobFrequencyResponseView instanceId={this.props.instanceId}
                        />)
                    );
                } else {
                    controls.splice(0, 0,
                        (<ToobFrequencyResponseView instanceId={this.props.instanceId} />)
                    );
                }
                return controls;
            }
            render() {
                return (<PluginControlView
                    instanceId={this.props.instanceId}
                    item={this.props.item}
                    customization={this}
                    customizationId={this.customizationId}
                    showModGui={false}
                    onSetShowModGui={(instanceId: number, showModGui: boolean) => { }}

                />);
            }
        },
        styles
    );



class ToobToneStackViewFactory implements IControlViewFactory {
    uri: string = "http://two-play.com/plugins/toob-tone-stack";

    Create(model: PiPedalModel, pedalboardItem: PedalboardItem): React.ReactNode {
        return (<ToobToneStackView instanceId={pedalboardItem.instanceId} item={pedalboardItem} />);
    }


}
export default ToobToneStackViewFactory;