// Copyright (c) 2025 Robin Davies
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
import { PiPedalModelFactory, PiPedalModel, ListenHandle, State } from "./PiPedalModel";
import { PedalboardItem,ControlValue } from './Pedalboard';
import PluginControlView, { ICustomizationHost, ControlGroup, ControlViewCustomization } from './PluginControlView';
import { UiPlugin,UiControl, ControlType } from './Lv2Plugin';


const TOOB_NAM__MODEL_METADATA = "http://two-play.com/plugins/toob-nam#model_metadata";

// offset 0: an integer with the folloing bits set.
class TOOB_NAM_METADATA_OFFSETS {
    static readonly flags = 0;
    static readonly preset_version = 1;
    static readonly loudness = 2;
    static readonly gain = 3;
    static readonly input_level_dbu = 4;
    static readonly output_level_dbu = 5;
    static readonly max_medatadata = 6;
};


class TOOB_NAM_METADATA_FLAGS {
    static readonly has_model = 1;
    static readonly has_loudness = 2;
    static readonly has_gain = 4;
    static readonly has_input_level_dbu = 8;
    static readonly has_output_level_dbu = 16;
};

class ModelMetadata {
    constructor(metadataValues?: number[]) 
    {

        
        if (metadataValues) {
            let flags = metadataValues[TOOB_NAM_METADATA_OFFSETS.flags];
            this.hasModel = (flags & TOOB_NAM_METADATA_FLAGS.has_model) !== 0;
            this.hasLoudness = (flags & TOOB_NAM_METADATA_FLAGS.has_loudness) !== 0;
            this.hasGain = (flags & TOOB_NAM_METADATA_FLAGS.has_gain) !== 0;
            this.hasInputLevelDBU = (flags & TOOB_NAM_METADATA_FLAGS.has_input_level_dbu) !== 0;
            this.hasOutputLevelDBU = (flags & TOOB_NAM_METADATA_FLAGS.has_output_level_dbu) !== 0;

            this.loudness = metadataValues[TOOB_NAM_METADATA_OFFSETS.loudness];
            this.gain = metadataValues[TOOB_NAM_METADATA_OFFSETS.gain];
            this.inputlevelDBU = metadataValues[TOOB_NAM_METADATA_OFFSETS.input_level_dbu];
            this.outputlevelDBU = metadataValues[TOOB_NAM_METADATA_OFFSETS.output_level_dbu];
            this.preset_version = metadataValues[TOOB_NAM_METADATA_OFFSETS.preset_version];
        } else {
            this.hasModel = false;
            this.hasLoudness = false;
            this.hasGain = false;
            this.hasInputLevelDBU = false;
            this.hasOutputLevelDBU = false;

            this.loudness = 0;
            this.gain = 0;
            this.inputlevelDBU = 0;
            this.outputlevelDBU = 0;
            this.preset_version = 1;
        }
    }

    preset_version: number;

    hasModel: boolean;
    hasLoudness: boolean;
    hasGain: boolean;
    hasInputLevelDBU: boolean;
    hasOutputLevelDBU: boolean;

    loudness: number;
    gain: number;
    inputlevelDBU: number;
    outputlevelDBU: number;

}


const styles = (theme: Theme) => createStyles({
});

interface ToobNamViewProps extends WithStyles<typeof styles> {
    instanceId: number;
    item: PedalboardItem;

}
interface ToobNamViewState {
    showEqSection: boolean;
    enableCalibration: boolean;
    enableOutputNormalization: boolean;
    modelMetadata: ModelMetadata;
}

