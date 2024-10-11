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

import { WithStyles } from '@mui/styles';

import './AppThemed.css';
//import {alpha} from '@mui/material/styles';
import AppBar from '@mui/material/AppBar';
import Modal from '@mui/material/Modal';
import Toolbar from '@mui/material/Toolbar';
import CssBaseline from '@mui/material/CssBaseline';
import Typography from '@mui/material/Typography';
import createStyles from '@mui/styles/createStyles';
import withStyles from '@mui/styles/withStyles';
import IconButton from '@mui/material/IconButton';
import VolunteerActivismIcon from '@mui/icons-material/VolunteerActivism';
import MenuButton from '@mui/icons-material/Menu';
import { TemporaryDrawer } from './TemporaryDrawer';
import FullscreenIcon from '@mui/icons-material/Fullscreen';
import FullscreenExitIcon from '@mui/icons-material/FullscreenExit';
import List from '@mui/material/List';
import Divider from '@mui/material/Divider';
// import ListItem from '@mui/material/ListItem';
import ListItemButton from '@mui/material/ListItemButton';

import ListItemIcon from '@mui/material/ListItemIcon';
import ListItemText from '@mui/material/ListItemText';
import CircularProgress from '@mui/material/CircularProgress';
import { OnChangedHandler } from './ObservableProperty';
import ErrorOutlineIcon from '@mui/icons-material/Error';
import ResizeResponsiveComponent from './ResizeResponsiveComponent';
import Button from '@mui/material/Button';
import PresetSelector from './PresetSelector';
import SettingsDialog from './SettingsDialog';
import AboutDialog from './AboutDialog';
import BankDialog from './BankDialog';

import { PiPedalModelFactory, PiPedalModel, State, ZoomedControlInfo, wantsReloadingScreen } from './PiPedalModel';
import ZoomedUiControl from './ZoomedUiControl'
import MainPage from './MainPage';
import DialogContent from '@mui/material/DialogContent';
import DialogContentText from '@mui/material/DialogContentText';
import DialogActions from '@mui/material/DialogActions';
import ListSubheader from '@mui/material/ListSubheader';
import { BankIndex, BankIndexEntry } from './Banks';
import RenameDialog from './RenameDialog';
import JackStatusView from './JackStatusView';
import { Theme } from '@mui/material/styles';
import { isDarkMode } from './DarkMode';
import UpdateDialog from './UpdateDialog';

import { ReactComponent as RenameOutlineIcon } from './svg/drive_file_rename_outline_black_24dp.svg';
import { ReactComponent as SaveBankAsIcon } from './svg/ic_save_bank_as.svg';
import { ReactComponent as EditBanksIcon } from './svg/ic_edit_banks.svg';
import { ReactComponent as SettingsIcon } from './svg/ic_settings.svg';
import { ReactComponent as HelpOutlineIcon } from './svg/ic_help_outline.svg';
import { ReactComponent as FxAmplifierIcon } from './svg/fx_amplifier.svg';
import { PerformanceView } from './PerformanceView';

import DialogEx, { DialogStackState } from './DialogEx';


