/* eslint-disable @typescript-eslint/no-unused-vars */
/*
* MIT License
* 
* Copyright (c) 2024 Robin E. R. Davies
* 
* Permission is hereby granted, free of charge, to any person obtaining a copy of
* this software and associated documentation files (the "Software"), to deal in
* the Software without restriction, including without limitation the rights to
* use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
* of the Software, and to permit persons to whom the Software is furnished to do
* so, subject to the following conditions:
* 
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
* 
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*/

import React from 'react';
import Typography from '@mui/material/Typography';
import DialogEx from './DialogEx';
import DialogContent from '@mui/material/DialogContent';
import DialogTitle from '@mui/material/DialogTitle';
import ArrowBackIcon from '@mui/icons-material/ArrowBack';
import IconButton from '@mui/material/IconButton';
import { PiPedalModel, PiPedalModelFactory } from './PiPedalModel';
import ResizeResponsiveComponent from './ResizeResponsiveComponent';
import { TransitionProps } from '@mui/material/transitions';

import Fade from '@mui/material/Fade';
import Slide from '@mui/material/Slide';
import SnapshotPanel from './SnapshotPanel';

const styles = {
    dialogPaper: {
        minHeight: '80vh',
        maxHeight: '80vh',
    },
};


const FadeTransition = React.forwardRef(function Transition(
    props: TransitionProps & {
        children: React.ReactElement<any, any>;
    },
    ref: React.Ref<unknown>,
) {
    return <Fade in={true} ref={ref} {...props} />;
});


const SlideTransition = React.forwardRef(function Transition(
    props: TransitionProps & {
      children: React.ReactElement<any, any>;
    },
    ref: React.Ref<unknown>,
  ) {
    return <Slide direction="up" ref={ref} {...props} />;
  });
  
  


export interface SnapshotDialogProps {
    open: boolean,
    onOk: () => void,
};

export interface SnapshotDialogState {
    fullScreen: boolean,
};

export default class SnapshotDialog extends ResizeResponsiveComponent<SnapshotDialogProps, SnapshotDialogState> {
    private model: PiPedalModel;

    constructor(props: SnapshotDialogProps) {
        super(props);
        this.model = PiPedalModelFactory.getInstance();
        this.state = {
            fullScreen: this.getFullScreen(),
        };
    }

    componentDidMount(): void {
        super.componentDidMount();
    }

    componentWillUnmount(): void {
        super.componentWillUnmount();

    }

    getFullScreen() {
        return window.innerHeight < 450 || window.innerWidth < 450;
    }
    onWindowSizeChanged(width: number, height: number): void {
        this.setState(
            {
                fullScreen: this.getFullScreen(),
            }
        );
    }


    render() {
        return (
            <DialogEx fullWidth={true} tag="shapshot" open={this.props.open} onClose={() => this.props.onOk()} fullScreen={this.state.fullScreen}
                TransitionComponent={this.state.fullScreen? SlideTransition: FadeTransition} keepMounted
                style={{ userSelect: "none", }}
                PaperProps={{
                    style: {
                      
                    },
                  }}
                onEnterKey={()=>{} }
            >
                <DialogTitle>
                    <div>
                        <IconButton edge="start" color="inherit" onClick={() => { this.props.onOk(); }} aria-label="back"
                        >
                            <ArrowBackIcon fontSize="small" style={{ opacity: "0.6" }} />
                        </IconButton>
                        <Typography display="inline" variant="body1" color="textSecondary" >Snapshots</Typography>
                    </div>

                </DialogTitle>
                <DialogContent style={{
                    overflow: "hidden", margin: 0, height: "80vh",
                    paddingLeft: 16, paddingRight: 16, paddingTop: 0, paddingBottom: 24,
                }}>
                    <SnapshotPanel panelHeight="80vh" />
                </DialogContent>
            </DialogEx>
        );
    }
}