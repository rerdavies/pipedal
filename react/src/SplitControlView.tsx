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
import { UiPlugin } from './Lv2Plugin';
import {
    Pedalboard, PedalboardSplitItem, ControlValue,
} from './Pedalboard';
import PluginControl from './PluginControl';
import ResizeResponsiveComponent from './ResizeResponsiveComponent';
import VuMeter from './VuMeter';
import { SplitAbControl, SplitTypeControl, SplitMixControl, SplitPanLeftControl,SplitVolLeftControl,SplitPanRightControl,SplitVolRightControl } from './SplitUiControls';


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
        paddingRight: 12,
        left: 0,
        background: theme.mainBackground,
        zIndex: 3

    },
    vuMeterR: {
        position: "fixed",
        right: 0,
        marginRight: 22,
        paddingLeft: 12,
        background: theme.mainBackground,
        zIndex: 3

    },
    vuMeterRLandscape: {
        position: "fixed",
        right: 0,
        paddingRight: 22,
        paddingLeft: 12,
        background: theme.mainBackground,
        zIndex: 3

    },

    normalGrid: {
        position: "relative",
        paddingLeft: 40,
        paddingRight: 40,
        flex: "1 1 auto",
        display: "flex", flexDirection: "row", flexWrap: "wrap",
        justifyContent: "flex-start", alignItems: "flex_start"

    },
    landscapeGrid: {
        paddingLeft: 40,
        marginRight: 40,
        display: "inline-flex", flexDirection: "row", flexWrap: "nowrap",
        justifyContent: "flex-start", alignItems: "flex_start",
        height: "100%"
    }

});

interface SplitControlViewProps extends WithStyles<typeof styles> {
    theme: Theme;
    instanceId: number;
    item: PedalboardSplitItem;
}
type SplitControlViewState = {
    landscapeGrid: boolean;
};

