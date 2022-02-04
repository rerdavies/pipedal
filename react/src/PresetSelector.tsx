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

import { SyntheticEvent, Component } from 'react';
import IconButton from '@material-ui/core/IconButton';
import Typography from '@material-ui/core/Typography';
import { PiPedalModel, PiPedalModelFactory, PresetIndex } from './PiPedalModel';
import SaveIconOutline from '@material-ui/icons/Save';
import ArrowDropDownIcon from "@material-ui/icons/ArrowDropDown";
import ButtonBase from "@material-ui/core/ButtonBase";
import MoreVertIcon from '@material-ui/icons/MoreVert';
import { Theme, withStyles, WithStyles, createStyles } from '@material-ui/core/styles';
import PresetDialog from './PresetDialog';
import Menu from '@material-ui/core/Menu';
import MenuItem from '@material-ui/core/MenuItem';
import Fade from '@material-ui/core/Fade';
import Divider from '@material-ui/core/Divider';
import RenameDialog from './RenameDialog'
import Select from '@material-ui/core/Select';
import UploadDialog from './UploadDialog';


interface PresetSelectorProps extends WithStyles<typeof styles> {

};


interface PresetSelectorState {
    presets: PresetIndex;
    enabled: boolean;
    showPresetsDialog: boolean;
    showEditPresetsDialog: boolean;
    presetsMenuAnchorRef: HTMLElement | null;

    renameDialogOpen: boolean;
    renameDialogDefaultName: string;
    renameDialogActionName: string;
    renameDialogOnOk?: (name: string) => void;
    openUploadDialog: boolean;

};


const selectColor = "white";

const styles = (theme: Theme) => createStyles({
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
    icon: {
        fill: selectColor,
    },
});


