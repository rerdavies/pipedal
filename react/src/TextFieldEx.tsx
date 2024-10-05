
import React from 'react';

import TextField, { TextFieldProps } from '@mui/material/TextField';
import { withStyles, WithStyles } from '@mui/styles';
import createStyles from '@mui/styles/createStyles';
import { Theme } from '@mui/material/styles';
import Modal from '@mui/material/Modal';

const styles = (theme: Theme) => createStyles({
});


export interface TextFieldExPropsBase extends WithStyles<typeof styles> {
    label?: string;
    containerStyle?: React.CSSProperties;
    autoFocus?: boolean;
    inputRef?: React.Ref<any>;
    theme: Theme;
};

export type TextFieldExProps = TextFieldExPropsBase & TextFieldProps;


export interface TextFieldExState {
    androidHosted: boolean;
    composingText: string;
    displayText: string;
    showIme: boolean;
    imeValue: string;
};


const TextFieldEx = withStyles(styles, { withTheme: true })(
    class extends React.Component<TextFieldExProps, TextFieldExState> {
        constructor(props: TextFieldExProps) {
            super(props);

            this.state = {
                composingText: "",
                displayText: "",
                androidHosted: true,
                showIme: false,
                imeValue: ""
            };
            this.handleClick = this.handleClick.bind(this);
            this.handleImeCompositionStart = this.handleImeCompositionStart.bind(this);
            this.handleCompositionEnd = this.handleCompositionEnd.bind(this);
            this.handleImeCompositionUpdate = this.handleImeCompositionUpdate.bind(this);
            this.handleImeCompositionEnd = this.handleImeCompositionEnd.bind(this);
        }

        private getUseImeMask() {
            return this.state.androidHosted && window.innerHeight < 4500;
        }

        private originalInputHref: HTMLInputElement | undefined = undefined;
        private imeInputHref: HTMLInputElement | undefined = undefined;

        handleInputRef(input: HTMLInputElement) {
            if (input) {
                this.originalInputHref = input;
                if (this.getUseImeMask()) {
                    input.addEventListener('click', this.handleClick);
                    input.addEventListener('input', this.handleInput);
                }
            } else {
                if (this.originalInputHref) {
                    this.originalInputHref.removeEventListener('click', this.handleClick);
                    this.originalInputHref.removeEventListener('input', this.handleInput);
                    this.originalInputHref = undefined;

                }
            }
        }

        handleClick(event: FocusEvent) {
            (event.target as any).blur();
            event.preventDefault();
            event.stopPropagation();
            this.setState({
                imeValue: (event.target as any).value as string,
                showIme: true
            });
            return false;
        }
        handleImeCompositionStart(event: any) {
            // this.setState({ 
            //     composingText: '' ,
            //     imeValue: '', //event.target.value as string,
            //     showIme: true
            // });
        }

        handleImeCompositionUpdate(event: CompositionEvent) {
            this.setState({ composingText: event.data });
        };

        handleCompositionEnd = (event: CompositionEvent) => {
            this.setState(prevState => ({
                displayText: prevState.displayText + event.data,
                composingText: '',
            }));
        };

        dismissIme() {
            if (this.originalInputHref && this.imeInputHref)
            {
                let value = this.imeInputHref.value;
                this.originalInputHref.value = this.imeInputHref.value;
                this.originalInputHref.blur();
                this.imeInputHref.blur();
                this.setState({
                    showIme: false, imeValue: value
                });
            }
        }
        handleImeCompositionEnd()
        {
            this.dismissIme();
        }
        handleInput = (event: Event) => {
            const target = event.target as HTMLInputElement;
            if (!this.state.composingText) {
                this.setState({ displayText: target.value });
            }
        };
        render() {
            let { autoFocus, containerStyle, label, ...extra } = this.props;
            return (
                <div style={this.props.containerStyle}>
                    <TextField {...extra} autoFocus={autoFocus} tabIndex={-1} inputRef={(input) => this.handleInputRef(input)} />
                    <p>Display Text: {this.state.imeValue}</p>
                    <p>Composing Text: {this.state.composingText}</p>
                    {this.state.showIme && (
                        <Modal open={this.state.showIme}>
                            <div style={{
                                position: "absolute", top: 0, left: 0, right: 0, bottom: 0,
                                background: this.props.theme.palette.background.default,
                                display: "flex", flexFlow: "row nowrap",justifyContent: "center"

                            }}>
                                <TextField variant="standard" 
                                    inputRef={(element: HTMLInputElement)=>{ 
                                        if (this.imeInputHref) {
                                            this.imeInputHref.removeEventListener("compositionend",this.handleImeCompositionEnd);
                                            this.imeInputHref.removeEventListener("compositionupdate",this.handleImeCompositionUpdate);
                                            this.imeInputHref.removeEventListener("compositionstart",this.handleImeCompositionStart);
                                        }
                                        this.imeInputHref = element;
                                        if (this.imeInputHref)
                                        {
                                            this.imeInputHref.addEventListener("compositionend",this.handleImeCompositionEnd);
                                            this.imeInputHref.addEventListener("compositionupdate",this.handleImeCompositionUpdate);
                                            this.imeInputHref.addEventListener("compositionstart",this.handleImeCompositionStart);

                                        }
                                    }}
                                    style={{width: 200,marginTop: 16}}
                                    autoFocus
                                    value={this.state.imeValue}
                                    onChange={(event: any) => { 
                                        this.setState({ imeValue: event.target.value as string }); 
                                    }} 
                                    onBlur={() => {
                                        this.dismissIme();
                                    }}
                                    onKeyDown={(event)=> {
                                        if (event.key === "Enter")
                                        {
                                            this.setState({showIme: false});
                                        }

                                    }}
                                    />
                            </div>
                        </Modal>
                    )}
                </div>
            );
        }
    });

export default TextFieldEx;
