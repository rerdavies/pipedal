/*
 * MIT License
 * 
 * Copyright (c) Robin E. R. Davies
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
import React, { useState, useEffect } from 'react';
import ButtonBase from '@mui/material/ButtonBase';
import Menu from '@mui/material/Menu';
import MenuItem from '@mui/material/MenuItem';
import { useTheme } from '@mui/material/styles';
import { getIconColorSelections,getIconColor,IconColorSelect } from './PluginIcon';
import ArrowDropDownIcon from '@mui/icons-material/ArrowDropDown';


export enum DropdownAlignment {
    SE, SW
}

interface ColorListDropdownButtonProps {
    colorKey: string;
    colorList?: IconColorSelect[];

    onColorChange: (selection: IconColorSelect) => void;
    dropdownAlignment: DropdownAlignment;
}

const IconColorDropdownButton: React.FC<ColorListDropdownButtonProps> = (props: ColorListDropdownButtonProps) => {
    let { colorList, colorKey, onColorChange, dropdownAlignment } = props;
    if (!colorList) colorList = getIconColorSelections();
    const theme = useTheme();

    const [anchorEl, setAnchorEl] = useState<null | HTMLElement>(null);

    let selectedColor = getIconColor(colorKey);
    if (selectedColor === undefined) {
        selectedColor = theme.palette.text.secondary;
    }

    const handleClick = (event: React.MouseEvent<HTMLButtonElement>) => {
        setAnchorEl(event.currentTarget);
    };

    const handleClose = () => {
        setAnchorEl(null);
    };

    const handleColorSelect = (selection: IconColorSelect) => {
        handleClose();
        onColorChange(selection);
    };
    return (
        <div>
            <ButtonBase
                onClick={handleClick}
                style={{
                    height: 48, padding: 12, borderRadius: 18
                }}
            >
                <div style={{ display: "flex", flexFlow: "row nowrap", alignItems: "center", gap: 0 }} >
                    <div style={{
                        width: 24, height: 24,
                        background: selectedColor,
                        borderRadius: 6,
                        borderColor: theme.palette.text.secondary,
                        borderWidth: 1,
                        borderStyle: "solid"
                    }} />
                    <ArrowDropDownIcon  />
                </div>

            </ButtonBase>
            <Menu
                anchorEl={anchorEl}
                open={anchorEl !== null}
                onClose={handleClose}
                anchorOrigin={{
                    vertical: 'bottom',
                    horizontal: (dropdownAlignment === DropdownAlignment.SW ?  'right' : 'left')
                }}
                transformOrigin={{
                    vertical: 'top',
                    horizontal: (dropdownAlignment === DropdownAlignment.SW ?  'right' : 'left'),
                }}
            >
                <div style={{display: "flex", flexFlow: "row nowrap", columnGap: 8,padding: 8}}>

                    <MenuItem key={"default"} onClick={() => handleColorSelect({key: "default", value: undefined})}
                        style={{
                            width: 48, height: 48, borderRadius: 6, margin: 4,
                            borderStyle: "solid",
                            borderWidth: colorKey ==="default" ? 2 : 0.25,
                            borderColor: colorKey == "default" ?
                                theme.palette.text.primary
                                : theme.palette.text.secondary,
                            backgroundColor: theme.palette.text.secondary
                        }}
                    ></MenuItem>

                    <div style={{ display: "flex", flexFlow: "row wrap", width: 56 * 4 + 2 }}>
                        {colorList.map((color) => {
                            let selected = color.key === selectedColor;
                            return (
                                <MenuItem key={color.key} onClick={() => handleColorSelect(color)}
                                    style={{
                                        width: 48, height: 48, borderRadius: 6, margin: 4,
                                        borderStyle: "solid",
                                        borderWidth: selected ? 2 : 0.25,
                                        borderColor: color.key === selectedColor ?
                                            theme.palette.text.primary
                                            : theme.palette.text.secondary,
                                        backgroundColor: color.value
                                    }}
                                ></MenuItem>
                            )
                        }
                        )}
                    </div>
                </div>
            </Menu>
        </div>
    );
};

export default IconColorDropdownButton;