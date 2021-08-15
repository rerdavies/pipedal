import React from 'react';
import Button from '@material-ui/core/Button';
import TextField from '@material-ui/core/TextField';
import Dialog from '@material-ui/core/Dialog';
import DialogActions from '@material-ui/core/DialogActions';
import DialogContent from '@material-ui/core/DialogContent';
import { nullCast } from './Utility';
import ResizeResponsiveComponent from './ResizeResponsiveComponent';


export interface RenameDialogProps {
    open: boolean,
    defaultName: string,
    acceptActionName: string,
    onOk: (text: string) => void,
    onClose: () => void
};

export interface RenameDialogState {
    fullScreen: boolean;
};

export default class RenameDialog extends ResizeResponsiveComponent<RenameDialogProps, RenameDialogState> {

    refText: React.RefObject<HTMLInputElement>;

    constructor(props: RenameDialogProps) {
        super(props);
        this.state = {
            fullScreen: false
        };
        this.refText = React.createRef<HTMLInputElement>();
        this.handlePopState = this.handlePopState.bind(this);
    }
    mounted: boolean = false;

    hasHooks: boolean = false;

    stateWasPopped: boolean = false;
    handlePopState(e: any): any {
        this.stateWasPopped = true;
        let shouldClose = (!e.state || !e.state.renameDialog);
        if (shouldClose)
        {
            this.props.onClose();
        }
    }

    updateHooks() : void {
        let wantHooks = this.mounted && this.props.open;
        if (wantHooks !== this.hasHooks)
        {
            this.hasHooks = wantHooks;

            if (this.hasHooks)
            {
                this.stateWasPopped = false;
                window.addEventListener("popstate",this.handlePopState);
                // eslint-disable-next-line no-restricted-globals
                let state = history.state;
                if (!state)
                {
                    state = {};
                }
                state.renameDialog = true;

                // eslint-disable-next-line no-restricted-globals
                history.pushState(
                    state,
                    this.props.acceptActionName,
                    "#RenameDialog"
                );
            } else {
                window.removeEventListener("popstate",this.handlePopState);
                if (!this.stateWasPopped)
                {
                    // eslint-disable-next-line no-restricted-globals
                    history.back();
                }
            }

        }
    }
    onWindowSizeChanged(width: number, height: number): void 
    {
        this.setState({fullScreen: height < 200})
    }


    componentDidMount()
    {
        super.componentDidMount();
        this.mounted = true;
        this.updateHooks();
    }
    componentWillUnmount()
    {
        super.componentDidMount();
        this.mounted = false;
        this.updateHooks();
    }

    componentDidUpdate()
    {
        this.updateHooks();
    }

    render() {
        let props = this.props;
        let { open, defaultName, acceptActionName, onClose, onOk } = props;

        const handleClose = () => {
            onClose();
        };

        const handleOk = () => {
            let text = nullCast(this.refText.current).value;
            onOk(text);
        }
        const handleKeyDown = (event: React.KeyboardEvent<HTMLDivElement>): void => {
            // 'keypress' event misbehaves on mobile so we track 'Enter' key via 'keydown' event
            if (event.key === 'Enter') {
                event.preventDefault();
                event.stopPropagation();
                handleOk();
            }
        };
        return (
            <Dialog open={open} fullWidth onClose={handleClose} aria-labelledby="Rename-dialog-title" style={{}}
                fullScreen={this.state.fullScreen}
                >
                <DialogContent>
                    <TextField
                        autoFocus
                        onKeyDown={handleKeyDown}
                        margin="dense"
                        id="name"
                        label="Name"
                        type="text"
                        fullWidth
                        defaultValue={defaultName}
                        inputRef={this.refText}
                    />
                </DialogContent>
                <DialogActions>
                    <Button onClick={handleClose} color="primary">
                        Cancel
                    </Button>
                    <Button onClick={handleOk} color="secondary" >
                        {acceptActionName}
                    </Button>
                </DialogActions>
            </Dialog>
        );
    }
}