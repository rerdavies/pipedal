// Copyright (c) 2023 Robin Davies
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

import React from 'react';
import { Theme, createStyles } from '@mui/material/styles';
import RenameDialog from './RenameDialog';
import Divider from '@mui/material/Divider';
import MenuItem from '@mui/material/MenuItem';
import Menu from '@mui/material/Menu';
import MoreIcon from '@mui/icons-material/MoreVert';
import { PiPedalModel, PiPedalModelFactory, FileEntry } from './PiPedalModel';
import Link from '@mui/material/Link';
import { isDarkMode } from './DarkMode';
import Button from '@mui/material/Button';
import FileUploadIcon from '@mui/icons-material/FileUpload';
import AudioFileIcon from '@mui/icons-material/AudioFile';
import ArrowBackIcon from '@mui/icons-material/ArrowBack';
import FolderIcon from '@mui/icons-material/Folder';
import InsertDriveFileOutlinedIcon from '@mui/icons-material/InsertDriveFileOutlined';
import DialogActions from '@mui/material/DialogActions';
import DialogTitle from '@mui/material/DialogTitle';
import IconButton from '@mui/material/IconButton';
import OldDeleteIcon from './OldDeleteIcon';
import Toolbar from '@mui/material/Toolbar';
import { withStyles, WithStyles } from '@mui/styles';


import ResizeResponsiveComponent from './ResizeResponsiveComponent';
import { UiFileProperty } from './Lv2Plugin';
import ButtonBase from '@mui/material/ButtonBase';
import Typography from '@mui/material/Typography';
import DialogEx from './DialogEx';
import UploadFileDialog from './UploadFileDialog';
import OkCancelDialog from './OkCancelDialog';
import Breadcrumbs from '@mui/material/Breadcrumbs';
import HomeIcon from '@mui/icons-material/Home';
import FilePropertyDirectorySelectDialog from './FilePropertyDirectorySelectDialog';



const styles = (theme: Theme) => createStyles({
    secondaryText: {
        color: theme.palette.text.secondary
    },
});


export interface FilePropertyDialogProps extends WithStyles<typeof styles>  {
    open: boolean,
    fileProperty: UiFileProperty,
    selectedFile: string,
    onOk: (fileProperty: UiFileProperty, selectedItem: string) => void,
    onCancel: () => void
};

export interface FilePropertyDialogState {
    fullScreen: boolean;
    selectedFile: string;
    navDirectory: string;
    hasSelection: boolean;
    canDelete: boolean;
    fileEntries: FileEntry[];
    columns: number;
    columnWidth: number;
    openUploadFileDialog: boolean;
    openConfirmDeleteDialog: boolean;
    menuAnchorEl: null | HTMLElement;
    newFolderDialogOpen: boolean;
    renameDialogOpen: boolean;
    moveDialogOpen: boolean;
};

function pathExtension(path: string)
{
    let dotPos = path.lastIndexOf('.');
    if (dotPos === -1) return "";

    let slashPos = path.lastIndexOf('/');
    if (slashPos !== -1)
    {
        if (dotPos <= slashPos+1) return "";
    }
    return path.substring(dotPos); // include the '.'.

}
function pathConcat(left: string, right: string) {
    if (left === "") return right;
    if (right === "") return left;
    if (left.endsWith('/')) {
        left = left.substring(0, left.length - 1);
    }
    if (right.startsWith("/")) {
        right = right.substring(1);
    }
    return left + "/" + right;
}
export function pathFileNameOnly(path: string): string {
    if (path === "..") return path;
    let slashPos = path.lastIndexOf('/');
    if (slashPos < 0) {
        slashPos = 0;
    } else {
        ++slashPos;
    }
    let extPos = path.lastIndexOf('.');
    if (extPos < 0 || extPos < slashPos) {
        extPos = path.length;
    }

    return path.substring(slashPos, extPos);
}

export function pathFileName(path: string): string {
    if (path === "..") return path;
    let slashPos = path.lastIndexOf('/');
    if (slashPos < 0) {
        slashPos = 0;
    } else {
        ++slashPos;
    }
    return path.substring(slashPos);
}


