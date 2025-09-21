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

import React, { SyntheticEvent } from 'react';
import ImportPresetFromBankDialog from './ImportPresetFromBankDialog';
import CopyPresetsToBankDialog from './CopyPresetsToBankDialog';
import Icon from '@mui/material/Icon';
import Divider from '@mui/material/Divider';
import { css } from '@emotion/react';
import { isDarkMode } from './DarkMode';
import IconButtonEx from './IconButtonEx';
import Typography from '@mui/material/Typography';
import { PiPedalModel, PiPedalModelFactory, PresetIndexEntry, PresetIndex } from './PiPedalModel';
import Button from '@mui/material/Button';
import DialogEx from './DialogEx';
import AppBar from '@mui/material/AppBar';
import Toolbar from '@mui/material/Toolbar';
import DraggableGrid, { ScrollDirection } from './DraggableGrid';
import CloseIcon from '@mui/icons-material/Close';
import MoreVertIcon from '@mui/icons-material/MoreVert';
import ListItemButton from '@mui/material/ListItemButton';
import ListItemIcon from '@mui/material/ListItemIcon';
import ListItemText from '@mui/material/ListItemText';
import Menu from '@mui/material/Menu';
import MenuItem from '@mui/material/MenuItem';
import Fade from '@mui/material/Fade';
import UploadPresetDialog from './UploadPresetDialog';
import ResizeResponsiveComponent from './ResizeResponsiveComponent';

import ArrowBackIcon from '@mui/icons-material/ArrowBack';
import RenameDialog from './RenameDialog';

import Slide, { SlideProps } from '@mui/material/Slide';
import { createStyles } from './WithStyles';

import { Theme } from '@mui/material/styles';
import WithStyles from './WithStyles';
import { withStyles } from "tss-react/mui";

import DownloadIcon from './svg/file_download_black_24dp.svg?react';
import UploadIcon from './svg/file_upload_black_24dp.svg?react';

// function isTouchUi() {
//     return 'ontouchstart' in window || navigator.maxTouchPoints > 0;
// }


interface PresetDialogProps extends WithStyles<typeof styles> {
    show: boolean;
    onDialogClose: () => void;

};

interface PresetDialogState {
    presets: PresetIndex;

    multiSelect: boolean;
    showTouchActionBar: boolean;


    currentItem: number;
    selectedItems: Set<number>;

    renameOpen: boolean;
    importOpen: boolean;
    copyToOpen: boolean;

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
        background: theme.palette.actionBar.main,
        color: theme.palette.actionBar.contrastText
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
        paddibomngLeft: 8
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
    class extends ResizeResponsiveComponent<PresetDialogProps, PresetDialogState> {

        model: PiPedalModel;



        constructor(props: PresetDialogProps) {
            super(props);
            this.model = PiPedalModelFactory.getInstance();

            this.handleDialogClose = this.handleDialogClose.bind(this);
            let presets = this.model.presets.get();
            this.state = {
                presets: presets,
                multiSelect: false,
                showTouchActionBar: false,
                currentItem: presets.selectedInstanceId,
                selectedItems: new Set<number>([presets.selectedInstanceId]),
                renameOpen: false,
                importOpen: false,
                copyToOpen: false,
                moreMenuAnchorEl: null,
                openUploadPresetDialog: false

            };
            this.handlePresetsChanged = this.handlePresetsChanged.bind(this);

        }

        onWindowSizeChanged(width: number, height: number) {
            super.onWindowSizeChanged(width, height);
        }
        handleSelectionUpdated(instanceIds: Set<number>) {
            this.setState({
                multiSelect: instanceIds.size > 1
            })

        }
        setSelections(instanceIds: Set<number>) {
            this.setState({
                selectedItems: instanceIds,
            });
            this.handleSelectionUpdated(instanceIds);
        }
        setSelection(instanceId: number) {
            let selectedItems = new Set<number>([instanceId]);
            this.setState({
                currentItem: instanceId,
                selectedItems: selectedItems
            });
            this.handleSelectionUpdated(selectedItems);
        }

        selectItemAtIndex(index: number) {
            let instanceId = this.state.presets.presets[index].instanceId;
            this.setSelection(instanceId);
        }

