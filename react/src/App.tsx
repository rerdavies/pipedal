import { SyntheticEvent } from 'react';
import { ThemeProvider } from '@material-ui/styles';
import { createMuiTheme } from '@material-ui/core/styles';

import './App.css';
import AppBar from '@material-ui/core/AppBar';
import Toolbar from '@material-ui/core/Toolbar';
import CssBaseline from '@material-ui/core/CssBaseline';
import Typography from '@material-ui/core/Typography';
import { createStyles, withStyles, WithStyles } from '@material-ui/core/styles';
import IconButton from '@material-ui/core/IconButton';
import MenuButton from '@material-ui/icons/Menu';
import { TemporaryDrawer } from './TemporaryDrawer';
import { Theme } from '@material-ui/core/styles/createMuiTheme';
import FullscreenIcon from '@material-ui/icons/Fullscreen';
import FullscreenExitIcon from '@material-ui/icons/FullscreenExit';
import List from '@material-ui/core/List';
import Divider from '@material-ui/core/Divider';
import ListItem from '@material-ui/core/ListItem';
import ListItemIcon from '@material-ui/core/ListItemIcon';
import ListItemText from '@material-ui/core/ListItemText';
import CircularProgress from '@material-ui/core/CircularProgress';
import { OnChangedHandler } from './ObservableProperty';
import ErrorOutlineIcon from '@material-ui/icons/Error';
import ResizeResponsiveComponent from './ResizeResponsiveComponent';
import Button from '@material-ui/core/Button';
import PresetSelector from './PresetSelector';
import SettingsDialog from './SettingsDialog';
import AboutDialog from './AboutDialog';
import BankDialog from './BankDialog';

import { PiPedalModelFactory, PiPedalModel, State } from './PiPedalModel';
import MainPage from './MainPage';
import DialogContent from '@material-ui/core/DialogContent';
import DialogContentText from '@material-ui/core/DialogContentText';
import Dialog from '@material-ui/core/Dialog';
import DialogActions from '@material-ui/core/DialogActions';
import ListSubheader from '@material-ui/core/ListSubheader';
import { BankIndex, BankIndexEntry } from './Banks';
import RenameDialog from './RenameDialog';
import JackStatusView from './JackStatusView';

const theme = createMuiTheme({
    palette: {
        primary: {
            main: "#324c6c"
        }
        // secondary: {
        //   main: "#00E676"
        // }
    }
});

const appStyles = ({ palette, spacing, mixins }: Theme) => createStyles({
    loadingContent: {
        display: "block",
        position: "absolute",
        minHeight: "10em",
        left: "0px",
        top: "0px",
        width: "100%",
        height: "100%",
        background: "#DDD",
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
        color: "#444",
        zIndex: 2000
    },
    errorContentMask: {
        position: "absolute", left: 0, top: 0,
        width: "100%", height: "100%",
        background: "#BBB",
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
        maxWidth: 250,
        opacity: 1,
        zIndex: 2010,
        paddingTop: 12,
    },

    errorMessageBox: {
        flex: "0 0 auto",
        width: 300,
        marginLeft: "auto",
        marginRight: "auto",
        zIndex: 2010


    },
    errorMessage: {
        color: '#000',
        textAlign: "left",
        zIndex: 2010

    },
    loadingBox: {
        position: "relative",
        top: "20%",
        width: "120px",
        color: "#888",
        marginLeft: "auto",
        marginRight: "auto",
        // border: "3px solid #888",
        borderRadius: "12px",
        padding: "12px",
        justifyContent: "center",
        textAlign: "center",
        opacity: 0.8,
        zIndex: 2010

    },
    loadingBoxItem: {
        justifyContent: "center",
        textAlign: "center",
        zIndex: 2010

    },


    toolBarContent:
    {
        position: "absolute", top: 0, width: "100%"
    },

    toolBarSpacer:
    {
        position: "relative", flex: "0 0 auto"
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
        backgroundColor: palette.background.paper,
        position: "relative",
        height: "100%",
        width: "100%"
    },

    drawerItem: {
        width: 350,
        background: "#FF8080"
    },
    drawerItemFullWidth: {
        width: 'auto',
    },

    icon: {
        marginRight: spacing(2),
    },

    heroButtons: {
        marginTop: spacing(4),
    },
    cardGrid: {
        paddingTop: spacing(8),
        paddingBottom: spacing(8),
    },
    card: {
        height: '100%',
        display: 'flex',
        flexDirection: 'column',
    },
    cardMedia: {
        paddingTop: '56.25%', // 16:9
    },
    cardContent: {
        flexGrow: 1,
    },
    footer: {
        backgroundColor: palette.background.paper,
        padding: spacing(6),
    },
});


