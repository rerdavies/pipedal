// Copyright (c) 2021 Robin Davies
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
import { createStyles, withStyles, WithStyles, Theme } from '@material-ui/core/styles';
import { PiPedalModel, PiPedalModelFactory } from './PiPedalModel';
import { UiPlugin, UiControl } from './Lv2Plugin';
import {
    PedalBoard, PedalBoardItem, ControlValue,
} from './PedalBoard';
import PluginControl from './PluginControl';
import ResizeResponsiveComponent from './ResizeResponsiveComponent';
import VuMeter from './VuMeter';
import { nullCast } from './Utility'
import GxTunerControl from './GxTunerControl';
import { PiPedalStateError } from './PiPedalError';
import Typography from '@material-ui/core/Typography';
import FullScreenIME from './FullScreenIME';


export const StandardItemSize = { width: 80, height: 110 };


const GXTUNER_URI = "http://guitarix.sourceforge.net/plugins/gxtuner#tuner";

const LANDSCAPE_HEIGHT_BREAK = 500;

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
        paddingLeft: 12,
        paddingRight: 4,
        paddingBottom: 24,
        left: 0,
        background: "white",
        zIndex: 3

    },
    vuMeterR: {
        position: "fixed",
        right: 0,
        marginRight: 22,
        paddingLeft: 4,
        paddingBottom: 24,
        background: "white",
        zIndex: 3

    },
    vuMeterRLandscape: {
        position: "fixed",
        right: 0,
        paddingRight: 22,
        paddingLeft: 12,
        paddingBottom: 24,
        background: "white",
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
        borderRadius: 4,
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
        borderRadius: 10,
        elevation: 12,
        display: "flex",
        flexDirection: "row", flexWrap: "nowrap",
        flex: "0 0 auto"
    },
    portGroupTitle: {
        position: "absolute",
        top: -10,
        background: "white",
        marginLeft: 20,
        paddingLeft: 8,
        paddingRight: 8
    },
    portGroupControls: {
        display: "flex",
        flexDirection: "row", flexWrap: "wrap",
        paddingTop: 6,
        paddingBottom: 8
    }


});

export class ControlGroup {
    constructor(name: string, controls: ReactNode[])
    {
        this.name = name;
        this.controls = controls;
    }
    name: string;
    controls: React.ReactNode[];
}

export type ControlNodes = (ReactNode | ControlGroup)[];


export interface ControlViewCustomization {
    ModifyControls(controls: (React.ReactNode | ControlGroup)[]): (React.ReactNode | ControlGroup)[];

};

