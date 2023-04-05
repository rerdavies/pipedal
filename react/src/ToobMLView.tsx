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

import { WithStyles } from '@mui/styles';
import createStyles from '@mui/styles/createStyles';
import withStyles from '@mui/styles/withStyles';

import IControlViewFactory from './IControlViewFactory';
import { PiPedalModelFactory, PiPedalModel,MonitorPortHandle } from "./PiPedalModel";
import { PedalboardItem } from './Pedalboard';
import PluginControlView, { ControlGroup,ControlViewCustomization } from './PluginControlView';
import ToobFrequencyResponseView from './ToobFrequencyResponseView';



const styles = (theme: Theme) => createStyles({
});

interface ToobMLProps extends WithStyles<typeof styles> {
    instanceId: number;
    item: PedalboardItem;

}
interface ToobMLState {
    gainEnabled: boolean;
}

const ToobMLView =
    withStyles(styles, { withTheme: true })(
        class extends React.Component<ToobMLProps, ToobMLState> 
        implements ControlViewCustomization
        {
            model: PiPedalModel;
            gainRef: React.RefObject<HTMLDivElement>;

            customizationId: number = 1; 

            constructor(props: ToobMLProps) {
                super(props);
                this.model = PiPedalModelFactory.getInstance();
                this.gainRef = React.createRef();
                this.state = {
                    gainEnabled: true
                }
            }

            subscribedId?: number = undefined;
            monitorPortHandle?: MonitorPortHandle = undefined;
            removeGainEnabledSubscription()
            {
                if (this.monitorPortHandle)
                {
                    this.model.unmonitorPort(this.monitorPortHandle);
                    this.monitorPortHandle = undefined;
                }
            }
            addGainEnabledSubscription(instanceId: number)
            {
                this.removeGainEnabledSubscription();
                this.subscribedId = instanceId;
                this.monitorPortHandle = this.model.monitorPort(instanceId,"gainEnable",0.1,
                    (value: number) => {
                        if (this.gainRef.current)
                        {
                            this.gainRef.current.style.opacity = value !== 0.0? "1.0": "0.4";
                        }
                        this.setState({gainEnabled: value !== 0.0 });
                    });
            }

            componentDidUpdate()
            {
                if (this.props.instanceId !== this.subscribedId)
                {
                    this.removeGainEnabledSubscription();
                    this.addGainEnabledSubscription(this.props.instanceId);

                }
            }
            componentDidMount()
            {
                this.addGainEnabledSubscription(this.props.instanceId);
            }
            componentWillUnmount()
            {
                this.removeGainEnabledSubscription();
            }

            ModifyControls(controls: (React.ReactNode| ControlGroup)[]): (React.ReactNode| ControlGroup)[]
            {
                let group = controls[4] as ControlGroup;
                group.controls.splice(0,0,
                    ( <ToobFrequencyResponseView instanceId={this.props.instanceId} minDb={-20} maxDb={20} />)
                    );

                let gainControl: React.ReactElement = controls[2] as React.ReactElement;
                if (gainControl)
                {
                    controls[2] = (<div ref={this.gainRef}> { gainControl} </div>);
                }
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



class ToobMLViewFactory implements IControlViewFactory {
    uri: string = "http://two-play.com/plugins/toob-ml";

    Create(model: PiPedalModel, pedalboardItem: PedalboardItem): React.ReactNode {
        return (<ToobMLView instanceId={pedalboardItem.instanceId} item={pedalboardItem} />);
    }


}
export default ToobMLViewFactory;