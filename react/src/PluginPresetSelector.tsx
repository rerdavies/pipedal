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
import { PiPedalModel, PiPedalModelFactory } from './PiPedalModel';
import { Theme, withStyles, WithStyles, createStyles } from '@material-ui/core/styles';
import PresetDialog from './PresetDialog';
import Menu from '@material-ui/core/Menu';
import MenuItem from '@material-ui/core/MenuItem';
import Fade from '@material-ui/core/Fade';
import RenameDialog from './RenameDialog'
import PluginPreset from './PluginPreset';


interface PluginPresetSelectorProps extends WithStyles<typeof styles> {
    pluginUri?: string;

    onSelectPreset: (presetName: string) => void;
};


interface PluginPresetSelectorState {
    presets: PluginPreset[] | null;
    enabled: boolean;
    showPresetsDialog: boolean;
    showEditPresetsDialog: boolean;
    presetsMenuAnchorRef: HTMLElement | null;

    renameDialogOpen: boolean;
    renameDialogDefaultName: string;
    renameDialogActionName: string;
    renameDialogOnOk?: (name: string) => void;

};


const styles = (theme: Theme) => createStyles({
});


const PluginPresetSelector =
    withStyles(styles, { withTheme: true })(
        class extends Component<PluginPresetSelectorProps, PluginPresetSelectorState> {

            model: PiPedalModel;

            constructor(props: PluginPresetSelectorProps) {
                super(props);
                this.model = PiPedalModelFactory.getInstance();
                this.state = {
                    presets: null,
                    enabled: false,
                    showPresetsDialog: false,
                    showEditPresetsDialog: false,
                    presetsMenuAnchorRef: null,
                    renameDialogOpen: false,
                    renameDialogDefaultName: "",
                    renameDialogActionName: "",
                    renameDialogOnOk: undefined

                };
                this.handleDialogClose = this.handleDialogClose.bind(this);
                this.handlePresetsMenuClose = this.handlePresetsMenuClose.bind(this);
            }

            handleLoadPluginPreset(presetName: string)
            {
                this.handlePresetsMenuClose();
                this.props.onSelectPreset(presetName);
            }

            handlePresetMenuClick(e: SyntheticEvent): void {
                this.setState({ presetsMenuAnchorRef: (e.currentTarget as HTMLElement) });
            }
            handlePresetsMenuClose(): void {
                this.setState({ presetsMenuAnchorRef: null });
            }

            handlePresetsMenuSave(e: SyntheticEvent): void {
                this.handlePresetsMenuClose();
                e.stopPropagation();

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

            currentUri?: string = "";
            componentDidUpdate()
            {
                if (this.currentUri !== this.props.pluginUri)
                {
                    this.currentUri = this.props.pluginUri;
                    if (!this.props.pluginUri)
                    {
                        this.setState({presets: null});
                    } else {
                        this.setState({presets: null});
                        let captureUri = this.currentUri;
                        this.model.getPluginPresets(this.props.pluginUri)
                        .then((presets: PluginPreset[]) => {
                            if (captureUri === this.currentUri)
                            {
                                this.setState({presets: presets});
                            }
                        })
                        .catch(error => { 
                            if (captureUri === this.currentUri)
                            {
                                this.setState({presets: null});
                            }
                        });
                    }
                }
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

            handleSave() {
                this.model.saveCurrentPreset();
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
                //let classes = this.props.classes;
                if ((!this.props.pluginUri) || (!this.state.presets) ||this.state.presets.length === 0)
                {
                    return (<div/>);
                }
                return (
                    <div >
                        <IconButton onClick={(e)=> this.handlePresetMenuClick(e)}>
                            <img src="img/ic_pluginpreset.svg" style={{ width: 24, height: 24,opacity: 0.6 }} alt="Plugin Presets" />
                        </IconButton>
                        <Menu
                            id="edit-plugin-presets-menu"
                            anchorEl={this.state.presetsMenuAnchorRef}
                            open={Boolean(this.state.presetsMenuAnchorRef)}
                            onClose={() => this.handlePresetsMenuClose()}
                            TransitionComponent={Fade}
                            MenuListProps={
                                {
                                    style: {minWidth: 180}
                                }
                            }
                            
                        >
                            <Typography variant="caption" color="textSecondary" style={{marginLeft: 16,marginTop:4,marginBottom: 4}}>Plugin presets</Typography>
                            {
                                this.state.presets.map((preset) => {
                                    return (<MenuItem onClick={(e) => this.handleLoadPluginPreset(preset.presetUri)}>{preset.name}</MenuItem>);
                                })
                            }
                            {/*
                            <Divider />
                            <MenuItem onClick={(e) => this.handlePresetsMenuSave(e)}>Save plugin preset</MenuItem>
                            <MenuItem onClick={(e) => this.handlePresetsMenuSaveAs(e)}>Save plugin preset as...</MenuItem>
                            <MenuItem onClick={(e) => this.handlePresetsMenuRename(e)}>Rename...</MenuItem>
                            <MenuItem onClick={(e) => this.handleMenuEditPresets()}>Manage presets...</MenuItem>
                        */}

                        </Menu>
                        <PresetDialog show={this.state.showPresetsDialog} isEditDialog={this.state.showEditPresetsDialog} onDialogClose={() => this.handleDialogClose()} />
                        <RenameDialog open={this.state.renameDialogOpen}
                            defaultName={this.state.renameDialogDefaultName}
                            acceptActionName={this.state.renameDialogActionName}
                            onClose={() => this.handleRenameDialogClose()}
                            onOk={(name: string) => this.handleRenameDialogOk(name)} />
                    </div>
                )

            }
        });

export default PluginPresetSelector;
