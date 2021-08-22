import React, { ReactNode, SyntheticEvent } from 'react';
import Grid from '@material-ui/core/Grid';
import { createStyles, withStyles, WithStyles, Theme } from '@material-ui/core/styles';
import { PiPedalModel, PiPedalModelFactory } from './PiPedalModel';
import { UiPlugin, PluginType } from './Lv2Plugin';
import Paper from '@material-ui/core/Paper';
import ButtonBase from '@material-ui/core/ButtonBase';
import Button from '@material-ui/core/Button';
import MenuItem from '@material-ui/core/MenuItem';
import Typography from '@material-ui/core/Typography';
import PluginInfoDialog from './PluginInfoDialog'
import PluginIcon from './PluginIcon'
import Dialog from '@material-ui/core/Dialog';
import DialogActions from '@material-ui/core/DialogActions';
import DialogContent from '@material-ui/core/DialogContent';
import DialogTitle from '@material-ui/core/DialogTitle';
import SelectHoverBackground from './SelectHoverBackground';
import ArrowBackIcon from '@material-ui/icons/ArrowBack';
import IconButton from '@material-ui/core/IconButton';
import Select from '@material-ui/core/Select';
import PluginClass from './PluginClass'
import ClearIcon from '@material-ui/icons/Clear';
import ResizeResponsiveComponent from './ResizeResponsiveComponent';
import { TransitionProps } from '@material-ui/core/transitions/transition';
import Slide from '@material-ui/core/Slide';


export type CloseEventHandler = () => void;
export type OkEventHandler = (pluginUri: string) => void;

const NARROW_DISPLAY_THRESHOLD = 600;
const FILTER_STORAGE_KEY = "com.twoplay.pipedal.load_dlg.filter";

const Transition = React.forwardRef(function Transition(
    props: TransitionProps & { children?: React.ReactElement },
    ref: React.Ref<unknown>,
) {
    return <Slide direction="up" ref={ref} {...props} />;
});



const pluginGridStyles = (theme: Theme) => createStyles({
    frame: {

        position: "relative",
        width: "100%",
        height: "100%"
    },

    top: {
        top: "0px",
        right: "0px",
        left: "0px",
        bottom: "64px",
        position: "relative",
        flexGrow: 1,
        background: "#eee",
        overflowX: "hidden",
        overflowY: "visible"
    },
    bottom: {
        position: "relative",
        bottom: "0px",
        height: "64px",
        width: "100%",
        display: "flex",
        flexDirection: "row",
        flexWrap: "nowrap",
        justifyContent: "space-between",
        paddingLeft: "24px",
        paddingRight: "48px",
        background: theme.palette.background.paper,
    },
    paper: {
        position: "relative",
        overflow: "hidden",
        height: "56px",
        width: "100%",
        background: theme.palette.background.paper
    },
    buttonBase: {

        width: "100%",
        height: "100%'"
    },
    content: {
        marginTop: "8px",
        marginBottom: "8px",
        marginLeft: "12px",
        marginRight: "12px",
        width: "100%",
        overflow: "hidden"
    },
    table: {
        borderCollapse: "collapse",
    },
    icon: {
        width: "24px",
        height: "24px",
        margin: "0px",
        opacity: "0.6"
    },
    label: {
        marginLeft: "8px",

    },
    control: {
        padding: theme.spacing(1),
    },
    tdText: {
        padding: "0px"

    }
})
    ;

interface PluginGridProps extends WithStyles<typeof pluginGridStyles> {
    onOk: OkEventHandler;
    onCancel: CloseEventHandler;
    uri?: string;
    minimumItemWidth?: number;
    theme: Theme;
    open: boolean
};

type PluginGridState = {
    selected_uri?: string,
    hover_uri?: string,
    filterType: PluginType,
    client_width: number,
    client_height: number,
    minimumItemWidth: number,
    
}


