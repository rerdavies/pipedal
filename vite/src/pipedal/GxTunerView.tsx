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
import {createStyles} from './WithStyles';

import { withStyles } from "tss-react/mui";

import IControlViewFactory from './IControlViewFactory';
import { PiPedalModelFactory, PiPedalModel } from "./PiPedalModel";
import { PedalboardItem } from './Pedalboard';
import PluginControlView, { ControlGroup,ControlViewCustomization } from './PluginControlView';
import GxTunerControl from './GxTunerControl';

const GXTUNER_URI = "http://guitarix.sourceforge.net/plugins/gxtuner#tuner";
const TOOBTUNER_URI = "http://two-play.com/plugins/toob-tuner";



const styles = (theme: Theme) => createStyles({
});

interface GxTunerProps extends WithStyles<typeof styles> {
    instanceId: number;
    item: PedalboardItem;
    isToobTuner: boolean;

}
interface GxTunerState {

}

const GxTunerView =
    withStyles(
        class extends React.Component<GxTunerProps, GxTunerState> 
        implements ControlViewCustomization
        {
            model: PiPedalModel;

            customizationId: number = 1; 

            constructor(props: GxTunerProps) {
                super(props);
                this.model = PiPedalModelFactory.getInstance();
                this.state = {
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
            ModifyControls(controls: (React.ReactNode| ControlGroup)[]): (React.ReactNode| ControlGroup)[]
            {
                let refFreqIndex = this.getControlIndex("REFFREQ");
                let thresholdIndex = this.getControlIndex("THRESHOLD");
                let muteIndex = -1;
                if (this.props.isToobTuner)
                {
                    muteIndex = this.getControlIndex("MUTE")
                }

                let result: (React.ReactNode | ControlGroup)[] = [];

                let tunerControl = (<GxTunerControl instanceId={this.props.instanceId} valueIsMidi={false} />);
                result.push(tunerControl);

                result.push(controls[refFreqIndex]);
                result.push(controls[thresholdIndex]);
                if (muteIndex !== -1)
                {
                    result.push(controls[muteIndex]);
                }
                return result;
            }
            render() {
                return (<PluginControlView
                    instanceId={this.props.instanceId}
                    item={this.props.item}
                    customization={this}
                    customizationId={this.customizationId}
                />);
            }
        },
        styles
    );



export class GxTunerViewFactory implements IControlViewFactory {
    uri: string = GXTUNER_URI;

    Create(model: PiPedalModel, pedalboardItem: PedalboardItem): React.ReactNode {
        return (<GxTunerView  isToobTuner={false} instanceId={pedalboardItem.instanceId} item={pedalboardItem} />);
    }


}

// ToobTuner uses the same controls and control semantics.
export class ToobTunerViewFactory implements IControlViewFactory {
    uri: string = TOOBTUNER_URI;

    Create(model: PiPedalModel, pedalboardItem: PedalboardItem): React.ReactNode {
        return (<GxTunerView isToobTuner={true} instanceId={pedalboardItem.instanceId} item={pedalboardItem} />);
    }


}

