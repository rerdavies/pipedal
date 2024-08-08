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

import { PiPedalModel, PiPedalModelFactory } from './PiPedalModel';


import Button from '@mui/material/Button';
import FileUploadIcon from '@mui/icons-material/FileUpload';
import AudioFileIcon from '@mui/icons-material/AudioFile';
import ArrowBackIcon from '@mui/icons-material/ArrowBack';

import DialogActions from '@mui/material/DialogActions';
import DialogTitle from '@mui/material/DialogTitle';
import IconButton from '@mui/material/IconButton';
import OldDeleteIcon from './OldDeleteIcon';

import ResizeResponsiveComponent from './ResizeResponsiveComponent';
import { UiFileProperty } from './Lv2Plugin';
import ButtonBase from '@mui/material/ButtonBase';
import Typography from '@mui/material/Typography';
import DialogEx from './DialogEx';
import UploadFileDialog from './UploadFileDialog';
import OkCancelDialog from './OkCancelDialog';

export interface FilePropertyDialogProps {
    open: boolean,
    fileProperty: UiFileProperty,
    selectedFile: string,
    onOk: (fileProperty: UiFileProperty, selectedItem: string) => void,
    onCancel: () => void
};

export interface FilePropertyDialogState {
    fullScreen: boolean;
    selectedFile: string;
    hasSelection: boolean;
    files: string[];
    columns: number;
    columnWidth: number;
    openUploadFileDialog: boolean;
    openConfirmDeleteDialog: boolean;
};

export default class FilePropertyDialog extends ResizeResponsiveComponent<FilePropertyDialogProps, FilePropertyDialogState> {


    constructor(props: FilePropertyDialogProps) {
        super(props);


        this.model = PiPedalModelFactory.getInstance();

        this.state = {
            fullScreen: false,
            selectedFile: props.selectedFile,
            hasSelection: false,
            columns: 0,
            columnWidth: 1,
            files: [],
            openUploadFileDialog: false,
            openConfirmDeleteDialog: false
        };
        this.requestScroll = true;
        this.requestFiles();
    }
     
    private scrollRef: HTMLDivElement | null = null;

    onScrollRef(element: HTMLDivElement | null) {
        this.scrollRef = element;
        this.maybeScrollIntoView();
    }
    private maybeScrollIntoView()
    {
        if (this.scrollRef !== null && this.state.files.length !== 0 && this.mounted && this.requestScroll)
        {
            this.requestScroll = false;
            let options: ScrollIntoViewOptions = { block: "nearest"};
            options.block = "nearest";

            this.scrollRef.scrollIntoView(options);
        }
    }
    private mounted: boolean = false;
    private model: PiPedalModel;

    private requestFiles() {
        if (!this.props.open) {
            return;
        }
        if (this.props.fileProperty.directory === "") {
            return;
        }

        this.model.requestFileList(this.props.fileProperty)
            .then((files) => {
                if (this.mounted) {
                    files.splice(0,0,"");
                    this.setState({ files: files, hasSelection: this.isFileInList(files, this.state.selectedFile) });
                }
            }).catch((error) => {
                this.model.showAlert(error.toString())
            });
    }

    private lastDivRef : HTMLDivElement | null = null;

