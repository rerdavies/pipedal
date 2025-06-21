
import React, { ReactElement } from 'react';
import { UiControl } from './Lv2Plugin';
import Typography from "@mui/material/Typography";
import Divider from '@mui/material/Divider';
import ToolTipEx from './ToolTipEx'


interface ControlTooltipProps {
    children: ReactElement,
    uiControl: UiControl
}


export default function ControlTooltip(props: ControlTooltipProps) {
    let { children, uiControl } = props;
    if (uiControl.comment && (uiControl.comment !== uiControl.name)) {
        return (
            <ToolTipEx title={(
                <React.Fragment>
                    <Typography variant="caption">{uiControl.name}</Typography>
                    <Divider />
                    <Typography variant="caption">{uiControl.comment}</Typography>

                </React.Fragment>
            )}
            >
                {children}
            </ToolTipEx>
        );
    } else {
        return (
            <ToolTipEx title={uiControl.name}
            >
                {children}
            </ToolTipEx>
        );
    }
}
