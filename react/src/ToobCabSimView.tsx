import React from 'react';
import { createStyles, withStyles, WithStyles, Theme } from '@material-ui/core/styles';

import IControlViewFactory from './IControlViewFactory';
import { PiPedalModelFactory, PiPedalModel } from "./PiPedalModel";
import { PedalBoardItem } from './PedalBoard';
import PluginControlView, { ControlGroup,ControlViewCustomization } from './PluginControlView';
import ToobFrequencyResponseView from './ToobFrequencyResponseView';



const styles = (theme: Theme) => createStyles({
});

interface ToobCabSimProps extends WithStyles<typeof styles> {
    instanceId: number;
    item: PedalBoardItem;

};
interface ToobCabSimState {

};

const ToobCabSimView =
    withStyles(styles, { withTheme: true })(
        class extends React.Component<ToobCabSimProps, ToobCabSimState> 
        implements ControlViewCustomization
        {
            model: PiPedalModel;

            customizationId: number = 1; 

            constructor(props: ToobCabSimProps) {
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



class ToobCabSimViewFactory implements IControlViewFactory {
    uri: string = "http://two-play.com/plugins/toob-cab-sim";

    Create(model: PiPedalModel, pedalBoardItem: PedalBoardItem): React.ReactNode {
        return (<ToobCabSimView instanceId={pedalBoardItem.instanceId} item={pedalBoardItem} />);
    }


};
export default ToobCabSimViewFactory;