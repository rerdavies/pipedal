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

import React from 'react';
import { Theme } from '@mui/material/styles';
import WithStyles from './WithStyles';
import {createStyles} from './WithStyles';

import { withStyles } from "tss-react/mui";
import Button from '@mui/material/Button';
import DialogEx from './DialogEx';
import MuiDialogTitle from '@mui/material/DialogTitle';
import MuiDialogContent from '@mui/material/DialogContent';
import MuiDialogActions from '@mui/material/DialogActions';
import IconButton from '@mui/material/IconButton';
import CloseIcon from '@mui/icons-material/Close';
import Typography from '@mui/material/Typography';
import Grid from '@mui/material/Grid';
import { PiPedalModelFactory } from "./PiPedalModel";
import InfoOutlinedIcon from '@mui/icons-material/InfoOutlined';
import { UiPlugin, UiControl } from './Lv2Plugin';
import PluginIcon from './PluginIcon';
import { Remark } from 'react-remark';
import { css } from '@emotion/react';



const styles = (theme: Theme) => {
    return createStyles({
        root: {
            margin: 0,
            padding: theme.spacing(2),
        },
        closeButton: css({
            position: 'absolute',
            right: theme.spacing(1),
            top: theme.spacing(1),
            color: theme.palette.grey[500],
        }),
        icon: {
            fill: theme.palette.text.primary,
            opacity: 0.6
        }
    });
};
export interface PluginInfoDialogTitleProps extends WithStyles<typeof styles> {
    id: string;
    children: React.ReactNode;
    onClose: () => void;
}

// const PluginInfoDialogTitle = withStyles(styles)((props: PluginInfoDialogTitleProps) => {
//     const { children, classes, onClose, ...other } = props;
//     return (
//         <MuiDialogTitle style={{ display: "flex", flexDirection: "row", alignItems: "start", flexWrap: "nowrap" }}>
//             <div style={{ flex: "0 0 auto", marginRight: 8 }}>
//                 <PluginIcon pluginType={plugin.plugin_type} pluginUri={plugin.uri} offsetY={5} />
//             </div>
//             <div style={{ flex: "1 1 auto" }}>
//                 {children}
//             </div>
//             <IconButton aria-label="close" className={classes.closeButton} onClick={() => handleClose()}
//                 style={{ flex: "0 0 auto" }}>
//                 <CloseIcon />
//             </IconButton>
//         </MuiDialogTitle>
//     );
// });

const PluginInfoDialogContent = withStyles(
    MuiDialogContent,
    (theme: Theme) => ({
        root: {
            padding: theme.spacing(2),
        }    
    })
);

const PluginInfoDialogActions = withStyles(MuiDialogActions,
    (theme: Theme) => ({
        root: {
            margin: 0,
            padding: theme.spacing(1),
        },
    })
);

export interface PluginInfoProps extends WithStyles<typeof styles> {
    plugin_uri: string
}

function displayChannelCount(count: number): string {
    if (count === 0) return "None";
    if (count === 1) return "Mono";
    if (count === 2) return "Stereo";
    return count + " channels";
}
function ioDescription(plugin: UiPlugin): string {
    let result = "Input: " + displayChannelCount(plugin.audio_inputs) + ". Output: " + displayChannelCount(plugin.audio_outputs) + ".";
    if (plugin.has_midi_input) {
        if (plugin.has_midi_output) {
            result += " Midi in/out.";
        } else {
            result += " Midi in.";
        }
    } else {
        if (plugin.has_midi_output) {
            result += "Midi out.";
        }
    }
    return result;


}

// function makeParagraphs(description: string) {
//     description = description.replaceAll('\r', '');
//     description = description.replaceAll('\n\n', '\r');
//     description = description.replaceAll('\n', ' ');

//     let paragraphs: string[] = description.split('\r');
//     return (
//         <div style={{ paddingLeft: "24px" }}>
//             {paragraphs.map((para) => (
//                 <Typography variant="body2" paragraph >
//                     {para}
//                 </Typography>
//             ))}
//         </div>

//     );
// }
function makeControls(controls: UiControl[]) {
    let hasComments = false;

    for (let i = 0; i < controls.length; ++i) {
        if (controls[i].comment !== "") {
            hasComments = true;
            break;
        }
    }
    hasComments = true;
    if (hasComments) {
        let trs: React.ReactElement[] = [];
        for (let i = 0; i < controls.length; ++i) {
            let control = controls[i];
            if (!(control.not_on_gui) && control.is_input)
                trs.push((
                    <tr>
                        <td style={{ verticalAlign: "top" }}>
                            <Typography variant="body2" style={{ whiteSpace: "nowrap" }}>
                                {control.name}
                            </Typography>
                        </td>
                        <td style={{ paddingLeft: "16px", verticalAlign: "top" }}>
                            <Typography variant="body2">
                                {control.comment}
                            </Typography>
                        </td>
                    </tr >
                ));

        }
        return (
            <table style={{ paddingLeft: "24px", verticalAlign: "top" }}>
                {
                    trs
                }
            </table>
        );

    } else {
        return (
            <Grid container direction="row" justifyContent="flex-start" alignItems="flex-start" spacing={1} style={{ paddingLeft: "24px" }}>
                {
                    controls.map((control) => (
                        <Grid xs={6} sm={4} key={control.symbol + "x"} >
                            <Typography variant="body2">
                                {control.name}
                            </Typography>
                        </Grid>
                    ))
                }
            </Grid>
        );
    }

}



