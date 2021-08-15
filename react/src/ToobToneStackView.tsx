import React from 'react';
import { createStyles, withStyles, WithStyles, Theme } from '@material-ui/core/styles';

import IControlViewFactory from './IControlViewFactory';
import { PiPedalModelFactory, PiPedalModel } from "./PiPedalModel";
import { PedalBoardItem } from './PedalBoard';
import PluginControlView, { ControlGroup,ControlViewCustomization } from './PluginControlView';
import ToobFrequencyResponseView from './ToobFrequencyResponseView';



const styles = (theme: Theme) => createStyles({
});

interface ToobToneStackProps extends WithStyles<typeof styles> {
    instanceId: number;
    item: PedalBoardItem;

};
interface ToobToneStackState {

};

const ToobToneStackView =
    withStyles(styles, { withTheme: true })(
        class extends React.Component<ToobToneStackProps, ToobToneStackState> 
        implements ControlViewCustomization
        {
            model: PiPedalModel;

            customizationId: number = 1; 

            constructor(props: ToobToneStackProps) {
                super(props);
                this.model = PiPedalModelFactory.getInstance();
                this.state = {
                }
            }

            ModifyControls(controls: (React.ReactNode| ControlGroup)[]): (React.ReactNode| ControlGroup)[]
            {
                controls.splice(0,0,
                    ( <ToobFrequencyResponseView instanceId={this.props.instanceId}  />)
                    );
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



class ToobToneStackViewFactory implements IControlViewFactory {
    uri: string = "http://two-play.com/plugins/toob-tone-stack";

    Create(model: PiPedalModel, pedalBoardItem: PedalBoardItem): React.ReactNode {
        return (<ToobToneStackView instanceId={pedalBoardItem.instanceId} item={pedalBoardItem} />);
    }


};
export default ToobToneStackViewFactory;