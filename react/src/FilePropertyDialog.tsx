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
import CreateNewFolderIcon from '@mui/icons-material/CreateNewFolder';
import RenameDialog from './RenameDialog';
import Divider from '@mui/material/Divider';
import MenuItem from '@mui/material/MenuItem';
import Menu from '@mui/material/Menu';
import MoreIcon from '@mui/icons-material/MoreVert';
import { PiPedalModel, PiPedalModelFactory, FileEntry, BreadcrumbEntry, FileRequestResult } from './PiPedalModel';
import { isDarkMode } from './DarkMode';
import Button from '@mui/material/Button';
import FileUploadIcon from '@mui/icons-material/FileUpload';
import AudioFileIcon from '@mui/icons-material/AudioFile';
import ArrowBackIcon from '@mui/icons-material/ArrowBack';
import FolderIcon from '@mui/icons-material/Folder';
import InsertDriveFileOutlinedIcon from '@mui/icons-material/InsertDriveFileOutlined';
import InsertDriveFileIcon from '@mui/icons-material/InsertDriveFile';
import DialogActions from '@mui/material/DialogActions';
import DialogTitle from '@mui/material/DialogTitle';
import DialogContent from '@mui/material/DialogContent';
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
import HomeIcon from '@mui/icons-material/Home';
import FilePropertyDirectorySelectDialog from './FilePropertyDirectorySelectDialog';



const styles = (theme: Theme) => createStyles({
    secondaryText: {
        color: theme.palette.text.secondary
    },
});

const audioFileExtensions: { [name: string]: boolean } = {
    ".wav": true,
    ".flac": true,
    ".ogg": true,
    ".aac": true,
    ".au": true,
    ".snd": true,
    ".mid": true,
    ".rmi": true,
    ".mp3": true,
    ".mp4": true,
    ".aif": true,
    ".aifc": true,
    ".aiff": true,
    ".ra": true
};

function isAudioFile(filename: string) {
    let npos = filename.lastIndexOf('.');
    let nposSlash = filename.lastIndexOf('/');
    if (nposSlash >= npos) {
        return false;
    }
    let extension = filename.substring(npos);
    return audioFileExtensions[extension] !== undefined;
}

export interface FilePropertyDialogProps extends WithStyles<typeof styles> {
    open: boolean,
    fileProperty: UiFileProperty,
    selectedFile: string,
    onOk: (fileProperty: UiFileProperty, selectedItem: string) => void,
    onCancel: () => void
};
export interface FilePropertyDialogState {
    fullScreen: boolean;
    selectedFile: string;
    selectedFileIsDirectory: boolean;
    selectedFileProtected: boolean;
    hasSelection: boolean;
    canDelete: boolean;
    fileResult: FileRequestResult;
    navDirectory: string;
    isProtectedDirectory: boolean;
    columns: number;
    columnWidth: number;
    openUploadFileDialog: boolean;
    openConfirmDeleteDialog: boolean;
    menuAnchorEl: null | HTMLElement;
    newFolderDialogOpen: boolean;
    renameDialogOpen: boolean;
    moveDialogOpen: boolean;
};

