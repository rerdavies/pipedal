// Copyright (c) 2024 Robin Davies
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


import { Theme } from '@mui/material/styles';
import SaveIconOutline from '@mui/icons-material/Save';
import WithStyles from './WithStyles';
import {createStyles} from './WithStyles';

import { withStyles } from "tss-react/mui";
import { PiPedalModel, PiPedalModelFactory, PresetIndex } from './PiPedalModel';
import ResizeResponsiveComponent from './ResizeResponsiveComponent';
import ArrowBackIcon from '@mui/icons-material/ArrowBack';
import IconButton from '@mui/material/IconButton';
import AppBar from '@mui/material/AppBar';
import SnapshotPanel from './SnapshotPanel';
import ArrowBackIosIcon from '@mui/icons-material/ArrowBackIos';
import ArrowForwardIosIcon from '@mui/icons-material/ArrowForwardIos';
//import ArrowLeftOutlined from '@mui/icons-material/ArrowLeft';
//import ArrowRightOutlined from '@mui/icons-material/ArrowRight';
import Select from '@mui/material/Select';
import MenuItem from '@mui/material/MenuItem';
import { isDarkMode } from './DarkMode';
import { BankIndex } from './Banks';
import SnapshotEditor from './SnapshotEditor';
import { Snapshot } from './Pedalboard';
import JackStatusView from './JackStatusView';
import {IDialogStackable, popDialogStack, pushDialogStack} from './DialogStack';
import { css } from '@emotion/react';


const selectColor = isDarkMode() ? "#888" : "#FFFFFF";

const styles = (theme: Theme) => createStyles({
    frame: css({
        position: "absolute", display: "flex", flexDirection: "column", flexWrap: "nowrap",
        justifyContent: "flex-start", left: "0px", top: "0px", bottom: "0px", right: "0px", overflow: "hidden"
    }),
    select: css({ // fu fu fu.Overrides for white selector on dark background.
        '&:before': {
            borderColor: selectColor,
        },
        '&:after': {
            borderColor: selectColor,
        },
        '&:hover:not(.Mui-disabled):before': {
            borderColor: selectColor,
        }
    }),
    select_icon: css({
        fill: selectColor,
    }),
});


interface PerformanceViewProps extends WithStyles<typeof styles> {
    open: boolean,
    onClose: () => void,
}

interface PerformanceViewState {
    wrapSelects: boolean
    presets: PresetIndex;
    banks: BankIndex;
    showSnapshotEditor: boolean;
    snapshotEditorIndex: number;
    presetModified: boolean;
}