const appStyles = (theme: Theme) => createStyles({
    "&": { // :root
        colorScheme: (isDarkMode() ? "dark" : "light")
    },
    menuListItem: {
        color: "#FE8!important" as any, //theme.palette.text.primary,
        fill: "#FE8!important" as any, //theme.palette.text.primary,
    },
    menuIcon: {
        fill: (theme.palette.text.primary + "!important") as any, //theme.palette.text.primary,
        opacity: 0.6
    },
    toolBar: {
        color: "white"
    },
    listSubheader: {
        backgroundImage: "linear-gradient(255,255,255,0.15),rgba(255,255,255,0.15)"
    },
    loadingMask: {
        position: "absolute",
        minHeight: "10em",
        left: "0px",
        top: "0px",
        right: "0px",
        bottom: "0px",
        opacity: 0.8,
        background:
            isDarkMode() ? "#222" : "#DDD",
    },
    loadingContent: {
        display: "block",
        position: "absolute",
        minHeight: "10em",
        left: "0px",
        top: "0px",
        width: "100%",
        height: "100%",
        opacity: "0.95",
        justifyContent: "center",
        textAlign: "center",
        zIndex: 2010
    },
    errorContent: {

        display: "flex",
        flexDirection: "column",
        justifyContent: "center",
        flexWrap: "nowrap",
        alignItems: "center",
        position: "fixed",
        minHeight: "10em",
        left: "0px",
        top: "0px",
        width: "100%",
        height: "100%",
        color: isDarkMode() ? "#CCC" : "#444",
        zIndex: 2000
    },
    errorContentMask: {
        position: "absolute", left: 0, top: 0,
        width: "100%", height: "100%",
        background: isDarkMode() ? "#121212" : "#BBB",
        opacity: 0.95,
        zIndex: 1999
    },
    errorText: {
        marginTop: 0,
        padding: 0,
        marginBottom: 12,
        fontWeight: 500,
        fontSize: "13pt",
        maxWidth: 250,
        opacity: 1,
        zIndex: 2010,
    },
    progressText: {
        marginTop: 0,
        fontWeight: 500,
        fontSize: "13pt",
        opacity: 1,
        zIndex: 2010,
        paddingTop: 12,
        gravity: "center",
        textAlign: "center"
    },

    errorMessageBox: {
        flex: "0 0 auto",
        width: 300,
        marginLeft: "auto",
        marginRight: "auto",
        zIndex: 2010


    },
    errorMessage: {
        color: theme.palette.text.secondary,
        textAlign: "left",
        zIndex: 2010

    },
    loadingBox: {
        display: "flex",
        flexFlow: "column nowrap",
        position: "absolute",
        top: "20%",
        left: 0, right: 0,
        color: isDarkMode() ? theme.palette.text.secondary : "#888",
        // border: "3px solid #888",
        alignItems: "center",
        textAlign: "center",
        opacity: 0.8,

    },
    loadingBoxItem: {
        display: "flex", flexFlow: "row nowrap",
        alignItems: "center",
        textAlign: "center",

    },


    toolBarContent:
    {
        position: "absolute", top: 0, width: "100%"
    },

    toolBarSpacer:
    {
        position: "relative", flex: "0 0 auto",
    },


    mainFrame: {
        overflow: "hidden",
        display: "flex",
        flexFlow: "column",
        flex: "1 1 100%"
    },

    toolbarSizingPosition: {
        flexGrow: 0,
        flexShrink: 1,
        width: "100%",
        flexBasis: "auto"
    },
    mainSizingPosition: {
        overflow: "hidden",
        flex: "1 1 auto"
    },

    heroContent: {
        backgroundColor: theme.mainBackground,
        position: "relative",
        height: "100%",
        width: "100%"
    },

    drawerItem: {
        width: 350,
    },
    drawerItemFullWidth: {
        width: 'auto',
    },

});



function supportsFullScreen(): boolean {
    let doc: any = window.document;
    let docEl: any = doc.documentElement;
    var requestFullScreen = docEl.requestFullscreen || docEl.mozRequestFullScreen || docEl.webkitRequestFullscreen || docEl.msRequestFullScreen;
    return (!!requestFullScreen);

}

function setFullScreen(value: boolean) {
    let doc: any = window.document;
    let docEl: any = doc.documentElement;

    if (docEl.requestFullscren) // the latest offical api.
    {
        if (value) {
            window.document.documentElement.requestFullscreen({ navigationUI: "show" });
        } else {
            window.document.exitFullscreen();
        }
    }

    var requestFullScreen = docEl.requestFullscreen || docEl.mozRequestFullScreen || docEl.webkitRequestFullscreen || docEl.msRequestFullScreen;
    var cancelFullScreen = docEl.exitFullscreen || doc.mozCancelFullScreen || doc.webkitExitFullscreen || doc.msExitFullscreen;

    if (value) {
        requestFullScreen.call(docEl);
    }
    else {
        cancelFullScreen.call(doc);
    }
}


// function preventDefault(e: SyntheticEvent): void {
//     e.stopPropagation();
//     e.preventDefault();
// }

type AppState = {
    zoomedControlInfo: ZoomedControlInfo | undefined;

    isDrawerOpen: boolean;
    errorMessage: string;
    displayState: State;

    performanceView: boolean;

    canFullScreen: boolean;
    isFullScreen: boolean;
    tinyToolBar: boolean;
    alertDialogOpen: boolean;
    alertDialogMessage: string;
    updateDialogOpen: boolean;
    isSettingsDialogOpen: boolean;
    onboarding: boolean;
    isDebug: boolean;

    renameBankDialogOpen: boolean;
    saveBankAsDialogOpen: boolean;

    aboutDialogOpen: boolean;
    bankDialogOpen: boolean;
    editBankDialogOpen: boolean;
    zoomedControlOpen: boolean;


    presetName: string;
    presetChanged: boolean;
    banks: BankIndex;
    bankDisplayItems: number;
    showStatusMonitor: boolean;
};
interface AppProps extends WithStyles<typeof appStyles> {
}

