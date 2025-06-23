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

import React, { Component } from 'react';
import IconButtonEx from './IconButtonEx';
import Typography from '@mui/material/Typography';
import { PiPedalModel, PiPedalModelFactory } from './PiPedalModel';
import { BankIndexEntry, BankIndex } from './Banks';
import Button from "@mui/material/Button";
import ButtonBase from "@mui/material/ButtonBase";
import Slide, { SlideProps } from '@mui/material/Slide';
import AppBar from '@mui/material/AppBar';
import Toolbar from '@mui/material/Toolbar';
import {createStyles} from './WithStyles';


import { Theme } from '@mui/material/styles';
import { withStyles } from "tss-react/mui";
import WithStyles from './WithStyles';

import DraggableGrid, { ScrollDirection } from './DraggableGrid';
import Fade from '@mui/material/Fade';

import SelectHoverBackground from './SelectHoverBackground';
import CloseIcon from '@mui/icons-material/Close';
import ArrowBackIcon from '@mui/icons-material/ArrowBack';
import EditIcon from '@mui/icons-material/Edit';
import RenameDialog from './RenameDialog';

import MoreVertIcon from '@mui/icons-material/MoreVert';
import ListItemIcon from '@mui/material/ListItemIcon';
import ListItemText from '@mui/material/ListItemText';
import Menu from '@mui/material/Menu';
import MenuItem from '@mui/material/MenuItem';
import DialogEx from './DialogEx';
import DialogContent from '@mui/material/DialogContent';
import DialogActions from '@mui/material/DialogActions';
import DownloadIcon from './svg/file_download_black_24dp.svg?react';
import UploadIcon from './svg/file_upload_black_24dp.svg?react';
import BankIcon from './svg/ic_bank.svg?react';
import { css } from '@emotion/react';

//import PublishIcon from '@mui/icons-material/Publish';
// import FileDownloadIcon from '@mui/icons-material/FileDownload';
// import AppsIcon from '@mui/icons-material/Apps';

interface BankDialogProps extends WithStyles<typeof styles> {
    show: boolean;
    isEditDialog: boolean;
    onDialogClose: () => void;

};

interface BankDialogState {
    banks: BankIndex;

    showActionBar: boolean;

    selectedItem: number;

    filenameDialogOpen: boolean;
    filenameSaveAs: boolean;
    moreMenuAnchorEl: HTMLElement | null;
    showDeletePrompt: boolean;

};


