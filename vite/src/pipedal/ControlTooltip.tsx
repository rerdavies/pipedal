
import React, { ReactElement } from 'react';
import { UiControl } from './Lv2Plugin';
import Typography from "@mui/material/Typography";
import Divider from '@mui/material/Divider';
import ToolTipEx from './ToolTipEx'


interface ControlTooltipProps {
    children: ReactElement,
    uiControl: UiControl
    valueTooltip?: React.ReactNode;
}


export default function ControlTooltip(props: ControlTooltipProps) {
    let { children, uiControl, valueTooltip } = props;
    if (uiControl.comment && (uiControl.comment !== uiControl.name)) {
        return (
            <ToolTipEx
                valueTooltip={valueTooltip}
                title={
                    (
                        <React.Fragment>
                            <Typography variant="caption">{uiControl.name}</Typography>
                            <Divider />
                            <Typography variant="caption">{uiControl.comment}</Typography>

                        </React.Fragment>
                    )}
            >
                <div>
                    {children}
                </div>
            </ToolTipEx>
        );
    } else {
        return (
            <ToolTipEx valueTooltip={valueTooltip}
                title={
                    (
                        <Typography variant="caption">{uiControl.name}</Typography>
                    )}
            >
                <div >
                    {children}
                </div>
            </ToolTipEx>
        );
    }
}