function supportsFullScreen(): boolean {
    let doc: any = window.document;
    let docEl: any = doc.documentElement;
    var requestFullScreen = docEl.requestFullscreen || docEl.mozRequestFullScreen || docEl.webkitRequestFullScreen || docEl.msRequestFullscreen;
    return (!!requestFullScreen);

}

function setFullScreen(value: boolean) {
    let doc: any = window.document;
    let docEl: any = doc.documentElement;

    var requestFullScreen = docEl.requestFullscreen || docEl.mozRequestFullScreen || docEl.webkitRequestFullScreen || docEl.msRequestFullscreen;
    var cancelFullScreen = doc.exitFullscreen || doc.mozCancelFullScreen || doc.webkitExitFullscreen || doc.msExitFullscreen;

    if (!doc.fullscreenElement && !doc.mozFullScreenElement && !doc.webkitFullscreenElement && !doc.msFullscreenElement) {
        requestFullScreen.call(docEl);
    }
    else {
        cancelFullScreen.call(doc);
    }
}


function preventDefault(e: SyntheticEvent): void {
    e.stopPropagation();
    e.preventDefault();
}

type AppState = {
    isDrawerOpen: boolean;
    errorMessage: string;
    displayState: State;

    canFullScreen: boolean;
    isFullScreen: boolean;
    tinyToolBar: boolean;
    alertDialogOpen: boolean;
    alertDialogMessage: string;
    isSettingsDialogOpen: boolean;
    isDebug: boolean;

    renameBankDialogOpen: boolean;
    saveBankAsDialogOpen: boolean;

    aboutDialogOpen: boolean;
    bankDialogOpen: boolean;
    editBankDialogOpen: boolean;


    presetName: string;
    presetChanged: boolean;
    banks: BankIndex;
    bankDisplayItems: number;
};
interface AppProps extends WithStyles<typeof appStyles> {
}

