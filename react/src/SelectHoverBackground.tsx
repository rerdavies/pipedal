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

import React,{ Component, SyntheticEvent } from 'react';
import { Theme } from '@mui/material/styles';


import { WithStyles } from '@mui/styles';
import createStyles from '@mui/styles/createStyles';
import withStyles from '@mui/styles/withStyles';


const styles = ({ palette }: Theme) => createStyles({
    frame: {
        position: "absolute",
        width: "100%",
        height: "100%"
    },
    hover: {
        position: "absolute",
        width: "100%",
        height: "100%",
        background: "#000000",
        opacity: 0.1
    },
    selected: {
        position: "absolute",
        width: "100%",
        height: "100%",
        background: palette.action.active,
        opacity: 0.3
    }

});



interface HoverProps extends WithStyles<typeof styles> {
    selected: boolean;
    showHover?: boolean;
    borderRadius?: number;
    theme: Theme;
    children?: React.ReactNode;
}

interface HoverState {
}


export const SelectHoverBackground =
    withStyles(styles, { withTheme: true })(
        class extends Component<HoverProps, HoverState>
        {
            constructor(props: HoverProps) {
                super(props);
                this.state = {
                };
                this.selfRef = React.createRef<HTMLDivElement>();
                this.hoverElementRef = React.createRef<HTMLDivElement>();

                this.handleMouseOver = this.handleMouseOver.bind(this);
                this.handleMouseLeave = this.handleMouseLeave.bind(this);

            }

            handleMouseOver(e: SyntheticEvent): void {
                console.log("onMouseOver")
                if (this.props.showHover ?? true) {
                    if (this.hoverElementRef.current)
                    {
                        this.hoverElementRef.current.style.display = "block";
                    } else {
                        console.log("No hoverElementRef")
                    }
                }

            }
            handleMouseLeave(e: SyntheticEvent): void {
                if (this.props.showHover ?? true) {
                    if (this.hoverElementRef.current)
                    {
                        this.hoverElementRef.current.style.display = "none";
                    } else {
                        console.log("No hoverElementRef")
                    }
                }
            }


            private selfRef: React.RefObject<HTMLDivElement>;
            private hoverElementRef: React.RefObject<HTMLDivElement>;

            render() {
                let classes = this.props.classes;
                let isSelected = this.props.selected;
                return (<div className={classes.frame} ref={this.selfRef}
                    onMouseOver={(e) => { this.handleMouseOver(e); }} onMouseLeave={(e) => { this.handleMouseLeave(e) }}
                >
                    <div className={classes.selected} style={{ display: (isSelected ? "block" : "none"), borderRadius: this.props.borderRadius }} />
                    <div ref={this.hoverElementRef} className={classes.hover} style={{ display: "none", borderRadius: this.props.borderRadius }} />
                    {
                        this.props.children
                    }
                </div>);

            }
        }
    );

export default SelectHoverBackground;



