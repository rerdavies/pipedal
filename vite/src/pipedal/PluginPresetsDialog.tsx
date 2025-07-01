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

import React, { SyntheticEvent,Component } from 'react';
import IconButtonEx from './IconButtonEx';
import Typography from '@mui/material/Typography';
import { PiPedalModel, PiPedalModelFactory  } from './PiPedalModel';
import Button from "@mui/material/Button";
import ButtonBase from "@mui/material/ButtonBase";
import DialogEx from './DialogEx';
import AppBar from '@mui/material/AppBar';
import Toolbar from '@mui/material/Toolbar';
import DraggableGrid, { ScrollDirection } from './DraggableGrid';
import MoreVertIcon from '@mui/icons-material/MoreVert';
import ListItemIcon from '@mui/material/ListItemIcon';
import ListItemText from '@mui/material/ListItemText';
import Menu from '@mui/material/Menu';
import MenuItem from '@mui/material/MenuItem';
import Fade from '@mui/material/Fade';
import UploadPresetDialog from './UploadPresetDialog';
import {PluginUiPresets,PluginUiPreset} from './PluginPreset';

import SelectHoverBackground from './SelectHoverBackground';
import CloseIcon from '@mui/icons-material/Close';
import ArrowBackIcon from '@mui/icons-material/ArrowBack';
import EditIcon from '@mui/icons-material/Edit';
import RenameDialog from './RenameDialog';

import Slide, {SlideProps} from '@mui/material/Slide';
import {createStyles} from './WithStyles';

import { Theme } from '@mui/material/styles';

import WithStyles from './WithStyles';
import { withStyles } from "tss-react/mui";
import PluginPresetIcon from "./svg/ic_pluginpreset2.svg?react";

import DownloadIcon from './svg/file_download_black_24dp.svg?react';
import UploadIcon from './svg/file_upload_black_24dp.svg?react';
import { css } from '@emotion/react';


interface PluginPresetsDialogProps extends WithStyles<typeof styles> {
    show: boolean;
    isEditDialog: boolean;
    instanceId: number;
    presets :PluginUiPresets;

    onDialogClose: () => void;

};

interface PluginPresetsDialogState {

    showActionBar: boolean;

    selectedItem: number;

    renameOpen: boolean;

    moreMenuAnchorEl: HTMLElement | null;
    openUploadPresetDialog: boolean;


};


const styles = (theme: Theme) => createStyles({
    listIcon: css({
        width: 24, height: 24, opacity: 0.6,fill: theme.palette.text.primary
    }),
    dialogAppBar: css({
        position: 'relative',
        top: 0, left: 0
    }),
    itemBackground: css({
        background: theme.palette.background.paper
    }),
    dialogActionBar: css({
        position: 'relative',
        top: 0, left: 0,
        background: "black"
    }),
    dialogTitle: css({
        marginLeft: theme.spacing(2),
        textOverflow: "ellipsis",
        flex: "1 1",
    }),
    itemFrame: css({
        display: "flex",
        flexDirection: "row",
        flexWrap: "nowrap",
        width: "100%",
        height: "56px",
        alignItems: "center",
        textAlign: "left",
        justifyContent: "center",
        paddingLeft: 8
    }),
    iconFrame: css({
        flex: "0 0 auto",

    }),
    itemIcon: css({
        width: 24, height: 24, margin: 12, opacity: 0.6,
        fill: theme.palette.text.primary
    }),
    itemLabel: css({
        flex: "1 1 1px",
        marginLeft: 8
    })

});


const Transition = React.forwardRef(function Transition(
    props: SlideProps, ref: React.Ref<unknown>
) {
    return (<Slide direction="up" ref={ref} {...props} />);
});


