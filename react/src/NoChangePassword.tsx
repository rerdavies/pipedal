import React from 'react';
import InputAdornment from '@material-ui/core/InputAdornment';
import IconButton from '@material-ui/core/IconButton';
import FormControl from '@material-ui/core/FormControl';
import FormHelperText from '@material-ui/core/FormHelperText';
import InputLabel from '@material-ui/core/InputLabel';
import Input from '@material-ui/core/Input';

import Visibility from '@material-ui/icons/Visibility';
import VisibilityOff from '@material-ui/icons/VisibilityOff';



interface NoChangePasswordProps {
    hasPassword: boolean;
    label: string;
    defaultValue: string;
    onPasswordChange: (text: string) => void;
    disabled?: boolean;
    helperText?: string;
    error?: boolean;
    inputRef?: React.RefObject<any>
};

interface NoChangePasswordState {
    focused: boolean;
    passwordText: string;
    showPassword: boolean;
};

class NoChangePassword extends React.Component<NoChangePasswordProps, NoChangePasswordState> {
    refText: React.Ref<HTMLInputElement | HTMLTextAreaElement>;

    constructor(props: NoChangePasswordProps) {
        super(props);
        this.refText = React.createRef();
        this.state = {
            focused: false,
            showPassword: false,
            passwordText: this.props.defaultValue as string
        };
    }

    handleShowPassword() {
        this.setState({
            showPassword: !this.state.showPassword
        });
    }
    handleChange(e: React.ChangeEvent<HTMLInputElement | HTMLTextAreaElement>) {
        this.textChanged = true;
        this.props.onPasswordChange(e.target.value);
    }

    textChanged: boolean = false;
    handleFocus(e: React.FocusEvent<HTMLInputElement | HTMLTextAreaElement>) {
        console.log("onFocus");
        if (!this.state.focused) {
            this.textChanged = false;
            e.target.value = this.state.passwordText;
        }
        e.currentTarget.removeAttribute("readonly");
        this.setState({ focused: true });
    }
    handleBlur(e: React.FocusEvent<HTMLInputElement | HTMLTextAreaElement>) {
        console.log("onBlur");
        let text = e.target.value as string;
        this.setState({ focused: false, passwordText: text });
        this.props.onPasswordChange(e.target.value);
        this.textChanged = false;
        if (text.length === 0 && this.props.hasPassword)
        {
            e.target.value = "(Unchanged)";
        }
    }

    render() {
        console.log("Render  focus: " + this.state.focused)
        let showUnchanged = (this.state.passwordText.length === 0) && this.props.hasPassword && (!this.state.focused);
        let thisDefaultValue = showUnchanged ? "(Unchanged)" : this.props.defaultValue;

        let isText = showUnchanged || this.state.showPassword;


        return (
            <FormControl disabled={this.props.disabled} fullWidth error={this.props.error} >
                <InputLabel htmlFor="standard-adornment-password">{this.props.label}</InputLabel>
                <Input
                    fullWidth
                    color="primary"
                    spellCheck="false"
                    autoComplete="off"
                    inputRef={this.props.inputRef}
                    readOnly
                    id="standard-adornment-password"
                    onChange={(e) => this.handleChange(e)}
                    value={thisDefaultValue}
                    onFocus={(e) => this.handleFocus(e)}
                    onBlur={(e) => this.handleBlur(e)}
                    type={isText ? 'text' : 'password'}
                    style={ { color: showUnchanged?  '#888': '#000'} }
                    
                    endAdornment={
                        <InputAdornment position="end">
                            <IconButton
                                aria-label="toggle password visibility"
                                onClick={(e) => this.handleShowPassword()}
                                onMouseDown={(e) => e.preventDefault()}
                            >
                                {this.state.showPassword ? <Visibility /> : <VisibilityOff />}
                            </IconButton>
                        </InputAdornment>
                    }
                />
                 <FormHelperText >{ this.props.helperText}</FormHelperText>
            </FormControl>
        );
    }
};

export default NoChangePassword;