    onMeasureRef(div: HTMLDivElement | null)
    {
        this.lastDivRef = div;
        if (div) {
            let width = div.offsetWidth;
            if (width === 0) return;
            let columns = Math.floor((width-40)/280);
            if(columns === 0)
            {
                columns = 1;
            }
            let columnWidth = (width-40)/columns;
            if (columns !== this.state.columns || columnWidth !== this.state.columnWidth)
            {
                this.setState({columns: columns, columnWidth: columnWidth});
            }
        }
    }
    onWindowSizeChanged(width: number, height: number): void {

        this.setState({ 
            fullScreen: height < 200 
        })
        if (this.lastDivRef !== null)
        {
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
        if (prevProps.open !== this.props.open)
        {
            if (this.props.open)
            {
                this.requestFiles()
                this.requestScroll = true;
                if (this.state.selectedFile !== this.props.selectedFile)
                {
                    this.setState({selectedFile: this.props.selectedFile});
                }
            }
        } else if (prevProps.fileProperty !== this.props.fileProperty) {
            this.requestFiles()
        }

    }

    private fileNameOnly(path: string): string {
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

    private isFileInList(files: string[], file: string) {

        let hasSelection = false;
        if (file === "") return true;
        for (var listFile of files) {
            if (listFile === file) {
                hasSelection = true;
                break;
            }
        }
        return hasSelection;
    }

    onSelectValue(selectedFile: string) {
        this.requestScroll = true;
        this.setState({ selectedFile: selectedFile, hasSelection: this.isFileInList(this.state.files, selectedFile) })
    }
    onDoubleClickValue(selectedFile: string) {
        this.props.onOk(this.props.fileProperty,selectedFile);
    }

    handleDelete() {
        this.setState({openConfirmDeleteDialog: true});
    }
    handleConfirmDelete()
    {
        this.setState({openConfirmDeleteDialog: false});
        if (this.state.hasSelection)
        {
            let selectedFile = this.state.selectedFile;
            let position = -1;
            for (let i = 0; i < this.state.files.length;++i)
            {
                let file = this.state.files[i];
                if (file === selectedFile)
                {
                    position = i;
                }
            }
            let newSelection = "";
            if (position >= 0 && position < this.state.files.length-1)
            {
                newSelection = this.state.files[position+1];
            } else if (position === this.state.files.length-1)
            {
                if (position !== 0) {
                    newSelection = this.state.files[position-1];
                }

            }

            this.model.deleteUserFile(this.state.selectedFile)
            .then(
                ()=> {
                    this.setState({selectedFile: newSelection,hasSelection: newSelection !== ""});
                    this.requestFiles();
                }
            ).catch(
                (e: any)=>
                {
                    this.model.showAlert(e + "");
                }
            );
        }
    }

    private wantsScrollRef: boolean = true;

    mapKey: number = 0;
    render() {
        this.mapKey = 0;
        let columnWidth = this.state.columnWidth;

        return this.props.open &&
            (
                <DialogEx onClose={() => this.props.onCancel()} open={this.props.open} tag="fileProperty" fullWidth maxWidth="xl" style={{ height: "90%" }}
                    PaperProps={{ style: { minHeight: "90%", maxHeight: "90%", overflowY: "visible" } }}
                >
                    <DialogTitle >
                        <div>
                            <IconButton edge="start" color="inherit" onClick={() => { this.props.onCancel(); }} aria-label="back"
                            >
                                <ArrowBackIcon fontSize="small" style={{ opacity: "0.6" }} />
                            </IconButton>
                            <Typography display="inline" >{this.props.fileProperty.label}</Typography>
                        </div>
                    </DialogTitle>
                    <div style={{ flex: "0 0 auto", height: "1px", background: "rgba(0,0,0,0.2" }}>&nbsp;</div>
                    <div style={{ flex: "1 1 auto", height: "300", display: "flex", flexFlow: "row wrap", overflowX: "hidden", overflowY: "auto" }}>
                        <div 
                            ref={(element)=> this.onMeasureRef(element)}
                            style={{ flex: "1 1 100%", display: "flex", flexFlow: "row wrap",
                                 justifyContent: "flex-start", alignContent: "flex-start", paddingLeft: 16, paddingTop: 16,paddingBottom: 16 }}>
                            {
                                (this.state.columns !== 0) && // don't render until we have number of columns derived from layout.
                                this.state.files.map(
                                    (value: string, index: number) => {
                                        let displayValue = value;
                                        if (displayValue === "")
                                        {
                                            displayValue = "<none>";
                                        } else {
                                            displayValue = this.fileNameOnly(value);
                                        }
                                        let selected = value === this.state.selectedFile;
                                        let selectBg = selected ? "rgba(0,0,0,0.15)" : "rgba(0,0,0,0.0)";
                                        let scrollRef: ((element: HTMLDivElement | null) => void) | undefined = undefined;
                                        if (selected)
                                        {
                                            scrollRef = (element)=> { this.onScrollRef(element)};
                                        }

                                        return (
                                            <ButtonBase key={this.mapKey++}
                                                
                                                style={{ width: columnWidth, flex: "0 0 auto", height: 48, position: "relative" }}
                                                onClick={() => this.onSelectValue(value)} onDoubleClick={()=> {this.onDoubleClickValue(value);}}
                                            >
                                                <div ref={scrollRef} style={{ position: "absolute", background: selectBg, width: "100%", height: "100%", borderRadius: 4 }} />
                                                <div style={{ display: "flex", flexFlow: "row nowrap", textOverflow: "ellipsis", justifyContent: "start", alignItems: "center", width: "100%", height: "100%" }}>
                                                    <AudioFileIcon style={{ flex: "0 0 auto", opacity: 0.7, marginRight: 8, marginLeft: 8 }} />
                                                    <Typography noWrap style={{ flex: "1 1 auto", textAlign: "left" }}>{displayValue}</Typography>
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
                                onClick={()=> this.handleDelete()} >
                                <OldDeleteIcon fontSize='small' />
                            </IconButton>

                            <Button style={{ flex: "0 0 auto" }} aria-label="upload" variant="text" startIcon={<FileUploadIcon />}
                            onClick={()=> { this.setState({openUploadFileDialog: true})}}
                            >
                                <div>Upload</div>
                            </Button>
                            <div style={{ flex: "1 1 auto" }}>&nbsp;</div>

                            <Button onClick={() => { this.props.onCancel(); }} aria-label="cancel">
                                Cancel
                            </Button>
                            <Button style={{ flex: "0 0 auto" }} 
                                onClick={() => { this.props.onOk(this.props.fileProperty, this.state.selectedFile); }} 
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
                        uploadPage={  "uploadUserFile?directory="+ encodeURIComponent(this.props.fileProperty.directory)}
                        onUploaded={()=> { this.requestFiles();}}
                        fileProperty={this.props.fileProperty}


                    />
                    <OkCancelDialog open={this.state.openConfirmDeleteDialog}
                        text="Are you sure you want to delete the selected file?"
                        okButtonText="Delete"
                        onOk={()=>this.handleConfirmDelete()}
                        onClose={()=>{
                            this.setState({openConfirmDeleteDialog: false});
                        }}

                    />
                </DialogEx>
            );
    }
}