// Copyright (c) 2024 Robin E. R. Davies
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

import { Component } from 'react';
import { Theme } from '@mui/material/styles';
import WithStyles, {withTheme} from './WithStyles';
import { withStyles } from "tss-react/mui";
import { PiPedalModel, PiPedalModelFactory } from './PiPedalModel';
import { pluginControlStyles } from './PluginControl';
import MidiChannelBinding from './MidiChannelBinding';
import Tooltip from '@mui/material/Tooltip';
import Typography from '@mui/material/Typography';
import Divider from '@mui/material/Divider';
import ButtonBase from '@mui/material/ButtonBase';
import MidiIcon from './svg/ic_midi.svg?react';
import MidiChannelBindingDialog from './MidiChannelBindingDialog';


export const StandardItemSize = { width: 80, height: 140 }



export interface MidiChannelBindingControlProps extends WithStyles<typeof pluginControlStyles> {
    midiChannelBinding: MidiChannelBinding
    theme: Theme;
    onChange: (result: MidiChannelBinding) => void;
}
type MidiChannelBindingControlState = {
    dialogOpen: boolean;
};

const MidiChannelBindingControl =
    withTheme(
    withStyles(
        class extends Component<MidiChannelBindingControlProps, MidiChannelBindingControlState> {
            model: PiPedalModel;

            constructor(props: MidiChannelBindingControlProps) {
                super(props);

                this.state = {
                    dialogOpen: false
                };
                this.model = PiPedalModelFactory.getInstance();

            }

            render() {
                const classes = withStyles.getClasses(this.props);
                let item_width = 80;
                return (
                    <div
                        className={classes.controlFrame}
                        style={{ width: item_width }}
                    >
                        {/* TITLE SECTION */}
                        <div className={classes.titleSection}
                            style={
                                {
                                    alignSelf: "stretch"

                                }}>
                            <Tooltip title={(
                                <React.Fragment>
                                    <Typography variant="caption">title</Typography>
                                    <Divider />
                                    <Typography variant="caption">Controls which MIDI messages get forward to the instrument.</Typography>

                                </React.Fragment>
                            )}
                                placement="top-start" arrow enterDelay={1500} enterNextDelay={1500}
                            >
                                <Typography variant="caption" display="block" noWrap style={{
                                    width: "100%",
                                    textAlign: "center"
                                }}> MIDI</Typography>
                            </Tooltip>

                        </div>
                        {/* CONTROL SECTION */}

                        <div className={classes.midSection}>
                            <ButtonBase style={{ width: 48, height: 48, borderRadius: 9999 }}
                                onClick={()=> { 
                                    this.setState({dialogOpen: true});
                                }}
                                sx={{
                                    ':hover': {
                                        bgcolor: 'action.hover', // theme.palette.primary.main
                                        color: 'white',
                                    },
                                }}
                            >
                                <MidiIcon fill={this.props.theme.palette.text.secondary} style={{ width: 36, height: 36 }} />
                            </ButtonBase>
                        </div>
                        <div className={classes.editSection}
                            style={{ display: "flex", flexFlow: "column nowrap", justifyContent: "center" }}
                        >
                            <Typography variant="caption" display="block" noWrap
                                style={{ width: "100%", textAlign: "center" }}
                            >OMNI</Typography>
                        </div>
                        {this.state.dialogOpen&&(
                            <MidiChannelBindingDialog 
                                open={this.state.dialogOpen}
                                midiChannelBinding={this.props.midiChannelBinding}
                                onClose={() =>{ this.setState({dialogOpen: false});}}
                                onChanged={(midiChannelBinding: MidiChannelBinding)=> {}}
                                midiDevices={[]}

                                />

                        )}
                    </div >
                );
            }
        },
        pluginControlStyles
    ));

export default MidiChannelBindingControl;

