import React from 'react';
import { PiPedalModel, PiPedalModelFactory } from './PiPedalModel';


import {PedalBoardItem, PedalBoardSplitItem} from './PedalBoard';
import PluginControlView from './PluginControlView';
import SplitControlView from './SplitControlView';
import Typography from '@material-ui/core/Typography';
import IControlViewFactory from './IControlViewFactory';
import ToobInputStageViewFactory from './ToobInputStageView';
import ToobToneStackViewFactory from './ToobToneStackView';
import ToobCabSimViewFactory from './ToobCabSimView';


let pluginFactories: IControlViewFactory[] = [
    new ToobInputStageViewFactory(),
    new ToobToneStackViewFactory(),
    new ToobCabSimViewFactory()
];


export function GetControlView(pedalBoardItem?: PedalBoardItem| null): React.ReactNode
{
    let model: PiPedalModel = PiPedalModelFactory.getInstance();

    if (!pedalBoardItem) {
        return (<div/>);
    }
    if (pedalBoardItem.isSplit())
    {
        return (
            <SplitControlView item={pedalBoardItem as PedalBoardSplitItem} instanceId={pedalBoardItem!.instanceId}
            />
        );
    } else {
        for (let i = 0; i < pluginFactories.length; ++i)
        {
            let factory = pluginFactories[i];
            if (factory.uri === pedalBoardItem.uri)
            {
                return factory.Create(model,pedalBoardItem);
            }
        }
        let uiPlugin = model.getUiPlugin(pedalBoardItem.uri);
        if (!uiPlugin)
        {
            <div style={{paddingLeft: 40, paddingRight: 40}}>
                <Typography color="error" variant="h6" >Missing plugin.</Typography>
                <Typography>The plugin '{pedalBoardItem.pluginName}' ({pedalBoardItem.uri}) is not currently installed.</Typography>
            </div>
        } else {
            return (
                <PluginControlView instanceId={pedalBoardItem.instanceId} item={pedalBoardItem} />
            )
        }
    }
}