export const LoadPluginDialog =
    withStyles(pluginGridStyles, { withTheme: true })(
        class extends ResizeResponsiveComponent<PluginGridProps, PluginGridState>
        {
            model: PiPedalModel;

            constructor(props: PluginGridProps) {
                super(props);
                this.model = PiPedalModelFactory.getInstance();

                let filterType_ = PluginType.Plugin; // i.e. "Any".
                let persistedFilter = window.localStorage.getItem(FILTER_STORAGE_KEY);
                if (persistedFilter) {
                    filterType_ = persistedFilter as PluginType;
                }

                this.state = {
                    selected_uri: this.props.uri,
                    hover_uri: "",
                    filterType: filterType_,
                    client_width: window.innerWidth,
                    client_height: window.innerHeight,
                    minimumItemWidth: props.minimumItemWidth ? props.minimumItemWidth : 220
                };

                this.updateWindowSize = this.updateWindowSize.bind(this);
                this.handleCancel = this.handleCancel.bind(this);
                this.handleOk = this.handleOk.bind(this);
            }

            updateWindowSize() {
                this.setState({
                    client_width: window.innerWidth,
                    client_height: window.innerHeight
                });
            }
            componentDidMount() {
                super.componentDidMount();
                this.updateWindowSize();
                window.addEventListener('resize', this.updateWindowSize);
            }
            componentWillUnmount() {
                super.componentWillUnmount();
                window.removeEventListener('resize', this.updateWindowSize);
            }

            onWindowSizeChanged(width: number,height: number): void {
                super.onWindowSizeChanged(width,height);

            }

            onFilterChange(e: any) {
                let value = e.target.value as PluginType;

                window.localStorage.setItem(FILTER_STORAGE_KEY, value as string);

                this.setState({ filterType: value });
            }
            onClearFilter(): void {
                let value = PluginType.Plugin;
                window.localStorage.setItem(FILTER_STORAGE_KEY, value as string);

                this.setState({ filterType: value });
            }

            selectItem(item: number): void {
                let uri: string = "";
                let plugins = this.model.ui_plugins.get();
                if (item >= 0 && item < plugins.length) {
                    uri = plugins[item].uri;
                }
                if (uri !== this.state.selected_uri) {
                    this.setState({ selected_uri: uri });
                }
            };


            handleCancel(e: SyntheticEvent): void {
                e.preventDefault();
                this.cancel();
            }
            handleOk(e: SyntheticEvent): void {
                e.preventDefault();
                if (this.state.selected_uri) {
                    this.props.onOk(this.state.selected_uri);
                }
            }

            onDoubleClick(e: SyntheticEvent, uri: string): void {
                this.props.onOk(uri);
            }


            cancel(): void {
                this.props.onCancel();
            }

            fireSelected(item?: UiPlugin) {
                if (item) {
                    this.setState({ selected_uri: item.uri });
                }
            }
            setHoverUri(uri: string) {
                this.setState({ hover_uri: uri });
            }

            onClick(e: SyntheticEvent, uri: string): void {
                this.setState({ selected_uri: uri });
            }
            handleMouseEnter(e: SyntheticEvent, uri: string): void {
                this.setHoverUri(uri);

            }
            handleMouseLeave(e: SyntheticEvent, uri: string): void {
                this.setHoverUri("");

            }
            onInfoClicked(): void {
                // let selectedUri = this.state.selected_uri;
            }

            stereo_indicator(uiPlugin?: UiPlugin): string {
                if (!uiPlugin) return "";
                if (uiPlugin.audio_inputs === 2 || uiPlugin.audio_outputs === 2) {
                    return " (Stereo)";
                }
                return "";
            }
            info_string(uiPlugin?: UiPlugin): string {
                if (uiPlugin === undefined) return "";
                let result = uiPlugin.name;
                if (uiPlugin.author_name !== "") {
                    result += ", " + uiPlugin.author_name;
                }
                result += this.stereo_indicator(uiPlugin);
                return result;
            }
            createFilterChildren(result: ReactNode[], classNode: PluginClass, level: number): void {
                for (let i = 0; i < classNode.children.length; ++i) {
                    let child = classNode.children[i];
                    let name = "\u00A0".repeat(level * 3+1) + child.display_name;
                    result.push((<MenuItem value={child.plugin_type}>{name}</MenuItem>));
                    if (child.children.length !== 0) {
                        this.createFilterChildren(result, child, level + 1);
                    }
                }
            }
            createFilterOptions(): ReactNode[] {
                let classes = this.model.plugin_classes.get();
                let result: ReactNode[] = [];

                result.push((<MenuItem value={PluginType.Plugin}>&nbsp;All</MenuItem>));
                this.createFilterChildren(result, classes, 1);
                return result;

            }
            filterPlugins(plugins: UiPlugin[]): ReactNode[] {
                let result: ReactNode[] = [];
                let filterType = this.state.filterType;
                let classes = this.props.classes;
                let rootClass = this.model.plugin_classes.get();

                for (let i = 0; i < plugins.length; ++i) {
                    let value = plugins[i];
                    if (filterType === PluginType.Plugin || rootClass.is_type_of(filterType, value.plugin_type)) {
                        result.push(
                            (
                                <Grid key={value.uri} xs={6} sm={6} md={4} lg={3} xl={3} item
                                    onDoubleClick={(e) => { this.onDoubleClick(e, value.uri) }}
                                    onClick={(e) => { this.onClick(e, value.uri) }}
                                    onMouseEnter={(e) => { this.handleMouseEnter(e, value.uri) }}
                                    onMouseLeave={(e) => { this.handleMouseLeave(e, value.uri) }}
                                >
                                    <Paper className={classes.paper} >
                                        <ButtonBase className={classes.buttonBase} focusRipple>
                                            <SelectHoverBackground selected={value.uri === this.state.selected_uri} showHover={true} />
                                            <div className={classes.content}>
                                                <div style={{ display: "flex", flexDirection: "row", justifyContent: "flex-start", alignItems: "start", flexWrap: "nowrap" }} >
                                                    <div style={{ flex: "0 1 auto" }}>
                                                        <PluginIcon pluginType={value.plugin_type} pluginUri={value.uri} size={24} />
                                                    </div>
                                                    <div style={{ display: "flex", flexDirection: "column", flex: "1 1 auto", width: "100%" }}>
                                                        <Typography className={classes.label}
                                                            display='block' variant='body2' align="left" color="textPrimary" noWrap
                                                        >{value.name}
                                                        </Typography>
                                                        <Typography className={classes.label}
                                                            display='block' variant='caption' align="left" noWrap
                                                        >
                                                            {value.plugin_display_type} {this.stereo_indicator(value)}
                                                        </Typography>
                                                    </div>
                                                </div>
                                            </div>
                                        </ButtonBase>
                                    </Paper>
                                </Grid>
                            )
                        );

                    }
                }
                return result;
            }
            render() {
                const { classes } = this.props;


                let model = this.model;
                let plugins = model.ui_plugins.get();

                let selectedPlugin: UiPlugin | undefined = undefined;
                if (this.state.selected_uri) {
                    let t = this.model.getUiPlugin(this.state.selected_uri);
                    if (t) selectedPlugin = t;
                }

                return (

                    <React.Fragment>
                        <Dialog
                        TransitionComponent={Transition}
                            fullScreen={true}
                            
                            maxWidth={false}
                            open={this.props.open}
                            scroll="body"
                            onClose={this.handleCancel}
                            style={{ overflowX: "hidden", overflowY: "hidden", display: "flex", flexDirection:"column",flexWrap:"nowrap" }}
                            aria-labelledby="select-plugin-dialog-title">
                            <DialogTitle id="select-plugin-dialog-title" style={{ flex: "0 0 auto"}}>
                                <div style={{ display: "flex", flexDirection: "row", flexWrap: "nowrap", width: "100%", alignItems: "center" }}>
                                    <IconButton onClick={() => { this.cancel(); }} style={{ flex: "0 0 auto" }} >
                                        <ArrowBackIcon />
                                    </IconButton>

                                    <Typography display="block" variant="h6" style={{ flex: "0 1 auto" }}>
                                        {  this.state.client_width > 520 ? "Select Plugin" : "" }
                                    </Typography>
                                    <div style={{ flex: "1 1 auto" }} />
                                    <Select defaultValue={this.state.filterType} key={this.state.filterType} onChange={(e) => { this.onFilterChange(e); }}
                                        style={{ flex: "0 0 160px" }}
                                    >
                                        {this.createFilterOptions()}
                                    </Select>
                                    <div style={{ flex: "0 0 auto", marginRight: 24, visibility: this.state.filterType === PluginType.Plugin? "hidden": "visible" }} >
                                        <IconButton onClick={() => { this.onClearFilter(); }}>
                                            <ClearIcon fontSize='small' style={{ opacity: 0.6 }} />
                                        </IconButton>
                                    </div>
                                </div>
                            </DialogTitle>
                            <DialogContent dividers style={{ height: this.state.client_height - (this.state.client_width < NARROW_DISPLAY_THRESHOLD ? 288: 220), 
                                background: "#eee", display: "1 1 flex"}} >
                                <Grid container justify="flex-start" spacing={1} style={{ background: "#EEE" }}>
                                    {
                                        this.filterPlugins(plugins)
                                    }


                                </Grid>
                            </DialogContent>
                            { (this.state.client_width >= NARROW_DISPLAY_THRESHOLD)? (
                                <DialogActions style={{flex: "0 0 auto"}} >
                                    <div className={classes.bottom}>
                                        <div style={{ display: "flex", justifyContent: "start", alignItems: "center", flex: "1 1 auto", height: "100%", overflow: "hidden" }} >
                                            <PluginInfoDialog plugin_uri={this.state.selected_uri ?? ""} />
                                            <Typography display='block' variant='body2' color="textPrimary" noWrap >
                                                {this.info_string(selectedPlugin)}
                                            </Typography>
                                        </div>
                                        <div style={{ position: "relative", float: "right", flex: "0 1 200px", width: 200, display: "flex", alignItems: "center" }}>
                                            <Button onClick={this.handleCancel} style={{ width: 120 }} >Cancel</Button>
                                            <Button onClick={this.handleOk} color="secondary" disabled={selectedPlugin === null} style={{ width: 180 }} >SELECT</Button>
                                        </div>
                                    </div>
                                </DialogActions>
                            ) :(
                                <DialogActions style={{flex: "0 0 auto", display: "block"}} >
                                    <div style={{ display: "flex", justifyContent: "start", alignItems: "center", flex: "1 1 auto", overflow: "hidden" }} >
                                        <PluginInfoDialog plugin_uri={this.state.selected_uri ?? ""} />
                                        <div style={{width: 1, height: 48}} /> 
                                        <Typography display='block' variant='body2' color="textPrimary" noWrap >
                                            {this.info_string(selectedPlugin)}
                                        </Typography>
                                    </div>
                                    <div className={classes.bottom}>
                                        <div style={{flex: "1 1 1px"}}/>
                                        <div style={{ position: "relative", flex: "0 1 auto", display: "flex", alignItems: "center" }}>
                                            <Button onClick={this.handleCancel} style={{ width: 120, height: 48 }} >Cancel</Button>
                                            <Button onClick={this.handleOk} color="secondary" disabled={selectedPlugin === null} style={{ width: 120, height: 48 }} >SELECT</Button>
                                        </div>
                                    </div>
                                </DialogActions>

                            )}
                        </Dialog>
                    </React.Fragment >
                );
            }
        }
    );
export default LoadPluginDialog;