function pathExtension(path: string) {
    let dotPos = path.lastIndexOf('.');
    if (dotPos === -1) return "";

    let slashPos = path.lastIndexOf('/');
    if (slashPos !== -1) {
        if (dotPos <= slashPos + 1) return "";
    }
    return path.substring(dotPos); // include the '.'.

}
function pathParentDirectory(path: string) {
    let npos = path.lastIndexOf('/');
    if (npos === -1) return "";
    return path.substring(0, npos);
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
            if (selectedFile === "") {
                return "";
            }
            return pathParentDirectory(selectedFile);
        }

        constructor(props: FilePropertyDialogProps) {
            super(props);


            this.model = PiPedalModelFactory.getInstance();

            this.state = {
                fullScreen: this.getFullScreen(),
                selectedFile: props.selectedFile,
                selectedFileProtected: true,
                selectedFileIsDirectory: false,
                navDirectory: this.getNavDirectoryFromFile(props.selectedFile, props.fileProperty),
                hasSelection: false,
                canDelete: false,
                columns: 0,
                columnWidth: 1,
                fileResult: new FileRequestResult(),
                isProtectedDirectory: true,
                openUploadFileDialog: false,
                openConfirmDeleteDialog: false,
                menuAnchorEl: null,
                newFolderDialogOpen: false,
                renameDialogOpen: false,
                moveDialogOpen: false
            };
            this.requestScroll = true;
        }
        getFullScreen() {
            return window.innerWidth < 450 || window.innerHeight < 450;
        }

        private scrollRef: HTMLDivElement | null = null;

        onScrollRef(element: HTMLDivElement | null) {
            this.scrollRef = element;
            this.maybeScrollIntoView();
        }
        private maybeScrollIntoView() {
            if (this.scrollRef !== null && this.state.fileResult.files.length !== 0 && this.mounted && this.requestScroll) {
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
                .then((filesResult) => {
                    if (this.mounted) {
                        // let insertionPoint = files.length;
                        // for (let i = 0; i < files.length; ++i) {
                        //     if (!files[i].isDirectory) {
                        //         insertionPoint = i;
                        //         break;
                        //     }
                        // }
                        filesResult.files.splice(0, 0, { pathname: "", displayName: "<none>", isDirectory: false, isProtected: true });

                        let fileEntry = this.getFileEntry(filesResult.files, this.state.selectedFile);
                        this.setState({
                            isProtectedDirectory: filesResult.isProtected,
                            fileResult: filesResult,
                            hasSelection: !!fileEntry,
                            selectedFileProtected: fileEntry ? fileEntry.isProtected: true,
                            navDirectory: navPath
                        });
                    }
                }).catch((error) => {
                    if (!this.mounted) return;
                    if (navPath !== "") // deleted sample directory maybe?
                    {
                        this.navigate("");
                    } else {
                        this.model.showAlert(error.toString())
                    }
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
                fullScreen: this.getFullScreen()
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
                        selectedFileIsDirectory: false,
                        selectedFileProtected: true,
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
            for (var fileEntry of this.state.fileResult.files) {
                if (fileEntry.pathname === path) {
                    return fileEntry.isDirectory;
                }
            }
            return false;
        }
        private isFileInList(files: FileEntry[], file: string) {

            let hasSelection = false;
            if (file === "") return true;
            for (var listFile of files) {
                if (listFile.pathname === file) {
                    hasSelection = true;
                    break;
                }
            }
            return hasSelection;
        }
        private getFileEntry(files: FileEntry[], file: string): FileEntry | undefined {

            if (file === "") return undefined;
            for (var listFile of files) {
                if (listFile.pathname === file) {
                    return listFile;
                }
            }
            return undefined;
        }

        onSelectValue(fileEntry: FileEntry) {
            this.requestScroll = true;
            this.setState({
                selectedFile: fileEntry.pathname,
                selectedFileIsDirectory: fileEntry.isDirectory,
                selectedFileProtected: fileEntry.isProtected,
                hasSelection: this.isFileInList(this.state.fileResult.files, fileEntry.pathname)
            })
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
            if (this.state.selectedFileProtected) { 
                return; 
            }
            this.setState({ openConfirmDeleteDialog: true });
        }
        handleConfirmDelete() {
            if (this.state.selectedFileProtected) { 
                return; 
            }
            this.setState({ openConfirmDeleteDialog: false });
            if (this.state.hasSelection) {
                let selectedFile = this.state.selectedFile;
                let position = -1;
                for (let i = 0; i < this.state.fileResult.files.length; ++i) {
                    let file = this.state.fileResult.files[i];
                    if (file.pathname === selectedFile) {
                        position = i;
                    }
                }
                let newSelection: FileEntry | undefined = undefined;
                if (position >= 0 && position < this.state.fileResult.files.length - 1) {
                    newSelection = this.state.fileResult.files[position + 1];
                } else if (position === this.state.fileResult.files.length - 1) {
                    if (position !== 0) {
                        newSelection = this.state.fileResult.files[position - 1];
                    }

                }

                this.model.deleteUserFile(this.state.selectedFile)
                    .then(
                        () => {
                            if (newSelection)
                            {
                                this.setState({ 
                                    selectedFile: newSelection.pathname,
                                    selectedFileIsDirectory: newSelection.isDirectory,
                                    selectedFileProtected: newSelection.isProtected,
                                    hasSelection: true
                                });
                            } else {
                                this.setState({
                                    selectedFile: "",
                                    selectedFileIsDirectory: false,
                                    selectedFileProtected: true,
                                    hasSelection: false
                                });

                            }
                            this.requestFiles(this.state.navDirectory);
                        }
                    ).catch(
                        (e: any) => {
                            this.model.showAlert(e.toString());
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
                return (<Divider />);
            }
            let breadcrumbs: React.ReactElement[] = [(
                <Button variant="text"
                    color="inherit"
                    key="h" onClick={() => { this.navigate(""); }}
                    style={{ textTransform: "none", flexShrink: 0 }}
                    startIcon={(<HomeIcon sx={{ mr: 0.6 }} fontSize="inherit" />)}
                >
                    <Typography style={{}} variant="body2" noWrap>Home</Typography>
                </Button>
            )
            ];

            for (let i = 1; i < this.state.fileResult.breadcrumbs.length - 1; ++i) {
                let breadcrumb: BreadcrumbEntry = this.state.fileResult.breadcrumbs[i];

                breadcrumbs.push((
                    <Typography key={"x" + (i)} variant="body2" style={{ marginTop: 6, marginBottom: 6 }} noWrap>/</Typography>

                )
                );
                breadcrumbs.push((
                    <Button variant="text"
                        key={"bc" + i}
                        color="inherit"
                        onClick={() => { this.navigate(breadcrumb.pathname) }}
                        style={{ minWidth: 0, textTransform: "none", flexShrink: 0 }}
                    >
                        <Typography variant="body2" style={{}} noWrap>{breadcrumb.displayName}</Typography>


                    </Button>
                ));
            }
            if (this.state.fileResult.breadcrumbs.length > 1) {
                let lastdirectory = this.state.fileResult.breadcrumbs[this.state.fileResult.breadcrumbs.length - 1];
                breadcrumbs.push((
                    <Typography key={"xLast"} variant="body2" style={{ userSelect: "none", marginTop: 6, marginBottom: 6 }} noWrap>/</Typography>

                )
                );
                breadcrumbs.push((
                    <div key={"bc-last"} style={{ flexShrink: 0, paddingLeft: 8, paddingRight: 8, paddingTop: 6, paddingBottom: 6, overflowX: "clip" }}>
                        <Typography noWrap
                            style={{}} variant="body2"
                            color="text.secondary"> {lastdirectory.displayName}</Typography>
                    </div>
                ));

            }
            return (
                <div>

                    <div aria-label="breadcrumb"
                        style={{ marginLeft: 16, marginBottom: 8, marginRight: 8, textOverflow: "", display: "flex", flexFlow: "row wrap", alignItems: "center", justifyContent: "start" }}>
                        {breadcrumbs}
                    </div>
                    <Divider />
                </div>
            );
        }

        getDefaultPath(): string {
            try {
                let storage = window.localStorage;
                let result = storage.getItem("fpDefaultPath");
                if (result) {
                    return result;
                }
            } catch (e) {

            }

            return this.state.navDirectory;
        }
        setDefaultPath(path: string) {
            try {
                let storage = window.localStorage;
                storage.setItem("fpDefaultPath", path);
            } catch (e) {

            }
        }
        hasSelectedFileOrFolder(): boolean {
            return this.state.hasSelection && this.state.selectedFile !== "";
        }
        getFileExtensionList(uiFileProperty: UiFileProperty): string {
            let result = "";
            for (var fileType of uiFileProperty.fileTypes) {
                if (fileType.fileExtension !== "" && fileType.fileExtension !== ".zip") {
                    if (result !== "") result = result + ",";
                    result += fileType.fileExtension;
                }
            }
            return result;
        }
        private getIcon(fileEntry: FileEntry) {
            if (fileEntry.pathname === "") {
                return (<InsertDriveFileOutlinedIcon style={{ flex: "0 0 auto", opacity: 0.5, marginRight: 8, marginLeft: 8 }} />);
            }
            if (fileEntry.isDirectory) {
                return (
                    <FolderIcon style={{ flex: "0 0 auto", opacity: 0.7, marginRight: 8, marginLeft: 8 }} />
                );
            }
            if (isAudioFile(fileEntry.pathname)) {
                return (<AudioFileIcon style={{ flex: "0 0 auto", opacity: 0.7, marginRight: 8, marginLeft: 8 }} />);
            }
            return (<InsertDriveFileIcon style={{ flex: "0 0 auto", opacity: 0.7, marginRight: 8, marginLeft: 8 }} />);
        }

        render() {
            let classes = this.props.classes;
            let columnWidth = this.state.columnWidth;
            let okButtonText = "Select";
            if (this.state.hasSelection && this.state.selectedFileIsDirectory) {
                okButtonText = "Open";
            }
            let protectedDirectory = this.state.fileResult.isProtected;
            let protectedItem = true;
            if (this.state.hasSelection)
            {
                protectedItem = this.state.selectedFileProtected;
            }
            let canMoveOrRename = this.hasSelectedFileOrFolder() && !protectedItem;

            return this.props.open &&
                (
                    <DialogEx
                        fullScreen={this.state.fullScreen}
                        onClose={() => {
                            this.props.onCancel();
                        }}
                        onEnterKey={() => {
                            this.openSelectedFile();
                        }}
                        open={this.props.open} tag="fileProperty"
                        fullWidth maxWidth="md"
                        PaperProps={
                            this.state.fullScreen ?
                                {}
                                :
                                {
                                    style: {
                                        minHeight: "90%",
                                        maxHeight: "90%",
                                    }
                                }}
                    >
                        <DialogTitle style={{ paddingBottom: 0 }} >
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
                                    aria-label="new folder"
                                    edge="end"
                                    color="inherit"
                                    style={{ opacity: 0.6, marginRight: 8 }}
                                    onClick={(ev) => { this.onNewFolder(); }}
                                    disabled={protectedDirectory}
                                >
                                    <CreateNewFolderIcon />
                                </IconButton>


                                <IconButton
                                    aria-label="display more actions"
                                    edge="end"
                                    color="inherit"
                                    style={{ opacity: 0.6 }}
                                    onClick={(ev) => { this.handleMenuOpen(ev); }}
                                    disabled={protectedDirectory}

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
                                    {canMoveOrRename && (<Divider />)}
                                    {canMoveOrRename && (<MenuItem onClick={() => { this.handleMenuClose(); this.onMove(); }}>Move</MenuItem>)}
                                    {canMoveOrRename && this.hasSelectedFileOrFolder() && !protectedItem && (<MenuItem onClick={() => { this.handleMenuClose(); this.onRename(); }}>Rename</MenuItem>)}
                                </Menu>
                            </Toolbar>
                            <div style={{ flex: "0 0 auto", marginLeft: 0, marginRight: 0, overflowX: "clip" }}>
                                {this.renderBreadcrumbs()}
                            </div>
                        </DialogTitle>
                        <DialogContent style={{ paddingLeft: 0, paddingRight: 0, paddingTop: 0, colorScheme: (isDarkMode() ? "dark" : "light") }}>

                            <div
                                ref={(element) => this.onMeasureRef(element)}
                                style={{
                                    flex: "1 1 100%", display: "flex", flexFlow: "row wrap",
                                    justifyContent: "flex-start", alignContent: "flex-start", paddingLeft: 16, paddingBottom: 16
                                }}>
                                {
                                    (this.state.columns !== 0) && // don't render until we have number of columns derived from layout.
                                    this.state.fileResult.files.map(
                                        (value: FileEntry, index: number) => {
                                            let displayValue = value.displayName;
                                            if (displayValue === "") {
                                                displayValue = "<none>";
                                            }
                                            let selected = value.pathname === this.state.selectedFile;
                                            let selectBg = selected ? "rgba(0,0,0,0.15)" : "rgba(0,0,0,0.0)";
                                            if (isDarkMode()) {
                                                selectBg = selected ? "rgba(255,255,255,0.10)" : "rgba(0,0,0,0.0)";
                                            }
                                            let scrollRef: ((element: HTMLDivElement | null) => void) | undefined = undefined;
                                            if (selected) {
                                                scrollRef = (element) => { this.onScrollRef(element) };
                                            }

                                            return (
                                                <ButtonBase key={value.pathname}

                                                    style={{ width: columnWidth, flex: "0 0 auto", height: 48, position: "relative" }}
                                                    onClick={() => this.onSelectValue(value)}
                                                    onDoubleClick={() => { this.onDoubleClickValue(value.pathname); }}
                                                >
                                                    <div ref={scrollRef} style={{ position: "absolute", background: selectBg, width: "100%", height: "100%", borderRadius: 4 }} />
                                                    <div style={{ display: "flex", flexFlow: "row nowrap", textOverflow: "ellipsis", justifyContent: "start", alignItems: "center", width: "100%", height: "100%" }}>
                                                        {this.getIcon(value)}
                                                        <Typography noWrap className={classes.secondaryText} variant="body2" style={{ flex: "1 1 auto", textAlign: "left" }}>{displayValue}</Typography>
                                                    </div>
                                                </ButtonBase>
                                            );
                                        }
                                    )
                                }
                            </div>
                        </DialogContent>
                        <Divider />
                        <DialogActions style={{ justifyContent: "stretch" }}>
                            <div style={{ display: "flex", width: "100%", alignItems: "center", flexFlow: "row nowrap" }}>
                                <IconButton style={{ visibility: (this.state.hasSelection ? "visible" : "hidden") }} aria-label="delete" component="label" color="primary"
                                    disabled={!this.state.hasSelection || this.state.selectedFile === "" || protectedItem}
                                    onClick={() => this.handleDelete()} >
                                    <OldDeleteIcon fontSize='small' />
                                </IconButton>

                                <Button style={{ flex: "0 0 auto" }} aria-label="upload" variant="text" startIcon={<FileUploadIcon />}
                                    onClick={() => { this.setState({ openUploadFileDialog: true }) }} disabled={protectedDirectory}
                                >
                                    <div>Upload</div>
                                </Button>
                                <div style={{ flex: "1 1 auto" }}>&nbsp;</div>

                                <Button variant="dialogSecondary" onClick={() => { this.props.onCancel(); }} aria-label="cancel">
                                    Cancel
                                </Button>
                                <Button variant="dialogPrimary" style={{ flex: "0 0 auto" }}
                                    onClick={() => { this.openSelectedFile(); }}

                                    disabled={(!this.state.hasSelection) && this.state.selectedFile !== ""} aria-label="select"
                                >
                                    {okButtonText}
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
                            uploadPage={
                                "uploadUserFile?directory=" + encodeURIComponent(this.state.navDirectory)
                                + "&ext="
                                + encodeURIComponent(this.getFileExtensionList(this.props.fileProperty))
                            }
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
                                                ? this.state.selectedFile : ""}
                                        onClose={() => { this.setState({ moveDialogOpen: false }); }}
                                        onOk={
                                            (path) => {
                                                this.setState({ moveDialogOpen: false });
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
                this.requestFiles(this.state.selectedFile);
                this.setState({ navDirectory: this.state.selectedFile });
            } else {
                this.props.onOk(this.props.fileProperty, this.state.selectedFile);
            }
        }

        private renameDefaultName(): string {
            let name = this.state.selectedFile;
            if (name === "") return "";
            if (this.isDirectory(name)) {
                return pathFileName(name);
            } else {
                return pathFileNameOnly(name);
            }
        }
        private onMove(): void {
            this.setState({ moveDialogOpen: true });
        }
        private onExecuteMove(newDirectory: string) {
            let fileName = pathFileName(this.state.selectedFile);
            let oldFilePath = pathConcat(this.state.navDirectory, fileName);
            let newFilePath = pathConcat(newDirectory, fileName);

            this.model.renameFilePropertyFile(oldFilePath, newFilePath, this.props.fileProperty)
                .then(() => {
                    this.requestFiles(this.state.navDirectory);
                })
                .catch((e) => {
                    this.model.showAlert(e.toString());
                });

        }

        private onRename(): void {
            this.setState({ renameDialogOpen: true });
        }
        private onExecuteRename(newName: string) {
            let newPath: string = "";
            let oldPath: string = "";
            if (this.isDirectory(this.state.selectedFile)) {
                let oldName = pathFileName(this.state.selectedFile);
                if (oldName === newName) return;
                oldPath = pathConcat(this.state.navDirectory, oldName);
                newPath = pathConcat(this.state.navDirectory, newName);;
            } else {
                let oldName = pathFileNameOnly(this.state.selectedFile);
                if (oldName === newName) return;
                let extension = pathExtension(this.state.selectedFile);
                oldPath = pathConcat(this.state.navDirectory, oldName + extension);
                newPath = pathConcat(this.state.navDirectory, newName + extension);
            }
            this.model.renameFilePropertyFile(oldPath, newPath, this.props.fileProperty)
                .then((newPath) => {
                    this.setState({ selectedFile: newPath });
                    this.requestFiles(this.state.navDirectory);
                    this.requestScroll = true

                })
                .catch((e) => {
                    this.model.showAlert(e.toString());
                });
        }

        private onNewFolder(): void {
            this.setState({ newFolderDialogOpen: true });
        }

        private onExecuteNewFolder(newName: string) {
            this.model.createNewSampleDirectory(pathConcat(this.state.navDirectory, newName), this.props.fileProperty)
                .then((newPath) => {
                    this.setState({ 
                        selectedFile: newPath,
                        selectedFileIsDirectory: 
                        true,selectedFileProtected: false 
                    });
                    this.requestFiles(this.state.navDirectory);
                    this.requestScroll = true
                })
                .catch((e) => {
                    this.model.showAlert(e.toString());
                }
                );
        }
    });