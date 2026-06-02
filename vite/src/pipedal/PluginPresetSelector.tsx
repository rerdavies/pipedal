// Copyright (c) Robin E.R. Davies
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
import IconButtonEx from './IconButtonEx';
import { PiPedalModel, PiPedalModelFactory, PluginPresetsChangedHandle } from './PiPedalModel';
import { Theme } from '@mui/material/styles';
import WithStyles from './WithStyles';
import { withStyles } from "tss-react/mui";
import { createStyles } from './WithStyles';

import PluginPresetsDialog from './PluginPresetsDialog';
import Menu from '@mui/material/Menu';
import MenuItem from '@mui/material/MenuItem';
import Fade from '@mui/material/Fade';
import RenameDialog from './RenameDialog'
import { PluginUiPresets } from './PluginPreset';

import Divider from "@mui/material/Divider";

import PluginPresetsIcon from "./svg/ic_pluginpreset.svg?react";
import PluginPresetIcon from "./svg/ic_pluginpreset2.svg?react";
import { Pedalboard, PedalboardItem } from './Pedalboard';

interface PluginPresetSelectorProps extends WithStyles<typeof styles> {
    pedalboardItem: PedalboardItem | null;
    instanceId: number;
}


interface PluginPresetSelectorState {
    presets: PluginUiPresets;
    //enabled: boolean;
    presetChanged: boolean;
    showPresetsDialog: boolean;
    showEditPresetsDialog: boolean;
    presetsMenuAnchorRef: HTMLElement | null;

    renameDialogOpen: boolean;
    renameDialogDefaultName: string;
    renameDialogActionName: string;
    renameDialogOnOk?: (name: string) => void;

    saveAsName: string;

    isVisible: boolean;
    hasPresets: boolean;
    isPastePluginEnabled: boolean;
}


const styles = (theme: Theme) => createStyles({
    itemIcon: {
        width: 24, height: 24, marginRight: "4px", opacity: 0.6
    },
    pluginIcon: {
        width: 24, height: 24, opacity: 0.6, fill: theme.palette.text.primary
    },
    pluginMenuIcon: {
        width: 24, height: 24, opacity: 0.6, fill: theme.palette.text.primary, marginRight: 4
    }

});


let pluginClipboardContents: PedalboardItem | null = null;

