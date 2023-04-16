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
import { PiPedalModel, PiPedalModelFactory } from './PiPedalModel';


import { PedalboardItem, PedalboardSplitItem } from './Pedalboard';
import PluginControlView from './PluginControlView';
import SplitControlView from './SplitControlView';
import Typography from '@mui/material/Typography';
import IControlViewFactory from './IControlViewFactory';
import ToobInputStageViewFactory from './ToobInputStageView';
import ToobToneStackViewFactory from './ToobToneStackView';
import ToobCabSimViewFactory from './ToobCabSimView';
import ToobPowerStage2Factory from './ToobPowerStage2View';
import ToobSpectrumAnalyzerViewFactory from './ToobSpectrumAnalyzerView';
import ToobMLViewFactory from './ToobMLView';
import { ToobTunerViewFactory, GxTunerViewFactory } from './GxTunerView';


let pluginFactories: IControlViewFactory[] = [
    new ToobInputStageViewFactory(),
    new ToobToneStackViewFactory(),
    new ToobCabSimViewFactory(),
    new ToobPowerStage2Factory(),
    new ToobSpectrumAnalyzerViewFactory(),
    new ToobMLViewFactory(),
    new GxTunerViewFactory(),
    new ToobTunerViewFactory()
];


export function GetControlView(pedalboardItem?: PedalboardItem | null): React.ReactNode {
    let model: PiPedalModel = PiPedalModelFactory.getInstance();

    if (!pedalboardItem) {
        return (<div />);
    }
    if (pedalboardItem.isStart() || pedalboardItem.isEnd()) {
        return (
            <PluginControlView instanceId={pedalboardItem.instanceId} item={pedalboardItem} />
        );
    }
    if (pedalboardItem.isSplit()) {
        return (
            <SplitControlView item={pedalboardItem as PedalboardSplitItem} instanceId={pedalboardItem!.instanceId}
            />
        );
    } else {
        for (let i = 0; i < pluginFactories.length; ++i) {
            let factory = pluginFactories[i];
            if (factory.uri === pedalboardItem.uri) {
                return factory.Create(model, pedalboardItem);
            }
        }
        let uiPlugin = model.getUiPlugin(pedalboardItem.uri);
        if (!uiPlugin) {
            <div style={{ paddingLeft: 40, paddingRight: 40 }}>
                <Typography color="error" variant="h6" >Missing plugin.</Typography>
                <Typography>The plugin '{pedalboardItem.pluginName}' ({pedalboardItem.uri}) is not currently installed.</Typography>
            </div>
        } else {
            return (
                <PluginControlView instanceId={pedalboardItem.instanceId} item={pedalboardItem} />
            )
        }
    }
}

