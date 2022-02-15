// Copyright (c) 2021 Robin Davies
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

import React, {SyntheticEvent} from 'react';
import { Theme } from '@mui/material/styles';
import { WithStyles,withStyles } from '@mui/styles';
import createStyles from '@mui/styles/createStyles';
import { ZoomedControlInfo } from './PiPedalModel';
import DialogEx from './DialogEx';
import Typography from '@mui/material/Typography';
import { PiPedalModelFactory,PiPedalModel } from './PiPedalModel';
import ZoomedDial from './ZoomedDial';


const styles = (theme: Theme) => createStyles({
});


interface ZoomedUiControlProps extends WithStyles<typeof styles> {
    dialogOpen: boolean;
    onDialogClose: () => void;
    onDialogClosed: () => void;
    controlInfo: ZoomedControlInfo | undefined
}

interface ZoomedUiControlState {
    value: number
}

/*
const Transition = React.forwardRef(function Transition(
    props: TransitionProps & { children?: React.ReactElement },
    ref: React.Ref<unknown>,
) {
    return <Slide direction="up" ref={ref} {...props} />;
});
*/


const ZoomedUiControl = withStyles(styles, { withTheme: true })(

    class extends React.Component<ZoomedUiControlProps, ZoomedUiControlState> {

        model: PiPedalModel;

        constructor(props: ZoomedUiControlProps)
        {
            super(props);
            this.model = PiPedalModelFactory.getInstance();
            this.state = {
                value: this.getCurrentValue()
            };
        }

        getCurrentValue(): number {
            if (!this.props.controlInfo)
            {
                return 0;
            } else {
                let uiControl = this.props.controlInfo.uiControl;
                let instanceId = this.props.controlInfo.instanceId;
                if (instanceId === -1) return 0;
                let pedalBoardItem = this.model.pedalBoard.get()?.getItem(instanceId);
                let value: number = pedalBoardItem?.getControlValue(uiControl.symbol)??0;
                return value;
            }
        }
        hasControlChanged(oldProps: ZoomedUiControlProps, newProps: ZoomedUiControlProps)
        {
            if (oldProps.controlInfo === null) 
            {
                return newProps.controlInfo !== null;
            } else if (newProps.controlInfo === null)
            {
                return oldProps.controlInfo !== null;
            } else {
                if (newProps.controlInfo?.instanceId !== oldProps.controlInfo?.instanceId) return true;
                if (newProps.controlInfo?.uiControl.symbol !== oldProps.controlInfo?.uiControl.symbol) return true;
            }
            return false;

        }

        componentDidUpdate(oldProps: ZoomedUiControlProps, oldState: ZoomedUiControlState)
        {
            if (this.hasControlChanged(oldProps,this.props))
            {
                let currentValue = this.getCurrentValue();
                if (this.state.value !== currentValue)
                {
                    this.setState({value: this.getCurrentValue()});
                }
            }
        }

        onDrag(e: SyntheticEvent) {
            e.preventDefault();
            e.stopPropagation();
            return false;
        }


        render() {
            let displayValue = this.props.controlInfo?.uiControl.formatDisplayValue(this.state.value)??"";
            return (
                <DialogEx tag="zoomedControlDlg" open={this.props.dialogOpen} 
                    onClose={()=> { this.props.onDialogClose()}}
                    onAbort={()=> { this.props.onDialogClose()}}
                >
                    <div style={{ width: 300, height: 300, background: "#FFF",
                        display: "flex", flexFlow: "column", alignItems: "center", alignContent: "center", justifyContent: "center"
                        
                        }}
                        onDrag={this.onDrag}
                        >
                            <Typography 
                                display="block" variant="h6" color="textPrimary" style={{textAlign: "center"}}
                                >
                                {this.props.controlInfo?.uiControl.name??""}
                            </Typography>
                            <ZoomedDial size={200} controlInfo={this.props.controlInfo}
                                onPreviewValue={(v)=> { 
                                    this.setState({value: v});
                                }}
                                onSetValue={(v)=> { 
                                    this.setState({value: v});
                                    if (this.props.controlInfo)
                                    {
                                        this.model.setPedalBoardControlValue(
                                            this.props.controlInfo.instanceId,
                                            this.props.controlInfo.uiControl.symbol,
                                            v);
                                    }
                                }}
                                />
                            
                            <Typography 
                                display="block" variant="h6" color="textPrimary" style={{textAlign: "center"}}
                                >
                                {displayValue}
                            </Typography>
                    </div>

                </DialogEx>
            );
        }
    }
);

export default ZoomedUiControl;