const ToobNamView =
    withStyles(
        class extends React.Component<ToobNamViewProps, ToobNamViewState>
            implements ControlViewCustomization {
            model: PiPedalModel;

            customizationId: number = 1;

            constructor(props: ToobNamViewProps) {
                super(props);
                this.handleConnectionStateChanged = this.handleConnectionStateChanged.bind(this);

                this.model = PiPedalModelFactory.getInstance();
                this.state = {
                    showEqSection: false,
                    enableCalibration: false,
                    enableOutputNormalization: false,
                    modelMetadata: new ModelMetadata()
                }
                let pluginInfo: UiPlugin | null = this.model.getUiPlugin(this.props.item.uri);
                if (pluginInfo === null)
                {
                    throw new Error("Plugin not fouund.");
                }
                let inputCalibrationControl = pluginInfo.getControl("inputCalibrationMode");
                if (!inputCalibrationControl)
                {
                    throw new Error("Control not found.");
                }
                let patchedInputControl = new UiControl().deserialize(inputCalibrationControl);
                patchedInputControl.controlType = ControlType.Select;
                this.patchedInputControl = patchedInputControl

                let outputCalibrationControl = pluginInfo.getControl("outputCalibration");
                if (!outputCalibrationControl)
                {
                    throw new Error("Control not found.");
                }
                let patchedOutputControl = new UiControl().deserialize(outputCalibrationControl);
                patchedOutputControl.controlType = ControlType.Select;
                this.noCalibrationOutputControl = patchedOutputControl
                this.noCalibrationOutputControl.scale_points.splice(1,1);
            }

            private patchedInputControl: UiControl;
            private noCalibrationOutputControl: UiControl;
            fullScreen() {
                return false;
            }

            disableControl(control: React.ReactNode, key: string) {
                return (
                    <div style={{ opacity: 0.3, pointerEvents: "none" }}
                       key={key}

                        onPointerDownCapture={(e) => {
                            e.stopPropagation();
                            e.preventDefault();
                        }}
                        onClick={(e) => {
                            e.stopPropagation();
                            e.preventDefault();
                        }}
                    >
                        {control}
                    </div>
                );
            }

            modifyControls(host: ICustomizationHost, controls: (React.ReactNode | ControlGroup)[]): (React.ReactNode | ControlGroup)[] {
                let EqPos = 7;
                let CalibrationGroupPos = 8;

                let calibrationGroup = controls[CalibrationGroupPos] as ControlGroup;

                calibrationGroup.controls[0] = host.makeStandardControl(this.patchedInputControl,this.props.item.controlValues);

                if (this.state.enableCalibration)
                {
                    calibrationGroup.controls[0] = host.makeStandardControl(this.patchedInputControl,this.props.item.controlValues);

                } else  {
                    calibrationGroup.controls[0] = host.makeStandardControl(this.patchedInputControl,
                        [ new ControlValue("inputCalibrationMode",0.0)]
                    );
                    calibrationGroup.controls[0] = this.disableControl(calibrationGroup.controls[0], "cg_01d");
                    calibrationGroup.controls[1] = this.disableControl(calibrationGroup.controls[1], "cg_02d");

                    if (this.state.enableOutputNormalization)
                    {
                        calibrationGroup.controls[2] = host.makeStandardControl(this.noCalibrationOutputControl, this.props.item.controlValues);
                    } else {
                        let tControl: UiControl = this.noCalibrationOutputControl.clone();
                        tControl.symbol = "_disabled_output";
                        calibrationGroup.controls[2] = host.makeStandardControl(
                            tControl, 
                            [ new ControlValue("_disabled_output",2.0)]);
                        calibrationGroup.controls[2] = this.disableControl(calibrationGroup.controls[2], "cg_03d");

                    }
                }
                if (!this.state.showEqSection) {
                    controls.splice(EqPos, 1);
                }
                return controls;
            }

            private handleConnectionStateChanged(state: State) {
                if (state === State.Ready) {
                    this.unsubscribeFromMetadata();
                    this.subscribeToMetdata();
                }
            }

            handleModelMetadata(atomData: any) {
                if (atomData && atomData.otype_ === "Vector" && atomData.value) {
                    let metadata = new ModelMetadata(atomData.value as number[]);
                    this.setState({ 
                        modelMetadata: metadata,
                        showEqSection: metadata.preset_version === 0,
                        enableCalibration: metadata.hasInputLevelDBU && metadata.hasModel,
                        enableOutputNormalization: metadata.hasLoudness && metadata.hasModel
                    });
                }
            }

            subscribeToMetdata() {
                this.subscribedId = this.props.instanceId;
                this.listenHandle = this.model.monitorPatchProperty(
                    this.props.instanceId,
                    TOOB_NAM__MODEL_METADATA,
                    (instanceId, propertyUri, atomData) => {
                        this.handleModelMetadata(atomData);
                    });
                this.model.getPatchProperty(
                    this.props.instanceId,
                    TOOB_NAM__MODEL_METADATA
                ).then((atomData) => {
                    this.handleModelMetadata(atomData);
                }).catch((e) => {

                });

            }
            unsubscribeFromMetadata() {
                this.subscribedId = null;
                if (this.listenHandle) {
                    this.model.cancelMonitorPatchProperty(this.listenHandle);
                    this.listenHandle = null;
                }
            }

            private listenHandle: ListenHandle | null = null;
            componentDidMount() {
                if (super.componentDidMount) {
                    super.componentDidMount();
                }
                this.subscribeToMetdata();
                this.model.state.addOnChangedHandler(this.handleConnectionStateChanged);
            }
            componentWillUnmount() {
                this.unsubscribeFromMetadata();
                if (super.componentWillUnmount) {
                    super.componentWillUnmount();
                }
                this.model.state.removeOnChangedHandler(this.handleConnectionStateChanged);

            }

            private subscribedId: number | null = null;
            componentDidUpdate() {
                if (this.props.instanceId !== this.subscribedId) {
                    this.unsubscribeFromMetadata();
                    this.subscribeToMetdata();
                }
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



class ToobNamViewFactory implements IControlViewFactory {
    uri: string = "http://two-play.com/plugins/toob-nam";

    Create(model: PiPedalModel, pedalboardItem: PedalboardItem): React.ReactNode {
        return (<ToobNamView instanceId={pedalboardItem.instanceId} item={pedalboardItem} />);
    }


}
export default ToobNamViewFactory;