const SplitControlView =
    withStyles(styles, { withTheme: true })(
        class extends ResizeResponsiveComponent<SplitControlViewProps, SplitControlViewState>
        {
            model: PiPedalModel;

            constructor(props: SplitControlViewProps) {
                super(props);
                this.model = PiPedalModelFactory.getInstance();

                this.state = {
                    landscapeGrid: false

                }
                this.onPedalboardChanged = this.onPedalboardChanged.bind(this);
                this.onValueChanged = this.onValueChanged.bind(this);
                this.onPreviewChange = this.onPreviewChange.bind(this);
            }


            onPreviewChange(key: string, value: number): void {
                this.model.previewPedalboardValue(this.props.instanceId, key, value);
            }

            onValueChanged(key: string, value: number): void {
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

            renderControl(plugin: UiPlugin, controlValue: ControlValue): ReactNode {
                return (
                    <div style={{ display: "flex", flexDirection: "row", alignContent: "center", justifyContent: "flex-start" }}>


                    </div>
                )
            }

            onWindowSizeChanged(width: number, height: number): void {
                this.setState({ landscapeGrid: height < LANDSCAPE_HEIGHT_BREAK });
            }


            render(): ReactNode {
                let classes = this.props.classes;
                let pedalboardItem = this.model.pedalboard.get().getItem(this.props.instanceId);

                if (!pedalboardItem)
                    return (<div className={classes.frame}></div>);


                let gridClass = this.state.landscapeGrid ? classes.landscapeGrid : classes.normalGrid;
                let vuMeterRClass = this.state.landscapeGrid ? classes.vuMeterRLandscape : classes.vuMeterR;
                let typeValue = pedalboardItem.getControl(PedalboardSplitItem.TYPE_KEY);
                let mixValue = pedalboardItem.getControl(PedalboardSplitItem.MIX_KEY);
                let selectValue = pedalboardItem.getControl(PedalboardSplitItem.SELECT_KEY);
                let panLValue = pedalboardItem.getControl(PedalboardSplitItem.PANL_KEY);
                let volumeLValue = pedalboardItem.getControl(PedalboardSplitItem.VOLL_KEY);
                let panRValue = pedalboardItem.getControl(PedalboardSplitItem.PANR_KEY);
                let volumeRValue = pedalboardItem.getControl(PedalboardSplitItem.VOLR_KEY);

                return (
                    <div className={classes.frame}>
                        <div className={classes.vuMeterL}>
                            <VuMeter display="input" instanceId={pedalboardItem.instanceId} />
                        </div>
                        <div className={vuMeterRClass}>
                            <VuMeter display="output" instanceId={pedalboardItem.instanceId} />
                        </div>
                        <div className={gridClass}  >
                            <div style={{ flex: "0 0 auto" }}>
                                <PluginControl instanceId={this.props.instanceId} uiControl={SplitTypeControl} value={typeValue.value}
                                    onChange={(value: number) => { this.onValueChanged(typeValue.key, value) }}
                                    onPreviewChange={(value: number) => { this.onPreviewChange(typeValue.key, value) }}
                                    requestIMEEdit={(uiControl, value) => { }}

                                />
                            </div>
                            {
                                typeValue.value === 0 && (
                                    <div style={{ flex: "0 0 auto" }}>
                                        <PluginControl instanceId={this.props.instanceId} uiControl={SplitAbControl} value={selectValue.value}
                                            onChange={(value: number) => { this.onValueChanged(selectValue.key, value) }}
                                            onPreviewChange={(value: number) => { this.onPreviewChange(selectValue.key, value) }}
                                            requestIMEEdit={(uiControl, value) => { }}

                                        />
                                    </div>

                                )
                            }
                            {
                                typeValue.value === 1 && (
                                    <div style={{ flex: "0 0 auto" }}>
                                        <PluginControl instanceId={this.props.instanceId} uiControl={SplitMixControl} value={mixValue.value}
                                            onChange={(value: number) => { this.onValueChanged(mixValue.key, value) }}
                                            onPreviewChange={(value: number) => { this.onPreviewChange(mixValue.key, value) }}
                                            requestIMEEdit={(uiControl, value) => { }}

                                        />
                                    </div>

                                )
                            }
                            {
                                typeValue.value === 2 && (
                                    <div style={{ flex: "0 0 auto" }}>
                                        <PluginControl instanceId={this.props.instanceId} uiControl={SplitPanLeftControl} value={panLValue.value}
                                            onChange={(value: number) => { this.onValueChanged(panLValue.key, value) }}
                                            onPreviewChange={(value: number) => { this.onPreviewChange(panLValue.key, value) }}
                                            requestIMEEdit={(uiControl, value) => { }}

                                        />
                                    </div>
                                )
                            }
                            {
                                typeValue.value === 2 && (
                                    <div style={{ flex: "0 0 auto" }}>
                                        <PluginControl instanceId={this.props.instanceId} uiControl={SplitVolLeftControl} value={volumeLValue.value}
                                            onChange={(value: number) => { this.onValueChanged(volumeLValue.key, value) }}
                                            onPreviewChange={(value: number) => { this.onPreviewChange(volumeLValue.key, value) }}
                                            requestIMEEdit={(uiControl, value) => { }}

                                        />
                                    </div>
                                )
                            }
                            {
                                typeValue.value === 2 && (
                                    <div style={{ flex: "0 0 auto" }}>
                                        <PluginControl instanceId={this.props.instanceId}  uiControl={SplitPanRightControl} value={panRValue.value}
                                            onChange={(value: number) => { this.onValueChanged(panRValue.key, value) }}
                                            onPreviewChange={(value: number) => { this.onPreviewChange(panRValue.key, value) }}
                                            requestIMEEdit={(uiControl, value) => { }}

                                        />
                                    </div>
                                )
                            }
                            {
                                typeValue.value === 2 && (
                                    <div style={{ flex: "0 0 auto" }}>
                                        <PluginControl instanceId={this.props.instanceId} uiControl={SplitVolRightControl} value={volumeRValue.value}
                                            onChange={(value: number) => { this.onValueChanged(volumeRValue.key, value) }}
                                            onPreviewChange={(value: number) => { this.onPreviewChange(volumeRValue.key, value) }}
                                            requestIMEEdit={(uiControl, value) => { }}

                                        />
                                    </div>
                                )
                            }


                        </div>
                    </div >
                );

            }

        }
    );

export default SplitControlView;

