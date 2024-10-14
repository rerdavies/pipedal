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

import { ReactNode } from 'react';
import { Theme } from '@mui/material/styles';
import { WithStyles } from '@mui/styles';
import createStyles from '@mui/styles/createStyles';
import withStyles from '@mui/styles/withStyles';
import { PiPedalModel, PiPedalModelFactory } from './PiPedalModel';
import { UiPlugin, UiControl, UiFileProperty,UiFrequencyPlot, ScalePoint } from './Lv2Plugin';
import {
    Pedalboard, PedalboardItem, ControlValue
} from './Pedalboard';
import PluginControl from './PluginControl';
import ResizeResponsiveComponent from './ResizeResponsiveComponent';
import VuMeter from './VuMeter';
import { nullCast } from './Utility'
import { PiPedalStateError } from './PiPedalError';
import Typography from '@mui/material/Typography';
import FullScreenIME from './FullScreenIME';
import FilePropertyControl from './FilePropertyControl';
import FilePropertyDialog from './FilePropertyDialog';
import JsonAtom from './JsonAtom';
import PluginOutputControl from './PluginOutputControl';
import Units from './Units';
import ToobFrequencyResponseView from './ToobFrequencyResponseView';


export const StandardItemSize = { width: 80, height: 110 };



const LANDSCAPE_HEIGHT_BREAK = 500;


function makeIoPluginInfo(name: string, uri: string): UiPlugin {
    let result = new UiPlugin();
    result.name = name;
    result.uri = uri;
    let volumeControl = new UiControl();
    volumeControl.name = "Volume";
    volumeControl.symbol = "volume_db";
    volumeControl.index = 0;
    volumeControl.is_input = true;

    volumeControl.min_value = -60;
    volumeControl.max_value = 30;
    volumeControl.default_value = 0;
    volumeControl.units = Units.db;
    volumeControl.scale_points = [
        new ScalePoint().deserialize({ label: "-INF", value: -60 })
    ];
    result.controls = [
        volumeControl
    ];
    return result;
}

let startPluginInfo: UiPlugin =
    makeIoPluginInfo("Input", Pedalboard.START_PEDALBOARD_ITEM_URI);

let endPluginInfo: UiPlugin =
    makeIoPluginInfo("Output", Pedalboard.END_PEDALBOARD_ITEM_URI);



const styles = (theme: Theme) => createStyles({
    frame: {
        display: "block",
        position: "relative",
        flexDirection: "row",
        flexWrap: "nowrap",
        paddingTop: "8px",
        paddingBottom: "0px",
        height: "100%",
        overflowX: "auto",
        overflowY: "auto"
    },
    vuMeterL: {
        position: "fixed",
        paddingLeft: 6,
        paddingRight: 4,
        paddingBottom: 24,
        left: 0,
        background: theme.mainBackground,
        zIndex: 3

    },
    vuMeterR: {
        position: "fixed",
        right: 0,
        marginRight: 22,
        paddingLeft: 4,
        paddingBottom: 24,
        background: theme.mainBackground,
        zIndex: 3

    },
    vuMeterRLandscape: {
        position: "fixed",
        right: 0,
        paddingRight: 22,
        paddingLeft: 12,
        paddingBottom: 24,
        background: theme.mainBackground,
        zIndex: 3

    },

    normalGrid: {
        position: "relative",
        paddingLeft: 22,
        paddingRight: 34,
        flex: "1 1 auto",
        display: "flex", flexDirection: "row", flexWrap: "wrap",
        justifyContent: "flex-start", alignItems: "flex_start",


    },
    landscapeGrid: {
        paddingLeft: 40,
        // marginRight: 40, : bug in chrome layout engine wrt/ right margin/padding. See the spacer div added after all controls in render() with provides the same effect.
        display: "flex", flexDirection: "row", flexWrap: "nowrap",
        justifyContent: "flex-start", alignItems: "flex_start",
        flex: "0 0 auto"
    },
    portgroupControlPadding: {
        flex: "0 0 auto",
        marginTop: 12,
        marginBottom: 8

    },
    controlPadding: {
        flex: "0 0 auto",
        marginTop: 0,
        marginBottom: 0

    },
    portGroup: {
        marginLeft: 8,
        marginTop: 0,
        marginRight: 8,
        marginBottom: 12,
        position: "relative",
        paddingLeft: 3,
        paddingRight: 0,
        paddingTop: 0,
        paddingBottom: 0,
        border: "2pt #AAA solid",
        borderRadius: 8,
        elevation: 12,
        display: "flex",
        flexDirection: "row", flexWrap: "wrap",
        flex: "0 1 auto",
    },
    portGroupLandscape: {
        marginLeft: 8,
        marginTop: 0,
        marginRight: 8,
        marginBottom: 12,
        position: "relative",
        paddingLeft: 0,
        paddingRight: 0,
        paddingTop: 0,
        paddingBottom: 0,
        border: "2pt #AAA solid",
        borderRadius: 8,
        elevation: 12,
        display: "flex",
        textOverflow: "ellipsis",
        flexDirection: "row", flexWrap: "nowrap",
        flex: "0 0 auto"
    },
    portGroupTitle: {
        position: "absolute",
        top: -15,
        background: theme.mainBackground,
        textOverflow: "ellipsis",
        minWidth: 0,
        marginLeft: 20,
        paddingLeft: 8,
        paddingRight: 8,
        margin_right: 28
    },
    portGroupControls: {
        display: "flex",
        flexDirection: "row", flexWrap: "wrap",
        paddingTop: 6,
        paddingBottom: 8
    }


});