const styles = (theme: Theme) => createStyles({
    listIcon: css({
        width: 24, height: 24,
        opacity: 0.6, fill: theme.palette.text.primary
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


const BankDialog = withStyles(
    class extends Component<BankDialogProps, BankDialogState> {

        model: PiPedalModel;

        refUpload: React.RefObject<HTMLInputElement|null>;

        constructor(props: BankDialogProps) {
            super(props);

            this.refUpload = React.createRef();
            this.model = PiPedalModelFactory.getInstance();

            this.handleDialogClose = this.handleDialogClose.bind(this);
            let banks = this.model.banks.get();
            this.state = {
                banks: banks,
                showActionBar: false,
                selectedItem: banks.selectedBank,
                filenameDialogOpen: false,
                filenameSaveAs: false,
                moreMenuAnchorEl: null,
                showDeletePrompt: false
            };
            this.handleBanksChanged = this.handleBanksChanged.bind(this);

        }
        onMoreClick(e: React.SyntheticEvent): void {
            this.setState({ moreMenuAnchorEl: e.currentTarget as HTMLElement })
        }

        handleDownloadBank() {
            this.handleMoreClose();
            this.model.download("downloadBank", this.state.selectedItem);
        }

        async uploadFiles(fileList: FileList): Promise<number> {
            let uploadAfter = this.state.selectedItem;
            try {
                for (let i = 0; i < fileList.length; ++i) {
                    uploadAfter = await this.model.uploadBank(fileList[i], uploadAfter);
                }
            } catch (error) {
                this.model.showAlert(error + "");
            };
            return uploadAfter;
        }

        handleUpload(e: any) {
            if (!e.target.files) return;
            if (e.target.files.length === 0) return;
            var fileList = e.target.files;
            this.uploadFiles(fileList)
                .then((newSelection) => {
                    e.target.value = ""; // clear the file list so we can get another change notice.
                    this.setState({ selectedItem: newSelection });
                })
                .catch(err => {
                    e.target.value = ""; // clear the file list so we can get another change notice.
                    this.model.showAlert(err.toString());
                });
        }
        handleUploadBank() {
            if (this.refUpload.current) {
                this.refUpload.current.click();
            }
            this.handleMoreClose();

        }
        handleMoreClose(): void {
            this.setState({ moreMenuAnchorEl: null });

        }


        selectItemAtIndex(index: number) {
            let instanceId = this.state.banks.entries[index].instanceId;
            this.setState({ selectedItem: instanceId });
        }
        isEditMode() {
            return this.state.showActionBar || this.props.isEditDialog;
        }

        handleBanksChanged() {
            let banks = this.model.banks.get();

            if (!banks.areEqual(this.state.banks, false)) // avoid a bunch of peculiar effects if we update while a drag is in progress
            {
                // if we don't have a valid selection, then use the current bank.
                if (this.state.banks.getEntry(this.state.selectedItem) == null) {
                    this.setState({ banks: banks, selectedItem: banks.selectedBank });
                } else {
                    this.setState({ banks: banks });
                }
            }
        }

        mounted: boolean = false;

        hasHooks: boolean = false;


        componentDidUpdate() {
        }
        componentDidMount() {
            this.model.banks.addOnChangedHandler(this.handleBanksChanged);
            this.handleBanksChanged();
            // scroll selected item into view.
            this.mounted = true;
        }
        componentWillUnmount() {
            this.model.banks.removeOnChangedHandler(this.handleBanksChanged);
            this.mounted = false;
        }

        getSelectedIndex() {
            let instanceId = this.isEditMode() ? this.state.selectedItem : this.state.banks.selectedBank;
            let banks = this.state.banks;
            for (let i = 0; i < banks.entries.length; ++i) {
                if (banks.entries[i].instanceId === instanceId) return i;
            }
            return -1;
        }

        handleDeleteClick() {
            if (!this.state.selectedItem) return;
            this.setState({ showDeletePrompt: true });
        }
        handleDeletePromptClose() {
            this.setState({ showDeletePrompt: false });
        }
        handleDeletePromptOk() {
            this.handleDeletePromptClose();

            let selectedItem = this.state.selectedItem;
            if (selectedItem !== -1) {
                this.model.deleteBankItem(selectedItem)
                    .then((newSelection: number) => {
                        this.setState({
                            selectedItem: newSelection
                        });
                    })
                    .catch((error) => {
                        this.model.showAlert(error.toString());
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
                this.model.openBank(instanceId);
                this.props.onDialogClose();
            }
        }
        showActionBar(show: boolean): void {
            this.setState({ showActionBar: show });

        }


        mapElement(el: any): React.ReactNode {
            let bankEntry = el as BankIndexEntry;
            const classes = withStyles.getClasses(this.props);
            let selectedItem = this.isEditMode() ? this.state.selectedItem : this.state.banks.selectedBank;
            return (
                <div key={bankEntry.instanceId}  >

                    <ButtonBase style={{ width: "100%", height: 48 }}
                        onClick={() => this.handleItemClick(bankEntry.instanceId)}
                    >
                        <SelectHoverBackground selected={bankEntry.instanceId === selectedItem} showHover={true} />
                        <div className={classes.itemFrame}>
                            <div className={classes.listIcon}>
                                <BankIcon />
                            </div>
                            <div className={classes.itemLabel}>
                                <Typography noWrap variant="body2" color="textPrimary">
                                    {bankEntry.name}
                                </Typography>
                            </div>
                        </div>
                    </ButtonBase>
                </div>

            );
        }

        moveElement(from: number, to: number): void {
            let newBanks = this.state.banks.clone();
            newBanks.moveBank(from, to);
            this.setState({
                banks: newBanks,
                selectedItem: newBanks.entries[to].instanceId
            });
            this.model.moveBank(from, to)
                .catch((error) => {
                    this.model.showAlert(error.toString());
                });
        }

        getSelectedName(): string {
            let item = this.state.banks.getEntry(this.state.selectedItem);
            if (item) return item.name;
            return "";
        }

        handleRenameClick() {
            let item = this.state.banks.getEntry(this.state.selectedItem);
            if (item) {
                this.setState({ filenameDialogOpen: true, filenameSaveAs: false });
            }
        }
        handleRenameOk(text: string) {
            let item = this.state.banks.getEntry(this.state.selectedItem);
            if (!item) return;
            if (item.name !== text) {
                if (this.state.banks.hasName(text)) {
                    this.model.showAlert("A bank with that name already exists.");
                }
                this.model.renameBank(this.state.selectedItem, text)
                    .catch((error) => {
                        this.onError(error);
                    });
            }

            this.setState({ filenameDialogOpen: false });
        }
        handleSaveAsOk(text: string) {
            let item = this.state.banks.getEntry(this.state.selectedItem);
            if (item) {
                if (item.name !== text) {
                    if (this.state.banks.hasName(text)) {
                        this.model.showAlert("A bank with that name already exists.");
                        return;
                    }
                    this.model.saveBankAs(this.state.selectedItem, text)
                        .then((newSelection) => {
                            this.setState({ selectedItem: newSelection });
                        })
                        .catch((error) => {
                            this.onError(error);
                        });
                }
            }
            this.setState({ filenameDialogOpen: false });
        }

        handleCopy() {
            let item = this.state.banks.getEntry(this.state.selectedItem);
            if (item) {
                this.setState({ filenameDialogOpen: true, filenameSaveAs: true });
            }
        }

        onError(error: string): void {
            this.model?.showAlert(error);
        }

        getSelectedBankName() {
            try {
                return this.model.banks.get()?.getEntry(this.state.selectedItem)?.name??"";
            } catch (error) {
                return "";
            }
        }



        render() {
            const classes = withStyles.getClasses(this.props);

            let actionBarClass = this.props.isEditDialog ? classes.dialogAppBar : classes.dialogActionBar;
            let defaultSelectedIndex = this.getSelectedIndex();

            return (
                <DialogEx tag="bank" fullScreen open={this.props.show}
                    onClose={() => { this.handleDialogClose() }} TransitionComponent={Transition}
                    onEnterKey={()=>{}}
                    style={{ userSelect: "none" }}
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
                                        Banks
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
                                        Banks
                                    </Typography>
                                    {(this.state.banks.getEntry(this.state.selectedItem) != null)
                                        && (

                                            <div style={{ flex: "0 0 auto", display: "flex", flexFlow: "row nowrap", alignItems: "center" }}>
                                                <Button color="inherit" onClick={(e) => this.handleCopy()}>
                                                    Copy
                                                </Button>
                                                <Button color="inherit" onClick={() => this.handleRenameClick()}>
                                                    Rename
                                                </Button>
                                                <RenameDialog
                                                    open={this.state.filenameDialogOpen}
                                                    defaultName={this.getSelectedName()}
                                                    acceptActionName={this.state.filenameSaveAs ? "SAVE AS" : "RENAME"}
                                                    onClose={() => { this.setState({ filenameDialogOpen: false }) }}
                                                    onOk={(text: string) => {
                                                        if (this.state.filenameSaveAs) {
                                                            this.handleSaveAsOk(text);
                                                        } else {
                                                            this.handleRenameOk(text);
                                                        }
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
                                                    <MenuItem onClick={() => { this.handleDownloadBank(); }} >
                                                        <ListItemIcon>
                                                            <DownloadIcon className={classes.listIcon}
                                                            />
                                                        </ListItemIcon>
                                                        <ListItemText>
                                                            Download bank
                                                        </ListItemText>

                                                    </MenuItem>
                                                    <label htmlFor="bankDialog_UploadInput">
                                                        <MenuItem onClick={(e) => this.handleUploadBank()}>


                                                            <ListItemIcon >
                                                                <UploadIcon className={classes.listIcon} />
                                                            </ListItemIcon>
                                                            <ListItemText>
                                                                Upload bank
                                                            </ListItemText>

                                                        </MenuItem>
                                                    </label>
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
                                    this.state.banks.entries.map((element) => {
                                        return this.mapElement(element);
                                    })
                                }
                            </DraggableGrid>
                        </div>
                    </div>
                    <input
                        id="bankDialog_UploadInput"
                        ref={this.refUpload}
                        type="file"
                        accept=".piBank"
                        multiple
                        onChange={(e) => this.handleUpload(e)}
                        style={{ display: "none" }}
                    />

                    <DialogEx tag="deletePrompt" open={this.state.showDeletePrompt} onClose={() => this.handleDeletePromptClose()}
                        style={{ userSelect: "none" }}
                        onEnterKey={()=>{ this.handleDeletePromptOk() }}
                        >
                        <DialogContent>
                            <Typography>Are you sure you want to delete bank '{this.getSelectedBankName()}'?</Typography>
                        </DialogContent>
                        <DialogActions>
                            <Button variant="dialogSecondary" onClick={() => this.handleDeletePromptClose()} color="primary">
                                Cancel
                            </Button>
                            <Button variant="dialogPrimary" onClick={() => this.handleDeletePromptOk()} color="secondary" >
                                DELETE
                            </Button>

                        </DialogActions>
                    </DialogEx>

                </DialogEx >

            );

        }

    },
    styles
);

export default BankDialog;