export interface PluginControlViewProps extends WithStyles<typeof styles> {
    theme: Theme;
    instanceId: number;
    item: PedalBoardItem;
    customization?: ControlViewCustomization;
    customizationId?: number;
};
type PluginControlViewState = {
    landscapeGrid: boolean;
    imeUiControl?: UiControl;
    imeValue: number;
    imeCaption: string;
    imeInitialHeight: number;
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
                    imeInitialHeight: 0

                }
                this.onPedalBoardChanged = this.onPedalBoardChanged.bind(this);
                this.onValueChanged = this.onValueChanged.bind(this);
                this.onPreviewChange = this.onPreviewChange.bind(this);
            }


            onPreviewChange(key: string, value: number): void {
                this.model.previewPedalBoardValue(this.props.instanceId, key, value);
            }

            onValueChanged(key: string, value: number): void {
                this.model.setPedalBoardControlValue(this.props.instanceId, key, value);
            }

            onPedalBoardChanged(value?: PedalBoard) {
                //let item = this.model.pedalBoard.get().maybeGetItem(this.props.instanceId);
                //this.setState({ pedalBoardItem: item });
            }


            componentDidMount() {
                super.componentDidMount();
                this.model.pedalBoard.addOnChangedHandler(this.onPedalBoardChanged);
            }
            componentWillUnmount() {
                this.model.pedalBoard.removeOnChangedHandler(this.onPedalBoardChanged);
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
                    if (control && !control.not_on_gui) {
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
            makeStandardControl(uiControl: UiControl, controlValues: ControlValue[]): ReactNode {
                let symbol = uiControl.symbol;

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
                            onChange={(value: number) => { this.onValueChanged(controlValue!.key, value) }}
                            onPreviewChange={(value: number) => { this.onPreviewChange(controlValue!.key, value) }}
                            requestIMEEdit={(uiControl, value) => this.requestImeEdit(uiControl, value)}

                        />
                ));

            }
            
            getStandardControlNodes(plugin: UiPlugin, controlValues: ControlValue[]): ControlNodes {
                let result: ControlNodes = [];

                for (let i = 0; i < plugin.controls.length; ++i) {
                    let pluginControl = plugin.controls[i];
                    if (!pluginControl.not_on_gui) {
                        if (pluginControl.port_group !== "") {
                            let portGroup = pluginControl.port_group;
                            let groupControls: ReactNode[] = [];
                            groupControls.push(
                                this.makeStandardControl(pluginControl, controlValues)
                            );
                            while (i + 1 < plugin.controls.length && plugin.controls[i + 1].port_group === portGroup) {
                                ++i;
                                pluginControl = plugin.controls[i];
                                if (!pluginControl.not_on_gui) {
                                    groupControls.push(
                                        this.makeStandardControl(pluginControl, controlValues)
                                    )
                                }
                            }
                            result.push(
                                new ControlGroup(plugin.getPortGroup(portGroup).name, groupControls)
                            )
                        } else {
                            result.push(
                                    this.makeStandardControl(pluginControl, controlValues)
                            );
                        }
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

            getTunerControls(plugin: UiPlugin, controlValues: ControlValue[]): ControlNodes {
                // display standard version of just a few controls.
                let shortList: ControlValue[] = [];
                // for (let i = 0; i < controlValues.length; ++i)
                // {
                //     shortList.push(controlValues[i]);
                // }
                shortList.push(this.getControl(controlValues, "REFFREQ"));
                shortList.push(this.getControl(controlValues, "THRESHOLD"));


                let controls = shortList.map((controlValue) => (
                            <PluginControl instanceId={this.props.instanceId} uiControl={plugin.getControl(controlValue.key)} value={controlValue.value}
                                onChange={(value: number) => { this.onValueChanged(controlValue.key, value) }}
                                onPreviewChange={(value: number) => { this.onPreviewChange(controlValue.key, value) }}
                                requestIMEEdit={(uiControl, value) => this.requestImeEdit(uiControl, value)}
                            />
                ));
                let tunerControl = (<GxTunerControl instanceId={this.props.instanceId} />);
                let result: ControlNodes = [];
                result.push(tunerControl);

                for (let i = 0; i < controls.length; ++i)
                {
                    result.push(controls[i]);
                }

                return result;
            }
            onImeValueChange(key: string, value: number) {
                this.model.setPedalBoardControlValue(this.props.instanceId, key, value);
                this.onImeClose();
            }
            onImeClose() {
                this.setState({
                    imeUiControl: undefined
                });
            }

            hasGroups(nodes: (ReactNode | ControlGroup)[]): boolean
            {
                for (let i = 0; i < nodes.length; ++i)
                {
                    let node = nodes[i];
                    if (node instanceof ControlGroup) return true;
                }
                return false;
            }

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
                        for (let j = 0; j < controlGroup.controls.length; ++j)
                        {
                            let item = controlGroup.controls[j];
                            controls.push(
                                (
                                    <div className={classes.controlPadding}>
                                        { item }
                                    </div>

                                )
                            );
                        }

                        result.push((
                            <div className={!isLandscapeGrid ? classes.portGroup : classes.portGroupLandscape}>
                                <div className={classes.portGroupTitle}>
                                    <Typography variant="caption">{controlGroup.name}</Typography>
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
                            <div className={ hasGroups ? classes.portgroupControlPadding: classes.controlPadding } >
                                {node as ReactNode}
                            </div>
                        ));
                    }

                }
                return result;
            }


            render(): ReactNode {
                let classes = this.props.classes;
                let pedalBoardItem = this.model.pedalBoard.get().getItem(this.props.instanceId);

                if (!pedalBoardItem)
                    return (<div className={classes.frame} ></div>);

                let controlValues = pedalBoardItem.controlValues;


                let plugin: UiPlugin = nullCast(this.model.getUiPlugin(pedalBoardItem.uri));


                controlValues = this.filterNotOnGui(controlValues, plugin);


                let gridClass = this.state.landscapeGrid ? classes.landscapeGrid : classes.normalGrid;
                let vuMeterRClass = this.state.landscapeGrid ? classes.vuMeterRLandscape : classes.vuMeterR;
                let controlNodes: ControlNodes;

                if (plugin.uri === GXTUNER_URI) {
                    controlNodes = this.getTunerControls(plugin, controlValues);
                } else {
                    controlNodes = this.getStandardControlNodes(plugin, controlValues);
                }
                if (this.props.customization)
                {
                    // allow wrapper class to insert/remove/rebuild controls.
                    controlNodes = this.props.customization.ModifyControls(controlNodes);
                }

                let nodes = this.controlNodesToNodes(controlNodes);

                return (
                    <div className={classes.frame}>
                        <div className={classes.vuMeterL}>
                            <VuMeter display="input" instanceId={pedalBoardItem.instanceId} />
                        </div>
                        <div className={vuMeterRClass}>
                            <VuMeter display="output" instanceId={pedalBoardItem.instanceId} />
                        </div>
                        <div className={gridClass}  >
                            {
                                nodes
                            }
                            <div style={{ flex: "0 0 40px", width: 40, height: 1 }} />
                        </div>
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

