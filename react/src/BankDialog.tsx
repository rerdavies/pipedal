import React, { Component } from 'react';
import IconButton from '@material-ui/core/IconButton';
import Typography from '@material-ui/core/Typography';
import { PiPedalModel, PiPedalModelFactory } from './PiPedalModel';
import { BankIndexEntry, BankIndex } from './Banks';
import Button from "@material-ui/core/Button";
import ButtonBase from "@material-ui/core/ButtonBase";
import { TransitionProps } from '@material-ui/core/transitions/transition';
import Slide from '@material-ui/core/Slide';
import Dialog from '@material-ui/core/Dialog';
import AppBar from '@material-ui/core/AppBar';
import Toolbar from '@material-ui/core/Toolbar';
import { Theme, withStyles, WithStyles, createStyles } from '@material-ui/core/styles';
import DraggableGrid, { ScrollDirection } from './DraggableGrid';

import SelectHoverBackground from './SelectHoverBackground';
import CloseIcon from '@material-ui/icons/Close';
import ArrowBackIcon from '@material-ui/icons/ArrowBack';
import EditIcon from '@material-ui/icons/Edit';
import RenameDialog from './RenameDialog';

interface BankDialogProps extends WithStyles<typeof styles> {
    show: boolean;
    isEditDialog: boolean;
    onDialogClose: () => void;

};

interface BankDialogState {
    banks: BankIndex;

    showActionBar: boolean;

    selectedItem: number;

    filenameDialogOpen: boolean;
    filenameSaveAs: boolean;

};


const styles = (theme: Theme) => createStyles({
    dialogAppBar: {
        position: 'relative',
        top: 0, left: 0
    },
    dialogActionBar: {
        position: 'relative',
        top: 0, left: 0,
        background: "black"
    },
    dialogTitle: {
        marginLeft: theme.spacing(2),
        flex: 1,
    },
    itemFrame: {
        display: "flex",
        flexDirection: "row",
        flexWrap: "nowrap",
        width: "100%",
        height: "56px",
        alignItems: "center",
        textAlign: "left",
        justifyContent: "center",
        paddingLeft: 8
    },
    iconFrame: {
        flex: "0 0 auto",

    },
    itemIcon: {
        width: 24, height: 24, margin: 12, opacity: 0.6
    },
    itemLabel: {
        flex: "1 1 1px",
        marginLeft: 8
    }

});


const Transition = React.forwardRef(function Transition(
    props: TransitionProps & { children?: React.ReactElement },
    ref: React.Ref<unknown>,
) {
    return <Slide direction="up" ref={ref} {...props} />;
});

// eslint-disable-next-line @typescript-eslint/no-unused-vars
const ActionBarTransition = React.forwardRef(function Transition(
    props: TransitionProps & { children?: React.ReactElement },
    ref: React.Ref<unknown>,
) {
    return <Slide direction="right" ref={ref} {...props} />;
});


