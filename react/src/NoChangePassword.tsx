// Copyright (c) 2021 Robin Davies
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

import React from 'react';
import InputAdornment from '@mui/material/InputAdornment';
import IconButton from '@mui/material/IconButton';
import FormControl from '@mui/material/FormControl';
import FormHelperText from '@mui/material/FormHelperText';
import InputLabel from '@mui/material/InputLabel';
import Input from '@mui/material/Input';

import Visibility from '@mui/icons-material/Visibility';
import VisibilityOff from '@mui/icons-material/VisibilityOff';



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
                                size="large">
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
