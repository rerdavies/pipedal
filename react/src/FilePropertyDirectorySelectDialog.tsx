// Copyright (c) 2024 Robin Davies
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
import Button from '@mui/material/Button';
import DialogEx from './DialogEx';
import DialogActions from '@mui/material/DialogActions';
import DialogContent from '@mui/material/DialogContent';
import DialogTitle from '@mui/material/DialogTitle';
import ResizeResponsiveComponent from './ResizeResponsiveComponent';
import ArrowRightIcon from '@mui/icons-material/ArrowRight';
import ArrowDropDownIcon from '@mui/icons-material/ArrowDropDown';
import { PiPedalModel, PiPedalModelFactory } from './PiPedalModel';
import { UiFileProperty } from './Lv2Plugin';
import FilePropertyDirectoryTree from './FilePropertyDirectoryTree';
import ButtonBase from '@mui/material/ButtonBase';
import Typography from '@mui/material/Typography'
import Divider from '@mui/material/Divider';
import FolderIcon from '@mui/icons-material/Folder';
import HomeIcon from '@mui/icons-material/Home';
import { isDarkMode } from './DarkMode';
import IconButton from '@mui/material/IconButton';


class DirectoryTree {
    name: string = "";
    path: string = "";
    expanded: boolean = false;
    isProtected: boolean = true;
    children: DirectoryTree[] = [];

    constructor(tree: FilePropertyDirectoryTree, parentTree: (DirectoryTree | null) = null) {
        this.name = tree.directoryName;
        this.path = tree.directoryName;
        this.name = tree.displayName;
        this.isProtected = tree.isProtected;
        for (var treeChild of tree.children) {
            this.children.push(new DirectoryTree(treeChild, this));
        }
    }
    canExpand(): boolean {
        return this.children.length !== 0;
    }
    expand(path: string): void {
        this.expanded = true;

        let currentNode: DirectoryTree = this;
        for (var segment of path.split('/')) {
            let found = false;
            for (var child of currentNode.children) {
                if (child.name === segment) {
                    found = true;
                    if (child.children.length !== 0) {
                        child.expanded = true;
                    }
                    currentNode = child;
                    break;
                }
            }
            if (!found) {
                break;
            }
        }
    }
    private static remove_(directoryTree: DirectoryTree, path: string): boolean
    {
        for (let i = 0; i < directoryTree.children.length; ++i)
        {
            let child = directoryTree.children[i];
            if (child.path === path)
            {
                directoryTree.children.splice(i,1)
                return true;
            }
            if (DirectoryTree.remove_(child,path))
            {
                return true;
            }
        }
        return false;
    }
    remove(path: string): boolean
    {
        return DirectoryTree.remove_(this,path);
    }
    find(path: string): DirectoryTree | null {
        let currentNode: DirectoryTree = this;
        for (var segment of path.split('/')) {
            let found = false;
            for (var child of currentNode.children) {
                if (child.name === segment) {
                    found = true;
                    currentNode = child;
                    break;
                }
            }
            if (!found) {
                return null;
            }
        }
        return currentNode;

    }
}

export interface FilePropertyDirectorySelectDialogProps {
    open: boolean,
    uiFileProperty: UiFileProperty;
    defaultPath: string,
    excludeDirectory: string,
    onOk: (path: string) => void,
    onClose: () => void
};

export interface FilePropertyDirectorySelectDialogState {
    fullScreen: boolean;
    selectedPath: string;
    directoryTree?: DirectoryTree;
    hasSelection: boolean;
    isProtected: boolean;
    directoryTreeInvalidatecount: number;

};

export default class FilePropertyDirectorySelectDialog extends ResizeResponsiveComponent<FilePropertyDirectorySelectDialogProps, FilePropertyDirectorySelectDialogState> {

    private model: PiPedalModel;
    private refSelected: React.Ref<HTMLElement>;

    constructor(props: FilePropertyDirectorySelectDialogProps) {
        super(props);
        this.model = PiPedalModelFactory.getInstance();
        this.state = {
            fullScreen: false,
            selectedPath: props.defaultPath,
            directoryTree: undefined,
            hasSelection: false,
            isProtected: true,
            directoryTreeInvalidatecount: 0

        };
        this.refSelected = React.createRef<HTMLElement>();

        this.requestDirectoryTree();
    }
    mounted: boolean = false;



    onWindowSizeChanged(width: number, height: number): void {
        this.setState({ fullScreen: height < 200 })
    }


    componentDidMount() {
        super.componentDidMount();
        this.mounted = true;
    }
    componentWillUnmount() {
        super.componentWillUnmount();
        this.mounted = false;
    }

    private requestScroll = true;
    requestDirectoryTree() {
        if (!this.props.open) return;

        this.model.getFilePropertyDirectoryTree(this.props.uiFileProperty)
            .then((filePropertyDirectoryTree) => {
                let myTree = new DirectoryTree(filePropertyDirectoryTree);
                if (this.props.excludeDirectory.length !== 0)
                {
                    myTree.remove(this.props.excludeDirectory);
                }
                myTree.expand(this.state.selectedPath);

                let selection = myTree.find(this.state.selectedPath);
                let hasSelection = !!selection;
                this.setState({ 
                    directoryTree: myTree, 
                    hasSelection: hasSelection,
                    isProtected: selection ? selection.isProtected : false
                 });
                this.requestScroll = true;
            })
            .catch((e) => {
                this.model.showAlert(e.toString());
                this.setState({ hasSelection: false });
            });

    }