const App = withStyles(appStyles)(class extends ResizeResponsiveComponent<AppProps, AppState> {
    // Before the component mounts, we initialise our state

    model_: PiPedalModel;
    errorChangeHandler_: OnChangedHandler<string>;
    stateChangeHandler_: OnChangedHandler<State>;

    constructor(props: AppProps) {
        super(props);
        this.model_ = PiPedalModelFactory.getInstance();

        this.state = {
            isDrawerOpen: false,
            errorMessage: this.model_.errorMessage.get(),
            displayState: this.model_.state.get(),
            canFullScreen: supportsFullScreen(),
            isFullScreen: !!document.fullscreenElement,
            tinyToolBar: false,
            alertDialogOpen: false,
            alertDialogMessage: "",
            presetName: this.model_.presets.get().getSelectedText(),
            isSettingsDialogOpen: false,
            isDebug: true,
            presetChanged: this.model_.presets.get().presetChanged,
            banks: this.model_.banks.get(),
            renameBankDialogOpen: false,
            saveBankAsDialogOpen: false,
            aboutDialogOpen: false,
            bankDialogOpen: false,
            editBankDialogOpen: false,
            bankDisplayItems: 5

        };

        this.errorChangeHandler_ = this.setErrorMessage.bind(this);
        this.stateChangeHandler_ = this.setDisplayState.bind(this);
        this.presetChangedHandler = this.presetChangedHandler.bind(this);
        this.alertMessageChangedHandler = this.alertMessageChangedHandler.bind(this);
        this.handleCloseAlert = this.handleCloseAlert.bind(this);
        this.banksChangedHandler = this.banksChangedHandler.bind(this);

    }

    onOpenBank(bankId: number) {
        this.model_.openBank(bankId)
            .catch((error) => this.model_.showAlert(error));

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
                this.model_.showAlert(error);
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
                this.model_.showAlert(error);
            });

    }
    handleSettingsDialogClose() {
        this.setState({
            isSettingsDialogOpen: false
        });
    }
    handleDrawerSettingsClick() {
        this.setState({
            isDrawerOpen: false,
            isSettingsDialogOpen: true
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
            isDrawerOpen: false,
            bankDialogOpen: true,
            editBankDialogOpen: false
        });

    }

    handleDrawerAboutClick() {
        this.setState({
            aboutDialogOpen: true
        });

    }
    handleDrawerRenameBank() {
        this.setState({
            isDrawerOpen: false,
            renameBankDialogOpen: true,
            saveBankAsDialogOpen: false
        });

    }
    handleDrawerSaveBankAs() {
        this.setState({
            isDrawerOpen: false,
            renameBankDialogOpen: false,
            saveBankAsDialogOpen: true
        });
    }


    handleCloseAlert(e?: any, reason?: any) {
        this.model_.alertMessage.set("");
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
        setFullScreen(this.state.isFullScreen);
        this.setState({ isFullScreen: !this.state.isFullScreen });
    }
    componentDidMount() {

        super.componentDidMount();

        window.addEventListener("beforeunload", (e) => {
            e.preventDefault();
            if (this.model_.state.get() === State.Ready)
            {
                e.returnValue = "Are you sure you want to leave this page?";
                return "Are you sure you want to leave this page?";
            }
        });

        this.model_.errorMessage.addOnChangedHandler(this.errorChangeHandler_);
        this.model_.state.addOnChangedHandler(this.stateChangeHandler_);
        this.model_.pedalBoard.addOnChangedHandler(this.presetChangedHandler);
        this.model_.alertMessage.addOnChangedHandler(this.alertMessageChangedHandler);
        this.model_.banks.addOnChangedHandler(this.banksChangedHandler);
        this.alertMessageChangedHandler();
    }


    updateOverscroll(): void {
        if (this.model_.serverVersion) {
            // no pull-down refresh on android devices once we're ready (unless we're debug)
            let preventOverscroll =
                this.model_.state.get() === State.Ready
                && !this.model_.serverVersion.debug;

            let overscrollBehavior = preventOverscroll ? "none" : "auto";
            document.body.style.overscrollBehavior = overscrollBehavior;
        }
    }

    componentDidUpdate() {
    }

    componentWillUnmount() {
        super.componentWillUnmount();
        this.model_.errorMessage.removeOnChangedHandler(this.errorChangeHandler_);
        this.model_.state.removeOnChangedHandler(this.stateChangeHandler_);
        this.model_.pedalBoard.removeOnChangedHandler(this.presetChangedHandler);
        this.model_.banks.addOnChangedHandler(this.banksChangedHandler);

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

        let bankEntries =  Math.floor((height-K)/ENTRY_HEIGHT);
        if (bankEntries < 1) bankEntries = 1;
        if (bankEntries > 7) bankEntries = 7;
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

        this.setState({ displayState: newState });
    }

    showDrawer() {
        this.setState({ isDrawerOpen: true })
    }
    hideDrawer() {
        this.setState({ isDrawerOpen: false })
    }
    shortBankList(banks: BankIndex): BankIndexEntry[] {
        let n = this.state.bankDisplayItems;
        let entries = banks.entries;
        if (entries.length < n + 1) {  // +1 for the .... entry.
            return entries;
        }
        let result: BankIndexEntry[] = [];
        let selectedIndex = -1;
        for (let i = 0; i < entries.length; ++i) {
            if (entries[i].instanceId === banks.selectedBank) {
                selectedIndex = i;
                break;
            }
        }
        if (n > entries.length) n = entries.length;
        if (selectedIndex > n) {
            for (let i = 0; i < n - 1; ++i) {
                result.push(entries[i]);
            }
            result.push(entries[selectedIndex]);
        } else {
            for (let i = 0; i < n; ++i) {
                result.push(entries[i]);
            }
        }
        return result;
    }
    render() {

        const { classes } = this.props;

        let shortBankList = this.shortBankList(this.state.banks);
        let showBankSelectDialog = shortBankList.length !== this.state.banks.entries.length;


        return (
            <ThemeProvider theme={theme}>
                <div style={{
                    minHeight: 345, minWidth: 390,
                    position: "absolute", width: "100%", height: "100%", background: "#F88", userSelect: "none",
                    display: "flex", flexDirection: "column", flexWrap: "nowrap",
                    overscrollBehavior: this.state.isDebug ? "auto" : "none"
                }}
                onContextMenu={(e) => { 
                    if (!this.model_.serverVersion?.debug??false)
                    {
                        e.preventDefault(); e.stopPropagation(); 
                    }
                }}
                >
                    <CssBaseline />
                    {(!this.state.tinyToolBar) ?
                        (
                            <AppBar position="absolute">
                                <Toolbar variant="dense" >
                                    <IconButton edge="start"
                                        color="inherit" aria-label="menu" onClick={() => { this.showDrawer() }}
                                    >
                                        <MenuButton />
                                    </IconButton>
                                    <div style={{ flex: "1 1 1px" }} />
                                    <div style={{ flex: "0 1 400px", minWidth: 100 }}>
                                        <PresetSelector />
                                    </div>
                                    <div style={{ flex: "2 2 30px" }} />
                                    {this.state.canFullScreen &&
                                        <IconButton
                                            color="inherit" aria-label="menu" onClick={() => { this.toggleFullScreen(); }}>
                                            {this.state.isFullScreen ? (
                                                <FullscreenExitIcon />
                                            ) : (
                                                <FullscreenIcon />

                                            )}

                                        </IconButton>
                                    }
                                </Toolbar>
                            </AppBar>
                        ) : (
                            <div className={classes.toolBarContent} >
                                <IconButton style={{ position: "absolute", left: 12, top: 8, zIndex: 2 }}
                                    color="inherit" aria-label="menu" onClick={() => { this.showDrawer() }}
                                >
                                    <MenuButton />
                                </IconButton>
                                {this.state.canFullScreen && (
                                    <IconButton style={{ position: "absolute", right: 8, top: 8, zIndex: 2 }}
                                        color="inherit" aria-label="menu" onClick={() => { this.toggleFullScreen(); }}>
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
                        is_open={this.state.isDrawerOpen} onClose={() => { this.hideDrawer(); }} >
                        <List subheader={
                            <ListSubheader component="div" id="nested-list-subheader">Banks</ListSubheader>
                        }>
                            {
                                shortBankList.map((bank) => {
                                    return (
                                        <ListItem button key={'bank' + bank.instanceId} selected={bank.instanceId === this.state.banks.selectedBank}
                                            onClick={() => this.onOpenBank(bank.instanceId)}
                                        >

                                            <ListItemText primary={bank.name} />
                                        </ListItem>

                                    );
                                })
                            }
                            {
                                showBankSelectDialog && (
                                    <ListItem button key={'bankDOTDOTDOT'} selected={false}
                                        onClick={() => this.handleDrawerSelectBank()}
                                    >

                                        <ListItemText primary={"..."} />
                                    </ListItem>


                                )
                            }
                        </List>
                        <Divider />
                        <List>
                            <ListItem button key='RenameBank' onClick={() => { this.handleDrawerRenameBank() }}>
                                <ListItemIcon><img src="img/drive_file_rename_outline_black_24dp.svg" alt="" style={{ opacity: 0.6 }} /></ListItemIcon>
                                <ListItemText primary='Rename Bank' />
                            </ListItem>
                            <ListItem button key='SaveBank' onClick={() => { this.handleDrawerSaveBankAs() }} >
                                <ListItemIcon>
                                    <img src="img/save_bank_as.svg" alt="" style={{ opacity: 0.6 }} />
                                </ListItemIcon>
                                <ListItemText primary='Save As New Bank' />
                            </ListItem>
                            <ListItem button key='CreateBank' onClick={() => { this.handleDrawerManageBanks(); }}>
                                <ListItemIcon>
                                    <img src="img/edit_banks.svg" alt="" style={{ opacity: 0.6 }} />
                                </ListItemIcon>
                                <ListItemText primary='Manage Banks...' />
                            </ListItem>
                        </List>
                        <Divider />
                        <List>
                            <ListItem button key='Settings' onClick={() => { this.handleDrawerSettingsClick() }}>
                                <ListItemIcon>
                                    <img src="img/settings_black_24dp.svg" alt="" style={{ opacity: 0.6 }} />
                                </ListItemIcon>
                                <ListItemText primary='Settings' />
                            </ListItem>
                            <ListItem button key='About' onClick={() => { this.handleDrawerAboutClick() }}>
                                <ListItemIcon>
                                    <img src="img/help_outline_black_24dp.svg" alt="" style={{ opacity: 0.6 }} />
                                </ListItemIcon>
                                <ListItemText primary='About' />
                            </ListItem>
                        </List>

                    </TemporaryDrawer>
                    {!this.state.tinyToolBar && (
                        <Toolbar className={classes.toolBarSpacer} variant="dense" />
                    )}
                    <main className={classes.mainFrame} >
                        <div className={classes.mainSizingPosition}>
                            <div className={classes.heroContent}>
                                {(this.state.displayState !== State.Loading) && (
                                    <MainPage hasTinyToolBar={this.state.tinyToolBar} />
                                )}
                            </div>

                        </div>
                    </main>
                    <BankDialog show={this.state.bankDialogOpen} isEditDialog={this.state.editBankDialogOpen} onDialogClose={() => this.setState({ bankDialogOpen: false })} />
                    <AboutDialog open={this.state.aboutDialogOpen} onClose={() => this.setState({ aboutDialogOpen: false })} />
                    <SettingsDialog open={this.state.isSettingsDialogOpen} onClose={() => this.handleSettingsDialogClose()} />
                    <RenameDialog
                        open={this.state.renameBankDialogOpen || this.state.saveBankAsDialogOpen}
                        defaultName={this.model_.banks.get().getSelectedEntryName()}
                        acceptActionName={"Rename"}
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

                    <Dialog
                        open={this.state.alertDialogOpen}
                        onClose={this.handleCloseAlert}
                        aria-describedby="alert-dialog-description"
                    >
                        <DialogContent>
                            <DialogContentText id="alert-dialog-description">
                                {
                                    this.model_.alertMessage.get()
                                }
                            </DialogContentText>
                        </DialogContent>
                        <DialogActions>
                            <Button onClick={this.handleCloseAlert} color="primary" autoFocus>
                                OK
                            </Button>
                        </DialogActions>
                    </Dialog>
                    <JackStatusView />
                    <div className={classes.errorContent} style={{ display: this.state.displayState === State.Reconnecting ? "block" : "none" }}
                    >
                        <div className={classes.errorContentMask} />

                        <div className={classes.loadingBox}>
                            <div className={classes.loadingBoxItem}>
                                <CircularProgress color="inherit" className={classes.loadingBoxItem} />
                            </div>
                            <Typography variant="body2" className={classes.progressText}>
                                Reconnecting...
                            </Typography>
                        </div>
                    </div>
                    <div className={classes.errorContent} style={{ display: this.state.displayState === State.Error ? "flex" : "none" }}
                        onMouseDown={preventDefault} onKeyDown={preventDefault}
                    >
                        <div className={classes.errorContentMask} />
                        <div style={{ flex: "2 2 3px", height: 20 }} >&nbsp;</div>
                        <div className={classes.errorMessageBox} style={{ position: "relative" }} >
                            <div style={{ fontSize: "30px", position: "absolute", left: 0, top: 0, color: "#A00" }}>
                                <ErrorOutlineIcon color="inherit" fontSize="inherit" style={{ float: "left", marginRight: "12px" }} />
                            </div>
                            <div style={{ marginLeft: 40, marginTop: 3 }}>
                                <p className={classes.errorText}>
                                    Error: {this.state.errorMessage}
                                </p>
                            </div>
                            <div style={{ paddingTop: 50, paddingLeft: 36, textAlign: "left" }}>
                                <Button variant='contained' color="primary" component='button'
                                    onClick={() => window.location.reload()} >
                                    Reload
                                </Button>

                            </div>

                        </div>

                        <div style={{ flex: "5 5 auto", height: 20 }} >&nbsp;</div>

                    </div>
                    <div className={classes.errorContent} style={{ display: this.state.displayState === State.Loading ? "block" : "none" }}>
                        <div className={classes.errorContentMask} />
                        <div className={classes.loadingBox}>
                            <div className={classes.loadingBoxItem}>
                                <CircularProgress color="inherit" className={classes.loadingBoxItem} />
                            </div>
                            <Typography variant="body2" className={classes.loadingBoxItem}>
                                Loading...
                            </Typography>
                        </div>
                    </div>

                </div >
            </ThemeProvider>
        );
    }
}
);

export default App;
