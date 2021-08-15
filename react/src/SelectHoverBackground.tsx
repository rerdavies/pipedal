import { Component, SyntheticEvent } from 'react';
import { createStyles, withStyles, WithStyles, Theme } from '@material-ui/core/styles';


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
    theme: Theme;
};

interface HoverState {
    hovering: boolean;
};


export const SelectHoverBackground =
    withStyles(styles, { withTheme: true })(
        class extends Component<HoverProps, HoverState>
        {
            constructor(props: HoverProps) {
                super(props);
                this.state = {
                    hovering: false
                };

                this.handleMouseEnter = this.handleMouseEnter.bind(this);
                this.handleMouseLeave = this.handleMouseLeave.bind(this);

            }

            handleMouseEnter(e: SyntheticEvent): void {
                if (this.props.showHover??true)
                {
                    this.setHovering(true);
                }

            }
            handleMouseLeave(e: SyntheticEvent): void {
                if (this.props.showHover??true)
                {
                    this.setHovering(false);
                }

            }


            setHovering(value: boolean): void {
                if (this.state.hovering !== value) {
                    this.setState({ hovering: value });
                }
            }
            render() {
                let classes = this.props.classes;
                let isSelected = this.props.selected;
                let hovering = this.state.hovering && (this.props.showHover??true);
                return (<div className={classes.frame}
                    onMouseEnter={(e) => { this.handleMouseEnter(e); }} onMouseLeave={(e) => { this.handleMouseLeave(e) }}
                >
                    <div className={classes.selected} style={{ display: (isSelected ? "block" : "none") }} />
                    <div className={classes.hover} style={{ display: (hovering  ? "block" : "none") }} />
                </div>);

            }
        }
    );

export default SelectHoverBackground;



