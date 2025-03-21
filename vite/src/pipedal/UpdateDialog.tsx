/*
 * MIT License
 * 
 * Copyright (c) 2022 Robin E. R. Davies
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

import Button from '@mui/material/Button';
import Typography from '@mui/material/Typography';
import Divider from '@mui/material/Divider';
import DialogEx from './DialogEx';
import DialogTitle from '@mui/material/DialogTitle';
import DialogActions from '@mui/material/DialogActions';
import DialogContent from '@mui/material/DialogContent';
import { UpdateStatus, UpdateRelease, UpdatePolicyT, intToUpdatePolicyT } from './Updater';
import { PiPedalModelFactory, PiPedalModel } from './PiPedalModel';
import Select from '@mui/material/Select';
import MenuItem from '@mui/material/MenuItem';
import ResizeResponsiveComponent from './ResizeResponsiveComponent';


const UPDATE_CHECK_DELAY = 86400000; // one day in ms.
// const UPDATE_CHECK_DELAY = 30 * 1000; // testing



export interface UpdateDialogProps {
    open: boolean;
};

export interface UpdateDialogState {
    updateStatus: UpdateStatus;
    compactWidth: boolean;
    alertDialogOpen: boolean;
    alertDialogMessage: string;
};

export default class UpdateDialog extends ResizeResponsiveComponent<UpdateDialogProps, UpdateDialogState> {
    private model: PiPedalModel;

    constructor(props: UpdateDialogProps) {
        super(props);
        this.model = PiPedalModelFactory.getInstance();
        let updateStatus = this.model.updateStatus.get();
        this.state = {
            updateStatus: updateStatus,
            compactWidth: this.isCompactWidth(),
            alertDialogOpen: false,
            alertDialogMessage: ""
        };
        this.onUpdateStatusChanged = this.onUpdateStatusChanged.bind(this);
    }
    isCompactWidth() { return this.windowSize.width < 420; }

    onUpdateStatusChanged(newValue: UpdateStatus) {
        if (!newValue.equals(this.state.updateStatus)) {
            this.setState({ updateStatus: newValue });
        }
    }


    componentDidMount() {
        super.componentDidMount();
        this.model.updateStatus.addOnChangedHandler(this.onUpdateStatusChanged);
    }
    componentWillUnmount() {
        super.componentWillUnmount();
        this.model.updateStatus.removeOnChangedHandler(this.onUpdateStatusChanged);

    }
    onWindowSizeChanged(width: number, height: number): void {
        super.onWindowSizeChanged(width, height);
        if (this.isCompactWidth() !== this.state.compactWidth) {
            this.setState({ compactWidth: this.isCompactWidth() });
        }
    }

    private handleOK() {
        this.model.showUpdateDialog(false);
    }
    private handleUpdateNow() {
        this.model.updateNow()
            .then(() => { })   // all handling is done by model, so we can run ui-less.
            .catch((e: any) => {
                this.showAlert(e.toString());
            });
    }
    private handleUpdateLater() {
        this.model.updateLater(UPDATE_CHECK_DELAY);
        this.model.showUpdateDialog(false); // allow close if launched from the settings window.
    }

    private handleClose() {
        // ideally, we'd like to not close at all, but it screws up the nav backstack if we dont.
        // so, close, but do so very briefly
        if (this.canUpgrade()) {
            this.model.updateLater(UPDATE_CHECK_DELAY); // close and re-open immediately.
        }
        this.model.showUpdateDialog(false);
    }

    handleCloseAlert() {
        this.setState({ alertDialogOpen: false, alertDialogMessage: "" })
    }

    showAlert(message: string) {
        this.setState({ alertDialogOpen: true, alertDialogMessage: message });
    }

    onViewReleaseNotes() {
        this.model.launchExternalUrl("https://rerdavies.github.io/pipedal/ReleaseNotes.html");
    }

    upToDate(): boolean {
        let updateStatus = this.state.updateStatus;
        let updateRelease: UpdateRelease = updateStatus.getActiveRelease();
        return !updateRelease.updateAvailable
    }
    canUpgrade(): boolean {
        let updateStatus = this.state.updateStatus;
        if (updateStatus.updatePolicy === UpdatePolicyT.Disable) {
            return false;
        }
        if (updateStatus.errorMessage !== "") {
            return false;
        }
        let updateRelease = updateStatus.getActiveRelease();

        return updateStatus.isValid && updateStatus.isOnline && updateRelease.updateAvailable;
    }

    private onPolicySelected(newValue: string | number): void {
        if (newValue.toString() === "") return;
        let updatePolicy = intToUpdatePolicyT(newValue as number);
        this.model.setUpdatePolicy(updatePolicy);

    }
    render() {
        let updateStatus = this.state.updateStatus;
        let updateRelease = updateStatus.getActiveRelease();
        let upToDate = this.upToDate();
        let canUpgrade = this.canUpgrade();
        let showUpgradeVersion = !upToDate && updateStatus.isValid && updateRelease.upgradeVersionDisplayName !== "";
        let compactWidth = this.state.compactWidth;
        return (
            <DialogEx tag="update" open={this.props.open} onClose={() => { this.handleClose(); }}
                style={{ userSelect: "none" }}
                onEnterKey={()=>{ this.handleOK();}}
            >
                <DialogTitle>
                    <div style={{ display: "flex", flexFlow: "row noWrap", alignItems: "center" }} >
                        <Typography style={{ flexGrow: 1, flexShrink: 1, marginRight: 20 }} noWrap>Updates</Typography>
                        <Select style={{ opacity: 0.6 }} variant="standard" value={updateStatus.updatePolicy} onChange={(ev) => { this.onPolicySelected(ev.target.value); }}>
                            <MenuItem value={UpdatePolicyT.ReleaseOnly}>Release only</MenuItem>
                            <MenuItem value={UpdatePolicyT.ReleaseOrBeta}>Release or Beta</MenuItem>
                            <MenuItem value={UpdatePolicyT.Development}>Development</MenuItem>
                            <MenuItem value={UpdatePolicyT.Disable}>Disable</MenuItem>
                        </Select>
                    </div>

                </DialogTitle>
                <Divider />

                <DialogContent>
                    {(upToDate) && (
                        <Typography variant="body2" color="textSecondary" style={{ marginBottom: 12 }}>
                            PiPedal is up to date.
                        </Typography>
                    )
                    }
                    {(canUpgrade) && (
                        <Typography variant="body2" color="textSecondary" style={{ marginBottom: 12 }}>
                            A new version of the PiPedal server is available. Would you like to update now?
                        </Typography>


                    )
                    }
                    {
                        (compactWidth) ? (
                            <div>
                                <Typography noWrap variant="body2" color="textSecondary" >
                                    Current version:
                                </Typography>
                                <Typography noWrap variant="body2" color="textSecondary" style={{ marginLeft: 16 }} >
                                    {updateStatus.currentVersionDisplayName}
                                </Typography>
                                {(showUpgradeVersion) && (
                                    <div style={{marginTop: 4}}>
                                        <Typography noWrap variant="body2" color="textSecondary"  >
                                            Update  version:
                                        </Typography>
                                        <Typography noWrap variant="body2" color="textSecondary" style={{ marginLeft: 16 }} >
                                            {updateRelease.upgradeVersionDisplayName}
                                        </Typography>
                                    </div>
                                )}

                            </div>
                        ) : (
                            <div style={{ display: "flex", flexFlow: "row noWrap" }}>
                                <div>
                                    <Typography noWrap variant="body2" color="textSecondary" >
                                        Current version:
                                    </Typography>
                                    {(showUpgradeVersion) && (
                                        <Typography noWrap variant="body2" color="textSecondary" style={{ marginTop: 4 }} >
                                            Update  version:
                                        </Typography>
                                    )
                                    }
                                </div>
                                <div style={{ flexShrink: 1, flexGrow: 1, marginLeft: 12 }}>
                                    <Typography noWrap variant="body2" color="textSecondary" >
                                        {updateStatus.currentVersionDisplayName}
                                    </Typography>
                                    {(showUpgradeVersion) && (
                                        <Typography noWrap variant="body2" color="textSecondary" style={{ marginTop: 4 }} >
                                            {updateRelease.upgradeVersionDisplayName}
                                        </Typography>
                                    )
                                    }
                                </div>
                            </div>

                        )
                    }

                    {
                        (updateStatus.errorMessage.length !== 0) &&
                        (
                            <Typography variant="body2" style={{ marginTop: 12 }}>
                                {
                                    updateStatus.errorMessage
                                }
                            </Typography>

                        )
                    }
                    <Button style={{ marginLeft: 8, marginTop: 12 }} onClick={() => { this.onViewReleaseNotes(); }}>View release notes</Button>
                </DialogContent>
                <Divider />
                <DialogActions>
                    {
                        (canUpgrade) ?
                            (
                                <div style={{ display: "flex", flexFlow: "row noWrap", alignItems: "center" }}>
                                    <Button variant="dialogSecondary" onClick={() => { this.handleUpdateLater(); }}>Update later</Button>
                                    <div style={{ flexGrow: 1, flexShrink: 1 }} />
                                    <Button variant="dialogPrimary" onClick={() => { this.handleUpdateNow(); }}>Update now</Button>
                                </div>


                            ) : (

                                <div style={{ display: "flex", flexFlow: "row noWrap", alignItems: "center" }}

                                >
                                    <div style={{ flexGrow: 1, flexShrink: 1 }} />
                                    <Button onClick={() => { this.handleOK(); }} variant="dialogPrimary">OK</Button>
                                </div>
                            )
                    }
                </DialogActions>

                <DialogEx
                    tag="UpdateAlert"
                    open={this.state.alertDialogOpen}
                    onClose={() => { this.handleCloseAlert(); }}
                    aria-describedby="Alert Dialog"
                    onEnterKey={()=>{ this.handleCloseAlert(); }}
                >
                    <DialogContent>
                        <Typography variant="body2" color="secondaryText">
                            {
                                this.state.alertDialogMessage
                            }
                        </Typography>
                    </DialogContent>
                    <DialogActions>
                        <Button variant="dialogPrimary" onClick={() => { this.handleCloseAlert(); }} autoFocus>
                            OK
                        </Button>
                    </DialogActions>
                </DialogEx>

            </DialogEx >
        );
    }
}