export const PerformanceView =
    withStyles( 
        class extends ResizeResponsiveComponent<PerformanceViewProps, PerformanceViewState> implements IDialogStackable {
            model: PiPedalModel;

            constructor(props: PerformanceViewProps) {
                super(props);
                this.model = PiPedalModelFactory.getInstance();
                // let pedalboard = this.model.pedalboard.get();
                // let selectedPedal = pedalboard.getFirstSelectableItem();

                this.state = {
                    presets: this.model.presets.get(),
                    banks: this.model.banks.get(),
                    wrapSelects: false,
                    showSnapshotEditor: false,
                    snapshotEditorIndex: 0,
                    presetModified: this.model.presetChanged.get()
                };
                this.onPresetsChanged = this.onPresetsChanged.bind(this);
                this.onBanksChanged = this.onBanksChanged.bind(this);
                this.onPresetChangedChanged = this.onPresetChangedChanged.bind(this);

            }

            getTag() { return "performView"}
            isOpen() { return this.props.open }
            onDialogStackClose() {
                this.props.onClose();
            }


            private hasHooks: boolean = false;

            updateHooks() : void {
                let wantHooks = this.mounted && this.props.open;

                if (wantHooks !== this.hasHooks)
                {
                    this.hasHooks = wantHooks;
        
                    if (this.hasHooks)
                    {

                        pushDialogStack(this);
                    } else {
                        popDialogStack(this);
                    }
                }
            }
        
            onWindowSizeChanged(width: number, height: number): void {
                this.setState({ wrapSelects: width < 700 });
            }

            onPresetsChanged(newValue: PresetIndex) {
                this.setState({ presets: this.model.presets.get() })
            }
            onBanksChanged(newValue: BankIndex) {
                this.setState({ banks: this.model.banks.get() })
            }
            onPresetChangedChanged(newValue: boolean) {
                this.setState({ presetModified:newValue});
            }

            private mounted: boolean = false;
            componentDidMount(): void {
                super.componentDidMount();
                this.mounted = true;
                this.model.presets.addOnChangedHandler(this.onPresetsChanged);
                this.model.banks.addOnChangedHandler(this.onBanksChanged);
                this.model.presetChanged.addOnChangedHandler(this.onPresetChangedChanged)
                this.setState({
                    presets: this.model.presets.get(),
                    banks: this.model.banks.get(),
                    presetModified: this.model.presetChanged.get()

                });

                this.updateHooks();

            }



            componentWillUnmount(): void {
                this.model.presetChanged.removeOnChangedHandler(this.onPresetChangedChanged)
                this.model.presets.removeOnChangedHandler(this.onPresetsChanged);
                this.model.banks.removeOnChangedHandler(this.onBanksChanged);

                this.mounted = false;
                this.updateHooks();

                super.componentWillUnmount();

            }
            handlePresetSelectClose(event: any): void {
                let value = event.currentTarget.getAttribute("data-value");
                if (value && value.length > 0) {
                    this.model.loadPreset(parseInt(value));
                }
            }

            handleBankSelectClose(event: any): void {
                let value = event.currentTarget.getAttribute("data-value");
                if (value && value.length > 0) {
                    this.model.openBank(parseInt(value));
                }
            }
            handleOnEdit(index: number): boolean {
                // load it so we can edit it.
                this.model.selectSnapshot(index);
                this.setState({
                    showSnapshotEditor: true,
                    snapshotEditorIndex: index
                });
                return true;
            }
            handleSnapshotEditOk(index: number, name: string, color: string,newSnapshots: (Snapshot|null)[])
            {
                // results have been sent to the server. we would perform a  short-cut commit to our local state, to avoid laggy display updates.
                // but we don't actually have that in our state.

            }

            handlePreviousBank()
            {
                this.model.previousBank();
            }

            handleNextBank()
            {
                this.model.nextBank();
            }
            handlePreviousPreset()
            {
                this.model.previousPreset();
            }
            handleNextPreset()
            {
                this.model.nextPreset();
            }

            render() {
                const classes = withStyles.getClasses(this.props);
                let wrapSelects = this.state.wrapSelects;
                let presets = this.state.presets;
                let banks = this.state.banks;
                return (
                    <div className={classes.frame} style={{ overflow: "clip", height: "100%" }} >
                        <div className={classes.frame} style={{ overflow: "clip", height: "100%" }} >
                            <AppBar id="select-plugin-dialog-title"
                                style={{
                                    position: "static", flex: "0 0 auto", height: wrapSelects ? 108 : 58,
                                    display: this.state.showSnapshotEditor ? "none" : undefined
                                }}
                            >
                                <div style={{
                                    display: "flex", flexFlow: "row nowrap", alignContent: "center",
                                    paddingTop: 3, paddingBottom: 3, paddingRight: 8, paddingLeft: 8,
                                    alignItems: "start"
                                }}>
                                    <IconButton aria-label="menu" color="inherit"
                                        onClick={() => {  this.props.onClose(); }} 
                                        style={{ 
                                            position: "relative",top: 3,
                                            flex: "0 0 auto" }} >
                                        <ArrowBackIcon />
                                    </IconButton>

                                    <div style={{
                                        flex: "1 1 1px", display: "flex",
                                        flexFlow: wrapSelects ? "column nowrap" : "row nowrap",
                                        alignItems: wrapSelects ? "stretch" : undefined,
                                        justifyContent: wrapSelects ? undefined : "space-evenly",
                                        marginLeft: 8,
                                        gap: 8

                                    }}>
                                        {/********* BANKS *******************/}
                                        <div style={{ flex: "1 1 1px", display: "flex", flexFlow: "row nowrap", maxWidth: 500 }}>
                                            <IconButton
                                                aria-label="previous-bank"
                                                onClick={() => { this.handlePreviousBank(); }}
                                                size="medium"
                                                color="inherit"
                                                style={{
                                                    borderTopRightRadius: "3px", borderBottomRightRadius: "3px", marginRight: 4
                                                }}
                                            >
                                                <ArrowBackIosIcon style={{ opacity: 0.75 }} />
                                            </IconButton>

                                            <Select variant="standard"
                                                className={classes.select}
                                                style={{ flex: "1 1 1px", width: "100%", position: "relative", top: 0, color: "#FFFFFF" }} disabled={false}
                                                displayEmpty
                                                onClose={(e) => this.handleBankSelectClose(e)}
                                                value={banks.selectedBank === 0 ? undefined : banks.selectedBank}
                                                inputProps={{
                                                    classes: { icon: classes.select_icon },
                                                    'aria-label': "Select preset"
                                                }}
                                            >
                                                {
                                                    banks.entries.map((entry) => {
                                                        return (
                                                            <MenuItem key={entry.instanceId} value={entry.instanceId} >
                                                                {entry.name}
                                                            </MenuItem>
                                                        );
                                                    })
                                                }
                                            </Select>

                                            <IconButton
                                                aria-label="next-bank"
                                                onClick={() => { this.handleNextBank(); }}
                                                color="inherit"
                                                style={{ borderTopLeftRadius: "3px", borderBottomLeftRadius: "3px", marginLeft: 4 }}
                                            >
                                                <ArrowForwardIosIcon style={{ opacity: 0.75 }} />
                                                
                                            </IconButton>

                                            {/** spacer */}
                                            <IconButton
                                                aria-label="next-bank"
                                                onClick={() => { this.handleNextBank(); }}
                                                size="medium"
                                                color="inherit"
                                                style={{visibility: "hidden"}}
                                            >
                                                <ArrowForwardIosIcon style={{ opacity: 0.75 }} />
                                                
                                            </IconButton>

                                            

                                        </div>
                                        {/********* PRESETS *******************/}
                                        <div style={{ flex: "1 1 1px", display: "flex", flexFlow: "row nowrap", maxWidth: 500 }}>
                                            <IconButton
                                                aria-label="previous-presest"
                                                onClick={() => { this.handlePreviousPreset(); }}
                                                color="inherit"
                                                style={{ borderTopRightRadius: "3px", borderBottomRightRadius: "3px", marginRight: 4 }}
                                            >
                                                <ArrowBackIosIcon style={{ opacity: 0.75 }} />
                                            </IconButton>
                                            <Select variant="standard"
                                                className={classes.select}
                                                style={{
                                                    flex: "1 1 1px", width: "100%", color: "#FFFFFF"
                                                }} disabled={false}
                                                displayEmpty
                                                onClose={(e) => this.handlePresetSelectClose(e)}
                                                value={presets.selectedInstanceId === 0 ? '' : presets.selectedInstanceId}

                                                inputProps={{
                                                    classes: { icon: classes.select_icon },
                                                    'aria-label': "Select preset"
                                                }
                                                }
                                            >
                                                {
                                                    presets.presets.map((preset) => {
                                                        let name = preset.name;
                                                        if (this.state.presets.selectedInstanceId === preset.instanceId && this.state.presetModified)
                                                        {
                                                            name += "*";
                                                        }
                                                        return (
                                                            <MenuItem key={preset.instanceId} value={preset.instanceId} >
                                                                {name}
                                                            </MenuItem>
                                                        );
                                                    })
                                                }
                                            </Select>

                                            <IconButton
                                                aria-label="next-preset"
                                                onClick={() => { this.handleNextPreset(); }}
                                                color="inherit"
                                                style={{ borderTopLeftRadius: "3px", borderBottomLeftRadius: "3px", marginLeft: 4 }}
                                            >
                                                <ArrowForwardIosIcon style={{ opacity: 0.75 }} />
                                            </IconButton>

                                            <IconButton
                                                aria-label="save-preset"
                                                onClick={() => { this.model.saveCurrentPreset(); }}
                                                size="medium"
                                                color="inherit"
                                                style={{flexShrink: 0,visibility: this.state.presetModified? "visible": "hidden"}}
                                            >
                                                <SaveIconOutline style={{ opacity: 0.75 }} color="inherit" />                                                
                                            </IconButton>

                                        </div>
                                    </div>
                                </div>
                            </AppBar >
                            <div style={{ flex: "1 0 auto", display: "flex", marginTop: 16,marginBottom: 20 }}>
                                <SnapshotPanel onEdit={(index) => { return this.handleOnEdit(index); }} />
                            </div>

                                                
                            {!this.state.showSnapshotEditor && (
                                <JackStatusView />)
                            }
                        </div>
                        {this.state.showSnapshotEditor && (
                            <SnapshotEditor snapshotIndex={this.state.snapshotEditorIndex} 
                                onClose={() => { 
                                    this.setState({ showSnapshotEditor: false });
                                }}
                                onOk={(index,name,color,newSnapshots)=>{
                                    this.setState({ showSnapshotEditor: false});
                                    this.handleSnapshotEditOk(index,name,color,newSnapshots);
                                }}
                             />
                        )}
                    </div >
                )
            }
        },
        styles
    );