export default withStyles(styles, { withTheme: true })(
    class FilePropertyDialog extends ResizeResponsiveComponent<FilePropertyDialogProps, FilePropertyDialogState> {

    getNavDirectoryFromFile(selectedFile: string, fileProperty: UiFileProperty): string {

        // would be easier if we had the data root (/var/pipedal/audio_downloads, but could be different when we're debugging. :-/
        let nPos = selectedFile.indexOf("/" + fileProperty.directory + "/");
        if (nPos === -1) return "";
        let relativePath = selectedFile.substring(nPos + fileProperty.directory.length + 2);
        let segments = relativePath.split('/');

        let result = "";
        for (let i = 0; i < segments.length - 1; ++i) {
            if (result !== "") result += '/';
            result += segments[i];
        }
        return result;
    }

    constructor(props: FilePropertyDialogProps) {
        super(props);


        this.model = PiPedalModelFactory.getInstance();

        this.state = {
            fullScreen: false,
            selectedFile: props.selectedFile,
            navDirectory: this.getNavDirectoryFromFile(props.selectedFile, props.fileProperty),
            hasSelection: false,
            canDelete: false,
            columns: 0,
            columnWidth: 1,
            fileEntries: [],
            openUploadFileDialog: false,
            openConfirmDeleteDialog: false,
            menuAnchorEl: null,
            newFolderDialogOpen: false,
            renameDialogOpen: false,
            moveDialogOpen: false
        };
        this.requestScroll = true;
    }

    private scrollRef: HTMLDivElement | null = null;

    onScrollRef(element: HTMLDivElement | null) {
        this.scrollRef = element;
        this.maybeScrollIntoView();
    }
    private maybeScrollIntoView() {
        if (this.scrollRef !== null && this.state.fileEntries.length !== 0 && this.mounted && this.requestScroll) {
            this.requestScroll = false;
            let options: ScrollIntoViewOptions = { block: "nearest" };
            options.block = "nearest";

            this.scrollRef.scrollIntoView(options);
        }
    }
    private mounted: boolean = false;
    private model: PiPedalModel;

    private requestFiles(navPath: string) {
        if (!this.props.open) {
            return;
        }
        if (this.props.fileProperty.directory === "") {
            return;
        }

        this.model.requestFileList2(navPath, this.props.fileProperty)
            .then((files) => {
                if (this.mounted) {
                    // let insertionPoint = files.length;
                    // for (let i = 0; i < files.length; ++i) {
                    //     if (!files[i].isDirectory) {
                    //         insertionPoint = i;
                    //         break;
                    //     }
                    // }
                    files.splice(0, 0, { filename: "", isDirectory: false });
                    this.setState({ fileEntries: files, hasSelection: this.isFileInList(files, this.state.selectedFile), navDirectory: navPath });
                }
            }).catch((error) => {
                this.model.showAlert(error.toString())
            });
    }

    private lastDivRef: HTMLDivElement | null = null;

    onMeasureRef(div: HTMLDivElement | null) {
        this.lastDivRef = div;
        if (div) {
            let width = div.offsetWidth;
            if (width === 0) return;
            let columns = 1;
            let columnWidth = (width - 40) / columns;
            if (columns !== this.state.columns || columnWidth !== this.state.columnWidth) {
                this.setState({ columns: columns, columnWidth: columnWidth });
            }
        }
    }
    onWindowSizeChanged(width: number, height: number): void {

        this.setState({
            fullScreen: height < 200
        })
        if (this.lastDivRef !== null) {
            this.onMeasureRef(this.lastDivRef);
        }

    }

    private requestScroll: boolean = false;

    componentDidMount() {
        super.componentDidMount();
        this.mounted = true;
    }
    componentWillUnmount() {
        super.componentWillUnmount();
        this.mounted = false;
        this.lastDivRef = null;
    }
    componentDidUpdate(prevProps: Readonly<FilePropertyDialogProps>, prevState: Readonly<FilePropertyDialogState>, snapshot?: any): void {
        super.componentDidUpdate?.(prevProps, prevState, snapshot);
        if (prevProps.open !== this.props.open || prevProps.fileProperty !== this.props.fileProperty || prevProps.selectedFile !== this.props.selectedFile) {
            if (this.props.open) {
                let navDirectory = this.getNavDirectoryFromFile(this.props.selectedFile, this.props.fileProperty);
                this.setState({ 
                    selectedFile: this.props.selectedFile,
                    newFolderDialogOpen: false,
                    renameDialogOpen: false,
                    moveDialogOpen: false
                    });
                this.requestFiles(navDirectory)
                this.requestScroll = true;
            }
        }

    }

    private isDirectory(path: string): boolean {
        for (var fileEntry of this.state.fileEntries) {
            if (fileEntry.filename === path) {
                return fileEntry.isDirectory;
            }
        }
        return false;
    }
    private isFileInList(files: FileEntry[], file: string) {

        let hasSelection = false;
        if (file === "") return true;
        for (var listFile of files) {
            if (listFile.filename === file) {
                hasSelection = true;
                break;
            }
        }
        return hasSelection;
    }

    onSelectValue(selectedFile: string) {
        this.requestScroll = true;
        this.setState({ selectedFile: selectedFile, hasSelection: this.isFileInList(this.state.fileEntries, selectedFile) })
    }
    onDoubleClickValue(selectedFile: string) {
        this.openSelectedFile();
    }

    handleMenuOpen(event: React.MouseEvent<HTMLElement>) {
        this.setState({ menuAnchorEl: event.currentTarget });

    }
    handleMenuClose() {
        this.setState({ menuAnchorEl: null });
    }
    handleDelete() {
        this.setState({ openConfirmDeleteDialog: true });
    }
    handleConfirmDelete() {
        this.setState({ openConfirmDeleteDialog: false });
        if (this.state.hasSelection) {
            let selectedFile = this.state.selectedFile;
            let position = -1;
            for (let i = 0; i < this.state.fileEntries.length; ++i) {
                let file = this.state.fileEntries[i];
                if (file.filename === selectedFile) {
                    position = i;
                }
            }
            let newSelection = "";
            if (position >= 0 && position < this.state.fileEntries.length - 1) {
                newSelection = this.state.fileEntries[position + 1].filename;
            } else if (position === this.state.fileEntries.length - 1) {
                if (position !== 0) {
                    newSelection = this.state.fileEntries[position - 1].filename;
                }

            }

            this.model.deleteUserFile(this.state.selectedFile)
                .then(
                    () => {
                        this.setState({ selectedFile: newSelection, hasSelection: newSelection !== "" });
                        this.requestFiles(this.state.navDirectory);
                    }
                ).catch(
                    (e: any) => {
                        this.model.showAlert(e + "");
                    }
                );
        }
    }

    private wantsScrollRef: boolean = true;

    navigate(relativeDirectory: string) {
        this.requestFiles(relativeDirectory);
        this.setState({ navDirectory: relativeDirectory });

    }
    renderBreadcrumbs() {
        if (this.state.navDirectory === "") {
            return (<div style={{ height: 0 }} />);
        }
        let breadcrumbs: React.ReactElement[] = [(
            <Link underline="hover"
                color="inherit"
                key="1" onClick={() => { this.navigate(""); }}
                sx={{ display: 'flex', alignItems: 'center' }}
            >
                <HomeIcon sx={{ mr: 0.6 }} fontSize="inherit" />
                Home
            </Link>
        )
        ];
        let directories = this.state.navDirectory.split("/");
        let target = "";
        for (let i = 0; i < directories.length - 1; ++i) {
            target = pathConcat(target, directories[i]);
            let myTarget = target;
            breadcrumbs.push((
                <Link underline="hover" key={(i + 1).toString()}
                    color="inherit" onClick={() => { this.navigate(myTarget) }}>
                    {directories[i]}
                </Link>
            ));
        }
        {
            let lastdirectory = directories[directories.length - 1];
            breadcrumbs.push((
                <Typography noWrap key={directories.length.toString()} color="text.secondary"> {lastdirectory}</Typography>
            ));

        }
        return (
            <Breadcrumbs separator='/' aria-label="breadcrumb" style={{ marginLeft: 24 }}>
                {breadcrumbs}
            </Breadcrumbs>
        );
    }

    getDefaultPath(): string {
        try {
            let storage = window.localStorage;
            let result = storage.getItem("fpDefaultPath");
            if (result)
            {
                return result;
            }
        } catch (e)
        {

        }

        return this.state.navDirectory;
    }
    setDefaultPath(path: string) {
        try {
            let storage = window.localStorage;
            storage.setItem("fpDefaultPath",path);
        } catch(e) {

        }
    }
    hasSelectedFileOrFolder(): boolean {
        return this.state.hasSelection && this.state.selectedFile !== "";
    }
    render() {
        let classes = this.props.classes;
        let columnWidth = this.state.columnWidth;
        return this.props.open &&
            (
                <DialogEx onClose={() => this.props.onCancel()} open={this.props.open} tag="fileProperty" fullWidth maxWidth="xl" style={{ height: "90%" }}
                    PaperProps={{ style: { minHeight: "90%", maxHeight: "90%", overflowY: "visible" } }}
                >
                    <DialogTitle >
                        <Toolbar style={{ padding: 0 }}>
                            <IconButton
                                edge="start"
                                color="inherit"
                                aria-label="back"
                                style={{ opacity: 0.6 }}
                                onClick={() => { this.props.onCancel(); }}
                            >
                                <ArrowBackIcon style={{ width: 24, height: 24 }} />
                            </IconButton>
                            <Typography noWrap component="div" sx={{ flexGrow: 1 }}>
                                {this.props.fileProperty.label}
                            </Typography>
                            <IconButton
                                aria-label="display more actions"
                                edge="end"
                                color="inherit"
                                style={{ opacity: 0.6 }}
                                onClick={(ev) => { this.handleMenuOpen(ev); }}
                            >
                                <MoreIcon />
                            </IconButton>
                            <Menu
                                id="menu-appbar"
                                anchorEl={this.state.menuAnchorEl}
                                anchorOrigin={{
                                    vertical: 'bottom',
                                    horizontal: 'right',
                                }}
                                keepMounted
                                transformOrigin={{
                                    vertical: 'top',
                                    horizontal: 'right',
                                }}
                                open={this.state.menuAnchorEl !== null}
                                onClose={() => { this.handleMenuClose(); }}
                            >
                                <MenuItem onClick={() => { this.handleMenuClose(); this.onNewFolder(); }}>New folder</MenuItem>
                                {this.hasSelectedFileOrFolder() && (<Divider />)}
                                {this.hasSelectedFileOrFolder() && (<MenuItem onClick={() => { this.handleMenuClose();this.onMove(); }}>Move</MenuItem>)}
                                {this.hasSelectedFileOrFolder() && (<MenuItem onClick={() => { this.handleMenuClose();this.onRename(); }}>Rename</MenuItem>)}
                            </Menu>
                        </Toolbar>
                        <div style={{ flex: "0 0 auto", marginLeft: "auto", marginRight: "auto" }}>
                            {this.renderBreadcrumbs()}
                        </div>
                    </DialogTitle>
                    <div style={{ flex: "0 0 auto", height: "1px", background: isDarkMode() ? "rgba(255,255,255,0.1" : "rgba(0,0,0,0.2" }}>&nbsp;</div>
                    <div style={{ flex: "1 1 auto", height: "300", display: "flex", flexFlow: "row wrap", overflowX: "hidden", overflowY: "auto" }}>
                        <div
                            ref={(element) => this.onMeasureRef(element)}
                            style={{
                                flex: "1 1 100%", display: "flex", flexFlow: "row wrap",
                                justifyContent: "flex-start", alignContent: "flex-start", paddingLeft: 16, paddingTop: 16, paddingBottom: 16
                            }}>
                            {
                                (this.state.columns !== 0) && // don't render until we have number of columns derived from layout.
                                this.state.fileEntries.map(
                                    (value: FileEntry, index: number) => {
                                        let displayValue = value.filename;
                                        if (displayValue === "") {
                                            displayValue = "<none>";
                                        } else {
                                            displayValue = pathFileNameOnly(displayValue);
                                        }
                                        let selected = value.filename === this.state.selectedFile;
                                        let selectBg = selected ? "rgba(0,0,0,0.15)" : "rgba(0,0,0,0.0)";
                                        if (isDarkMode()) {
                                            selectBg = selected ? "rgba(255,255,255,0.10)" : "rgba(0,0,0,0.0)";
                                        }
                                        let scrollRef: ((element: HTMLDivElement | null) => void) | undefined = undefined;
                                        if (selected) {
                                            scrollRef = (element) => { this.onScrollRef(element) };
                                        }

                                        return (
                                            <ButtonBase key={value.filename}

                                                style={{ width: columnWidth, flex: "0 0 auto", height: 48, position: "relative" }}
                                                onClick={() => this.onSelectValue(value.filename)} onDoubleClick={() => { this.onDoubleClickValue(value.filename); }}
                                            >
                                                <div ref={scrollRef} style={{ position: "absolute", background: selectBg, width: "100%", height: "100%", borderRadius: 4 }} />
                                                <div style={{ display: "flex", flexFlow: "row nowrap", textOverflow: "ellipsis", justifyContent: "start", alignItems: "center", width: "100%", height: "100%" }}>
                                                    {value.filename === "" ?
                                                        (<InsertDriveFileOutlinedIcon style={{ flex: "0 0 auto", opacity: 0.7, marginRight: 8, marginLeft: 8 }} />)
                                                        : (
                                                            value.isDirectory ? (
                                                                <FolderIcon style={{ flex: "0 0 auto", opacity: 0.7, marginRight: 8, marginLeft: 8 }} />
                                                            ) : (
                                                                <AudioFileIcon style={{ flex: "0 0 auto", opacity: 0.7, marginRight: 8, marginLeft: 8 }} />
                                                            ))}
                                                    <Typography noWrap className={classes.secondaryText} variant="body2" style={{ flex: "1 1 auto", textAlign: "left" }}>{displayValue}</Typography>
                                                </div>
                                            </ButtonBase>
                                        );
                                    }
                                )
                            }
                        </div>
                    </div>
                    <div style={{ flex: "0 0 auto", height: "1px", background: "rgba(0,0,0,0.2" }}>&nbsp;</div>
                    <DialogActions style={{ justifyContent: "stretch" }}>
                        <div style={{ display: "flex", width: "100%", alignItems: "center", flexFlow: "row nowrap" }}>
                            <IconButton style={{ visibility: (this.state.hasSelection ? "visible" : "hidden") }} aria-label="delete" component="label" color="primary"
                                disabled={!this.state.hasSelection || this.state.selectedFile === ""}
                                onClick={() => this.handleDelete()} >
                                <OldDeleteIcon fontSize='small' />
                            </IconButton>

                            <Button style={{ flex: "0 0 auto" }} aria-label="upload" variant="text" startIcon={<FileUploadIcon />}
                                onClick={() => { this.setState({ openUploadFileDialog: true }) }}
                            >
                                <div>Upload</div>
                            </Button>
                            <div style={{ flex: "1 1 auto" }}>&nbsp;</div>

                            <Button onClick={() => { this.props.onCancel(); }} aria-label="cancel">
                                Cancel
                            </Button>
                            <Button style={{ flex: "0 0 auto" }}
                                onClick={() => { this.openSelectedFile(); }}
                                color="secondary"
                                disabled={(!this.state.hasSelection) && this.state.selectedFile !== ""} aria-label="select"
                            >
                                Select
                            </Button>
                        </div>
                    </DialogActions>
                    <UploadFileDialog
                        open={this.state.openUploadFileDialog}
                        onClose=
                        {
                            () => {
                                this.setState({ openUploadFileDialog: false });
                            }
                        }
                        uploadPage={"uploadUserFile?directory=" + encodeURIComponent(this.props.fileProperty.directory)}
                        onUploaded={() => { this.requestFiles(this.state.navDirectory); }}
                        fileProperty={this.props.fileProperty}


                    />
                    <OkCancelDialog open={this.state.openConfirmDeleteDialog}
                        text={
                            (this.isDirectory(this.state.selectedFile))
                                ? "Are you sure you want to delete the selected directory and all its contents?"
                                : "Are you sure you want to delete the selected file?"}
                        okButtonText="Delete"
                        onOk={() => this.handleConfirmDelete()}
                        onClose={() => {
                            this.setState({ openConfirmDeleteDialog: false });
                        }}

                    />
                    {
                        this.state.newFolderDialogOpen && (
                            <RenameDialog open={this.state.newFolderDialogOpen} defaultName=""
                                onOk={(newName) => { this.setState({ newFolderDialogOpen: false }); this.onExecuteNewFolder(newName) }}
                                onClose={() => { this.setState({ newFolderDialogOpen: false }); }}
                                acceptActionName="OK"
                            />
                        )
                    }
                    {
                        this.state.renameDialogOpen && (
                            <RenameDialog open={this.state.renameDialogOpen} defaultName={this.renameDefaultName()}
                                onOk={(newName) => { this.setState({ renameDialogOpen: false }); this.onExecuteRename(newName) }}
                                onClose={() => { this.setState({ renameDialogOpen: false }); }}
                                acceptActionName="OK"
                            />

                        )
                    }
                    {
                        this.state.moveDialogOpen && (
                            (
                                <FilePropertyDirectorySelectDialog 
                                    open={this.state.moveDialogOpen}
                                    uiFileProperty={this.props.fileProperty}
                                    defaultPath={this.getDefaultPath()}
                                    excludeDirectory={
                                        this.isDirectory(this.state.selectedFile) 
                                        ? pathConcat(this.state.navDirectory,pathFileName(this.state.selectedFile)): ""}
                                    onClose={() => {this.setState({moveDialogOpen: false});}}
                                    onOk={
                                        (path) => { 
                                            this.setState({moveDialogOpen: false});
                                            this.setDefaultPath(path);
                                            this.onExecuteMove(path);
                                        }
                                    }
                                        
                                />
                            )
                        )
                    }
                </DialogEx>
            );
    }
    openSelectedFile(): void {
        if (this.isDirectory(this.state.selectedFile)) {
            let directoryName = pathFileNameOnly(this.state.selectedFile);
            let navDirectory = pathConcat(this.state.navDirectory, directoryName);
            this.requestFiles(navDirectory);
            this.setState({ navDirectory: navDirectory });
        } else {
            this.props.onOk(this.props.fileProperty, this.state.selectedFile);
        }
    }

    private renameDefaultName(): string {
        let name = this.state.selectedFile;
        if (name === "") return "";
        if (this.isDirectory(name))
        {
            return pathFileName(name);
        } else {
            return pathFileNameOnly(name);
        }
    }
    private onMove(): void {
        this.setState({ moveDialogOpen: true });
    }
    private onExecuteMove(newDirectory: string)
    {
        let fileName = pathFileName(this.state.selectedFile);
        let oldFilePath = pathConcat(this.state.navDirectory,fileName);
        let newFilePath = pathConcat(newDirectory,fileName);

        this.model.renameFilePropertyFile(oldFilePath,newFilePath,this.props.fileProperty)
        .then(()=>{
            this.requestFiles(this.state.navDirectory);
        })
        .catch((e)=>{
            this.model.showAlert(e.toString());
        });

    }

    private onRename(): void {
        this.setState({ renameDialogOpen: true });
    }
    private onExecuteRename(newName: string) {
        let newPath: string = "";
        let oldPath: string = "";
        if (this.isDirectory(this.state.selectedFile))
        {
            let oldName = pathFileName(this.state.selectedFile);
            if (oldName === newName) return;
            oldPath = pathConcat(this.state.navDirectory,oldName);
            newPath = pathConcat(this.state.navDirectory,newName);;
        } else {
            let oldName = pathFileNameOnly(this.state.selectedFile);
            if (oldName === newName) return;
            let extension = pathExtension(this.state.selectedFile);
            oldPath = pathConcat(this.state.navDirectory,oldName+extension);
            newPath = pathConcat(this.state.navDirectory,newName + extension);
        }
        this.model.renameFilePropertyFile(oldPath,newPath,this.props.fileProperty)
        .then((newPath)=>{
            this.setState({selectedFile: newPath});
            this.requestFiles(this.state.navDirectory);
            this.requestScroll = true
        
        })
        .catch((e) =>{
            this.model.showAlert(e.toString());
        });
    }

    private onNewFolder(): void {
        this.setState({ newFolderDialogOpen: true });
    }

    private onExecuteNewFolder(newName: string) {
        this.model.createNewSampleDirectory(pathConcat(this.state.navDirectory,newName), this.props.fileProperty)
            .then((newPath) => {
                this.setState({selectedFile: newPath});
                this.requestFiles(this.state.navDirectory);
                this.requestScroll = true
            })
            .catch((e) => {
                this.model.showAlert(e.toString());
            }
            );
    }
});