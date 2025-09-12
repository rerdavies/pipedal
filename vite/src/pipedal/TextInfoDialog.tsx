// Copyright (c) 2025 Robin E. R. Davies
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
import IconButtonEx from './IconButtonEx';
import Typography from '@mui/material/Typography';
import { PiPedalModel, PiPedalModelFactory, State } from './PiPedalModel';
import AppBar from '@mui/material/AppBar';
import Toolbar from '@mui/material/Toolbar';
import ArrowBackIcon from '@mui/icons-material/ArrowBack';
import ReactMarkdown from 'react-markdown';
import rehypeExternalLinks from 'rehype-external-links'
import { PiPedalError } from './PiPedalError';
import DialogEx from './DialogEx';
import ResizeResponsiveComponent from './ResizeResponsiveComponent';



import Slide, { SlideProps } from '@mui/material/Slide';

interface TextInfoDialogProps  {
    open: boolean;
    fileName: string;
    title: string;
    onClose: () => void;
};

interface TextInfoDialogState {

    textFileContent: string;
    fullScreen: boolean;
};


const Transition = React.forwardRef(function Transition(
    props: SlideProps, ref: React.Ref<unknown>
) {
    return (<Slide direction="up" ref={ref} {...props} />);
});


const TextInfoDialog = class extends ResizeResponsiveComponent<TextInfoDialogProps, TextInfoDialogState> {

    model: PiPedalModel;
    refNotices: React.RefObject<HTMLDivElement | null >;


    constructor(props: TextInfoDialogProps) {
        super(props);
        this.model = PiPedalModelFactory.getInstance();
        this.refNotices = React.createRef();
        this.handleDialogClose = this.handleDialogClose.bind(this);
        this.state = {
            textFileContent: "",
            fullScreen: this.windowSize.width < 600

        };
    }

    onWindowSizeChanged(width: number, height: number): void {
        this.setState({fullScreen: width < 600});
    }

    mounted: boolean = false;
    subscribed: boolean = false;

    tick() {
        if (this.model.state.get() === State.Ready) {
        }
    }

    timerHandle?: number;


    startTextFileRequest() {
        let url = this.model.varServerUrl + "downloadMediaFile?path=" + encodeURIComponent(this.props.fileName);
        if (this.state.textFileContent === "") {
            fetch(url)
               .then((request) => {
                    if (!request.ok) {
                        throw new PiPedalError("Info file request failed.");
                    }
                    return request.text();
                })
                .then((text) => {
                    if (this.mounted) {
                        this.setState({ textFileContent: text });
                    }

                })
                .catch((err) => {
                    // ok in debug builds. File doesn't get placed until install time.
                    console.log("Failed to fetch text file. " + err.toString());
                    this.setState({textFileContent: err.toString()});
                });
        }
    }

    componentDidMount() {
        super.componentDidMount?.();
        this.mounted = true;
        this.startTextFileRequest();
    }
    componentWillUnmount() {
        super.componentWillUnmount?.();
        this.mounted = false;
    }
    componentDidUpdate(prevProps: Readonly<TextInfoDialogProps>, prevState: Readonly<TextInfoDialogState>, snapshot: any): void {
        super.componentDidUpdate?.(prevProps, prevState, snapshot);
    }

    handleDialogClose(_e: SyntheticEvent) {
        this.props.onClose();
    }


    render() {
        return (
            <DialogEx tag="info" fullScreen={this.state.fullScreen} open={this.props.open}
                onClose={() => { this.props.onClose() }} TransitionComponent={Transition}
                onEnterKey={() => { this.props.onClose() }}
                style={{ userSelect: "none" }}
            >

                <div style={{ display: "flex", flexDirection: "column", flexWrap: "nowrap", width: "100%", height: "100%", overflow: "hidden" }}>
                    <div style={{ flex: "0 0 auto" }}>
                        <AppBar style={{
                            position: 'relative',
                            top: 0, left: 0
                        }}  >
                            <Toolbar>
                                <IconButtonEx tooltip="Back" edge="start" color="inherit" onClick={this.handleDialogClose} aria-label="back"
                                >
                                    <ArrowBackIcon />
                                </IconButtonEx>
                                <Typography noWrap variant="h6"
                                    sx={{ marginLeft: 2, flex: 1 }}
                                >
                                    {this.props.title}
                                </Typography>
                            </Toolbar>
                        </AppBar>
                    </div>
                    <div style={{
                        flex: "1 1 auto", position: "relative", overflow: "hidden",
                        overflowX: "hidden", overflowY: "auto", userSelect: "text"
                    }}
                    >
                        <div style={{ padding: "16px 32px",
                            display: "flex", flexDirection: "row", flexWrap: "nowrap", alignItems: "start",
                        }}>
                            <div style={{ flex: "1 1 0px" }} />
                            <div style={{ flex: "1 1 auto",maxWidth: 650 }} >
                                <ReactMarkdown   rehypePlugins={[[rehypeExternalLinks, {target: '_blank'}]]}>
                                    {this.state.textFileContent}
                                </ReactMarkdown>
                            </div>
                            <div style={{ flex: " 5 5 0px" }} />
                        </div>
                    </div>
                </div>
            </DialogEx >

        );

    }

};


export default TextInfoDialog;