export class ControlGroup {
    constructor(name: string, indexes: number[], controls: ReactNode[]) {
        this.name = name;
        this.indexes = indexes;
        this.controls = controls;
    }
    name: string;
    indexes: number[];
    controls: React.ReactNode[];
}

export type ControlNodes = (ReactNode | ControlGroup)[];


export interface ControlViewCustomization {
    ModifyControls(controls: (React.ReactNode | ControlGroup)[]): (React.ReactNode | ControlGroup)[];

}

export interface PluginControlViewProps extends WithStyles<typeof styles> {
    theme: Theme;
    instanceId: number;
    item: PedalboardItem;
    customization?: ControlViewCustomization;
    customizationId?: number;
}
type PluginControlViewState = {
    landscapeGrid: boolean;
    imeUiControl?: UiControl;
    imeValue: number;
    imeCaption: string;
    imeInitialHeight: number;
    showFileDialog: boolean,
    dialogFileProperty: UiFileProperty,
    dialogFileValue: string
};

const PluginControlView =
    withStyles(styles, { withTheme: true })(
        class extends ResizeResponsiveComponent<PluginControlViewProps, PluginControlViewState>
        {
            model: PiPedalModel;

            constructor(props: PluginControlViewProps) {
                super(props);
                this.model = PiPedalModelFactory.getInstance();

                this.state = {
                    landscapeGrid: false,
                    imeUiControl: undefined,
                    imeValue: 0,
                    imeCaption: "",
                    imeInitialHeight: 0,
                    showFileDialog: false,
                    dialogFileProperty: new UiFileProperty(),
                    dialogFileValue: ""

                }
                this.onPedalboardChanged = this.onPedalboardChanged.bind(this);
                this.onControlValueChanged = this.onControlValueChanged.bind(this);
                this.onPreviewChange = this.onPreviewChange.bind(this);
            }


            onPreviewChange(key: string, value: number): void {
                this.model.previewPedalboardValue(this.props.instanceId, key, value);
            }

            onControlValueChanged(key: string, value: number): void {
                this.model.setPedalboardControl(this.props.instanceId, key, value);
            }

            onPedalboardChanged(value?: Pedalboard) {
                //let item = this.model.pedalboard.get().maybeGetItem(this.props.instanceId);
                //this.setState({ pedalboardItem: item });
            }


            componentDidMount() {
                super.componentDidMount();
                this.model.pedalboard.addOnChangedHandler(this.onPedalboardChanged);
            }
            componentWillUnmount() {
                this.model.pedalboard.removeOnChangedHandler(this.onPedalboardChanged);
                super.componentWillUnmount();
            }



            onWindowSizeChanged(width: number, height: number): void {
                this.setState({ landscapeGrid: height < LANDSCAPE_HEIGHT_BREAK });
            }
            filterNotOnGui(controlValues: ControlValue[], uiPlugin: UiPlugin): ControlValue[] {
                let result: ControlValue[] = [];

                for (let i = 0; i < controlValues.length; ++i) {
                    let controlValue = controlValues[i];
                    let control = uiPlugin.getControl(controlValue.key);
                    if (control && !control.isHidden()) {
                        result.push(controlValue);
                    }
                }
                return result;
            }

            requestImeEdit(uiControl: UiControl, value: number) {
                // eslint-disable-next-line no-restricted-globals

                this.setState({
                    imeUiControl: uiControl,
                    imeValue: value,
                    imeCaption: uiControl.name,
                    imeInitialHeight: window.innerHeight

                });
            }

            makeFilePropertyUI(fileProperty: UiFileProperty): ReactNode {
                return ((

                    <FilePropertyControl pedalboardItem={this.props.item}
                        fileProperty={fileProperty}
                        onFileClick={(fileProperty, selectedFile) => {
                            this.setState({ showFileDialog: true, dialogFileProperty: fileProperty, dialogFileValue: selectedFile });
                        }}
                    />
                ));
            }
            makeFrequencyPlotUI(frequencyPlot: UiFrequencyPlot): ReactNode {
                return ((
                    <ToobFrequencyResponseView instanceId={this.props.instanceId}
                        propertyName={frequencyPlot.patchProperty}
                        width={frequencyPlot.width}
                    />
                ));
            }
            makeStandardControl(uiControl: UiControl, controlValues: ControlValue[]): ReactNode {
                let symbol = uiControl.symbol;
                if (!uiControl.is_input) {
                    return (
                        <PluginOutputControl instanceId={this.props.instanceId} uiControl={uiControl} />

                    );
                }

                let controlValue: ControlValue | undefined = undefined;
                for (let i = 0; i < controlValues.length; ++i) {
                    if (controlValues[i].key === symbol) {
                        controlValue = controlValues[i];
                        break;
                    }
                }
                if (!controlValue) {
                    throw new PiPedalStateError("Missing control value.");
                }
                return ((

                    <PluginControl instanceId={this.props.instanceId} uiControl={uiControl} value={controlValue.value}
                        onChange={(value: number) => { this.onControlValueChanged(controlValue!.key, value) }}
                        onPreviewChange={(value: number) => { this.onPreviewChange(controlValue!.key, value) }}
                        requestIMEEdit={(uiControl, value) => this.requestImeEdit(uiControl, value)}

                    />
                ));

            }

            getStandardControlNodes(plugin: UiPlugin, controlValues: ControlValue[]): ControlNodes {
                let result: ControlNodes = [];
                let portGroupMap: { [id: string]: ControlGroup } = {};

                for (let i = 0; i < plugin.controls.length; ++i) {
                    let pluginControl = plugin.controls[i];
                    if (!pluginControl.isHidden()) {
                        if (pluginControl.port_group !== "" && plugin.getPortGroupBySymbol(pluginControl.port_group)) {
                            let portGroup = nullCast(plugin.getPortGroupBySymbol(pluginControl.port_group));

                            let groupControls: ReactNode[] = [];
                            let indexes: number[] = [];
                            groupControls.push(
                                this.makeStandardControl(pluginControl, controlValues)
                            );
                            indexes.push(pluginControl.index);
                            while (i + 1 < plugin.controls.length && plugin.controls[i + 1].port_group === pluginControl.port_group) {
                                ++i;
                                pluginControl = plugin.controls[i];
                                if (!pluginControl.isHidden()) {
                                    groupControls.push(
                                        this.makeStandardControl(pluginControl, controlValues)
                                    )
                                    indexes.push(pluginControl.index);
                                }
                            }
                            let controlGroup = new ControlGroup(portGroup.name, indexes, groupControls);
                            result.push(
                                controlGroup
                            )
                            portGroupMap[pluginControl.port_group] = controlGroup;
                        } else {
                            result.push(
                                this.makeStandardControl(pluginControl, controlValues)
                            );
                        }
                    }
                }
                for (let i = 0; i < plugin.fileProperties.length; ++i) {
                    let fileProperty = plugin.fileProperties[i];
                    let filePropertyUi = this.makeFilePropertyUI(fileProperty);

                    if (fileProperty.portGroup !== "" && plugin.getPortGroupByUri(fileProperty.portGroup)) {
                        let portGroup = nullCast(plugin.getPortGroupByUri(fileProperty.portGroup));
                        let controlGroup = portGroupMap[portGroup.symbol];
                        if (controlGroup) {
                            let insertPosition = controlGroup.indexes.length;
                            if (fileProperty.index !== -1) {
                                for (let i = 0; i < controlGroup.controls.length; ++i) {
                                    if (controlGroup.indexes[i] >= fileProperty.index) {
                                        insertPosition = i;
                                        break;
                                    }
                                }
                            }
                            let index = fileProperty.index !== -1 ? fileProperty.index : 100;
                            controlGroup.controls.splice(insertPosition, 0, filePropertyUi);
                            controlGroup.indexes.splice(insertPosition, 0, index);

                        } else {
                            let index = fileProperty.index !== -1 ? fileProperty.index : 100;
                            let controlGroup = new ControlGroup(
                                portGroup.name,
                                [index],
                                [filePropertyUi]);
                            result.push(
                                controlGroup
                            );
                            portGroupMap[portGroup.symbol] = controlGroup;

                        }
                    } else if (fileProperty.index !== -1) {
                        result.splice(fileProperty.index, 0, filePropertyUi);
                    } else {
                        result.push(
                            filePropertyUi
                        );
                    }
                }
                for (let i = 0; i < plugin.frequencyPlots.length; ++i) {
                    let frequencyPlot = plugin.frequencyPlots[i];
                    let frequencyPlotUi = this.makeFrequencyPlotUI(frequencyPlot);

                    if (frequencyPlot.portGroup !== "" && plugin.getPortGroupByUri(frequencyPlot.portGroup)) {
                        let portGroup = nullCast(plugin.getPortGroupByUri(frequencyPlot.portGroup));
                        let controlGroup = portGroupMap[portGroup.symbol];
                        if (controlGroup) {
                            let insertPosition = controlGroup.indexes.length;
                            if (frequencyPlot.index !== -1) {
                                for (let i = 0; i < controlGroup.controls.length; ++i) {
                                    if (controlGroup.indexes[i] >= frequencyPlot.index) {
                                        insertPosition = i;
                                        break;
                                    }
                                }
                            }
                            let index = frequencyPlot.index !== -1 ? frequencyPlot.index : 100;
                            controlGroup.controls.splice(insertPosition, 0, frequencyPlotUi);
                            controlGroup.indexes.splice(insertPosition, 0, index);

                        } else {
                            let index = frequencyPlot.index !== -1 ? frequencyPlot.index : 100;
                            let controlGroup = new ControlGroup(
                                portGroup.name,
                                [index],
                                [frequencyPlotUi]);
                            result.push(
                                controlGroup
                            );
                            portGroupMap[portGroup.symbol] = controlGroup;

                        }
                    } else if (frequencyPlot.index !== -1) {
                        result.splice(frequencyPlot.index, 0, frequencyPlotUi);
                    } else {
                        result.push(
                            frequencyPlotUi
                        );
                    }
                }

                return result;
            }

            getControl(controlValues: ControlValue[], key: string) {
                for (let i = 0; i < controlValues.length; ++i) {
                    if (controlValues[i].key === key) {
                        return controlValues[i];
                    }
                }
                throw new Error("Not found.");
            }

            onImeValueChange(key: string, value: number) {
                this.model.setPedalboardControl(this.props.instanceId, key, value);
                this.onImeClose();
            }
            onImeClose() {
                this.setState({
                    imeUiControl: undefined
                });
            }

            hasGroups(nodes: (ReactNode | ControlGroup)[]): boolean {
                for (let i = 0; i < nodes.length; ++i) {
                    let node = nodes[i];
                    if (node instanceof ControlGroup) return true;
                }
                return false;
            }

            controlKeyIndex : number = 0;
            controlNodesToNodes(nodes: (ReactNode | ControlGroup)[]): ReactNode[] {
                let classes = this.props.classes;
                let isLandscapeGrid = this.state.landscapeGrid;
                let hasGroups = this.hasGroups(nodes);

                let result: ReactNode[] = [];

                for (let i = 0; i < nodes.length; ++i) {
                    let node = nodes[i];
                    if (node instanceof ControlGroup) {
                        let controlGroup = node as ControlGroup;
                        let controls: ReactNode[] = [];
                        for (let j = 0; j < controlGroup.controls.length; ++j) {
                            let item = controlGroup.controls[j];
                            controls.push(
                                (
                                    <div key={"ctl"+(this.controlKeyIndex++)} className={classes.controlPadding}>
                                        {item}
                                    </div>

                                )
                            );
                        }

                        result.push((
                            <div key={"ctl"+(this.controlKeyIndex++)} className={!isLandscapeGrid ? classes.portGroup : classes.portGroupLandscape}>
                                <div className={classes.portGroupTitle}>
                                    <Typography noWrap variant="caption" >{controlGroup.name}</Typography>
                                </div>
                                <div className={classes.portGroupControls} >
                                    {
                                        controls
                                    }
                                </div>
                            </div>
                        ));

                    } else {
                        result.push((
                            <div key={"ctl"+(this.controlKeyIndex++)} className={hasGroups ? classes.portgroupControlPadding : classes.controlPadding} >
                                {node as ReactNode}
                            </div>
                        ));
                    }

                }
                return result;
            }

            static startPluginInfo: UiPlugin =
                makeIoPluginInfo("Input", Pedalboard.START_PEDALBOARD_ITEM_URI);

            static endPluginInfo: UiPlugin =
                makeIoPluginInfo("Output", Pedalboard.END_PEDALBOARD_ITEM_URI);


            render(): ReactNode {
                this.controlKeyIndex = 0; 
                let classes = this.props.classes;
                let pedalboardItem: PedalboardItem;
                let pedalboard = this.model.pedalboard.get();
                if (this.props.instanceId === Pedalboard.START_CONTROL) {
                    pedalboardItem = pedalboard.makeStartItem();
                } else if (this.props.instanceId === Pedalboard.END_CONTROL) {
                    pedalboardItem = pedalboard.makeEndItem();
                } else {
                    pedalboardItem = pedalboard.getItem(this.props.instanceId);
                }

                if (!pedalboardItem)
                    return (<div className={classes.frame} ></div>);

                let controlValues = pedalboardItem.controlValues;


                let plugin: UiPlugin;
                if (pedalboardItem.isStart()) {
                    plugin = startPluginInfo;
                    controlValues = [new ControlValue("volume_db", pedalboard.input_volume_db)];
                } else if (pedalboardItem.isEnd()) {
                    plugin = endPluginInfo;
                    controlValues = [new ControlValue("volume_db", pedalboard.output_volume_db)];
                } else {
                    plugin = nullCast(this.model.getUiPlugin(pedalboardItem.uri));
                }


                controlValues = this.filterNotOnGui(controlValues, plugin);



                let gridClass = this.state.landscapeGrid ? classes.landscapeGrid : classes.normalGrid;
                let vuMeterRClass = this.state.landscapeGrid ? classes.vuMeterRLandscape : classes.vuMeterR;
                let controlNodes: ControlNodes;

                controlNodes = this.getStandardControlNodes(plugin, controlValues);

                if (this.props.customization) {
                    // allow wrapper class to insert/remove/rebuild controls.
                    controlNodes = this.props.customization.ModifyControls(controlNodes);
                }

                let nodes = this.controlNodesToNodes(controlNodes);

                return (
                    <div className={classes.frame}>
                        <div className={classes.vuMeterL}>
                            <VuMeter displayText={true} display="input" instanceId={pedalboardItem.instanceId} />
                        </div>
                        <div className={vuMeterRClass}>
                            <VuMeter displayText={true} display="output" instanceId={pedalboardItem.instanceId} />
                        </div>
                        <div className={gridClass}  >
                            {
                                nodes
                            }
                            <div style={{ flex: "0 0 40px", width: 40, height: 40 }} />
                            {
                                (!this.state.landscapeGrid) && (
                                    <div style={{ flex: "0 1 100%", width: "0px", height: 40 }} />
                                )
                            }
                        </div>
                        <FilePropertyDialog open={this.state.showFileDialog}
                            fileProperty={this.state.dialogFileProperty}
                            selectedFile={this.state.dialogFileValue}
                            onCancel={() => { 
                                this.setState({ showFileDialog: false }); 
                            }}
                            onOk={(fileProperty, selectedFile) => {

                                this.model.setPatchProperty(
                                    this.props.instanceId,
                                    fileProperty.patchProperty,
                                    JsonAtom.Path(selectedFile)
                                )
                                    .then(() => {

                                    })
                                    .catch((error) => {
                                        this.model.showAlert("Unable to complete the operation. Audio is not running." + error);
                                    });
                                this.setState({ showFileDialog: false });
                            }
                            }
                        />
                        <FullScreenIME uiControl={this.state.imeUiControl} value={this.state.imeValue}

                            onChange={(key, value) => this.onImeValueChange(key, value)}
                            initialHeight={
                                this.state.imeInitialHeight}
                            caption={this.state.imeCaption}
                            onClose={() => this.onImeClose()} />
                    </div >
                );

            }

        }
    );

export default PluginControlView;

