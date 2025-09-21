// Copyright (c) Robin E. R. Davies
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

import Divider from '@mui/material/Divider';
import Toolbar from "@mui/material/Toolbar";
import IconButtonEx from "./IconButtonEx";
import ArrowBackIcon from "@mui/icons-material/ArrowBack";
import Typography from "@mui/material/Typography";
import List from '@mui/material/List';
import ListItem from '@mui/material/ListItem';
import ListItemIcon from '@mui/material/ListItemIcon';
import ListItemText from '@mui/material/ListItemText';
import Checkbox from '@mui/material/Checkbox';
import Select from '@mui/material/Select';
import Button from '@mui/material/Button';
import InputLabel from '@mui/material/InputLabel';
import DialogEx from './DialogEx';
import DialogTitle from '@mui/material/DialogTitle';
import MenuItem from '@mui/material/MenuItem';
import DialogActions from '@mui/material/DialogActions';
import DialogContent from '@mui/material/DialogContent';
import ResizeResponsiveComponent from './ResizeResponsiveComponent';
import { PiPedalModel, PresetIndexEntry } from './PiPedalModel';
import { BankIndexEntry } from './Banks';


export interface ImportPresetFromBankDialogProps {
    open: boolean,
    onOk: (bankInstanceId: number, presets: number[]) => void,
    onClose: () => void
};

export interface ImportPresetFromBankDialogState {
    selectedBank: number;
    banks: BankIndexEntry[];
    bankPresets: PresetIndexEntry[] | null;
    selectedPresets: number[];

    fullScreen: boolean;
};

function setSavedDefaultBankSelection(bankInstanceId: number) {
    localStorage.setItem('pipedal_defaultBankSelection', bankInstanceId.toString());
}
function getSavedDefaultBankSelection() {
    let defaultValue = localStorage.getItem('pipedal_defaultBankSelection');
    if (defaultValue) {
        return parseInt(defaultValue);
    }
    return -1;
}
export default class ImportPresetFromBankDialog extends ResizeResponsiveComponent<ImportPresetFromBankDialogProps, ImportPresetFromBankDialogState> {

    model: PiPedalModel;
    constructor(props: ImportPresetFromBankDialogProps) {
        super(props);
        this.model = PiPedalModel.getInstance();
        let banks = this.getBanks();
        let selectedBank = this.getDefaultBankSelection(banks);
        this.state = {
            banks: banks,
            selectedBank: selectedBank,
            fullScreen: this.windowSize.height < 550,
            bankPresets: null,
            selectedPresets: []


        };
    }
    private getBanks() {
        let bankIndex = this.model.banks.get();
        let result: BankIndexEntry[] = [];
        for (let entry of bankIndex.entries) {
            if (entry.instanceId === bankIndex.selectedBank) continue;
            result.push(entry);
        }
        return result;
    }
    private getDefaultBankSelection(banks: BankIndexEntry[]) {
        if (banks.length === 0) return -1;
        let defaultSelection = getSavedDefaultBankSelection();
        for (let bank of banks) {
            if (bank.instanceId === defaultSelection) return defaultSelection;
        }
        return banks[0].instanceId;
    }

    private mounted: boolean = false;



    onWindowSizeChanged(width: number, height: number): void {
        this.setState({ fullScreen: height < 550 })
    }


    requestPresets(bankInstanceId: number) {
        this.model.requestBankPresets(bankInstanceId).then((presets) => {
            if (!this.mounted) return;
            this.setState({ bankPresets: presets, selectedPresets: [] });
        }).catch((error) => {
            this.model.showAlert("Error requesting presets: " + error);
        });
    }
    componentDidMount() {
        super.componentDidMount();
        this.mounted = true;
        this.requestPresets(this.state.selectedBank);
    }
    componentWillUnmount() {
        super.componentWillUnmount();
        this.mounted = false;
    }

    componentDidUpdate() {
    }
    checkForIllegalCharacters(filename: string) {
    }

    handleBankChanged(bankId: number) {
        setSavedDefaultBankSelection(bankId);
        this.setState({ selectedBank: bankId });
        this.requestPresets(bankId);
    }

    handleOk() {
        if (this.state.selectedPresets.length === 0) return;
        this.props.onOk(this.state.selectedBank, this.state.selectedPresets);
    }