        onMoreClick(e: SyntheticEvent): void {
            this.setState({ moreMenuAnchorEl: e.currentTarget as HTMLElement })
        }

        handleDownloadPreset() {
            this.handleMoreClose();
            this.model.download("downloadPreset", this.state.currentItem);
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
                if (this.state.presets.getItem(this.state.currentItem) == null) {
                    this.setState({
                        presets: presets, currentItem: presets.selectedInstanceId,
                        selectedItems: new Set<number>([presets.selectedInstanceId])
                    });
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

        getSelectedIndex(instanceId?: number): number {
            if (instanceId === undefined) {
                instanceId = this.state.currentItem;
            }
            let presets = this.state.presets;
            for (let i = 0; i < presets.presets.length; ++i) {
                if (presets.presets[i].instanceId === instanceId) return i;
            }
            return -1;
        }

        handleDeleteClick() {
            if (this.state.selectedItems.size === 0) {
                return;
            }
            this.model.deletePresetItems(this.state.selectedItems)
                .then((currentItem: number) => {
                    this.setSelection(currentItem);
                })
                .catch((error) => {
                    this.model.showAlert(error);
                });
        }
        handleDialogClose() {
            this.props.onDialogClose();
        }

        handleItemClick(e: React.MouseEvent<HTMLElement>, instanceId: number): void {
            e.stopPropagation();
            if (e.ctrlKey || this.state.showTouchActionBar) {
                let selectedItems = new Set<number>(this.state.selectedItems);
                if (selectedItems.has(instanceId)) {
                    selectedItems.delete(instanceId);
                    this.setSelections(selectedItems);
                } else {
                    selectedItems.add(instanceId);
                    this.setState({
                        selectedItems: selectedItems,
                        currentItem: instanceId
                    });
                    this.handleSelectionUpdated(selectedItems);
                }
            } else if (e.shiftKey) {
                let presets = this.state.presets;
                let startIndex = this.getSelectedIndex();
                let endIndex = this.getSelectedIndex(instanceId);
                if (startIndex === -1 || endIndex === -1) return; // should not happen.

                let selectedItems = new Set<number>();

                if (endIndex < startIndex) {
                    let t = startIndex;
                    startIndex = endIndex;
                    endIndex = t;
                }
                for (let i = startIndex; i <= endIndex; ++i) {
                    selectedItems.add(presets.presets[i].instanceId);
                }
                this.setSelections(selectedItems);
            } else {
                this.setSelection(instanceId);
            }
        }
        showTouchActionBar(show: boolean): void {
            this.setState({ showTouchActionBar: show });

        }


        mapElement(el: any): React.ReactNode {
            let presetEntry = el as PresetIndexEntry;
            const classes = withStyles.getClasses(this.props);
            let selected = this.state.selectedItems.has(presetEntry.instanceId);
            return (
                <div key={presetEntry.instanceId} id={"psetdlgItem_" + presetEntry.instanceId} className="itemBackground">
                    <ListItemButton
                        selected={selected}
                        onClick={(e) => this.handleItemClick(e, presetEntry.instanceId)}
                        style={{ height: 48 }}
                    >
                        <ListItemIcon>
                            <img
                                src={isDarkMode() ? "img/ic_presets_white.svg" : "img/ic_presets.svg"}
                                className={classes.itemIcon} alt="" />
                        </ListItemIcon>
                        <ListItemText primary={presetEntry.name}
                        />

                    </ListItemButton>
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
            let toInstanceId = newPresets.presets[to].instanceId;
            this.setState({
                presets: newPresets,
                currentItem: toInstanceId,
                selectedItems: new Set<number>([toInstanceId])

            });
            this.updateServerPresets(newPresets);
        }

        getSelectedName(): string {
            let item = this.state.presets.getItem(this.state.currentItem);
            if (item) return item.name;
            return "";
        }

        handleRenameClick() {
            let item = this.state.presets.getItem(this.state.currentItem);
            if (item) {
                this.setState({ renameOpen: true });
            }
        }
        handleRenameOk(text: string) {
            let item = this.state.presets.getItem(this.state.currentItem);
            if (!item) return;
            if (item.name !== text) {
                this.model.renamePresetItem(this.state.currentItem, text)
                    .catch((error) => {
                        this.onError(error);
                    });
            }

            this.setState({ renameOpen: false });
        }
        handleCopy() {
            let item = this.state.presets.getItem(this.state.currentItem);
            if (!item) return;
            this.model.duplicatePreset(this.state.currentItem)
                .then((newId) => {
                    this.setState({
                        currentItem: newId,
                        selectedItems: new Set<number>([newId])
                    });
                }).catch((error) => {
                    this.onError(error);
                });
        }

        onError(error: string): void {
            this.model?.showAlert(error);
        }

        handleCloseActionBar(e: SyntheticEvent) {
            e.stopPropagation();
            this.setState({
                showTouchActionBar: false,
                selectedItems: new Set<number>([this.state.currentItem]),
                multiSelect: false
            });
        }
        handleImportPresetsFromBank() {
            this.handleMoreClose();
            this.setState({ importOpen: true });
        }
        handleImportDialogOk(bankInstanceId: number, presets: number[]): void {
            this.setState({ importOpen: false });
            this.model.importPresetsFromBank(bankInstanceId, presets)
                .then((instanceId) => {
                    if (instanceId !== -1) {
                        this.setSelection(instanceId);
                        setTimeout(() => {
                            let el = document.getElementById("psetdlgItem_" + instanceId);
                            if (el) {
                                el.scrollIntoView({ behavior: "smooth", block: "center" });
                            }
                        }, 0);
                    }
                })
                .catch((error) => {
                    this.model.showAlert(error);
                });
        }
        handleCopyToBankDialogOk(bankInstanceId: number): void {
            this.setState({ copyToOpen: false });
            let selectedItems: number[] = [];
            for (let i = 0; i < this.state.presets.presets.length; ++i) {
                let preset = this.state.presets.presets[i];
                if (this.state.selectedItems.has(preset.instanceId)) {
                    selectedItems.push(preset.instanceId);
                }
            }

            this.model.copyPresetsToBank(bankInstanceId, selectedItems)
                .then((instanceId) => {
                })
                .catch((error) => {
                    this.model.showAlert(error);
                });
        }

        handleCopyPresetsToBank() {
            this.handleMoreClose();
            this.setState({ copyToOpen: true });
        }

        render() {
            const classes = withStyles.getClasses(this.props);

            const showActionBar = this.state.multiSelect || this.state.showTouchActionBar;
            return (
                <DialogEx tag="preset" fullScreen open={this.props.show}
                    onClose={() => { this.handleDialogClose() }} TransitionComponent={Transition}
                    style={{ userSelect: "none" }}
                    onEnterKey={() => { }}
                >
                    <div style={{ display: "flex", flexDirection: "column", flexWrap: "nowrap", width: "100%", height: "100%", overflow: "hidden" }}>
                        <div style={{ flex: "0 0 auto" }}>
                            <AppBar className={classes.dialogActionBar} style={{ display: showActionBar ? "block" : "none" }}
                            >
                                <Toolbar>
                                    <IconButtonEx tooltip="Back" edge="start" color="inherit" aria-label="back"
                                        onClick={(e) => { this.handleCloseActionBar(e); }}
                                    >
                                        <CloseIcon />
                                    </IconButtonEx>
                                    <Typography variant="h6" className={classes.dialogTitle} noWrap>
                                        {this.state.selectedItems.size.toString() + " items selected"}
                                    </Typography>
                                    <Button color="inherit"
                                        onClick={() => {
                                            this.handleCopyPresetsToBank();
                                        }} >
                                        Copy to
                                    </Button>
                                    <IconButtonEx tooltip="Delete" color="inherit" onClick={(e) => { this.handleDeleteClick(); }} >
                                        <img src="/img/old_delete_outline_white_24dp.svg" alt="Delete" style={{ width: 24, height: 24, opacity: 0.6 }} />
                                    </IconButtonEx>
                                </Toolbar>
                            </AppBar>
                            <AppBar className={classes.dialogAppBar} style={{ display: showActionBar ? "none" : "block" }}
                                onClick={(e) => { e.stopPropagation(); e.preventDefault(); }}
                            >
                                <Toolbar>
                                    <IconButtonEx edge="start" tooltip="Back" color="inherit" onClick={this.handleDialogClose} aria-label="back"
                                    >
                                        <ArrowBackIcon />
                                    </IconButtonEx>
                                    <Typography variant="h6" className={classes.dialogTitle} noWrap>
                                        Presets
                                    </Typography>
                                    {(this.state.presets.getItem(this.state.currentItem) != null)
                                        && (

                                            <div style={{
                                                flex: "0 0 auto", display: "flex", flexFlow: "row nowrap", alignItems: "center"

                                            }}>
                                                {this.state.selectedItems.size === 1 && (
                                                    <Button color="inherit" onClick={(e) => this.handleCopy()}
                                                        >
                                                        Copy
                                                    </Button>
                                                )}
                                                {this.state.selectedItems.size === 1 && (

                                                    <Button color="inherit" onClick={() => this.handleRenameClick()}
                                                        >
                                                        Rename
                                                    </Button>
                                                )}
                                                {this.state.renameOpen && (
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
                                                )}
                                                {this.state.selectedItems.size === 1 && (
                                                    <IconButtonEx tooltip="Delete" color="inherit" onClick={(e) => this.handleDeleteClick()}
                                                        style={{ opacity: this.state.selectedItems.size !== 1 ? 0.5 : 1.0 }}
                                                    >
                                                        <img src="/img/old_delete_outline_white_24dp.svg" alt="Delete" style={{ width: 24, height: 24, opacity: 0.6 }} />
                                                    </IconButtonEx>
                                                )}
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
                                                    {this.state.selectedItems.size !== 0 && (
                                                        <MenuItem onClick={() => {
                                                            this.handleMoreClose();
                                                            this.handleCopyPresetsToBank();
                                                        }}
                                                        >
                                                            <ListItemIcon>
                                                                <Icon />
                                                            </ListItemIcon>
                                                            <ListItemText>
                                                                Copy to bank...
                                                            </ListItemText>

                                                        </MenuItem>
                                                    )}
                                                    <MenuItem onClick={() => { this.handleImportPresetsFromBank(); }} >
                                                        <ListItemIcon>
                                                            <Icon />
                                                        </ListItemIcon>
                                                        <ListItemText>
                                                            Import presets from bank...
                                                        </ListItemText>

                                                    </MenuItem>

                                                    <Divider />
                                                    {this.state.selectedItems.size !== 0 && (
                                                        <MenuItem onClick={() => { this.handleDownloadPreset(); }} >
                                                            <ListItemIcon>
                                                                <DownloadIcon className={classes.listIcon} />
                                                            </ListItemIcon>
                                                            <ListItemText>
                                                                Download preset
                                                            </ListItemText>

                                                        </MenuItem>
                                                    )}
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
                                onLongPress={(e,item) => {
                                    if (e.pointerType === "touch") 
                                    {
                                        this.showTouchActionBar(true);
                                    }
                                }}
                            canDrag={!this.state.multiSelect && !this.state.showTouchActionBar}
                                onDragStart={(index, x, y) => { this.selectItemAtIndex(index) }}
                                moveElement={(from, to) => { this.moveElement(from, to); }}
                                scroll={ScrollDirection.Y}
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
                        onUploaded={(instanceId) => this.setSelection(instanceId)}
                        uploadAfter={this.state.currentItem}
                        open={this.state.openUploadPresetDialog}
                        onClose={() => { this.setState({ openUploadPresetDialog: false }) }} />
                    {this.state.importOpen && (
                        <ImportPresetFromBankDialog
                            open={this.state.importOpen}
                            onClose={() => this.setState({ importOpen: false })}
                            onOk={(bankInstanceId, presets) => this.handleImportDialogOk(bankInstanceId, presets)}
                        />
                    )}
                    {this.state.copyToOpen && (
                        <CopyPresetsToBankDialog
                            open={this.state.copyToOpen}
                            onClose={() => this.setState({ copyToOpen: false })}
                            onOk={(bankInstanceId) => this.handleCopyToBankDialogOk(bankInstanceId)}
                        />
                    )}

                </DialogEx>

            );

        }

    },
    styles
);

export default PresetDialog;