const PresetSelector =
    withStyles(styles, { withTheme: true })(
        class extends Component<PresetSelectorProps, PresetSelectorState> {

            model: PiPedalModel;

            constructor(props: PresetSelectorProps) {
                super(props);
                this.model = PiPedalModelFactory.getInstance();
                this.state = {
                    presets:
                        this.model.presets.get(),
                    enabled: false,
                    showPresetsDialog: false,
                    showEditPresetsDialog: false,
                    presetsMenuAnchorRef: null,
                    renameDialogOpen: false,
                    renameDialogDefaultName: "",
                    renameDialogActionName: "",
                    renameDialogOnOk: undefined,
                    openUploadDialog: false


                };
                this.handlePresetsChanged = this.handlePresetsChanged.bind(this);
                this.handleDialogClose = this.handleDialogClose.bind(this);
                this.handlePresetsMenuClose = this.handlePresetsMenuClose.bind(this);
            }


            handlePresetMenuClick(event: SyntheticEvent): void {
                this.setState({ presetsMenuAnchorRef: (event.currentTarget as HTMLElement) });
            }
            handlePresetsMenuClose(): void {
                this.setState({ presetsMenuAnchorRef: null });
            }

            handleDownloadPreset(e: SyntheticEvent) {
                this.handlePresetsMenuClose();
                e.preventDefault();

                this.model.download("downloadPreset", this.model.presets.get().selectedInstanceId);
            }
            handleUploadPreset(e: SyntheticEvent) {
                this.handlePresetsMenuClose();
                e.preventDefault();

                this.setState({ openUploadDialog: true });

            }

            handlePresetsMenuSave(e: SyntheticEvent): void {
                this.handlePresetsMenuClose();
                e.preventDefault();

                this.model.saveCurrentPreset();
            }
            handlePresetsMenuSaveAs(e: SyntheticEvent): void {
                this.handlePresetsMenuClose();
                e.stopPropagation();

                let currentPresets = this.model.presets.get();
                let item = currentPresets.getItem(currentPresets.selectedInstanceId);
                if (item == null) return;
                let name = item.name;

                this.renameDialogOpen(name, "Save As")
                    .then((newName) => {
                        return this.model.saveCurrentPresetAs(newName);
                    })
                    .then((newInstanceId) => {
                        // s'fine. dealt with by updates, but we do need error handling.
                    })
                    .catch((error) => {
                        this.showError(error);
                    })
                    ;


            }
            handlePresetsMenuRename(e: SyntheticEvent): void {
                this.handlePresetsMenuClose();
                e.stopPropagation();

                let currentPresets = this.model.presets.get();
                let item = currentPresets.getItem(currentPresets.selectedInstanceId);
                if (item == null) return;
                let name = item.name;

                this.renameDialogOpen(name, "Rename")
                    .then((newName) => {
                        if (newName === name) return;
                        return this.model.renamePresetItem(this.model.presets.get().selectedInstanceId, newName);
                    })
                    .catch((error) => {
                        this.showError(error);
                    })
                    ;
            }

            showError(error: string) {
                this.model.showAlert(error);
            }
            renameDialogOpen(defaultText: string, acceptButtonText: string): Promise<string> {
                let result = new Promise<string>(
                    (resolve, reject) => {
                        this.setState(
                            {
                                renameDialogOpen: true,
                                renameDialogDefaultName: defaultText,
                                renameDialogActionName: acceptButtonText,
                                renameDialogOnOk: (name) => {
                                    resolve(name);
                                }

                            }
                        );

                    }
                );
                return result;
            }

            handleRenameDialogClose(): void {
                this.setState({
                    renameDialogOpen: false,
                    renameDialogOnOk: undefined
                });
            }

            handleRenameDialogOk(name: string): void {
                let renameDialogOnOk = this.state.renameDialogOnOk;
                this.handleRenameDialogClose();

                if (renameDialogOnOk) {
                    renameDialogOnOk(name);
                }

            }


            handleMenuEditPresets(): void {
                this.handlePresetsMenuClose();
                this.showEditPresetsDialog(true);
            }

            updatePresetState() {
                let presets = this.model.presets.get();
                let enabled = presets.presets.length > 0 && presets.selectedInstanceId !== -1;
                this.setState(
                    {
                        presets: presets,
                        enabled: enabled,
                    }
                );
            }
            handleSave() {
                this.model.saveCurrentPreset();
            }
            handlePresetsChanged() {
                this.updatePresetState();
            }
            componentDidMount() {
                this.model.presets.addOnChangedHandler(this.handlePresetsChanged);
                this.updatePresetState();
            }
            componentWillUnmount() {
                this.model.presets.removeOnChangedHandler(this.handlePresetsChanged)
            }
            showPresetDialog(show: boolean) {
                this.setState({
                    showPresetsDialog: show,
                    showEditPresetsDialog: false
                });
            }
            showEditPresetsDialog(show: boolean) {
                this.setState({
                    showPresetsDialog: show,
                    showEditPresetsDialog: true
                });
            }

            handleDialogClose(): void {
                this.showPresetDialog(false);
            }
            handleChange(event: any, extra: any): void {
                // misses click on default.
                // this.model.loadPreset(event.target.value as number);
            }
            handleSelectClose(event: any): void {
                let value = event.currentTarget.getAttribute("data-value");
                if (value && value.length > 0) {
                    this.model.loadPreset(parseInt(value));
                }
                //this.model.loadPreset(event.target.value as number);
            }

            render() {
                //let classes = this.props.classes;
                let presets = this.state.presets;
                let classes = this.props.classes;
                return (
                    <div style={{ marginLeft: 12, display: "flex", flexDirection: "row", justifyContent: "left", flexWrap: "nowrap", alignItems: "center", height: "100%", position: "relative" }}>
                        <div style={{ flex: "1 1 auto", minWidth: 60, maxWidth: 300, position: "relative", paddingRight: 12 }} >
                            {true ? (
                                <Select
                                    className={classes.select}
                                    style={{ width: "100%", position: "relative", top: 0, color: "white" }} disabled={!this.state.enabled}
                                    onChange={(e, extra) => this.handleChange(e, extra)}
                                    onClose={(e) => this.handleSelectClose(e)}
                                    displayEmpty
                                    value={presets.selectedInstanceId}
                                    inputProps={{
                                        classes: { icon: classes.icon },
                                        'aria-label': "Select preset"
                                    }
                                    }
                                >
                                    {
                                        presets.presets.map((preset) => {
                                            return (
                                                <MenuItem key={preset.instanceId} value={preset.instanceId} >
                                                    {(presets.presetChanged && preset.instanceId === presets.selectedInstanceId)
                                                        ? (preset.name + "*")
                                                        : (preset.name)
                                                    }
                                                </MenuItem>
                                            );
                                        })
                                    }
                                </Select>
                            ) : (
                                <ButtonBase style={{ width: "100%", textAlign: "left", borderBottom: "1px white solid", position: "relative", top: 2 }} disabled={!this.state.enabled}
                                    onClick={(e) => { this.showPresetDialog(true); }}
                                >
                                    <div style={{ width: "100%", textAlign: "left", display: "flex", flexWrap: "nowrap", flexDirection: "row", overflow: "hidden" }}>
                                        <div style={{ flex: "0 1 auto", paddingLeft: 8, overflow: "hidden" }}>
                                            <Typography noWrap  >
                                                {this.state.presets.getSelectedText()}
                                            </Typography>
                                        </div>
                                        <div style={{ flex: "1 1 1px" }} />
                                        <div style={{ flex: "0 0 auto" }}>
                                            <ArrowDropDownIcon style={{ opacity: 0.75 }} />
                                        </div>

                                    </div>
                                </ButtonBase>
                            )}
                        </div>
                        <div style={{ flex: "0 0 auto" }}>
                            <IconButton color="inherit" style={{ flex: "0 0 auto", opacity: this.state.presets.presetChanged ? 1.0 : 0.0 }}
                                onClick={(e) => { this.handleSave(); }}  >
                                <SaveIconOutline style={{ opacity: 0.75 }} />
                            </IconButton>
                        </div>
                        <div style={{ flex: "0 0 auto" }}>
                            <IconButton color="inherit" style={{ flex: "0 0 auto" }} onClick={(e) => this.handlePresetMenuClick(e)}>
                                <MoreVertIcon style={{ opacity: 0.75 }} />
                            </IconButton>
                            <Menu
                                id="edit-presets-menu"
                                anchorEl={this.state.presetsMenuAnchorRef}
                                open={Boolean(this.state.presetsMenuAnchorRef)}
                                onClose={() => this.handlePresetsMenuClose()}
                                TransitionComponent={Fade}
                            >
                                <MenuItem onClick={(e) => this.handlePresetsMenuSave(e)}>Save preset</MenuItem>
                                <MenuItem onClick={(e) => this.handlePresetsMenuSaveAs(e)}>Save preset as...</MenuItem>
                                <MenuItem onClick={(e) => this.handlePresetsMenuRename(e)}>Rename...</MenuItem>
                                <Divider />
                                <MenuItem onClick={(e) => { this.handleDownloadPreset(e); }} >Download preset</MenuItem>
                                <MenuItem onClick={(e) => { this.handleUploadPreset(e) }}>Upload preset</MenuItem>
                                <Divider />
                                <MenuItem onClick={(e) => this.handleMenuEditPresets()}>Manage presets...</MenuItem>
                            </Menu>
                        </div>
                        <PresetDialog show={this.state.showPresetsDialog} isEditDialog={this.state.showEditPresetsDialog} onDialogClose={() => this.handleDialogClose()} />
                        <RenameDialog open={this.state.renameDialogOpen}
                            defaultName={this.state.renameDialogDefaultName}
                            acceptActionName={this.state.renameDialogActionName}
                            onClose={() => this.handleRenameDialogClose()}
                            onOk={(name: string) => this.handleRenameDialogOk(name)} />
                        <UploadDialog onUploaded={(instanceId) => { this.model.loadPreset(instanceId); }}
                            open={this.state.openUploadDialog}
                            uploadAfter={-1}
                            onClose={() => { this.setState({ openUploadDialog: false }) }} />

                    </div>
                )

            }
        });

export default PresetSelector;