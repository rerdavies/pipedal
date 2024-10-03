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
import { WithStyles } from '@mui/styles';
import createStyles from '@mui/styles/createStyles';
import withStyles from '@mui/styles/withStyles';
import { PiPedalModel, PiPedalModelFactory, PresetIndex } from './PiPedalModel';
import ResizeResponsiveComponent from './ResizeResponsiveComponent';
import ArrowBackIcon from '@mui/icons-material/ArrowBack';
import IconButton from '@mui/material/IconButton';
import AppBar from '@mui/material/AppBar';
import SnapshotPanel from './SnapshotPanel';
import ArrowLeftOutlined from '@mui/icons-material/ArrowLeft';
import ArrowRightOutlined from '@mui/icons-material/ArrowRight';
import Select from '@mui/material/Select';
import MenuItem from '@mui/material/MenuItem';
import { isDarkMode } from './DarkMode';
import { BankIndex } from './Banks';
import SnapshotEditor from './SnapshotEditor';
import { Snapshot } from './Pedalboard';


const selectColor = isDarkMode() ? "#888" : "#FFFFFF";

const styles = (theme: Theme) => createStyles({
    frame: {
        position: "absolute", display: "flex", flexDirection: "column", flexWrap: "nowrap",
        justifyContent: "flex-start", left: "0px", top: "0px", bottom: "0px", right: "0px", overflow: "hidden"
    },
    select: { // fu fu fu.Overrides for white selector on dark background.
        '&:before': {
            borderColor: selectColor,
        },
        '&:after': {
            borderColor: selectColor,
        },
        '&:hover:not(.Mui-disabled):before': {
            borderColor: selectColor,
        }
    },
    select_icon: {
        fill: selectColor,
    },
});


interface PerformanceViewProps extends WithStyles<typeof styles> {
    onClose: () => void,
    theme: Theme
}

interface PerformanceViewState {
    wrapSelects: boolean
    presets: PresetIndex;
    banks: BankIndex;
    showSnapshotEditor: boolean;
    snapshotEditorIndex: number;
}


export const PerformanceView =
    withStyles(styles, { withTheme: true })(
        class extends ResizeResponsiveComponent<PerformanceViewProps, PerformanceViewState> {
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
                    snapshotEditorIndex: 0
                };
                this.onPresetsChanged = this.onPresetsChanged.bind(this);
                this.onBanksChanged = this.onBanksChanged.bind(this);
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
            componentDidMount(): void {
                super.componentDidMount();
                this.model.presets.addOnChangedHandler(this.onPresetsChanged);
                this.model.banks.addOnChangedHandler(this.onBanksChanged);
                this.setState({
                    presets: this.model.presets.get(),
                    banks: this.model.banks.get()
                });
            }
            componentWillUnmount(): void {
                this.model.presets.removeOnChangedHandler(this.onPresetsChanged);
                this.model.banks.removeOnChangedHandler(this.onBanksChanged);
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
                let classes = this.props.classes;
                let wrapSelects = this.state.wrapSelects;
                let presets = this.state.presets;
                let banks = this.state.banks;
                return (
                    <div className={this.props.classes.frame} style={{ overflow: "clip", height: "100%" }} >
                        <div className={this.props.classes.frame} style={{ overflow: "clip", height: "100%" }} >
                            <AppBar id="select-plugin-dialog-title"
                                style={{
                                    position: "static", flex: "0 0 auto", height: wrapSelects ? 104 : 54,
                                    display: this.state.showSnapshotEditor ? "none" : undefined
                                }}
                            >
                                <div style={{
                                    display: "flex", flexFlow: "row nowrap", alignContent: "center",
                                    paddingTop: 3, paddingBottom: 3, paddingRight: 8, paddingLeft: 8,
                                    alignItems: "start"
                                }}>
                                    <IconButton aria-label="menu" color="inherit"
                                        onClick={() => { this.props.onClose(); }} style={{ flex: "0 0 auto" }} >
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
                                                color="inherit"
                                                style={{
                                                    borderTopRightRadius: "3px", borderBottomRightRadius: "3px", marginRight: 4
                                                }}
                                            >
                                                <ArrowLeftOutlined style={{ opacity: 0.75 }} />
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
                                                <ArrowRightOutlined style={{ opacity: 0.75 }} />
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
                                                <ArrowLeftOutlined style={{ opacity: 0.75 }} />
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
                                                        return (
                                                            <MenuItem key={preset.instanceId} value={preset.instanceId} >
                                                                {preset.name}
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
                                                <ArrowRightOutlined style={{ opacity: 0.75 }} />
                                            </IconButton>

                                        </div>
                                    </div>
                                </div>
                            </AppBar >
                            <div style={{ flex: "1 0 auto", display: "flex", marginTop: 16 }}>
                                <SnapshotPanel onEdit={(index) => { return this.handleOnEdit(index); }} />
                            </div>
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
        }
    );

