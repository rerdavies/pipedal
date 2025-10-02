/*
 * MIT License
 * 
 * Copyright (c) Robin E. R. Davies
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

import { Component } from 'react';
import { Theme } from '@mui/material/styles';
import { css } from '@emotion/react';

import DialogEx from './DialogEx';
import DialogTitle from '@mui/material/DialogTitle';
import ArrowBackIcon from '@mui/icons-material/ArrowBack';
import Toolbar from '@mui/material/Toolbar';
import DialogContent from '@mui/material/DialogContent';
import IconButtonEx from './IconButtonEx';
import Typography from '@mui/material/Typography';
import { PiPedalModel, PiPedalModelFactory, State } from './PiPedalModel';
import Select from '@mui/material/Select';
import MenuItem from '@mui/material/MenuItem';


export const StandardItemSize = { width: 80, height: 140 }


import { withStyles } from "tss-react/mui";
import WithStyles from './WithStyles';
import { withTheme } from './WithStyles';

const styles = (theme: Theme) => {
    return {
        frame: css({
            position: "relative",
            margin: "12px"
        }),
        controlFrame: css({
            display: "flex", flexDirection: "column", alignItems: "start", justifyContent: "space-between", height: 116
        }),
        displayValue: css({
            position: "absolute",
            top: 0,
            left: 0,
            right: 0,
            bottom: 4,
            textAlign: "center",
            background: theme.mainBackground,
            color: theme.palette.text.secondary,
            // zIndex: -1,
        }),
        titleSection: css({
            flex: "0 0 auto", alignSelf: "stretch", marginBottom: 8, marginLeft: 8, marginRight: 0
        }),
        midSection: css({
            flex: "1 1 1", display: "flex", flexFlow: "column nowrap", alignContent: "center", justifyContent: "center"
        }),
        editSection: css({
            flex: "0 0 0", position: "relative", width: 60, height: 28, minHeight: 28
        })
    }
}
    ;


export interface SideChainSelectControlProps extends WithStyles<typeof styles> {
    title: string;
    selectItems: { instanceId: number, title: string }[];
    selectedInstanceId: number;
    onChanged(instanceId: number): void;
    theme: Theme;
}
type SideChainSelectControlState = {
};

const SideChainSelectControl =
    withTheme(withStyles(
        class extends Component<SideChainSelectControlProps, SideChainSelectControlState> {

            model: PiPedalModel;


            constructor(props: SideChainSelectControlProps) {
                super(props);

                this.state = {
                };
                this.model = PiPedalModelFactory.getInstance();
                this.onStateChanged = this.onStateChanged.bind(this);
            }


            onStateChanged(state: State) {
            }
            mounted: boolean = false;
            componentDidMount() {
                this.model.state.addOnChangedHandler(this.onStateChanged);
                this.mounted = true;
            }
            componentWillUnmount() {
                this.mounted = false;
                this.model.state.removeOnChangedHandler(this.onStateChanged);
            }
            componentDidUpdate(prevProps: Readonly<SideChainSelectControlProps>, prevState: Readonly<SideChainSelectControlState>, snapshot?: any): void {
            }

            render() {
                //const classes = withStyles.getClasses(this.props);

                const classes = withStyles.getClasses(this.props);

                return (
                    <div className={classes.controlFrame}
                        style={{ width: 120 }}>
                        {/* TITLE SECTION */}
                        <div className={classes.titleSection}
                        >
                            <Typography variant="caption" display="block" noWrap style={{
                                width: "100%",
                                textAlign: "start"
                            }}> {this.props.title}</Typography>
                        </div>
                        {/* CONTROL SECTION */}

                        <div className={classes.midSection} style={{ width: "100%", paddingLeft: 8 }}>
                            <Select
                                variant="standard"
                                onChange={(event) => {
                                    this.props.onChanged(event.target.value as number);
                                }}
                                value={this.props.selectedInstanceId}
                                style={{ width: "100%", fontSize: "0.8rem" }}
                            >
                                {this.props.selectItems.map((item) => {
                                    return (
                                        <MenuItem key={item.instanceId} value={item.instanceId}
                                            selected={item.instanceId === this.props.selectedInstanceId}>
                                            {item.title}
                                        </MenuItem>
                                    );
                                })}
                            </Select>

                        </div>

                        {/* LABEL/EDIT SECTION*/}
                        <div className={classes.editSection} >
                        </div>
                    </div >
                );
            }
        }
        , styles)
    )
    ;
export default SideChainSelectControl;

export interface SideChainSelectDialogProps {
    open: boolean,
    onClose: () => void,

    selectItems: { instanceId: number, title: string }[];
    selectedInstanceId: number;
    onChanged(instanceId: number): void;

}
export function SideChainSelectDialog(props: SideChainSelectDialogProps) {
    let { open, onClose, selectItems, selectedInstanceId, onChanged } = props;
    return (
        <DialogEx tag="nameDialog" open={open} fullWidth maxWidth="xs" onClose={onClose} aria-labelledby="Rename-dialog-title"
            style={{ userSelect: "none" }}
            onEnterKey={() => { }}
        >
            <DialogTitle id="alert-dialog-title" style={{ userSelect: "none", padding: 0, margin: 0 }}>
                <Toolbar style={{ paddingLeft: 24, paddingRight: 24, margin: 0 }} >
                    <IconButtonEx
                        tooltip="Back" edge="start" color="inherit" onClick={() => { onClose(); }} aria-label="back"
                    >
                        <ArrowBackIcon style={{opacity: 0.6}} />
                    </IconButtonEx>

                    <Typography noWrap variant="body1" style={{ flex: "1 1 auto" }}>Sidechain Input</Typography>
                </Toolbar>
            </DialogTitle>

            <DialogContent >
                <Select
                    variant="standard"
                    onChange={(event) => {
                        onChanged(event.target.value as number);
                    }}
                    value={selectedInstanceId}
                    style={{ width: "100%" }}
                >
                    {selectItems.map((item) => {
                        return (
                            <MenuItem key={item.instanceId} value={item.instanceId}
                                selected={item.instanceId === selectedInstanceId}>
                                {item.title}
                            </MenuItem>
                        );
                    })}
                </Select>
            </DialogContent>
        </DialogEx>
    );
}