    componentDidUpdate(prevProps: Readonly<FilePropertyDirectorySelectDialogProps>, prevState: Readonly<FilePropertyDirectorySelectDialogState>, snapshot: any): void {
        super.componentDidUpdate?.(prevProps, prevState, snapshot);

        if (prevProps.open !== this.props.open) {
            if (this.props.open) {
                this.setState({ selectedPath: this.props.defaultPath, hasSelection: false });
                this.requestDirectoryTree();
                this.requestScroll = true;
            }
        }
    }


    onOK() {
        if (this.state.isProtected) {
            return;
        }
        if (this.state.hasSelection) {
            this.props.onOk(this.state.selectedPath);
        }
    }
    onClose() {
        this.props.onClose();
    }
    private onToggleExpand(directoryTree: DirectoryTree)
    {
        directoryTree.expanded = !directoryTree.expanded;
        this.setState({directoryTreeInvalidatecount: this.state.directoryTreeInvalidatecount+1});
    }
    private onTreeClick(directoryTree: DirectoryTree) {
        if (!this.state.directoryTree) return;
        this.state.directoryTree.expand(directoryTree.path);
        this.setState({ 
            selectedPath: directoryTree.path, 
            hasSelection: true, 
            isProtected: directoryTree.isProtected,
            directoryTreeInvalidatecount: this.state.directoryTreeInvalidatecount + 1 });
    }
    private renderTree_(directoryTree: DirectoryTree) {
        let ref: ((element: HTMLButtonElement | null) => void) | undefined = undefined;
        let selected = directoryTree.path === this.state.selectedPath;
        if (selected) {
            ref = (element) => {
                if (this.requestScroll) {
                    this.requestScroll = false;
                }

            }
        }
        let selectBg = selected ? "rgba(0,0,0,0.15)" : "rgba(0,0,0,0.0)";
        if (isDarkMode()) {
            selectBg = selected ? "rgba(255,255,255,0.10)" : "rgba(0,0,0,0.0)";
        }
        let showToggle = directoryTree.canExpand() && directoryTree.path.length !== 0;
        return (
            <div key={directoryTree.path} style={{flexGrow: 1}}>
                <div style={{ width: "100%", display: "flex", flexFlow: "row", flexWrap: "nowrap", justifyContent: "flex-start", alignItems: "center" }}>
                    <IconButton onClick={()=>{ this.onToggleExpand(directoryTree); }} 
                        style={{opacity: showToggle ? 1: 0}}
                        disabled={!showToggle}
                    >
                        {
                            directoryTree.expanded &&
                            (
                                <ArrowDropDownIcon />
                            )
                        }
                        {
                            !directoryTree.expanded &&
                            (
                                <ArrowRightIcon/>
                            )
                        }
                    </IconButton>
                    <ButtonBase ref={ref} style={{ flexGrow: 1, flexShrink: 1, position: "relative" }} onClick={() => { this.onTreeClick(directoryTree); }}>
                        <div style={{ position: "absolute", background: selectBg, width: "100%", height: "100%", borderRadius: 4 }} />
                        <div style={{ width: "100%", display: "flex", flexFlow: "row", flexWrap: "nowrap", justifyContent: "flex-start", alignItems: "center", paddingLeft: 8 }}>
                            {
                                directoryTree.path === "" ? (
                                    <HomeIcon fontSize="small" />
                                ) : (
                                    <FolderIcon />
                                )
                            }

                            <Typography noWrap style={{ marginLeft: 8, marginTop: 8, marginBottom: 8, textAlign: "left", flexGrow: 1, flexShrink: 1 }} variant="body2">{directoryTree.name === "" ? "Home" : directoryTree.name}</Typography>
                        </div>
                    </ButtonBase>
                </div>
                {directoryTree.expanded
                    && directoryTree.canExpand()
                    && (
                        <div style={{ marginLeft: 32 }}>
                            {
                                directoryTree.children.map(
                                    (child, index) => {
                                        return this.renderTree_(child);
                                    }
                                )
                            }
                        </div>
                    )
                }
            </div>
        );
    }
    private renderTree() {
        if (!this.state.directoryTree) { return undefined; }
        return this.renderTree_(this.state.directoryTree);
    }
    render() {


        return (
            <DialogEx tag="fpDirectorySelect" open={this.props.open} fullWidth onClose={() => { this.onClose(); }} aria-labelledby="Rename-dialog-title"
                fullScreen={this.state.fullScreen}
                style={{ userSelect: "none" }}
                onEnterKey={()=>{ this.onOK(); }}
                PaperProps={{ style: { minHeight: "80%", maxHeight: "80%", minWidth: "75%", maxWidth: "75%", overflowY: "visible" } }}
            >
                <DialogTitle>
                    <Typography variant="body1">Select folder</Typography>
                </DialogTitle>
                <Divider />
                <DialogContent style={{marginLeft: 0, paddingLeft: 0}} >
                    <div style={{ display: "flex", flexGrow: 1, flexShrink: 1, overflowY: "auto" }} >
                        {
                            this.renderTree()
                        }
                    </div>
                </DialogContent>
                <Divider />
                <DialogActions>
                    <Button variant="dialogSecondary"  onClick={() => { this.onClose(); }} color="primary">
                        Cancel
                    </Button>
                    <Button variant="dialogPrimary" onClick={() => { this.onOK(); }} color="secondary" 
                        disabled={(!this.state.hasSelection) || this.state.isProtected} >
                        OK
                    </Button>
                </DialogActions>
            </DialogEx>
        );
    }
}