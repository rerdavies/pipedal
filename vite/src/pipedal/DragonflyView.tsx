// Copyright (c) Robin E.R. Davies
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
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

import React from "react";
import { css } from "@emotion/react";
import { Theme } from "@mui/material/styles";
import { withStyles } from "tss-react/mui";

import IControlViewFactory from "./IControlViewFactory";
import { PedalboardItem } from "./Pedalboard";
import { PiPedalModel, PiPedalModelFactory } from "./PiPedalModel";
import PluginControlView, {
    ControlGroup,
    ControlViewCustomization,
    ICustomizationHost
} from "./PluginControlView";
import WithStyles, { createStyles } from "./WithStyles";

interface DragonflySection {
    name: string;
    controls: string[];
}

interface DragonflyDefinition {
    uri: string;
    title: string;
    engine: string;
    accent: string;
    secondary: string;
    sections: DragonflySection[];
}

const DRAGONFLY_DEFINITIONS: DragonflyDefinition[] = [
    {
        uri: "https://github.com/michaelwillis/dragonfly-reverb",
        title: "Hall Reverb",
        engine: "HALL ENGINE",
        accent: "#4fb8a5",
        secondary: "#e1b953",
        sections: [
            { name: "Mix", controls: ["dry_level", "early_level", "early_send", "late_level"] },
            { name: "Space", controls: ["size", "width", "delay", "decay", "diffuse"] },
            { name: "Motion", controls: ["spin", "wander", "modulation"] },
            { name: "Tone", controls: ["low_cut", "low_xo", "low_mult", "high_cut", "high_xo", "high_mult"] },
        ]
    },
    {
        uri: "urn:dragonfly:room",
        title: "Room Reverb",
        engine: "ROOM ENGINE",
        accent: "#df6658",
        secondary: "#5bb7a7",
        sections: [
            { name: "Mix", controls: ["dry_level", "early_level", "early_send", "late_level"] },
            { name: "Space", controls: ["size", "width", "predelay", "decay", "diffuse"] },
            { name: "Motion", controls: ["spin", "wander"] },
            { name: "Tone", controls: ["in_low_cut", "in_high_cut", "early_damp", "late_damp"] },
            { name: "Body", controls: ["low_boost", "boost_freq"] },
        ]
    },
    {
        uri: "urn:dragonfly:plate",
        title: "Plate Reverb",
        engine: "PLATE ENGINE",
        accent: "#e1b953",
        secondary: "#63a9b7",
        sections: [
            { name: "Mix", controls: ["dry_level", "early_level"] },
            { name: "Plate", controls: ["algorithm", "width", "predelay", "decay"] },
            { name: "Tone", controls: ["low_cut", "high_cut", "early_damp"] },
        ]
    },
    {
        uri: "urn:dragonfly:early",
        title: "Early Reflections",
        engine: "REFLECTION ENGINE",
        accent: "#63a9b7",
        secondary: "#df6658",
        sections: [
            { name: "Mix", controls: ["dry_level", "early_level"] },
            { name: "Space", controls: ["program", "size", "width"] },
            { name: "Tone", controls: ["low_cut", "high_cut"] },
        ]
    },
];

const styles = (_theme: Theme) => createStyles({
    skin: css({
        width: "100%",
        height: "100%",
        color: "#e8ece9",
        background: "#141819",
        "& [data-pipedal-role='plugin-control-frame']": {
            background: "#141819",
        },
        "& [data-pipedal-role='control-grid']": {
            gap: 0,
            rowGap: 0,
            paddingTop: 0,
            paddingLeft: 30,
            paddingRight: 45,
            alignContent: "flex-start",
        },
        "& [data-pipedal-role='custom-control']": {
            width: "100%",
            flex: "1 0 100%",
        },
        "& [data-pipedal-role='control-group']": {
            minHeight: 162,
            margin: 0,
            padding: "13px 10px 8px",
            color: "#e8ece9",
            background: "#191d1e",
            border: "0 solid #414748",
            borderRightWidth: 1,
            borderBottomWidth: 1,
            borderRadius: 0,
            boxShadow: "none",
        },
        "& [data-pipedal-role='control-group-title']": {
            minWidth: 0,
            position: "absolute",
            top: 9,
            left: 14,
            margin: 0,
            padding: 0,
            color: "#cfd5d1",
            background: "transparent",
            textTransform: "uppercase",
        },
        "& [data-pipedal-role='control-group-title'] p": {
            fontSize: 12,
            fontWeight: 600,
            letterSpacing: 0,
        },
        "& [data-pipedal-role='control-group-controls']": {
            minWidth: 190,
            paddingTop: 25,
            paddingBottom: 0,
        },
        "& [data-group-name='Mix']": {
            borderTop: "3px solid var(--dragonfly-accent)",
        },
        "& [data-group-name='Space'], & [data-group-name='Plate']": {
            borderTop: "3px solid var(--dragonfly-secondary)",
        },
        "& [data-group-name='Motion']": {
            borderTop: "3px solid #d7b757",
        },
        "& [data-group-name='Tone']": {
            borderTop: "3px solid #78a676",
        },
        "& [data-group-name='Body']": {
            borderTop: "3px solid #c66a73",
        },
    }),
    header: css({
        width: "calc(100% - 16px)",
        maxWidth: "calc(100% - 16px)",
        minHeight: 108,
        display: "flex",
        alignItems: "center",
        gap: 22,
        margin: "0 8px",
        padding: "15px 26px",
        color: "#171b1b",
        background: "#d9ddd8",
        borderBottom: "5px solid var(--dragonfly-accent)",
        boxSizing: "border-box",
        "@media (max-width: 700px)": {
            gap: 14,
            padding: "13px 18px",
            flexWrap: "wrap",
        },
    }),
    mark: css({
        width: 84,
        height: 62,
        position: "relative",
        flex: "0 0 auto",
        "&::before, &::after": {
            content: '""',
            width: 35,
            height: 48,
            position: "absolute",
            top: 2,
            border: "3px solid var(--dragonfly-accent)",
            borderRadius: "68% 22% 66% 28%",
            background: "rgba(255,255,255,0.3)",
        },
        "&::before": {
            left: 4,
            transform: "rotate(-22deg)",
        },
        "&::after": {
            right: 4,
            transform: "scaleX(-1) rotate(-22deg)",
        },
    }),
    markBody: css({
        width: 6,
        height: 58,
        position: "absolute",
        top: 1,
        left: 39,
        zIndex: 2,
        background: "#242929",
        borderRadius: 3,
        "&::before": {
            content: '""',
            width: 14,
            height: 14,
            position: "absolute",
            top: -3,
            left: -4,
            background: "#242929",
            border: "3px solid var(--dragonfly-secondary)",
            borderRadius: "50%",
            boxSizing: "border-box",
        },
    }),
    title: css({
        minWidth: 250,
        display: "flex",
        flexDirection: "column",
        "& strong": {
            fontSize: 26,
            lineHeight: "29px",
            fontWeight: 700,
        },
        "& span": {
            marginTop: 4,
            fontSize: 12,
            lineHeight: "16px",
            fontWeight: 700,
            color: "#5a6160",
            textTransform: "uppercase",
        },
        "@media (max-width: 700px)": {
            minWidth: 190,
            "& strong": { fontSize: 21, lineHeight: "24px" },
        },
    }),
    status: css({
        display: "flex",
        alignItems: "center",
        gap: 9,
        marginLeft: "auto",
        flexWrap: "wrap",
    }),
    pill: css({
        minHeight: 30,
        display: "flex",
        alignItems: "center",
        padding: "5px 10px",
        color: "#242929",
        background: "#edf0ed",
        border: "1px solid #9ca4a1",
        borderRadius: 3,
        fontSize: 12,
        fontWeight: 700,
        whiteSpace: "nowrap",
    }),
    accentPill: css({
        borderColor: "var(--dragonfly-accent)",
        background: "color-mix(in srgb, var(--dragonfly-accent) 17%, #edf0ed)",
    }),
});