const PluginPresetSelector =
    withStyles(
        class extends Component<PluginPresetSelectorProps, PluginPresetSelectorState> {

            model: PiPedalModel;

            constructor(props: PluginPresetSelectorProps) {
                super(props);
                this.model = PiPedalModelFactory.getInstance();
                this.state = {
                    presets: new PluginUiPresets(),
                    isVisible: this.isVisible(props.pedalboardItem),
                    presetChanged: false,
                    showPresetsDialog: false,
                    showEditPresetsDialog: false,
                    presetsMenuAnchorRef: null,
                    renameDialogOpen: false,
                    renameDialogDefaultName: "",
                    renameDialogActionName: "",
                    renameDialogOnOk: undefined,
                    saveAsName: "",
                    hasPresets: this.hasPresets(this.props.pedalboardItem),
                    isPastePluginEnabled: pluginClipboardContents !== null


                };
                this.handleDialogClose = this.handleDialogClose.bind(this);
                this.handlePresetsMenuClose = this.handlePresetsMenuClose.bind(this);
            }

            hasPresets(pedalboardItem: PedalboardItem | null): boolean {
                if (pedalboardItem === null) return false;
                if (!pedalboardItem.uri) return false;
                if (pedalboardItem.isStart() || pedalboardItem.isEnd() 
                    || pedalboardItem.isEmpty() || pedalboardItem.isSplit()) {
                    return false;
                }
                return true;
            }
            isVisible(pedalboardItem: PedalboardItem | null): boolean {
                if (pedalboardItem === null) return false;
                if (!pedalboardItem.uri) return false;
                if (pedalboardItem.isStart() || pedalboardItem.isEnd() 
                    || pedalboardItem.isSplit()) {
                    return false;
                }
                return true;
            }

            handleLoadPluginPreset(instanceId: number) {
                this.handlePresetsMenuClose();
                this.model.loadPluginPreset(this.props.instanceId, instanceId);
                let presetName = this.getPresetName(instanceId);
                this.setState({ saveAsName: presetName });
            }
            getPresetName(instanceId: number) {
                for (let preset of this.state.presets.presets) {
                    if (preset.instanceId === instanceId) {
                        return preset.label;
                    }
                }
                return "";
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
                        this.setState({ saveAsName: newName });
                        return this.model.saveCurrentPluginPresetAs(this.props.instanceId, newName);
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

            loadPresets(): void {
                if (this.props.pedalboardItem === null) return;
                if (!this.hasPresets(this.props.pedalboardItem)) return;   
                let captureUri: string = this.props.pedalboardItem.uri;
                this.model.getPluginPresets(captureUri)
                    .then((presets: PluginUiPresets) => {
                        if (captureUri === this.props.pedalboardItem?.uri) {
                            this.setState({ presets: presets });
                        }
                    })
                    .catch(error => {
                        if (captureUri === this.props.pedalboardItem?.uri) {
                            this.setState({ presets: new PluginUiPresets() });
                        }
                    });
            }
            onPluginPresetsChanged(pluginUri: string): void {
                if (pluginUri === this.props.pedalboardItem?.uri) {
                    this.loadPresets();

                }
            }
            _pluginPresetsChangedHandle?: PluginPresetsChangedHandle;

            componentDidMount() {
                this._pluginPresetsChangedHandle = this.model.addPluginPresetsChangedListener(
                    (pluginUri: string) => {
                        this.onPluginPresetsChanged(pluginUri);
                    }
                );
            }
            componentWillUnmount() {
                if (this._pluginPresetsChangedHandle) {
                    this.model.removePluginPresetsChangedListener(this._pluginPresetsChangedHandle);
                    this._pluginPresetsChangedHandle = undefined;
                }
            }

            currentPedalboard?: PedalboardItem;
            componentDidUpdate(
                prevProps: Readonly<PluginPresetSelectorProps>, 
                prevState: Readonly<PluginPresetSelectorState>, 
                snapshot?: any): void 
            {
                if (this.props.pedalboardItem !== prevProps.pedalboardItem) {
                    this.setState({ 
                        isVisible: this.isVisible(this.props.pedalboardItem),
                        hasPresets: this.hasPresets(this.props.pedalboardItem) });
                    if (this.props.pedalboardItem) {
                        if (this.props.pedalboardItem.isEmpty())
                        {
                            this.setState({ presets: new PluginUiPresets() });
                        } else 
                        {
                            this.loadPresets();
                        }
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

            isMenuCopyPluginEnabled(): boolean {
                return this.state.hasPresets;
            }
            handleMenuCopyPlugin(): void {
                this.handlePresetsMenuClose();
                let pedalboard: Pedalboard = this.model.pedalboard.get();;


                let pedalboardItem: PedalboardItem | null = pedalboard.tryGetItem(this.props.instanceId);
                if (pedalboardItem === null) {
                    pluginClipboardContents = null;
                    this.setState({ isPastePluginEnabled: false });
                    return;
                }
                pluginClipboardContents = pedalboardItem.clone();
                this.setState({ isPastePluginEnabled: true });

            }

            private isMenuPastePluginEnabled(): boolean {
                return this.state.isPastePluginEnabled;
            }
            private handleMenuPastePlugin(): void {
                this.handlePresetsMenuClose();

                if (pluginClipboardContents) {
                    this.model.replacePedalboarditem(this.props.instanceId, pluginClipboardContents.clone());
                }
            }


            private handleMenuEditPluginPresets(): void {
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

            buildMenuItems() {
                let result : React.ReactNode[] = [];
                if (this.state.hasPresets) {
                    result.push((<MenuItem key="menuSaveAs" onClick={(e) => this.handlePluginPresetsMenuSaveAs(e)}>Save plugin preset...</MenuItem>));
                    result.push((<MenuItem key="menuEdit" onClick={(e) => this.handleMenuEditPluginPresets()}>Manage plugin presets...</MenuItem>));
                    result.push((<Divider key="divider1" />));
                }
                let hasPresets = false;
                for (let preset of this.state.presets.presets) {
                    result.push((
                        <MenuItem key={preset.instanceId}
                            onClick={(e) => this.handleLoadPluginPreset(preset.instanceId)}
                        >
                            <PluginPresetIcon className={this.props.classes?.pluginMenuIcon ?? ""} />

                            {preset.label}</MenuItem>
                        ));
                    hasPresets = true;
                }
                if (hasPresets) {
                    result.push((<Divider key="divider2" />));
                }
                result.push(
                    <MenuItem key="menuCopy" disabled={!this.isMenuCopyPluginEnabled()} onClick={(e) => this.handleMenuCopyPlugin()}>Copy plugin</MenuItem>
                );
                result.push(
                    <MenuItem key="menuPaste" disabled={!this.isMenuPastePluginEnabled()} style={{ opacity: this.isMenuPastePluginEnabled() ? 1 : 0.4 }} onClick={(e) => this.handleMenuPastePlugin()}>Paste plugin</MenuItem>
                );
                return result;
            }
            render() {
                const classes = withStyles.getClasses(this.props);

                //const classes = withStyles.getClasses(this.props);
                //const classes = withStyles.getClasses(this.props);
                if (!this.state.isVisible) {
                    return (<div />);
                }
                return (
                    <div >
                        <IconButtonEx
                            tooltip="Plugin presets"
                            onClick={(e) => this.handlePresetMenuClick(e)} size="large">
                            <PluginPresetsIcon className={classes.pluginIcon} />
                        </IconButtonEx>
                        <Menu
                            id="edit-plugin-presets-menu"
                            anchorEl={this.state.presetsMenuAnchorRef}
                            open={Boolean(this.state.presetsMenuAnchorRef)}
                            onClose={() => this.handlePresetsMenuClose()}
                            TransitionComponent={Fade}
                            MenuListProps={
                                {
                                    style: { minWidth: 180 }
                                }
                            }

                        >
                            {
                                this.buildMenuItems()
                            }
                        </Menu>
                        {this.state.hasPresets && this.state.showPresetsDialog && (
                        <PluginPresetsDialog
                            instanceId={this.props.instanceId}
                            presets={this.state.presets}
                            show={this.state.showPresetsDialog}
                            isEditDialog={this.state.showEditPresetsDialog}
                            onDialogClose={() => this.handleDialogClose()} />
                        )}
                        <RenameDialog open={this.state.renameDialogOpen}
                            title="Rename"
                            defaultName={this.state.renameDialogDefaultName}
                            acceptActionName={this.state.renameDialogActionName}
                            useSafeFilenames={false}
                            onClose={() => this.handleRenameDialogClose()}
                            onOk={(name: string) => this.handleRenameDialogOk(name)} />
                    </div>
                );

            }
        },
        styles);

export default PluginPresetSelector;