const BankDialog = withStyles(styles, { withTheme: true })(

    class extends Component<BankDialogProps, BankDialogState> {

        model: PiPedalModel;



        constructor(props: BankDialogProps) {
            super(props);
            this.model = PiPedalModelFactory.getInstance();

            this.handleDialogClose = this.handleDialogClose.bind(this);
            let banks = this.model.banks.get();
            this.state = {
                banks: banks,
                showActionBar: false,
                selectedItem: banks.selectedBank,
                filenameDialogOpen: false,
                filenameSaveAs: false
            };
            this.handleBanksChanged = this.handleBanksChanged.bind(this);
            this.handlePopState = this.handlePopState.bind(this);

        }

        selectItemAtIndex(index: number)
        {
            let instanceId = this.state.banks.entries[index].instanceId;
            this.setState({selectedItem: instanceId});
        }
        isEditMode() {
            return this.state.showActionBar || this.props.isEditDialog;
        }

        handleBanksChanged() {
            let banks = this.model.banks.get();

            if (!banks.areEqual(this.state.banks,false)) // avoid a bunch of peculiar effects if we update while a drag is in progress
            {
                // if we don't have a valid selection, then use the current preset.
                if (this.state.banks.getEntry(this.state.selectedItem) == null)
                {
                    this.setState({  banks: banks, selectedItem: banks.selectedBank});
                } else {
                    this.setState({ banks: banks });
                }
                }
        }

        mounted: boolean = false;

        hasHooks: boolean = false;
    
        stateWasPopped: boolean = false;
        handlePopState(e: any): any {
            let state: any = e.state;
            if (!state || !state.bankDialog)
            {
                this.stateWasPopped = true;
                this.props.onDialogClose();
            }
        }
    
        updateBackButtonHooks() : void {
            let wantHooks = this.mounted && this.props.show;
            if (wantHooks !== this.hasHooks)
            {
                this.hasHooks = wantHooks;
    
                if (this.hasHooks)
                {
                    this.stateWasPopped = false;
                    window.addEventListener("popstate",this.handlePopState);
                    // eslint-disable-next-line no-restricted-globals
                    let newState: any = history.state;
                    if (!newState)
                    {
                        newState = {};
                    }
                    newState.bankDialog = true;
                    // eslint-disable-next-line no-restricted-globals
                    history.pushState(
                        newState,
                        "PiPedal - Banks",
                        "#Banks"
                    );
                } else {
                    window.removeEventListener("popstate",this.handlePopState);
                    if (!this.stateWasPopped)
                    {
                        // eslint-disable-next-line no-restricted-globals
                        history.back();
                    }
                    // eslint-disable-next-line no-restricted-globals
                    history.replaceState({},"PiPedal","#");
                }
    
            }
        }
    
    
        componentDidUpdate()
        {
            this.updateBackButtonHooks();
        }
        componentDidMount() {
            this.model.banks.addOnChangedHandler(this.handleBanksChanged);
            this.handleBanksChanged();
            // scroll selected item into view.
            this.mounted = true;
            this.updateBackButtonHooks();
        }
        componentWillUnmount() {
            this.model.banks.removeOnChangedHandler(this.handleBanksChanged);
            this.mounted = false;
            this.updateBackButtonHooks();
        }

        getSelectedIndex() {
            let instanceId = this.isEditMode() ? this.state.selectedItem: this.state.banks.selectedBank;
            let banks = this.state.banks;
            for (let i = 0; i < banks.entries.length; ++i) {
                if (banks.entries[i].instanceId === instanceId) return i;
            }
            return -1; 
        }

        handleDeleteClick() {
            if (!this.state.selectedItem) return;
            let selectedItem = this.state.selectedItem;
            if (selectedItem !== -1) {
                this.model.deleteBankItem(selectedItem)
                .then((newSelection: number) => {
                    this.setState({
                        selectedItem: newSelection
                    });
                })
                .catch((error) => {
                        this.model.showAlert(error);
                    });
            }
        }
        handleDialogClose()
        {
            this.props.onDialogClose();
        }

        handleItemClick(instanceId: number): void {
            if (this.isEditMode()) {
                this.setState({ selectedItem: instanceId });
            } else {
                this.model.openBank(instanceId);
                this.props.onDialogClose();
            }
        }
        showActionBar(show: boolean): void {
            this.setState({ showActionBar: show });

        }


        mapElement(el: any): React.ReactNode {
            let presetEntry = el as BankIndexEntry;
            let classes = this.props.classes;
            let selectedItem = this.isEditMode() ? this.state.selectedItem : this.state.banks.selectedBank;
            return (
                <div key={presetEntry.instanceId} style={{ background: "white" }} >

                    <ButtonBase style={{ width: "100%", height: 48 }}
                        onClick={() => this.handleItemClick(presetEntry.instanceId)}
                    >
                        <SelectHoverBackground selected={presetEntry.instanceId === selectedItem} showHover={true} />
                        <div className={classes.itemFrame}>
                            <div className={classes.iconFrame}>
                                <img src="img/ic_bank.svg" className={classes.itemIcon} alt="" />
                            </div>
                            <div className={classes.itemLabel}>
                                <Typography>
                                    {presetEntry.name}
                                </Typography>
                            </div>
                        </div>
                    </ButtonBase>
                </div>

            );
        }

        moveElement(from: number, to: number): void {
            let newBanks = this.state.banks.clone();
            newBanks.moveBank(from, to);
            this.setState({
                banks: newBanks,
                selectedItem: newBanks.entries[to].instanceId
            });
            this.model.moveBank(from, to)
            .catch((error)=> {
                this.model.showAlert(error);
            });
        }

        getSelectedName(): string {
            let item = this.state.banks.getEntry(this.state.selectedItem);
            if (item) return item.name;
            return "";
        }

        handleRenameClick() {
            let item = this.state.banks.getEntry(this.state.selectedItem);
            if (item) {
                this.setState({ filenameDialogOpen: true, filenameSaveAs: false });
            }
        }
        handleRenameOk(text: string) {
            let item = this.state.banks.getEntry(this.state.selectedItem);
            if (!item) return;
            if (item.name !== text) {
                if (this.state.banks.hasName(text))
                {
                    this.model.showAlert("A bank with that name already exists.");
                }
                this.model.renameBank(this.state.selectedItem, text)
                    .catch((error) => {
                        this.onError(error);
                    });
            }

            this.setState({ filenameDialogOpen: false });
        }
        handleSaveAsOk(text: string) {
            let item = this.state.banks.getEntry(this.state.selectedItem);
            if (item) 
            {
                if (item.name !== text) {
                    if (this.state.banks.hasName(text))
                    {
                        this.model.showAlert("A bank with that name already exists.");
                        return;
                    }
                    this.model.saveBankAs(this.state.selectedItem, text)
                        .then((newSelection) => {
                            this.setState({selectedItem: newSelection});
                        })
                        .catch((error) => {
                            this.onError(error);
                        });
                }
            }
            this.setState({ filenameDialogOpen: false });
        }

        handleCopy() {
            let item = this.state.banks.getEntry(this.state.selectedItem);
            if (item) {
                this.setState({ filenameDialogOpen: true, filenameSaveAs: true });
            }
        }

        onError(error: string): void {
            this.model?.showAlert(error);
        }



        render() {
            let classes = this.props.classes;

            let actionBarClass = this.props.isEditDialog ? classes.dialogAppBar : classes.dialogActionBar;
            let defaultSelectedIndex = this.getSelectedIndex();

            return (
                <Dialog fullScreen open={this.props.show}
                    onClose={() => {  this.handleDialogClose() }} TransitionComponent={Transition}>
                    <div style={{ display: "flex", flexDirection: "column", flexWrap: "nowrap", width: "100%", height: "100%", overflow: "hidden" }}>
                        <div style={{ flex: "0 0 auto" }}>
                            <AppBar className={classes.dialogAppBar} style={{ display: this.isEditMode() ? "none" : "block" }} >
                                <Toolbar>
                                    <IconButton edge="start" color="inherit" onClick={this.handleDialogClose} aria-label="back"
                                        disabled={this.isEditMode()}
                                    >
                                        <ArrowBackIcon />
                                    </IconButton>
                                    <Typography variant="h6" className={classes.dialogTitle}>
                                        Banks
                                    </Typography>
                                    <IconButton color="inherit" onClick={(e) => this.showActionBar(true)} >
                                        <EditIcon />
                                    </IconButton>
                                </Toolbar>
                            </AppBar>
                            <AppBar className={actionBarClass} style={{ display: this.isEditMode() ? "block" : "none" }}
                                onClick={(e) => { e.stopPropagation(); e.preventDefault(); }}
                            >
                                <Toolbar>
                                    {(!this.props.isEditDialog) ? (
                                        <IconButton edge="start" color="inherit" onClick={(e) => this.showActionBar(false)} aria-label="close">
                                            <CloseIcon />
                                        </IconButton>
                                    ) : (
                                        <IconButton edge="start" color="inherit" onClick={this.handleDialogClose} aria-label="back"
                                        >
                                            <ArrowBackIcon />
                                        </IconButton>

                                    )}
                                    <Typography variant="h6" className={classes.dialogTitle}>
                                        Banks
                                    </Typography>
                                    {(this.state.banks.getEntry(this.state.selectedItem) != null)
                                        && (

                                            <div style={{ flex: "0 0 auto" }}>
                                                <Button color="inherit" onClick={(e) => this.handleCopy()}>
                                                    Copy
                                                </Button>
                                                <Button color="inherit" onClick={() => this.handleRenameClick()}>
                                                    Rename
                                                </Button>
                                                <RenameDialog 
                                                    open={this.state.filenameDialogOpen} 
                                                    defaultName={this.getSelectedName()}
                                                    acceptActionName={ this.state.filenameSaveAs?  "SAVE AS": "RENAME"}
                                                    onClose={() => { this.setState({ filenameDialogOpen: false }) }}
                                                    onOk={(text: string) => {
                                                        if (this.state.filenameSaveAs)
                                                        {
                                                            this.handleSaveAsOk(text);
                                                        } else {
                                                            this.handleRenameOk(text);
                                                        }
                                                    }
                                                    }
                                                />
                                                <IconButton color="inherit" onClick={(e) => this.handleDeleteClick()} >
                                                    <img src="/img/old_delete_outline_white_24dp.svg" alt="Delete" style={{width: 24, height: 24, opacity: 0.6}} />
                                                </IconButton>
                                            </div>
                                        )
                                    }
                                </Toolbar>

                            </AppBar>
                        </div>
                        <div style={{ flex: "1 1 auto", position: "relative", overflow: "hidden" }} >
                            <DraggableGrid
                                onLongPress={(item) => this.showActionBar(true)}
                                canDrag={this.isEditMode()}
                                onDragStart={(index,x,y)=> {this.selectItemAtIndex(index) } }
                                moveElement={(from, to) => { this.moveElement(from, to); }}
                                scroll={ScrollDirection.Y}
                                defaultSelectedIndex={defaultSelectedIndex}
                                >
                                {
                                    this.state.banks.entries.map((element) =>
                                    {
                                        return this.mapElement(element);
                                    })
                                }
                            </DraggableGrid>
                        </div>
                    </div>
                </Dialog >

            );

        }

    }

);

export default BankDialog;