const PluginPresetsDialog = withStyles(
    class extends Component<PluginPresetsDialogProps, PluginPresetsDialogState> {

        model: PiPedalModel;



        constructor(props: PluginPresetsDialogProps) {
            super(props);
            this.model = PiPedalModelFactory.getInstance();

            this.handleDialogClose = this.handleDialogClose.bind(this);
            this.state = {
                showActionBar: false,
                selectedItem: -1,
                renameOpen: false,
                moreMenuAnchorEl: null,
                openUploadPresetDialog: false

            };

        }

        selectItemAtIndex(index: number) {
            let instanceId    = this.props.presets.presets[index].instanceId;
            this.setState({ selectedItem: instanceId });
        }
        isEditMode() {
            return this.state.showActionBar || this.props.isEditDialog;
        }

        onMoreClick(e: SyntheticEvent): void {
            this.setState({ moreMenuAnchorEl: e.currentTarget as HTMLElement })
        }

        handleDownloadPresets() {
            this.handleMoreClose();
            if (this.props.presets.pluginUri !== "")
            {
                this.model.download("downloadPluginPresets", this.props.presets.pluginUri);
            }
        }
        handleUploadPresets() {
            this.handleMoreClose();
            this.setState({ openUploadPresetDialog: true });

        }
        handleMoreClose(): void {
            this.setState({ moreMenuAnchorEl: null });

        }


        componentDidUpdate()
        {
            if (!this.props.presets.getItem(this.state.selectedItem))
            {
                if (this.props.presets.presets.length !== 0)
                {
                    this.setState({selectedItem: this.props.presets.presets[0].instanceId});
                }
            }
        }


        componentDidMount() {
            // scroll selected item into view.
        }
        componentWillUnmount() {
        }

        getSelectedIndex() {
            let instanceId = this.state.selectedItem;
            let presets = this.props.presets;
            for (let i = 0; i < presets.presets.length; ++i) {
                if (presets.presets[i].instanceId === instanceId) return i;
            }
            return -1;
        }

        handleDeleteClick() {
            let selectedItem = this.state.selectedItem;
            if (selectedItem !== -1) {
                let newPresets = this.props.presets.clone();
                for (let i = 0; i < newPresets.presets.length; ++i)
                {
                    if (newPresets.presets[i].instanceId === selectedItem)
                    {
                        newPresets.presets.splice(i,1);
                        this.model.updatePluginPresets(newPresets.pluginUri,newPresets)
                        .catch((error) => {
                            this.model.showAlert(error);

                        });
                        let newPos = i;
                        if (newPos >= newPresets.presets.length) {
                            --i;
                        }
                        let newSelection = i < 0? -1: newPresets.presets[i].instanceId;
                        this.setState({selectedItem: newSelection});
                    }
                }
            }
        }
        handleDialogClose() {
            this.props.onDialogClose();
        }

        handleItemClick(instanceId: number): void {
            if (this.isEditMode()) {
                this.setState({ selectedItem: instanceId });
            } else {
                this.model.loadPreset(instanceId);
                this.props.onDialogClose();
            }
        }
        showActionBar(show: boolean): void {
            this.setState({ showActionBar: show });

        }


        mapElement(el: any): React.ReactNode {
            let presetEntry = el as PluginUiPreset;
            const classes = withStyles.getClasses(this.props);
            let selectedItem = this.state.selectedItem;
            return (
                <div key={presetEntry.instanceId} className="backgroundItem"  >

                    <ButtonBase style={{ width: "100%", height: 48 }}
                        onClick={() => this.handleItemClick(presetEntry.instanceId)}
                    >
                        <SelectHoverBackground selected={presetEntry.instanceId === selectedItem} showHover={true} />
                        <div className={classes.itemFrame}>
                            <div className={classes.iconFrame}>
                                <PluginPresetIcon className={classes.itemIcon}  />
                            </div>
                            <div className={classes.itemLabel}>
                                <Typography noWrap> 
                                    {presetEntry.label}
                                </Typography>
                            </div>
                        </div>
                    </ButtonBase>
                </div>

            );
        }

        updateServerPluginPresets(newPresets: PluginUiPresets) {
            newPresets = newPresets.clone();
            this.model.updatePluginPresets(this.getPluginUri(),newPresets)
                .catch((error) => {
                    this.model.showAlert(error);
                });
        }
        moveElement(from: number, to: number): void {
            let newPresets: PluginUiPresets = this.props.presets.clone();
            newPresets.movePreset(from, to);
            this.setState({
                selectedItem: newPresets.presets[to].instanceId
            });
            this.updateServerPluginPresets(newPresets);
        }

        getSelectedName(): string {
            let item = this.props.presets.getItem(this.state.selectedItem);
            if (item) return item.label;
            return "";
        }

        handleRenameClick() {
            let item = this.props.presets.getItem(this.state.selectedItem);
            if (item) {
                this.setState({ renameOpen: true });
            }
        }
        handleRenameOk(text: string) {
            let item = this.props.presets.getItem(this.state.selectedItem);
            if (!item) return;
            if (item.label !== text) {
                let newPresets = this.props.presets.clone();
                let newItem = newPresets.getItem(this.state.selectedItem);
                if (!newItem) return;
                newItem.label = text;
                this.updateServerPluginPresets(newPresets);
            }

            this.setState({ renameOpen: false });
        }
        getPluginUri() : string {
            let pedalboardItem = this.model.pedalboard.get().getItem(this.props.instanceId);
            return pedalboardItem.uri;
        }
        handleCopy() {
            let item = this.props.presets.getItem(this.state.selectedItem);
            if (!item) return;

            this.model.duplicatePluginPreset(this.getPluginUri(),this.state.selectedItem)
                .then((newId) => {
                    this.setState({ selectedItem: newId });
                }).catch((error) => {
                    this.onError(error);
                });
        }

        onError(error: string): void {
            this.model?.showAlert(error);
        }



        render() {
            const classes = withStyles.getClasses(this.props);

            let actionBarClass = this.props.isEditDialog ? classes.dialogAppBar : classes.dialogActionBar;
            let defaultSelectedIndex = this.getSelectedIndex();
            let title: string = "";
            let pluginUri = this.getPluginUri();
            let plugin = this.model.getUiPlugin(pluginUri);
            if (plugin) {
                title = "Plugin Presets - " + plugin.name;
            }
            

            return (
                <DialogEx tag="pluginPresets" fullScreen open={this.props.show}
                    onClose={() => { this.handleDialogClose() }} TransitionComponent={Transition}
                    style={{userSelect: "none"}}
                    onEnterKey={()=>{}}
                    >
                    <div style={{ display: "flex", flexDirection: "column", flexWrap: "nowrap", width: "100%", height: "100%", overflow: "hidden" }}>
                        <div style={{ flex: "0 0 auto" }}>
                            <AppBar className={classes.dialogAppBar} style={{ display: this.isEditMode() ? "none" : "block" }} >
                                <Toolbar>
                                    <IconButtonEx tooltip="Back" edge="start" color="inherit" onClick={this.handleDialogClose} aria-label="back"
                                        disabled={this.isEditMode()}
                                    >
                                        <ArrowBackIcon />
                                    </IconButtonEx>
                                    <Typography noWrap variant="h6" className={classes.dialogTitle}>
                                        {title}
                                    </Typography>
                                    <IconButtonEx tooltip="Edit" color="inherit" onClick={(e) => this.showActionBar(true)} >
                                        <EditIcon />
                                    </IconButtonEx>
                                </Toolbar>
                            </AppBar>
                            <AppBar className={actionBarClass} style={{ display: this.isEditMode() ? "block" : "none" }}
                                onClick={(e) => { e.stopPropagation(); e.preventDefault(); }}
                            >
                                <Toolbar>
                                    {(!this.props.isEditDialog) ? (
                                        <IconButtonEx tooltip="Close" edge="start" color="inherit" onClick={(e) => this.showActionBar(false)} aria-label="close">
                                            <CloseIcon />
                                        </IconButtonEx>
                                    ) : (
                                        <IconButtonEx tooltip="Back" edge="start" color="inherit" onClick={this.handleDialogClose} aria-label="back"
                                        >
                                            <ArrowBackIcon />
                                        </IconButtonEx>

                                    )}
                                    <Typography noWrap variant="h6" className={classes.dialogTitle}>
                                        { title }
                                    </Typography>
                                    {(this.props.presets.getItem(this.state.selectedItem) != null)
                                        && (

                                            <div style={{ flex: "0 0 auto" }}>
                                                <Button color="inherit" onClick={(e) => this.handleCopy()}>
                                                    Copy
                                                </Button>
                                                <Button color="inherit" onClick={() => this.handleRenameClick()}>
                                                    Rename
                                                </Button>
                                                <RenameDialog
                                                    title="Rename"
                                                    open={this.state.renameOpen}
                                                    defaultName={this.getSelectedName()}
                                                    acceptActionName={"Rename"}
                                                    onClose={() => { this.setState({ renameOpen: false }) }}
                                                    onOk={(text: string) => {
                                                        this.handleRenameOk(text);
                                                    }
                                                    }
                                                />
                                                <IconButtonEx tooltip="Delete"   color="inherit" onClick={(e) => this.handleDeleteClick()} >
                                                    <img src="/img/old_delete_outline_white_24dp.svg" alt="Delete" style={{ width: 24, height: 24, opacity: 0.6 }} />
                                                </IconButtonEx>
                                                <IconButtonEx tooltip="More..." color="inherit" onClick={(e) => { this.onMoreClick(e) }} >
                                                    <MoreVertIcon />
                                                </IconButtonEx>
                                                <Menu
                                                    id="more-menu"
                                                    anchorEl={this.state.moreMenuAnchorEl}
                                                    keepMounted
                                                    open={Boolean(this.state.moreMenuAnchorEl)}
                                                    onClose={() => this.handleMoreClose()}
                                                    TransitionComponent={Fade}
                                                >
                                                    <MenuItem onClick={() => { this.handleDownloadPresets(); }} 
                                                        disabled={this.props.presets.presets.length === 0}
                                                    >
                                                        <ListItemIcon>
                                                            <DownloadIcon className={classes.listIcon} />
                                                        </ListItemIcon>
                                                        <ListItemText>
                                                            Download plugin presets
                                                        </ListItemText>

                                                    </MenuItem>
                                                    <MenuItem onClick={() => { this.handleUploadPresets() }}>
                                                        <ListItemIcon>
                                                            <UploadIcon className={classes.listIcon} />
                                                        </ListItemIcon>
                                                        <ListItemText>
                                                            Upload plugin presets
                                                        </ListItemText>

                                                    </MenuItem>
                                                </Menu>


                                            </div>
                                        )
                                    }
                                </Toolbar>

                            </AppBar>
                        </div>
                        <div style={{ flex: "1 1 auto", position: "relative", overflow: "hidden" }} >
                            <DraggableGrid
                                onLongPress={(item) => this.showActionBar(true)}
                                canDrag={this.isEditMode()}
                                onDragStart={(index, x, y) => { this.selectItemAtIndex(index) }}
                                moveElement={(from, to) => { this.moveElement(from, to); }}
                                scroll={ScrollDirection.Y}
                                defaultSelectedIndex={defaultSelectedIndex}
                            >
                                {
                                    this.props.presets.presets.map((element) => {
                                        return this.mapElement(element);
                                    })
                                }
                            </DraggableGrid>
                        </div>
                    </div>
                    <UploadPresetDialog 
                            title="Upload Plugin Presets"
                            extension='.piPluginPresets'
                            uploadPage='uploadPluginPresets'
                            onUploaded={(instanceId) => {} }
                            uploadAfter={this.state.selectedItem} 
                            open={this.state.openUploadPresetDialog} 
                            onClose={() => { this.setState({ openUploadPresetDialog: false }) }} />

                </DialogEx>

            );

        }

    },
    styles
);

export default PluginPresetsDialog;