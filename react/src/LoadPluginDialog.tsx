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

import React, { ReactNode, SyntheticEvent, CSSProperties, Fragment, ReactElement } from 'react';

import { PiPedalModel, PiPedalModelFactory, FavoritesList } from './PiPedalModel';
import { UiPlugin, PluginType } from './Lv2Plugin';
import TextField from '@mui/material/TextField';
import ButtonBase from '@mui/material/ButtonBase';
import Button from '@mui/material/Button';
import MenuItem from '@mui/material/MenuItem';
import Typography from '@mui/material/Typography';
import PluginInfoDialog from './PluginInfoDialog'
import PluginIcon from './PluginIcon'
import DialogEx from './DialogEx';
import DialogActions from '@mui/material/DialogActions';
import DialogContent from '@mui/material/DialogContent';
import DialogTitle from '@mui/material/DialogTitle';
import SelectHoverBackground from './SelectHoverBackground';
import ArrowBackIcon from '@mui/icons-material/ArrowBack';
import IconButton from '@mui/material/IconButton';
import PluginClass from './PluginClass'
import ResizeResponsiveComponent from './ResizeResponsiveComponent';
import SearchControl from './SearchControl';
import SearchFilter from './SearchFilter';
import { FixedSizeGrid } from 'react-window';
import AutoSizer from 'react-virtualized-auto-sizer';
import StarBorderIcon from '@mui/icons-material/StarBorder';
import { createStyles, Theme } from '@mui/material/styles';
import { WithStyles, withStyles } from '@mui/styles';
import FilterListIcon from '@mui/icons-material/FilterList';
import FilterListOffIcon from '@mui/icons-material/FilterListOff';



export type CloseEventHandler = () => void;
export type OkEventHandler = (pluginUri: string) => void;