    render() {
        let props = this.props;
        let { open, onClose } = props;

        const handleClose = () => {
            onClose();
        };
        return (
            <DialogEx tag="importPreset" open={open} fullWidth maxWidth="xs" onClose={handleClose} aria-labelledby="Rename-dialog-title"
                fullScreen={this.state.fullScreen}
                style={{ userSelect: "none" }}
                onEnterKey={() => { }}
            >
                <div style={{ display: "flex", flexFlow: "column nowrap", height: this.state.fullScreen ? "100vh" : "80vh" }}>
                    {!this.state.fullScreen ? (
                        <>
                            <DialogTitle style={{ paddingTop: 8, paddingBottom: 0 }}>
                                <Toolbar style={{ padding: 0 }}>
                                    <IconButtonEx
                                        tooltip="Close"
                                        edge="start"
                                        color="inherit"
                                        aria-label="cancel"
                                        style={{ opacity: 0.6 }}
                                        onClick={() => { onClose(); }}
                                    >
                                        <ArrowBackIcon style={{ width: 24, height: 24 }} />
                                    </IconButtonEx>

                                    <Typography noWrap component="div" sx={{ flexGrow: 1 }}>
                                        Import Presets from Bank
                                    </Typography>

                                </Toolbar>
                            </DialogTitle>
                            <DialogContent style={{ flex: "0 0 auto", paddingTop: 8, paddingBottom: 8, }}>
                                <InputLabel style={{ fontSize: "0.75rem", fontWeight: 400, marginTop: 0 }}>Bank</InputLabel>

                                <Select variant="standard" fullWidth value={this.state.selectedBank} style={{ marginBottom: 8 }}
                                    onChange={(e) => { this.handleBankChanged(e.target.value as number); }}>
                                    {this.state.banks.map((bankEntry) => {
                                        return (
                                            <MenuItem key={bankEntry.instanceId} value={bankEntry.instanceId}
                                                selected={bankEntry.instanceId === this.state.selectedBank}
                                            >
                                                {bankEntry.name}
                                            </MenuItem>
                                        );
                                    })}
                                </Select>
                            </DialogContent>
                        </>
                    ) : (
                        <div style={{ display: "flex", flexFlow: "row nowrap", alignItems: "top", flex: "0 0 auto", paddingTop: 8, paddingBottom: 8, paddingLeft: 16, paddingRight: 16 }}>
                            <IconButtonEx
                                tooltip="Close"
                                edge="start"
                                color="inherit"
                                aria-label="cancel"
                                style={{ opacity: 0.6 }}
                                onClick={() => { onClose(); }}
                            >
                                <ArrowBackIcon style={{ width: 24, height: 24 }} />
                            </IconButtonEx>

                            <div style={{ marginLeft: 24, display: "flex", flex: "0 0 auto", flexFlow: "column nowrap", justifyContent: "start" }}>
                                <InputLabel style={{ fontSize: "0.75rem", fontWeight: 400, marginTop: 0 }}>Bank</InputLabel>

                                <Select variant="standard" fullWidth value={this.state.selectedBank} style={{ marginBottom: 8 }}
                                    onChange={(e) => { this.handleBankChanged(e.target.value as number); }}>
                                    {this.state.banks.map((bankEntry) => {
                                        return (
                                            <MenuItem key={bankEntry.instanceId} value={bankEntry.instanceId}
                                                selected={bankEntry.instanceId === this.state.selectedBank}
                                            >
                                                {bankEntry.name}
                                            </MenuItem>
                                        );
                                    })}
                                </Select>
                            </div>
                        </div>


                    )}

                    <Divider />
                    <List sx={{ flex: "1 1 auto", width: '100%', bgColor: 'background.paper', overflowX: 'auto' }}>
                        {this.state.bankPresets?.map((presetEntry) => {
                            const labelId = `checkbox-list-label-${presetEntry.instanceId}`;

                            return (
                                <ListItem key={presetEntry.instanceId}
                                    onClick={() => {
                                        let selectedPresets = this.state.selectedPresets;
                                        let index = selectedPresets.indexOf(presetEntry.instanceId);
                                        if (index === -1) {
                                            selectedPresets.push(presetEntry.instanceId);
                                        } else {
                                            selectedPresets.splice(index, 1);
                                        }
                                        this.setState({ selectedPresets: [...selectedPresets] });
                                    }}>
                                    <ListItemIcon style={{ marginLeft: 16 }}>
                                        <Checkbox
                                            edge="start"
                                            checked={this.state.selectedPresets.indexOf(presetEntry.instanceId) !== -1}
                                            tabIndex={-1}
                                            disableRipple
                                            inputProps={{ 'aria-labelledby': labelId }}
                                        />
                                    </ListItemIcon>
                                    <ListItemText id={labelId} primary={presetEntry.name} />
                                </ListItem>
                            );
                        })}
                    </List>

                    <Divider />
                    <DialogActions style={{ flex: "0 0 auto" }}>
                        <Button onClick={handleClose} variant="dialogSecondary" >
                            Cancel
                        </Button>
                        <Button
                            onClick={() => { this.handleOk(); }} variant="dialogPrimary"
                            disabled={this.state.selectedPresets.length === 0}
                            style={{ opacity: this.state.selectedPresets.length === 0 ? 0.5 : 1.0 }}
                        >
                            OK
                        </Button>
                    </DialogActions>
                </div>
            </DialogEx >
        );
    }
}
