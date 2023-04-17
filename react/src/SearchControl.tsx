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

import React from 'react';
import { Theme } from '@mui/material/styles';
import { WithStyles } from '@mui/styles';
import withStyles from '@mui/styles/withStyles';
import createStyles from '@mui/styles/createStyles';
import Input from '@mui/material/Input';
import IconButton from '@mui/material/IconButton';
import SearchIcon from '@mui/icons-material/Search';
import ClearIcon from '@mui/icons-material/Clear';
import InputAdornment from '@mui/material/InputAdornment';

const styles = (theme: Theme) => createStyles({
});


interface SearchControlProps extends WithStyles<typeof styles> {
    theme: Theme,
    collapsed?: boolean;
    searchText: string;
    onTextChanged: (searchString: string) => void;
    onClick: () => void;
    onClearFilterClick: () => void;
    inputRef?: React.RefObject<HTMLInputElement>;
    showSearchIcon?: boolean

}

interface SearchControlState {
}

const SearchControl = withStyles(styles, { withTheme: true })(

    class extends React.Component<SearchControlProps, SearchControlState> {

        defaultInputRef: React.RefObject<HTMLInputElement>;

        constructor(props: SearchControlProps) {
            super(props);
            this.state = {

            };
            this.defaultInputRef = React.createRef<HTMLInputElement>();
        }
        getInputRef(): React.RefObject<HTMLInputElement> {
            if (this.props.inputRef) {
                return this.props.inputRef;
            }
            return this.defaultInputRef;
        }
        componentDidUpdate(oldProps: SearchControlProps) {
            if (oldProps.collapsed ?? false !== this.props.collapsed ?? false) {
                let inputRef = this.getInputRef();
                if (inputRef.current) {
                    if (!(this.props.collapsed ?? false)) {
                        setTimeout(() => {
                            let inputRef = this.getInputRef();
                            if (inputRef.current) {
                                inputRef.current.focus();
                            }
                        }, 0);

                    } else {
                        inputRef.current.value = "";
                        this.props.onTextChanged("");
                    }
                }
            }
        }

        render() {
            return (
                <div style={{ display: "flex", flexFlow: "row nowrap", justifyContent: "flex-end", alignItems: "center" }}>
                    {(this.props.showSearchIcon ?? true) && (
                        <IconButton onClick={() => { this.props.onClick(); }} size="large">
                            <SearchIcon />
                        </IconButton>
                    )

                    }
                    <Input spellCheck={false} style={{
                        flex: "1 1", visibility: (this.props.collapsed ?? false) ? "collapse" : "visible", marginRight: 12, height: 32,
                    }}
                        inputRef={this.getInputRef()}
                        defaultValue={this.props.searchText}
                        placeholder="Search"
                        endAdornment={(
                            <InputAdornment position="end">
                                <IconButton
                                    aria-label="clear search filter"
                                    onClick={() => {
                                        this.props.onClearFilterClick();
                                        let inputRef = this.getInputRef();
                                        if (inputRef.current) {
                                            inputRef.current.value = "";
                                        }
                                    }}
                                    edge="end"
                                    size="large">
                                    <ClearIcon fontSize='small' style={{ opacity: 0.6 }} />
                                </IconButton>
                            </InputAdornment>

                        )}
                        onChange={
                            (e) => {
                                this.props.onTextChanged(e.target.value);
                            }
                        }

                    />


                </div>
            );
        }
    }
);

export default SearchControl;