const NARROW_DISPLAY_THRESHOLD = 600;
const FILTER_STORAGE_KEY = "com.twoplay.pipedal.load_dlg.filter";



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
        marginTop: "4px",
        marginBottom: "4px",
        marginLeft: "8px",
        marginRight: "8px",
        width: "100%",
        overflow: "hidden",
        textAlign: "left",
        display: "flex",
        flexDirection: "row",
        justifyContent: "flex-start",
        height: 48,
        alignItems: "start",
        flexWrap: "nowrap"
    },
    content2: {
        display: "flex", flexDirection: "column", flex: "1 1 auto", width: "100%",
        paddingLeft: 12, whiteSpace: "nowrap", textOverflow: "ellipsis"

    },
    favoriteDecoration: {
        flex: "0 0 auto"
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
    iconBorder: {
        flex: "0 0 auto",
        paddingTop: "4px"
    },
    label: {
        width: "100%"
    },
    label2: {
        marginTop: 8,
        color: theme.palette.text.secondary
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
    search_string: string;
    search_collapsed: boolean;
    filterType: PluginType,
    client_width: number,
    client_height: number,
    grid_cell_width: number,
    grid_cell_columns: number,
    minimumItemWidth: number,
    favoritesList: FavoritesList
    gridItems: UiPlugin[];


}


export const LoadPluginDialog =
    withStyles(pluginGridStyles, { withTheme: true })(
        class extends ResizeResponsiveComponent<PluginGridProps, PluginGridState>
        {
            model: PiPedalModel;
            searchInputRef: React.RefObject<HTMLInputElement>;

            constructor(props: PluginGridProps) {
                super(props);
                this.model = PiPedalModelFactory.getInstance();
                this.searchInputRef = React.createRef<HTMLInputElement>();

                let filterType_ = PluginType.Plugin; // i.e. "Any".
                let persistedFilter = window.localStorage.getItem(FILTER_STORAGE_KEY);
                if (persistedFilter) {
                    filterType_ = persistedFilter as PluginType;
                }

                this.state = {
                    selected_uri: this.props.uri,
                    search_string: "",
                    search_collapsed: true,
                    filterType: filterType_,
                    client_width: window.innerWidth,
                    client_height: window.innerHeight,
                    grid_cell_width: this.getCellWidth(window.innerWidth),
                    grid_cell_columns: this.getCellColumns(window.innerWidth),
                    minimumItemWidth: props.minimumItemWidth ? props.minimumItemWidth : 220,
                    favoritesList: this.model.favorites.get(),
                    gridItems: this.getFilteredPlugins("", filterType_, this.model.favorites.get())

                };

                this.updateWindowSize = this.updateWindowSize.bind(this);
                this.handleCancel = this.handleCancel.bind(this);
                this.handleOk = this.handleOk.bind(this);
                this.handleSearchStringReady = this.handleSearchStringReady.bind(this);
                this.handleKeyPress = this.handleKeyPress.bind(this);
                this.handleFavoritesChanged = this.handleFavoritesChanged.bind(this);

                this.requestScrollTo();
            }

            nominal_column_width: number = 250;
            margin_reserve: number = 30;

            getCellColumns(width: number) {
                let gridWidth = width - this.margin_reserve;
                let columns = Math.floor((gridWidth) / this.nominal_column_width);
                if (columns < 1) columns = 1;
                if (columns > 3) columns = 3;
                return columns;
            }


            getCellWidth(width: number) {
                return Math.floor(width / this.getCellColumns(width));
            }

            handleKeyPress(e: React.KeyboardEvent<HTMLDivElement>) {
                let searchInput = this.searchInputRef.current;
                if (searchInput && e.target !== searchInput) // if the search input doesn't have focus.
                {
                    if (this.searchInputRef.current) // we do have one, right?
                    {
                        if (e.key.length === 1) {
                            if (/[a-zA-Z0-9]/.exec(e.key))  // if it's alpha-numeric.
                            {
                                if (this.state.search_collapsed) // and search is collapsed.
                                {
                                    e.preventDefault();

                                    let newValue = searchInput.value + e.key;
                                    searchInput.value = newValue; // add the key. to the value.
                                    this.handleSearchStringChanged(newValue);
                                    searchInput.focus();
                                    this.setState({
                                        search_collapsed: false
                                    })

                                }
                            }
                        }
                    }
                }
            }

            updateWindowSize() {
                this.setState({
                    client_width: window.innerWidth,
                    client_height: window.innerHeight,
                    grid_cell_width: this.getCellWidth(window.innerWidth),
                    grid_cell_columns: this.getCellColumns(window.innerWidth)
                });
            }

            handleFavoritesChanged() {
                let favorites = this.model.favorites.get();
                this.requestScrollTo();
                this.setState(
                    {
                        favoritesList: favorites,
                        gridItems: this.getFilteredPlugins(null, null, favorites)
                    });
            }
            componentDidMount() {
                super.componentDidMount();
                this.updateWindowSize();
                window.addEventListener('resize', this.updateWindowSize);
                this.model.favorites.addOnChangedHandler(this.handleFavoritesChanged);
                let favorites = this.model.favorites.get();
                this.setState({
                    favoritesList: favorites,
                    gridItems: this.getFilteredPlugins(null, null, favorites)

                });

            }
            componentWillUnmount() {
                this.model.favorites.removeOnChangedHandler(this.handleFavoritesChanged);
                super.componentWillUnmount();
                window.removeEventListener('resize', this.updateWindowSize);
            }

            componentDidUpdate(oldProps: PluginGridProps) {
                if (oldProps.open !== this.props.open) {
                    if (this.props.open) {
                        // reset state now.
                        if (this.state.search_string !== "" || !this.state.search_collapsed) {
                            this.setState({
                                search_string: "",
                                search_collapsed: true,
                                gridItems: this.getFilteredPlugins("", null, null)
                            });
                        }
                        this.requestScrollTo();
                    }
                }
            }

            onWindowSizeChanged(width: number, height: number): void {
                super.onWindowSizeChanged(width, height);

            }

            onFilterChange(e: any) {
                let filterValue = e.target.value as PluginType;

                window.localStorage.setItem(FILTER_STORAGE_KEY, filterValue as string);

                this.requestScrollTo();
                this.setState({
                    filterType: filterValue,
                    gridItems: this.getFilteredPlugins(null, filterValue, null)
                });
            }
            onClearFilter(): void {
                let value = PluginType.Plugin;
                window.localStorage.setItem(FILTER_STORAGE_KEY, value as string);
                if (this.state.filterType !== value) {
                    this.requestScrollTo();
                    this.setState({
                        filterType: value,
                        gridItems: this.getFilteredPlugins(null, value, null)
                    });
                }
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
                // handled with onClick e.detail which provides better behaviour on Chrome.
                // this.props.onOk(uri);
            }


            cancel(): void {
                this.props.onCancel();
            }

            fireSelected(item?: UiPlugin) {
                if (item) {
                    this.setState({ selected_uri: item.uri });
                }
            }


            onClick(e: React.MouseEvent<HTMLDivElement>, uri: string): void {
                e.preventDefault();


                this.setState({ selected_uri: uri });

                // a better doubleclick: tracks how many recent clicks in the same area.
                // regardless of whether we re-rendered in the interrim. 
                let isDoubleClick = e.detail === 2;
                // we have to synthesize double clicks because 
                // DOM rewrites interfere with natural double click.
                if (isDoubleClick) {
                    this.props.onOk(uri);
                }

            }
            handleMouseEnter(e: SyntheticEvent, uri: string): void {
                // this.setHoverUri(uri);

            }
            handleMouseLeave(e: SyntheticEvent, uri: string): void {
                //  this.setHoverUri("");

            }
            onInfoClicked(): void {
                // let selectedUri = this.state.selected_uri;
            }

            vst3_indicator(uiPlugin?: UiPlugin) {
                if (uiPlugin?.is_vst3) {
                    return (<img alt="vst" src="/img/vst.svg" style={{ marginLeft: 8, height: 22, opacity: 0.6, position: "relative", top: -1 }} />)
                }
                return null;
            }
            stereo_indicator(uiPlugin?: UiPlugin): string {
                if (!uiPlugin) return "";
                if (uiPlugin.audio_inputs === 2 || uiPlugin.audio_outputs === 2) {
                    return " (Stereo)";
                }
                return "";
            }
            info_string(uiPlugin?: UiPlugin): ReactElement {
                if (!uiPlugin) {
                    return (<Fragment />);
                } else {
                    let stereoIndicator = "\u00A0" + this.stereo_indicator(uiPlugin)
                    if (uiPlugin.author_name !== "") {
                        if (uiPlugin.author_homepage !== "") {
                            return (<Fragment>
                                <div style={{flex: "0 1 auto"}}>
                                    <Typography variant='body2' color="textSecondary" noWrap>{uiPlugin.name + ",\u00A0"}</Typography>
                                </div>
                                <div style={{flex: "0 1 auto"}}>
                                    <a href={uiPlugin.author_homepage} target="_blank" rel="noopener noreferrer" >
                                        <Typography variant='body2' color="textSecondary" noWrap>{uiPlugin.author_name}</Typography>
                                    </a>
                                </div>
                                <div style={{flex: "0 0 auto"}}>
                                    <Typography variant='body2' color="textSecondary" noWrap>{stereoIndicator}</Typography>
                                </div>
                            </Fragment>);

                        } else {
                            return (<Fragment>
                                <Typography variant='body2' color="textSecondary" noWrap>{uiPlugin.name + ", " + uiPlugin.author_name + stereoIndicator}</Typography>
                            </Fragment>);
                        }
                    }
                    return (
                        <Fragment>
                            <Typography variant='body2' color="textSecondary" noWrap>{uiPlugin?.name ?? "" + stereoIndicator}</Typography>
                        </Fragment>);

                }
            }
            createFilterChildren(result: ReactNode[], classNode: PluginClass, level: number): void {
                for (let i = 0; i < classNode.children.length; ++i) {
                    let child = classNode.children[i];
                    let name = "\u00A0".repeat(level * 3 + 1) + child.display_name;
                    result.push((<MenuItem key={child.plugin_type} value={child.plugin_type}>{name}</MenuItem>));
                    if (child.children.length !== 0) {
                        this.createFilterChildren(result, child, level + 1);
                    }
                }
            }
            createFilterOptions(): ReactNode[] {
                let classes = this.model.plugin_classes.get();
                let result: ReactNode[] = [];

                result.push((<MenuItem key={PluginType.Plugin} value={PluginType.Plugin}>&nbsp;All</MenuItem>));
                this.createFilterChildren(result, classes, 1);
                return result;

            }
            getFilteredPlugins(searchString: string | null, filterType: PluginType | null, favoritesList: FavoritesList | null): UiPlugin[] {
                if (searchString === null) {
                    searchString = this.state.search_string;
                }
                if (filterType === null) {
                    filterType = this.state.filterType;
                }
                if (favoritesList === null) {
                    favoritesList = this.state.favoritesList;
                }
                let plugins = this.model.ui_plugins.get();

                let results: { score: number; plugin: UiPlugin }[] = [];
                let searchFilter = new SearchFilter(searchString);
                let rootClass = this.model.plugin_classes.get();


                for (let i = 0; i < plugins.length; ++i) {
                    let plugin = plugins[i];
                    try {
                        if (filterType === PluginType.Plugin || rootClass.is_type_of(filterType, plugin.plugin_type)) {
                            let score: number = 0;
                            if (plugin.is_vst3) {
                                score = searchFilter.score(plugin.name, plugin.plugin_display_type, plugin.author_name, "vst3");
                            } else {
                                score = searchFilter.score(plugin.name, plugin.plugin_display_type, plugin.author_name);
                            }

                            if (score !== 0) {
                                if (favoritesList[plugin.uri]) {
                                    score += 32768;
                                }
                                results.push({ score: score, plugin: plugin });
                            }
                        }
                    } catch (e)
                    {
                        alert("Bad plugin:" + (plugin?.name??"null") + " plugin type: " + (plugin?.plugin_type??"null"));
                    }
                }
                results.sort((left: { score: number; plugin: UiPlugin }, right: { score: number; plugin: UiPlugin }) => {
                    if (right.score < left.score) return -1;
                    if (right.score > left.score) return 1;
                    return left.plugin.name.localeCompare(right.plugin.name);
                });
                let t: UiPlugin[] = [];
                for (let i = 0; i < results.length; ++i) {
                    t.push(results[i].plugin);
                }
                return t;
            }

            changedSearchString?: string = undefined;
            hSearchTimeout?: NodeJS.Timeout;

            private scrollToRequested = false;
            requestScrollTo() {
                this.scrollToRequested = true;
            }

            handleScrollToCallback(element: FixedSizeGrid): void {
                if (element) {
                    if (this.scrollToRequested) {
                        this.scrollToRequested = false;
                        let position = -1;
                        let gridItems = this.state.gridItems;
                        for (let i = 0; i < gridItems.length; ++i) {
                            if (this.state.selected_uri === gridItems[i].uri) {
                                position = i;
                                break;
                            }
                        }
                        if (position !== -1) {
                            element.scrollToItem({ rowIndex: Math.floor(position / this.state.grid_cell_columns) });
                        }
                    }
                }
            }


            handleSearchStringReady() {
                if (this.changedSearchString !== undefined) {
                    this.requestScrollTo();
                    this.setState({
                        search_string: this.changedSearchString,
                        gridItems: this.getFilteredPlugins(this.changedSearchString, null, null)
                    });
                    this.changedSearchString = undefined;
                }
                this.hSearchTimeout = undefined;
            }


            handleSearchStringChanged(text: string): void {
                if (this.hSearchTimeout) {
                    clearTimeout(this.hSearchTimeout);
                    this.hSearchTimeout = undefined;
                }
                this.changedSearchString = text;
                this.hSearchTimeout = setTimeout(
                    this.handleSearchStringReady,
                    2000);

            }
            renderItem(row: number, column: number): React.ReactNode {
                let item: number = (row) * this.gridColumnCount + (column);
                let gridItems = this.state.gridItems;
                if (item >= gridItems.length) {
                    return (<div />);
                }
                let value = gridItems[item];

                let classes = this.props.classes;
                let isFavorite: boolean = this.state.favoritesList[value.uri] ?? false;

                return (
                    <div key={value.uri}
                        onDoubleClick={(e) => { this.onDoubleClick(e, value.uri) }}
                        onClick={(e) => { this.onClick(e, value.uri) }}
                        onMouseEnter={(e) => { this.handleMouseEnter(e, value.uri) }}
                        onMouseLeave={(e) => { this.handleMouseLeave(e, value.uri) }}
                    >
                        <ButtonBase className={classes.buttonBase} >
                            <SelectHoverBackground selected={value.uri === this.state.selected_uri} showHover={true} />
                            <div className={classes.content}>
                                <div className={classes.iconBorder} >
                                    <PluginIcon pluginType={value.plugin_type} size={24} opacity={0.6} />
                                </div>
                                <div className={classes.content2}>
                                    <div className={classes.label} style={{ display: "flex", flexFlow: "row nowrap", alignItems: "center" }} >

                                        <Typography color="textPrimary" noWrap sx={{ display: "block", flex: "0 1 auto", }} >
                                            {value.name}
                                        </Typography>
                                        {
                                            isFavorite && (
                                                <div style={{ flex: "0 0 auto" }}>
                                                    <StarBorderIcon sx={{ color: "#C80", fontSize: 16, marginRight: "2px" }} />
                                                </div>
                                            )
                                        }

                                    </div>
                                    <Typography color="textSecondary" noWrap>
                                        {value.plugin_display_type} {this.stereo_indicator(value)}

                                    </Typography>
                                </div>
                                <div className={classes.favoriteDecoration}>

                                </div>
                            </div>
                        </ButtonBase>
                    </div>
                );

            }
            setFavorite(pluginUri: string | undefined, isFavorite: boolean): void {
                if (!pluginUri) return;
                this.model.setFavorite(pluginUri, isFavorite);
            }
            gridColumnCount: number = 1;

            render() {
                const { classes } = this.props;

                let selectedPlugin: UiPlugin | undefined = undefined;
                if (this.state.selected_uri) {
                    let t = this.model.getUiPlugin(this.state.selected_uri);
                    if (t) selectedPlugin = t;
                }

                let showSearchIcon = true;
                if (this.state.client_width < 500) {
                    showSearchIcon = this.state.search_collapsed
                }
                let gridColumnCount = this.state.grid_cell_columns;
                this.gridColumnCount = gridColumnCount;
                let isFavorite = this.state.favoritesList[this.state.selected_uri ?? ""];
                return (
                    <React.Fragment>
                        <DialogEx tag="plugins"
                            onKeyPress={(e) => { this.handleKeyPress(e); }}
                            fullScreen={true}
                            TransitionComponent={undefined}
                            maxWidth={false}
                            open={this.props.open}
                            scroll="body"
                            onClose={this.handleCancel}
                            style={{ overflowX: "hidden", overflowY: "hidden", display: "flex", flexDirection: "column", flexWrap: "nowrap" }}
                            aria-labelledby="select-plugin-dialog-title">
                            <div style={{ display: "flex", flexDirection: "column", flexWrap: "nowrap", height: "100%" }}>
                                <DialogTitle id="select-plugin-dialog-title" style={{ flex: "0 0 auto", padding: "0px", height: 54 }}>
                                    <div style={{ display: "flex", flexDirection: "row", paddingTop: 3, paddingBottom: 3, flexWrap: "nowrap", width: "100%", alignItems: "center" }}>
                                        <IconButton onClick={() => { this.cancel(); }} style={{ flex: "0 0 auto" }} >
                                            <ArrowBackIcon />
                                        </IconButton>

                                        <Typography display="inline" noWrap variant="h6" style={{
                                            flex: "0 0 auto", height: "auto", verticalAlign: "center",
                                            visibility: (this.state.search_collapsed ? "visible" : "collapse"),
                                            width: (this.state.search_collapsed ? undefined : "0px")
                                        }}>
                                            {this.state.client_width > 520 ? "Select Plugin" : ""}
                                        </Typography>
                                        <div style={{ flex: "1 1 auto" }} >
                                            <SearchControl collapsed={this.state.search_collapsed} searchText={this.state.search_string}
                                                inputRef={this.searchInputRef}
                                                showSearchIcon={showSearchIcon}
                                                onTextChanged={(text) => {
                                                    this.handleSearchStringChanged(text);
                                                }}
                                                onClearFilterClick={() => {
                                                    if (this.state.search_string !== "") {
                                                        this.requestScrollTo();
                                                        this.setState({
                                                            search_collapsed: true,
                                                            search_string: "",
                                                            gridItems: this.getFilteredPlugins("", null, null)
                                                        });

                                                    } else {
                                                        this.setState({ search_collapsed: true });
                                                    }
                                                }}
                                                onClick={() => {
                                                    if (this.state.search_collapsed) {
                                                        this.setState({ search_collapsed: false });
                                                    } else {

                                                        if (this.state.search_string !== "") {
                                                            this.requestScrollTo();
                                                            this.setState({
                                                                search_collapsed: true,
                                                                search_string: "",
                                                                gridItems: this.getFilteredPlugins("", null, null)
                                                            });

                                                        } else {
                                                            this.setState({ search_collapsed: true });
                                                        }
                                                    }

                                                }}
                                            />
                                        </div>
                                        <div style={{ flex: "0 0 auto" }} >
                                            <IconButton onClick={() => { this.onClearFilter(); }}>
                                                {this.state.filterType === PluginType.Plugin ? (
                                                    <FilterListIcon fontSize='small' style={{ opacity: 0.75 }} />
                                                ) : (
                                                    <FilterListOffIcon fontSize='small' style={{ opacity: 0.75 }} />
                                                )}
                                            </IconButton>
                                        </div>
                                        <div style={{ flex: "0 0 160px", marginRight: 24 }} >

                                            <TextField select variant="standard"
                                                defaultValue={this.state.filterType}
                                                key={this.state.filterType}
                                                onChange={(e) => { this.onFilterChange(e); }}
                                                sx={{ minWidth: 160 }}
                                            >
                                                {this.createFilterOptions()}
                                            </TextField>
                                        </div>
                                    </div>
                                </DialogTitle>
                                <DialogContent dividers style={{
                                    height: "100px",
                                    padding: 8,
                                    flex: "1 1 100px",
                                }} >
                                    <AutoSizer>
                                        {(arg: any) => {
                                            if ((!arg.width) || (!arg.height)) {
                                                return (<div></div>);
                                            }
                                            let width = arg.width ?? 1;
                                            let height = arg.height ?? 1;
                                            let gridItems = this.state.gridItems;

                                            let scrollRef = (grid: FixedSizeGrid) => { this.handleScrollToCallback(grid); }


                                            return (
                                                <FixedSizeGrid
                                                    ref={scrollRef}
                                                    width={width}
                                                    columnCount={this.gridColumnCount}
                                                    columnWidth={(width - 40) / this.gridColumnCount}
                                                    height={height}
                                                    rowHeight={64}
                                                    overscanRowCount={10}
                                                    rowCount={Math.ceil(gridItems.length / this.gridColumnCount)}
                                                    itemKey={(args: { columnIndex: number, data: any, rowIndex: number }) => {
                                                        let index = args.columnIndex + this.gridColumnCount * args.rowIndex;
                                                        if (index >= gridItems.length) {
                                                            return "blank-" + args.columnIndex + "-" + args.rowIndex;
                                                        }
                                                        let plugin = gridItems[index];
                                                        return plugin.uri + "-" + args.rowIndex + "-" + args.columnIndex;

                                                    }}

                                                >
                                                    {(arg: { columnIndex: number, rowIndex: number, style: CSSProperties }) => (
                                                        <div style={arg.style} >
                                                            {
                                                                this.renderItem(arg.rowIndex, arg.columnIndex)
                                                            }
                                                        </div>
                                                    )}
                                                </FixedSizeGrid>
                                            );
                                        }
                                        }
                                    </AutoSizer>
                                </DialogContent>
                                {(this.state.client_width >= NARROW_DISPLAY_THRESHOLD) ? (
                                    <DialogActions style={{ flex: "0 0 auto" }} >
                                        <div className={classes.bottom}>
                                            <div style={{ display: "flex", textOverflow: "ellipsis", justifyContent: "start", alignItems: "center", flex: "1 1 auto", height: "100%", overflow: "hidden" }} >
                                                <PluginInfoDialog plugin_uri={this.state.selected_uri ?? ""} />
                                                {this.info_string(selectedPlugin)}
                                                {this.vst3_indicator(selectedPlugin)}
                                                <div style={{
                                                    color: isFavorite ? "#F80" : "#CCC", display: (this.state.selected_uri !== "" ? "block" : "none"),
                                                    position: "relative", top: -2
                                                }}>
                                                    <IconButton color="inherit" aria-label="Set as favorite"
                                                        onClick={() => { this.setFavorite(this.state.selected_uri, !isFavorite); }}
                                                    >
                                                        <StarBorderIcon />
                                                    </IconButton>
                                                </div>
                                            </div>
                                            <div style={{ position: "relative", float: "right", flex: "0 1 200px", width: 200, display: "flex", alignItems: "center" }}>
                                                <Button variant="dialogSecondary" onClick={this.handleCancel} style={{ width: 120 }} >Cancel</Button>
                                                <Button variant="dialogPrimary" onClick={this.handleOk} disabled={selectedPlugin === null} style={{ width: 180 }} >SELECT</Button>
                                            </div>
                                        </div>
                                    </DialogActions>
                                ) : (
                                    <DialogActions style={{ flex: "0 0 auto", display: "block" }} >
                                        <div style={{ display: "flex", justifyContent: "start", alignItems: "center", flex: "1 1 auto", overflow: "hidden" }} >
                                            <PluginInfoDialog plugin_uri={this.state.selected_uri ?? ""} />
                                            <div style={{ width: 1, height: 48 }} />
                                            {this.info_string(selectedPlugin)}
                                            <div style={{
                                                color: isFavorite ? "#F80" : "#CCC", display: (this.state.selected_uri ? "block" : "block"),
                                                position: "relative"
                                            }}>
                                                <IconButton color="inherit" aria-label="Set as favorite"
                                                    onClick={() => { this.setFavorite(this.state.selected_uri, !isFavorite); }}
                                                >
                                                    <StarBorderIcon />
                                                </IconButton>
                                            </div>
                                        </div>
                                        <div className={classes.bottom} style={{height: 64}}>
                                            <div style={{ flex: "1 1 1px" }} />
                                            <div style={{ position: "relative", flex: "0 1 auto", display: "flex", alignItems: "center" }}>
                                                <Button variant="dialogSecondary"  onClick={this.handleCancel} style={{ height: 48 }} >Cancel</Button>
                                                <Button variant="dialogPrimary" onClick={this.handleOk} disabled={selectedPlugin === null} style={{ height: 48 }} >SELECT</Button>
                                            </div>
                                        </div>
                                    </DialogActions>

                                )}
                            </div>
                        </DialogEx>
                    </React.Fragment >
                );
            }
        }
    );
export default LoadPluginDialog;