const PluginInfoDialog = withStyles((props: PluginInfoProps) => {

    let model = PiPedalModelFactory.getInstance();
    const [open, setOpen] = React.useState(false);
    let { plugin_uri } = props;
    let classes = withStyles.getClasses(props);

    const handleClickOpen = () => {
        setOpen(true);
    };
    const handleClose = () => {
        setOpen(false);
    };
    let uri = props.plugin_uri;
    let visible = true;
    if (uri === null || uri === "") {
        visible = false;
    }
    if (!visible) {
        return (<div></div>);
    };
    let plugin = model.getUiPlugin(plugin_uri);

    if (plugin === null) {
        return (<div></div>)
    }

    return (
        <div>
            <IconButton
                style={{ display: (props.plugin_uri !== "") ? "inline-flex" : "none" }}
                onClick={handleClickOpen}
                size="large">
                <InfoOutlinedIcon className={classes.icon} color='inherit' />
            </IconButton>
            {open && (
                <DialogEx tag="info" onClose={handleClose} open={open} fullWidth maxWidth="md"
                    onEnterKey={handleClose}
                >
                    <MuiDialogTitle >
                        <div style={{ display: "flex", flexDirection: "row", alignItems: "start", flexWrap: "nowrap" }}>
                            <div style={{ flex: "0 0 auto", marginRight: 16 }}>
                                <PluginIcon pluginType={plugin.plugin_type} offsetY={3} />
                            </div>
                            <div style={{ flex: "1 1 auto" }}>
                                <Typography variant="h6">{plugin.name}</Typography>
                            </div>
                            <IconButton
                                aria-label="close"
                                className={classes.closeButton}
                                onClick={() => handleClose()}
                                style={{ flex: "0 0 auto" }}
                                size="large">
                                <CloseIcon />
                            </IconButton>
                        </div>
                    </MuiDialogTitle>
                    <PluginInfoDialogContent dividers style={{ width: "100%", maxHeight: "80%", overflowX: "hidden" }}>
                        <div style={{ width: "100%", display: "flex", flexFlow: "row", justifyItems: "stretch", flexWrap: "nowrap" }} >
                            <div style={{ flex: "1 1 auto", whiteSpace: "nowrap", minWidth: "auto" }}>
                                <Typography gutterBottom variant="body2" >
                                    Author:&nbsp;
                                    {(plugin.author_homepage !== "")
                                        ? (<a href={plugin.author_homepage} target="_blank" rel="noopener noreferrer">
                                            {plugin.author_name}
                                        </a>
                                        )
                                        : (
                                            <span>{plugin.author_name}</span>
                                        )

                                    }
                                </Typography>
                            </div>
                            <div style={{ flex: "0 0 auto" }}>
                                <Typography gutterBottom variant="body2" >
                                    {ioDescription(plugin)}
                                </Typography>
                            </div>

                        </div>
                        <Typography variant="body2" gutterBottom style={{ paddingTop: "1em" }}>
                            Controls:
                        </Typography>
                        {
                            makeControls(plugin.controls)
                        }
                        {plugin.description.length > 0 && (
                            <div>
                                <Typography gutterBottom variant="body2" style={{ paddingTop: "1em" }}>
                                    Description:
                                </Typography>
                                <div style={{ marginLeft: 24, marginTop: 16 }}>
                                    <Remark
                                        rehypeReactOptions={{
                                            components: {
                                                p: (props: any) => {
                                                    // return (
                                                    //     <p className="MuiTypography-root MuiTypography-body2" {...props} />
                                                    // );
                                                    return (
                                                        <Typography variant="body2" paragraph={true} {...props} />
                                                    );

                                                },
                                                code: (props: any) => {
                                                    return (<code style={{ fontSize: 14 }} {...props} />);
                                                },
                                                a: (props: any) => {
                                                    return (
                                                        <a target="_blank" {...props} />
                                                    );

                                                }
                                            },
                                        }}
                                    >
                                        {plugin.description}
                                    </Remark>
                                </div>
                            </div>
                        )}
                    </PluginInfoDialogContent>
                    <PluginInfoDialogActions>
                        <Button variant="dialogPrimary" autoFocus onClick={handleClose} style={{ width: "130px" }}>
                            OK
                        </Button>
                    </PluginInfoDialogActions>
                </DialogEx>
            )}
        </div >
    );
},
styles);

export default PluginInfoDialog;
