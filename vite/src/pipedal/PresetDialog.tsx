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
import { css } from '@emotion/react';
import {isDarkMode} from './DarkMode';
import IconButtonEx from './IconButtonEx';
import Typography from '@mui/material/Typography';
import { PiPedalModel, PiPedalModelFactory, PresetIndexEntry, PresetIndex } from './PiPedalModel';
import Button from '@mui/material/Button';
import ButtonEx from './ButtonEx';
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

import DownloadIcon from './svg/file_download_black_24dp.svg?react';
import UploadIcon from './svg/file_upload_black_24dp.svg?react';


interface PresetDialogProps extends WithStyles<typeof styles> {
    show: boolean;
    isEditDialog: boolean;
    onDialogClose: () => void;

};

interface PresetDialogState {
    presets: PresetIndex;

    showActionBar: boolean;

    selectedItem: number;

    renameOpen: boolean;

    moreMenuAnchorEl: HTMLElement | null;
    openUploadPresetDialog: boolean;


};


const styles = (theme: Theme) => createStyles({
    listIcon: css({
         width: 24, height: 24, opacity: 0.6, fill: theme.palette.text.primary
    }),
    dialogAppBar: css({
        position: 'relative',
        top: 0, left: 0
    }),
    dialogActionBar: css({
        position: 'relative',
        top: 0, left: 0,
        background: "black"
    }),
    dialogTitle: css({
        marginLeft: theme.spacing(2),
        flex: 1,
    }),
    itemBackground: css({
        background: theme.palette.background.paper
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
        width: 24, height: 24, margin: 12, opacity: 0.6
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


const PresetDialog = withStyles(
    class extends Component<PresetDialogProps, PresetDialogState> {

        model: PiPedalModel;



        constructor(props: PresetDialogProps) {
            super(props);
            this.model = PiPedalModelFactory.getInstance();

            this.handleDialogClose = this.handleDialogClose.bind(this);
            let presets = this.model.presets.get();
            this.state = {
                presets: presets,
                showActionBar: false,
                selectedItem: presets.selectedInstanceId,
                renameOpen: false,
                moreMenuAnchorEl: null,
                openUploadPresetDialog: false

            };
            this.handlePresetsChanged = this.handlePresetsChanged.bind(this);

        }

        selectItemAtIndex(index: number) {
            let instanceId = this.state.presets.presets[index].instanceId;
            this.setState({ selectedItem: instanceId });
        }
        isEditMode() {
            return this.state.showActionBar || this.props.isEditDialog;
        }

        onMoreClick(e: SyntheticEvent): void {
            this.setState({ moreMenuAnchorEl: e.currentTarget as HTMLElement })
        }

        handleDownloadPreset() {
            this.handleMoreClose();
            this.model.download("downloadPreset", this.state.selectedItem);
        }
        handleUploadPreset() {
            this.handleMoreClose();
            this.setState({ openUploadPresetDialog: true });

        }
        handleMoreClose(): void {
            this.setState({ moreMenuAnchorEl: null });

        }


        handlePresetsChanged() {
            let presets = this.model.presets.get();

            if (!presets.areEqual(this.state.presets, false)) // avoid a bunch of peculiar effects if we update while a drag is in progress
            {
                // if we don't have a valid selection, then use the current preset.
                if (this.state.presets.getItem(this.state.selectedItem) == null) {
                    this.setState({ presets: presets, selectedItem: presets.selectedInstanceId });
                } else {
                    this.setState({ presets: presets });
                }
            }
        }


        componentDidMount() {
            this.model.presets.addOnChangedHandler(this.handlePresetsChanged);
            this.handlePresetsChanged();
            // scroll selected item into view.
        }
        componentWillUnmount() {
            this.model.presets.removeOnChangedHandler(this.handlePresetsChanged);
        }

        getSelectedIndex() {
            let instanceId = this.isEditMode() ? this.state.selectedItem : this.state.presets.selectedInstanceId;
            let presets = this.state.presets;
            for (let i = 0; i < presets.presets.length; ++i) {
                if (presets.presets[i].instanceId === instanceId) return i;
            }
            return -1;
        }

        handleDeleteClick() {
            if (!this.state.selectedItem) return;
            let selectedItem = this.state.selectedItem;
            if (selectedItem !== -1) {
                this.model.deletePresetItem(selectedItem)
                    .then((selectedItem: number) => {
                        this.setState({ selectedItem: selectedItem });
                    })
                    .catch((error) => {
                        this.model.showAlert(error);
                    });

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
            let presetEntry = el as PresetIndexEntry;
            const classes = withStyles.getClasses(this.props);
            let selectedItem = this.isEditMode() ? this.state.selectedItem : this.state.presets.selectedInstanceId;
            return (
                <div key={presetEntry.instanceId} className="itemBackground">

                    <ButtonBase style={{ width: "100%", height: 48 }}
                        onClick={() => this.handleItemClick(presetEntry.instanceId)}
                    >
                        <SelectHoverBackground selected={presetEntry.instanceId === selectedItem} showHover={true} />
                        <div className={classes.itemFrame}>
                            <div className={classes.iconFrame}>
                                <img 
                                    src={isDarkMode()? "img/ic_presets_white.svg": "img/ic_presets.svg"}
                                 className={classes.itemIcon} alt="" />
                            </div>
                            <div className={classes.itemLabel}>
                                <Typography>
                                    {presetEntry.name}
                                </Typography>
                            </div>
                        </div>
                    </ButtonBase>
                </div>

            );
        }

        updateServerPresets(newPresets: PresetIndex) {
            newPresets = newPresets.clone();
            this.model.updatePresets(newPresets)
                .catch((error) => {
                    this.model.showAlert(error);
                });
        }
        moveElement(from: number, to: number): void {
            let newPresets = this.state.presets.clone();
            newPresets.movePreset(from, to);
            this.setState({
                presets: newPresets,
                selectedItem: newPresets.presets[to].instanceId
            });
            this.updateServerPresets(newPresets);
        }

        getSelectedName(): string {
            let item = this.state.presets.getItem(this.state.selectedItem);
            if (item) return item.name;
            return "";
        }

        handleRenameClick() {
            let item = this.state.presets.getItem(this.state.selectedItem);
            if (item) {
                this.setState({ renameOpen: true });
            }
        }
        handleRenameOk(text: string) {
            let item = this.state.presets.getItem(this.state.selectedItem);
            if (!item) return;
            if (item.name !== text) {
                this.model.renamePresetItem(this.state.selectedItem, text)
                    .catch((error) => {
                        this.onError(error);
                    });
            }

            this.setState({ renameOpen: false });
        }
        handleCopy() {
            let item = this.state.presets.getItem(this.state.selectedItem);
            if (!item) return;
            this.model.duplicatePreset(this.state.selectedItem)
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

            return (
                <DialogEx tag="preset" fullScreen open={this.props.show}
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
                                    <Typography variant="h6" className={classes.dialogTitle}>
                                        Presets
                                    </Typography>
                                    <IconButtonEx tooltip="Edit"color="inherit" onClick={(e) => this.showActionBar(true)} >
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
                                        <IconButtonEx edge="start" tooltip="Back" color="inherit" onClick={this.handleDialogClose} aria-label="back"
                                        >
                                            <ArrowBackIcon />
                                        </IconButtonEx>

                                    )}
                                    <Typography variant="h6" className={classes.dialogTitle}>
                                        Presets
                                    </Typography>
                                    {(this.state.presets.getItem(this.state.selectedItem) != null)
                                        && (

                                            <div style={{ flex: "0 0 auto" }}>
                                                <Button color="inherit" onClick={(e) => this.handleCopy()}>
                                                    Copy
                                                </Button>
                                                <Button color="inherit" onClick={() => this.handleRenameClick()}>
                                                    Rename
                                                </Button>
                                                <RenameDialog
                                                    open={this.state.renameOpen}
                                                    defaultName={this.getSelectedName()}
                                                    acceptActionName={"Rename"}
                                                    onClose={() => { this.setState({ renameOpen: false }) }}
                                                    onOk={(text: string) => {
                                                        this.handleRenameOk(text);
                                                    }
                                                    }
                                                />
                                                <IconButtonEx tooltip="Delete" color="inherit" onClick={(e) => this.handleDeleteClick()} >
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
                                                    <MenuItem onClick={() => { this.handleDownloadPreset(); }} >
                                                        <ListItemIcon>
                                                            <DownloadIcon className={classes.listIcon} />
                                                        </ListItemIcon>
                                                        <ListItemText>
                                                            Download preset
                                                        </ListItemText>

                                                    </MenuItem>
                                                    <MenuItem onClick={() => { this.handleUploadPreset() }}>
                                                        <ListItemIcon>
                                                            <UploadIcon className={classes.listIcon} />
                                                        </ListItemIcon>
                                                        <ListItemText>
                                                            Upload preset
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
                                    this.state.presets.presets.map((element) => {
                                        return this.mapElement(element);
                                    })
                                }
                            </DraggableGrid>
                        </div>
                    </div>
                    <UploadPresetDialog 
                            title='Upload preset'
                            extension='.piPreset'
                            uploadPage='uploadPreset'
                            onUploaded={(instanceId) => this.setState({selectedItem: instanceId}) }
                            uploadAfter={this.state.selectedItem} 
                            open={this.state.openUploadPresetDialog} 
                            onClose={() => { this.setState({ openUploadPresetDialog: false }) }} />

                </DialogEx>

            );

        }

    },
    styles
);

export default PresetDialog;