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
import IconButton from '@mui/material/IconButton';
import { PiPedalModel, PiPedalModelFactory, PluginPresetsChangedHandle } from './PiPedalModel';
import { Theme } from '@mui/material/styles';
import { WithStyles } from '@mui/styles';
import withStyles from '@mui/styles/withStyles';
import createStyles from '@mui/styles/createStyles';
import PluginPresetsDialog from './PluginPresetsDialog';
import Menu from '@mui/material/Menu';
import MenuItem from '@mui/material/MenuItem';
import Fade from '@mui/material/Fade';
import RenameDialog from './RenameDialog'
import {PluginUiPresets} from './PluginPreset';

import Divider from "@mui/material/Divider";


interface PluginPresetSelectorProps extends WithStyles<typeof styles> {
    pluginUri?: string;
    instanceId: number;
}


interface PluginPresetSelectorState {
    presets : PluginUiPresets;
    enabled: boolean;
    presetChanged: boolean;
    showPresetsDialog: boolean;
    showEditPresetsDialog: boolean;
    presetsMenuAnchorRef: HTMLElement | null;

    renameDialogOpen: boolean;
    renameDialogDefaultName: string;
    renameDialogActionName: string;
    renameDialogOnOk?: (name: string) => void;

    saveAsName: string;
}


const styles = (theme: Theme) => createStyles({
    itemIcon: {
        width: 24, height: 24, marginRight: "4px", opacity: 0.6
    }

});


const PluginPresetSelector =
    withStyles(styles, { withTheme: true })(
        class extends Component<PluginPresetSelectorProps, PluginPresetSelectorState> {

            model: PiPedalModel;

            constructor(props: PluginPresetSelectorProps) {
                super(props);
                this.model = PiPedalModelFactory.getInstance();
                this.state = {
                    presets: new PluginUiPresets(),
                    enabled: false,
                    presetChanged: false,
                    showPresetsDialog: false,
                    showEditPresetsDialog: false,
                    presetsMenuAnchorRef: null,
                    renameDialogOpen: false,
                    renameDialogDefaultName: "",
                    renameDialogActionName: "",
                    renameDialogOnOk: undefined,
                    saveAsName: ""

                };
                this.handleDialogClose = this.handleDialogClose.bind(this);
                this.handlePresetsMenuClose = this.handlePresetsMenuClose.bind(this);
            }

            handleLoadPluginPreset(instanceId: number)
            {
                this.handlePresetsMenuClose();
                this.model.loadPluginPreset(this.props.instanceId,instanceId);
            }

            handlePresetMenuClick(e: SyntheticEvent): void {
                this.setState({ presetsMenuAnchorRef: (e.currentTarget as HTMLElement) });
            }
            handlePresetsMenuClose(): void {
                this.setState({ presetsMenuAnchorRef: null });
            }

            handlePluginPresetsMenuSaveAs(e: SyntheticEvent): void {
                this.handlePresetsMenuClose();
                e.stopPropagation();

                let name = this.state.saveAsName;

                this.renameDialogOpen(name, "Save As")
                    .then((newName) => {
                        this.setState({saveAsName: newName});
                        return this.model.saveCurrentPluginPresetAs(this.props.instanceId,newName);
                    })
                    .then((newInstanceId) => {
                        // s'fine. dealt with by updates, but we do need error handling.
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

            loadPresets():void {
                if (!this.currentUri) return;
                let captureUri: string = this.currentUri;
                this.model.getPluginPresets(captureUri)
                .then((presets: PluginUiPresets) => {
                    if (captureUri === this.currentUri)
                    {
                        this.setState({presets: presets});
                    }
                })
                .catch(error => { 
                    if (captureUri === this.currentUri)
                    {
                        this.setState({presets: new PluginUiPresets()});
                    }
                });
            }
            onPluginPresetsChanged(pluginUri: string): void {
                if (pluginUri === this.props.pluginUri)
                {
                    this.loadPresets();

                }
            }
            _pluginPresetsChangedHandle?: PluginPresetsChangedHandle;

            componentDidMount()
            {
                this._pluginPresetsChangedHandle = this.model.addPluginPresetsChangedListener(
                    (pluginUri: string) => {
                        this.onPluginPresetsChanged(pluginUri);
                    }
                );
            }
            componentWillUnmount()
            {
                if (this._pluginPresetsChangedHandle)
                {
                    this.model.removePluginPresetsChangedListener(this._pluginPresetsChangedHandle);
                    this._pluginPresetsChangedHandle = undefined;
                }
            }

            currentUri?: string = "";
            componentDidUpdate()
            {
                if (this.currentUri !== this.props.pluginUri)
                {
                    this.currentUri = this.props.pluginUri;
                    if (!this.props.pluginUri)
                    {
                        this.setState({presets: new PluginUiPresets()});
                    } else {
                        this.loadPresets();
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


            handleMenuEditPluginPresets(): void {
                this.handlePresetsMenuClose();
                this.showEditPluginPresetsDialog(true);
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
            showEditPluginPresetsDialog(show: boolean) {
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
                let classes = this.props.classes;

                //let classes = this.props.classes;
                //let classes = this.props.classes;
                if ((!this.props.pluginUri))
                {
                    return (<div/>);
                }
                return (
                    <div >
                        <IconButton onClick={(e)=> this.handlePresetMenuClick(e)} size="large">
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
                            <MenuItem onClick={(e) => this.handlePluginPresetsMenuSaveAs(e)}>Save plugin preset...</MenuItem>
                            <MenuItem onClick={(e) => this.handleMenuEditPluginPresets()}>Manage plugin presets...</MenuItem>

                            { this.state.presets.presets.length !== 0 && 
                                (<Divider/>)
                            }
                            {
                                this.state.presets.presets.map((preset) => {
                                    return (<MenuItem key={preset.instanceId}
                                        onClick={(e) => this.handleLoadPluginPreset(preset.instanceId)}
                                        ><img src="img/ic_pluginpreset2.svg" className={classes.itemIcon} alt="" />
                                        {preset.label}</MenuItem>);
                                })
                            }
                        </Menu>
                        <PluginPresetsDialog 
                            instanceId={this.props.instanceId}
                            presets={this.state.presets}
                            show={this.state.showPresetsDialog}
                            isEditDialog={this.state.showEditPresetsDialog} 
                            onDialogClose={() => this.handleDialogClose()} />
                        <RenameDialog open={this.state.renameDialogOpen}
                            defaultName={this.state.renameDialogDefaultName}
                            acceptActionName={this.state.renameDialogActionName}
                            onClose={() => this.handleRenameDialogClose()}
                            onOk={(name: string) => this.handleRenameDialogOk(name)} />
                    </div>
                );

            }
        });

export default PluginPresetSelector;