interface DragonflyViewProps extends WithStyles<typeof styles> {
    instanceId: number;
    item: PedalboardItem;
    definition: DragonflyDefinition;
}

const DragonflyView = withStyles(
    class extends React.Component<DragonflyViewProps> implements ControlViewCustomization {
        model: PiPedalModel;
        customizationId = 30;

        constructor(props: DragonflyViewProps) {
            super(props);
            this.model = PiPedalModelFactory.getInstance();
        }

        fullScreen(): boolean {
            return false;
        }

        makeGroup(host: ICustomizationHost, section: DragonflySection): ControlGroup | null {
            let plugin = this.model.getUiPlugin(this.props.item.uri);
            if (!plugin) return null;

            let indexes: number[] = [];
            let controls: React.ReactNode[] = [];
            for (let symbol of section.controls) {
                let control = plugin.getControl(symbol);
                if (control && !control.isHidden()) {
                    indexes.push(control.index);
                    controls.push(host.makeStandardControl(control, this.props.item.controlValues));
                }
            }
            if (controls.length === 0) return null;
            return new ControlGroup(section.name, indexes, controls);
        }

        modifyControls(
            host: ICustomizationHost,
            _controls: (React.ReactNode | ControlGroup)[]
        ): (React.ReactNode | ControlGroup)[] {
            const classes = withStyles.getClasses(this.props);
            let result: (React.ReactNode | ControlGroup)[] = [
                <div className={classes.header} key="dragonfly-header">
                    <div className={classes.mark} aria-hidden="true">
                        <span className={classes.markBody} />
                    </div>
                    <div className={classes.title}>
                        <strong>Dragonfly {this.props.definition.title}</strong>
                        <span>Michael Willis / Dragonfly Reverb</span>
                    </div>
                    <div className={classes.status}>
                        <div className={`${classes.pill} ${classes.accentPill}`}>
                            {this.props.definition.engine}
                        </div>
                        <div className={classes.pill}>STEREO</div>
                    </div>
                </div>
            ];

            for (let section of this.props.definition.sections) {
                let group = this.makeGroup(host, section);
                if (group) result.push(group);
            }
            return result;
        }

        render() {
            const classes = withStyles.getClasses(this.props);
            let skinStyle = {
                "--dragonfly-accent": this.props.definition.accent,
                "--dragonfly-secondary": this.props.definition.secondary,
            } as React.CSSProperties;

            return (
                <div className={classes.skin} style={skinStyle}>
                    <PluginControlView
                        instanceId={this.props.instanceId}
                        item={this.props.item}
                        customization={this}
                        customizationId={this.customizationId}
                        showModGui={false}
                        onSetShowModGui={() => { }}
                    />
                </div>
            );
        }
    },
    styles
);

class DragonflyViewFactory implements IControlViewFactory {
    uri: string;
    private definition: DragonflyDefinition;

    constructor(definition: DragonflyDefinition) {
        this.definition = definition;
        this.uri = definition.uri;
    }

    Create(_model: PiPedalModel, pedalboardItem: PedalboardItem): React.ReactNode {
        return (
            <DragonflyView
                instanceId={pedalboardItem.instanceId}
                item={pedalboardItem}
                definition={this.definition}
            />
        );
    }
}

export const dragonflyViewFactories: IControlViewFactory[] =
    DRAGONFLY_DEFINITIONS.map(definition => new DragonflyViewFactory(definition));
