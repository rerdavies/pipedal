
import React, { ReactElement } from 'react';
import Tooltip from "@mui/material/Tooltip"
import { UiControl } from './Lv2Plugin';
import Typography from "@mui/material/Typography";
import Divider from '@mui/material/Divider';


interface ControlTooltipProps {
    children: ReactElement,
    uiControl: UiControl
}


export default function ControlTooltip(props: ControlTooltipProps) {
    let { children, uiControl } = props;
    if (uiControl.comment && (uiControl.comment !== uiControl.name)) {
        return (
            <Tooltip title={(
                <React.Fragment>
                    <Typography variant="caption">{uiControl.name}</Typography>
                    <Divider />
                    <Typography variant="caption">{uiControl.comment}</Typography>

                </React.Fragment>
            )}
                placement="top-start" arrow enterDelay={1500} enterNextDelay={1500}
            >
                {children}
            </Tooltip>
        );
    } else {
        return (
            <Tooltip title={uiControl.name}
                placement="top-start" arrow enterDelay={1500} enterNextDelay={1500}
            >
                {children}
            </Tooltip>
        );
    }
}