const AppThemed = withStyles(appStyles)(class extends ResizeResponsiveComponent<AppProps, AppState> {
    // Before the component mounts, we initialise our state

    model_: PiPedalModel;
    errorChangeHandler_: OnChangedHandler<string>;
    stateChangeHandler_: OnChangedHandler<State>;

    constructor(props: AppProps) {
        super(props);
        this.model_ = PiPedalModelFactory.getInstance();

        this.model_.zoomedUiControl.addOnChangedHandler(
            () => {
                this.setState({
                    zoomedControlOpen: this.model_.zoomedUiControl.get() !== undefined,
                    zoomedControlInfo: this.model_.zoomedUiControl.get()
                });
            }
        );

        this.state = {
            performanceView: false,
            zoomedControlInfo: this.model_.zoomedUiControl.get(),
            isDrawerOpen: false,
            errorMessage: this.model_.errorMessage.get(),
            displayState: this.model_.state.get(),
            canFullScreen: supportsFullScreen() && !this.model_.isAndroidHosted(),
            isFullScreen: !!document.fullscreenElement,
            tinyToolBar: false,
            alertDialogOpen: false,
            alertDialogMessage: "",
            presetName: this.model_.presets.get().getSelectedText(),
            isSettingsDialogOpen: false,
            updateDialogOpen: false,
            onboarding: false,
            isDebug: true,
            presetChanged: this.model_.presets.get().presetChanged,
            banks: this.model_.banks.get(),
            renameBankDialogOpen: false,
            saveBankAsDialogOpen: false,
            aboutDialogOpen: false,
            bankDialogOpen: false,
            editBankDialogOpen: false,
            zoomedControlOpen: false,
            bankDisplayItems: 5,
            showStatusMonitor: this.model_.showStatusMonitor.get()

        };

        this.promptForUpdateHandler = this.promptForUpdateHandler.bind(this);
        this.errorChangeHandler_ = this.setErrorMessage.bind(this);
        this.beforeUnloadListener = this.beforeUnloadListener.bind(this);
        this.unloadListener = this.unloadListener.bind(this);
        this.stateChangeHandler_ = this.setDisplayState.bind(this);
        this.presetChangedHandler = this.presetChangedHandler.bind(this);
        this.alertMessageChangedHandler = this.alertMessageChangedHandler.bind(this);
        this.handleCloseAlert = this.handleCloseAlert.bind(this);
        this.banksChangedHandler = this.banksChangedHandler.bind(this);
        this.showStatusMonitorHandler = this.showStatusMonitorHandler.bind(this);
        this.onPopStateHandler = this.onPopStateHandler.bind(this);
    }

    showDrawer() {
        if (!this.state.isDrawerOpen) {
            this.setState({ isDrawerOpen: true })
            this.pushOpenMenuState();
        }
    }
    hideDrawer(loadingDialog: boolean = false) {
        if (!loadingDialog && window.location.hash === "#menu") {
            window.history.back();
        } else {
            this.setState({ isDrawerOpen: false })
        }
    }

    pushOpenMenuState() {
        let stackState = { tag: "OpenMenu", previousState: null };
        window.history.replaceState(stackState, "", window.location.href);
        window.history.pushState({}, "", "#menu");
    }
    onPopStateHandler(ev: PopStateEvent) {
        let stackState: DialogStackState | null = ev.state as (DialogStackState | null);
        if (stackState && stackState.tag === "OpenMenu") {
            if (this.state.isDrawerOpen) {
                this.setState({ isDrawerOpen: false });
            }
            ev.stopPropagation();
            window.history.replaceState(stackState.previousState, "", window.location.href);
            return true;
        }
        return false;
    }

    handleSpecificBank(bankId: number) {

        this.model_.openBank(bankId)
            .catch((error) => this.model_.showAlert(error.toString()));

    }

    handleSaveBankAsOk(newName: string) {
        let currentName = this.model_.banks.get().getSelectedEntryName();
        if (currentName === newName) {
            this.setState({
                renameBankDialogOpen: false,
                saveBankAsDialogOpen: false
            });
            return;

        }

        if (this.model_.banks.get().nameExists(newName)) {
            this.model_.showAlert("A bank by that name already exists.");
            return;
        }
        this.setState({
            renameBankDialogOpen: false,
            saveBankAsDialogOpen: false
        });
        this.model_.saveBankAs(this.model_.banks.get().selectedBank, newName)
            .catch((error) => {
                this.model_.showAlert(error.toString());
            });

    }
    handleBankRenameOk(newName: string) {
        let currentName = this.model_.banks.get().getSelectedEntryName();
        if (currentName === newName) {
            this.setState({
                renameBankDialogOpen: false,
                saveBankAsDialogOpen: false
            });
            return;

        }

        if (this.model_.banks.get().nameExists(newName)) {
            this.model_.showAlert("A bank by that name already exists.");
            return;
        }
        this.setState({
            renameBankDialogOpen: false
        });
        this.model_.renameBank(this.model_.banks.get().selectedBank, newName)
            .catch((error) => {
                this.model_.showAlert(error.toString());
            });

    }
    handleSettingsDialogClose() {
        this.setState({
            isSettingsDialogOpen: false,
            onboarding: false
        });
    }
    handleDrawerSettingsClick() {
        this.setState({
            isDrawerOpen: false,
            isSettingsDialogOpen: true,
            onboarding: false
        });

    }

    handleDisplayOnboarding() {
        this.setState({
            isDrawerOpen: false,
            isSettingsDialogOpen: true,
            onboarding: true
        });

    }

    handleDrawerManageBanks() {
        this.setState({
            isDrawerOpen: false,
            bankDialogOpen: true,
            editBankDialogOpen: true
        });

    }
    handleDrawerSelectBank() {

        this.setState({
            bankDialogOpen: true,
            editBankDialogOpen: false
        });

    }
    handleDrawerDonationClick() {
        this.hideDrawer(false);
        if (this.model_.isAndroidHosted()) {
            this.model_.showAndroidDonationActivity();
        } else {
            if (window) {
                window.open("https://github.com/sponsors/rerdavies", '_blank');
            }
        }

    }

    handleDrawerAboutClick() {
        this.setState({
            aboutDialogOpen: true
        });

    }
    handleDrawerRenameBank() {
        this.setState({
            renameBankDialogOpen: true,
            saveBankAsDialogOpen: false
        });

    }
    handleDrawerSaveBankAs() {
        this.setState({
            renameBankDialogOpen: false,
            saveBankAsDialogOpen: true
        });
    }


    handleCloseAlert(e?: any, reason?: any) {
        this.model_.alertMessage.set("");
    }
    showStatusMonitorHandler() {
        this.setState({
            showStatusMonitor: this.model_.showStatusMonitor.get()
        });
    }
    banksChangedHandler() {
        this.setState({
            banks: this.model_.banks.get()
        });
    }
    presetChangedHandler() {
        let presets = this.model_.presets.get();

        this.setState({
            presetName: presets.getSelectedText(),
            presetChanged: presets.presetChanged
        });
    }
    toggleFullScreen(): void {
        setFullScreen(!this.state.isFullScreen);
        this.setState({ isFullScreen: !this.state.isFullScreen });
    }

    private unloadListener(e: Event) {
        this.model_.close();
        return undefined;
    }

    private beforeUnloadListener(e: Event) {
        this.model_.close();
        return undefined;
    }
    promptForUpdateHandler(newValue: boolean) {
        if (this.model_.enableAutoUpdate) {
            if (this.state.updateDialogOpen !== newValue) {
                this.setState({ updateDialogOpen: newValue });
            }
        }
    }
    componentDidMount() {

        super.componentDidMount();
        window.addEventListener("beforeunload", this.beforeUnloadListener);
        window.addEventListener("unload", this.unloadListener);
        window.addEventListener("popstate", this.onPopStateHandler);

        this.model_.errorMessage.addOnChangedHandler(this.errorChangeHandler_);
        this.model_.state.addOnChangedHandler(this.stateChangeHandler_);
        this.model_.pedalboard.addOnChangedHandler(this.presetChangedHandler);
        this.model_.alertMessage.addOnChangedHandler(this.alertMessageChangedHandler);
        this.model_.banks.addOnChangedHandler(this.banksChangedHandler);
        this.model_.showStatusMonitor.addOnChangedHandler(this.showStatusMonitorHandler);
        this.model_.promptForUpdate.addOnChangedHandler(this.promptForUpdateHandler);
        this.alertMessageChangedHandler();

    }


    updateOverscroll(): void {
        if (this.model_.serverVersion) {
            // no pull-down refresh on android devices once we're ready (unless we're debug)
            let preventOverscroll =
                this.model_.state.get() === State.Ready
                && !this.model_.debug;

            let overscrollBehavior = preventOverscroll ? "none" : "auto";
            document.body.style.overscrollBehavior = overscrollBehavior;
        }
    }

    componentDidUpdate() {
    }

    componentWillUnmount() {
        super.componentWillUnmount();
        window.removeEventListener("beforeunload", this.beforeUnloadListener);

        window.removeEventListener("popstate", this.onPopStateHandler);


        this.model_.promptForUpdate.removeOnChangedHandler(this.promptForUpdateHandler);
        this.model_.errorMessage.removeOnChangedHandler(this.errorChangeHandler_);
        this.model_.state.removeOnChangedHandler(this.stateChangeHandler_);
        this.model_.pedalboard.removeOnChangedHandler(this.presetChangedHandler);
        this.model_.banks.removeOnChangedHandler(this.banksChangedHandler);
        this.model_.banks.removeOnChangedHandler(this.showStatusMonitorHandler);

        this.model_.close();

    }

    alertMessageChangedHandler() {
        let message = this.model_.alertMessage.get();
        if (message === "") {
            this.setState({ alertDialogOpen: false });
            // leave the message intact so the dialog can fade.
        } else {
            this.setState({
                alertDialogOpen: true,
                alertDialogMessage: message
            });
        }

    }
    updateResponsive() {
        // functional, but disabled.
        // let tinyToolBar_ = this.windowSize.height < 600;
        // this.setState({ tinyToolBar: tinyToolBar_ });

        let height = this.windowSize.height;

        const ENTRY_HEIGHT = 48;
        //   ENTRY_HEIGHT*6 +K = 727 from observation.
        const K = 450;

        let bankEntries = Math.floor((height - K) / ENTRY_HEIGHT);
        if (bankEntries < 1) bankEntries = 1;
        if (bankEntries > 7) bankEntries = 7;
        if (bankEntries === 2) bankEntries = 1;

        this.setState({ bankDisplayItems: bankEntries });

    }
    onWindowSizeChanged(width: number, height: number): void {
        super.onWindowSizeChanged(width, height);
        this.updateResponsive();
    }


    setErrorMessage(message: string): void {
        this.setState({ errorMessage: message });
    }


    setDisplayState(newState: State): void {
        this.updateOverscroll();

        this.setState({
            displayState: newState,
            canFullScreen: supportsFullScreen() && !this.model_.isAndroidHosted()
        });
        if (newState === State.Ready) {
            if (this.model_.isOnboarding()) {
                this.handleDisplayOnboarding();
            }
        }
    }

    shortBankList(banks: BankIndex): BankIndexEntry[] {
        let nDisplayEntries = this.state.bankDisplayItems;
        let entries = banks.entries;
        let nListEntries = entries.length;

        let result: BankIndexEntry[] = [];

        if (nListEntries <= nDisplayEntries) {
            for (let i = 0; i < nListEntries; ++i) {
                result.push(entries[i]);
            }
        } else {
            // a subset of the list CENTERED on the currently selected entry.
            let selectedIndex = 0;
            for (let i = 0; i < nListEntries; ++i) {
                if (entries[i].instanceId === banks.selectedBank) {
                    selectedIndex = i;
                    break;
                }
            }

            let minN = selectedIndex - Math.floor(nDisplayEntries / 2);
            if (minN < 0) minN = 0;
            let maxN = minN + nDisplayEntries;
            if (maxN > entries.length) {
                maxN = entries.length;
            }
            for (let i = minN; i < maxN; ++i) {
                result.push(entries[i]);
            }
        }
        return result;
    }
    handleReload() {
        if (this.model_.isAndroidHosted()) {
            this.model_.chooseNewDevice();
        } else {
            window.location.reload();
        }
    }
    getReloadingMessage(): string {
        switch (this.state.displayState) {
            case State.ApplyingChanges:
                return "Applying\u00A0changes...";
            case State.ReloadingPlugins:
                return "Reloading\u00A0plugins...";
            case State.DownloadingUpdate:
                return "Downloading update...";
            case State.InstallingUpdate:
                return "Installing update....";
            case State.HotspotChanging:
                return "Network connection changing..."
            default:
                return "Reconnecting...";
        }
    }
    render() {

        const { classes } = this.props;

        let shortBankList = this.shortBankList(this.state.banks);
        let showBankSelectDialog = shortBankList.length !== this.state.banks.entries.length;


        return (
            <div style={{
                colorScheme: isDarkMode() ? "dark" : "light", // affects scrollbar color
                minHeight: 300, minWidth: 300,
                position: "absolute", left: 0, top: 0, right: 0, bottom: 0,
                overscrollBehavior: this.state.isDebug ? "auto" : "none"
            }}
                onContextMenu={(e) => {
                    if (!this.model_.debug) {
                        e.preventDefault(); e.stopPropagation();
                    }
                }}
            >
                <CssBaseline />
                {this.state.performanceView ? (
                    <PerformanceView open={this.state.performanceView}
                        onClose={() => { this.setState({ performanceView: false }); }}
                    />
                ) : (
                    <div style={{
                        position: "absolute", width: "100%", height: "100%", userSelect: "none",
                        display: "flex", flexDirection: "column", flexWrap: "nowrap"
                    }}
                    >


                        {(!this.state.tinyToolBar) && !this.state.performanceView ?
                            (
                                <AppBar position="absolute"  >
                                    <Toolbar variant="dense" className={classes.toolBar}  >
                                        <IconButton
                                            edge="start"
                                            aria-label="menu"
                                            color="inherit"
                                            onClick={() => { this.showDrawer() }}
                                            size="large">
                                            <MenuButton style={{ opacity: 0.75 }} />
                                        </IconButton>
                                        <div style={{ flex: "0 1 400px", minWidth: 100 }}>
                                            <PresetSelector />
                                        </div>
                                        <div style={{ flex: "2 2 30px" }} />
                                        {this.state.canFullScreen &&
                                            <IconButton
                                                aria-label="menu"
                                                onClick={() => { this.toggleFullScreen(); }}
                                                color="inherit"
                                                size="large">
                                                {this.state.isFullScreen ? (
                                                    <FullscreenExitIcon style={{ opacity: 0.75 }} />
                                                ) : (
                                                    <FullscreenIcon style={{ opacity: 0.75 }} />

                                                )}

                                            </IconButton>
                                        }
                                    </Toolbar>
                                </AppBar>
                            ) : (
                                <div className={classes.toolBarContent} >
                                    <IconButton
                                        style={{ position: "absolute", left: 12, top: 8, zIndex: 2 }}
                                        aria-label="menu"
                                        onClick={() => { this.showDrawer() }}
                                        color="inherit"
                                        size="large">
                                        <MenuButton />
                                    </IconButton>
                                    {this.state.canFullScreen && (
                                        <IconButton
                                            style={{ position: "absolute", right: 8, top: 8, zIndex: 2 }}
                                            aria-label="menu"
                                            color="inherit"
                                            onClick={() => { this.toggleFullScreen(); }}
                                            size="large">
                                            {this.state.isFullScreen ? (
                                                <FullscreenExitIcon />
                                            ) : (
                                                <FullscreenIcon />

                                            )}

                                        </IconButton>
                                    )}
                                </div>
                            )}
                        <TemporaryDrawer position='left' title="PiPedal"
                            is_open={this.state.isDrawerOpen} onClose={() => { this.hideDrawer(false); }} >

                            <List>
                                <ListItemButton key='PerformanceView'
                                    onClick={(ev: any) => {
                                        ev.stopPropagation();
                                        this.hideDrawer(true);
                                        this.setState({ performanceView: true });
                                    }}>
                                    <ListItemIcon >
                                        <FxAmplifierIcon color='inherit' className={classes.menuIcon} style={{ width: 24, height: 24 }} />
                                    </ListItemIcon>
                                    <ListItemText primary='Performance View' />
                                </ListItemButton>
                            </List>
                            <Divider />
                            <ListSubheader className="listSubheader" component="div" id="xnested-list-subheader" style={{ lineHeight: "24px", height: 24, background: "rgba(12,12,12,0.0)" }}
                                disableSticky={true}
                            >
                                <Typography variant="caption" style={{}}>Banks</Typography></ListSubheader>

                            <List >
                                {
                                    shortBankList.map((bank) => {
                                        return (
                                            <ListItemButton key={'bank' + bank.instanceId} selected={bank.instanceId === this.state.banks.selectedBank}
                                                onClick={(ev: any) => {
                                                    ev.stopPropagation();
                                                    this.hideDrawer(false);
                                                    this.handleSpecificBank(bank.instanceId);
                                                }}
                                            >

                                                <ListItemText primary={bank.name} />
                                            </ListItemButton>

                                        );
                                    })
                                }
                                {
                                    showBankSelectDialog && (
                                        <ListItemButton key={'bankDOTDOTDOT'} selected={false}
                                            onClick={(ev: any) => {
                                                ev.stopPropagation();
                                                this.hideDrawer(true);
                                                this.handleDrawerSelectBank();
                                            }}
                                        >

                                            <ListItemText primary={"..."} />
                                        </ListItemButton>


                                    )
                                }
                            </List>
                            <Divider />
                            <List>
                                <ListItemButton key='RenameBank'
                                    onClick={(ev: any) => {
                                        ev.stopPropagation();
                                        this.hideDrawer(true);
                                        this.handleDrawerRenameBank()
                                    }}>
                                    <ListItemIcon >
                                        <RenameOutlineIcon color='inherit' className={classes.menuIcon} />
                                    </ListItemIcon>
                                    <ListItemText primary='Rename bank' />
                                </ListItemButton>
                                <ListItemButton key='SaveBank'
                                    onClick={(ev: any) => {
                                        ev.stopPropagation();
                                        this.hideDrawer(true);
                                        this.handleDrawerSaveBankAs();
                                    }} >
                                    <ListItemIcon>
                                        <SaveBankAsIcon color="inherit" className={classes.menuIcon} />
                                    </ListItemIcon>
                                    <ListItemText primary='Save as new bank' />
                                </ListItemButton>
                                <ListItemButton key='EditBanks'
                                    onClick={(ev: any) => {
                                        ev.stopPropagation();
                                        this.hideDrawer(true);
                                        this.handleDrawerManageBanks();
                                    }}>
                                    <ListItemIcon>
                                        <EditBanksIcon color="inherit" className={classes.menuIcon} />
                                    </ListItemIcon>
                                    <ListItemText primary='Manage banks...' />
                                </ListItemButton>
                            </List>
                            <Divider />
                            <List>
                                <ListItemButton key='Settings'
                                    onClick={(ev: any) => {
                                        ev.stopPropagation();
                                        this.hideDrawer(true);
                                        this.handleDrawerSettingsClick()
                                    }}>
                                    <ListItemIcon>
                                        <SettingsIcon color="inherit" className={classes.menuIcon} />
                                    </ListItemIcon>
                                    <ListItemText primary='Settings' />
                                </ListItemButton>
                                <ListItemButton key='About'
                                    onClick={(ev: any) => {
                                        ev.stopPropagation();
                                        this.hideDrawer(true);
                                        this.handleDrawerAboutClick();
                                    }}>
                                    <ListItemIcon>
                                        <HelpOutlineIcon color="inherit" className={classes.menuIcon} />
                                    </ListItemIcon>
                                    <ListItemText primary='About' />
                                </ListItemButton>
                                <ListItemButton key='Donations'
                                    onClick={(ev: any) => {
                                        ev.stopPropagation();
                                        this.handleDrawerDonationClick();
                                    }}>
                                    <ListItemIcon >
                                        <VolunteerActivismIcon className={classes.menuIcon} color="inherit" />
                                    </ListItemIcon>
                                    <ListItemText primary='Donations' />
                                </ListItemButton>
                            </List>

                        </TemporaryDrawer>
                        {!this.state.tinyToolBar && (
                            <Toolbar className={classes.toolBarSpacer} variant="dense"
                            />
                        )}
                        <main className={classes.mainFrame} >
                            <div className={classes.mainSizingPosition}>
                                <div className={classes.heroContent}>
                                    {(this.state.displayState !== State.Loading) &&
                                        (
                                            <MainPage hasTinyToolBar={this.state.tinyToolBar} enableStructureEditing={true} />
                                        )
                                    }
                                </div>

                            </div>
                        </main>
                        <BankDialog show={this.state.bankDialogOpen} isEditDialog={this.state.editBankDialogOpen} onDialogClose={() => this.setState({ bankDialogOpen: false })} />
                        {(this.state.aboutDialogOpen) &&
                            (
                                <AboutDialog open={this.state.aboutDialogOpen} onClose={() => this.setState({ aboutDialogOpen: false })} />
                            )}
                        <SettingsDialog
                            open={this.state.isSettingsDialogOpen}
                            onboarding={this.state.onboarding}
                            onClose={() => this.handleSettingsDialogClose()} />
                        <RenameDialog
                            open={this.state.renameBankDialogOpen || this.state.saveBankAsDialogOpen}
                            defaultName={this.model_.banks.get().getSelectedEntryName()}
                            acceptActionName={this.state.renameBankDialogOpen ? "Rename" : "Save as"}
                            onClose={() => {
                                this.setState({
                                    renameBankDialogOpen: false,
                                    saveBankAsDialogOpen: false
                                })
                            }}
                            onOk={(text: string) => {
                                if (this.state.renameBankDialogOpen) {
                                    this.handleBankRenameOk(text);
                                } else if (this.state.saveBankAsDialogOpen) {
                                    this.handleSaveBankAsOk(text);
                                }
                            }
                            }
                        />

                        <ZoomedUiControl
                            dialogOpen={this.state.zoomedControlOpen}
                            controlInfo={this.state.zoomedControlInfo}
                            onDialogClose={() => { this.setState({ zoomedControlOpen: false }); }}
                            onDialogClosed={() => { this.model_.zoomedUiControl.set(undefined); }
                            }
                        />
                        <UpdateDialog open={this.state.updateDialogOpen} />
                        {this.state.showStatusMonitor && (<JackStatusView />)}
                    </div>
                )
                }



                <DialogEx tag="Alert"
                    open={this.state.alertDialogOpen}
                    onClose={this.handleCloseAlert}
                    onEnterKey={this.handleCloseAlert}
                    aria-describedby="alert-dialog-description"
                >
                    <DialogContent>
                        <DialogContentText id="alert-dialog-description">
                            <Typography variant="body2">
                                {
                                    this.state.alertDialogMessage
                                }
                            </Typography>
                        </DialogContentText>
                    </DialogContent>
                    <DialogActions>
                        <Button variant="dialogPrimary" onClick={this.handleCloseAlert} color="primary" autoFocus>
                            OK
                        </Button>
                    </DialogActions>
                </DialogEx>


                <Modal open={this.state.displayState === State.Error}
                    aria-label="fatal-error"
                    aria-describedby="aria-error-text"
                >
                    <div style={{
                        display: "flex", flexFlow: "column nowrap",
                        position: "absolute", top: 0, left: 0, right: 0, bottom: 0
                    }}
                    >
                        <div className={classes.loadingMask} />
                        <div style={{ flex: "2 2 3px", height: 20 }} >&nbsp;</div>
                        <div className={classes.errorMessageBox} style={{ position: "relative" }} >
                            <div style={{ fontSize: "30px", position: "absolute", left: 0, top: 3, color: "#A00" }}>
                                <ErrorOutlineIcon color="inherit" fontSize="inherit" style={{ float: "left", marginRight: "12px" }} />
                            </div>
                            <div style={{ marginLeft: 40, marginTop: 3 }}>
                                <p className={classes.errorText} id="aria-error-text">
                                    Error: {this.state.errorMessage}
                                </p>
                            </div>
                            <div style={{ paddingTop: 50, paddingLeft: 36, textAlign: "left" }}>
                                <Button variant='contained' color="primary"
                                    onClick={() => this.handleReload()} >
                                    Reload
                                </Button>

                            </div>
                        </div>
                        <div style={{ flex: "5 5 auto", height: 20 }} >&nbsp;</div>
                    </div>

                </Modal >
                {/* Reloading mask */}
                < Modal
                    open={wantsReloadingScreen(this.state.displayState) || this.state.displayState === State.Loading}
                    aria-label="loading"
                    aria-describedby="reloading-modal-description"
                >
                    <div style={{display: "flex",flexFlow: "column nowrap", alignItems: "center"}}>
                        <div className={classes.loadingMask} />
                        <div className={classes.loadingBox}>
                            <div className={classes.loadingBoxItem}>
                                <CircularProgress color="inherit" className={classes.loadingBoxItem} />
                            </div>
                            <Typography id="reloading-modal-description" display="block" noWrap variant="body2" className={classes.progressText}>
                                {
                                    this.state.displayState === State.Loading ?
                                        "Loading..."
                                        :this.getReloadingMessage()
                                }
                            </Typography>
                        </div>
                    </div>
                </Modal >
            </div >

        );
    }
});

export default AppThemed;
