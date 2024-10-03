import React, { useState, useEffect } from 'react';
import ButtonBase from '@mui/material/ButtonBase';
import Menu from '@mui/material/Menu';
import MenuItem from '@mui/material/MenuItem';
import { colorKeys, getBackgroundColor } from './MaterialColors';
import { useTheme } from '@mui/styles';

const colors: string[] = colorKeys;

interface ColorDropdownButtonProps {
    currentColor?: string;
    onColorChange: (color: string) => void;
}

const ColorDropdownButton: React.FC<ColorDropdownButtonProps> = ({
    currentColor = colors[0],
    onColorChange,
}) => {
    const [anchorEl, setAnchorEl] = useState<null | HTMLElement>(null);
    const [selectedColor, setSelectedColor] = useState<string>(currentColor);

    const theme = useTheme();

    useEffect(() => {
        setSelectedColor(currentColor);
    }, [currentColor]);

    const handleClick = (event: React.MouseEvent<HTMLButtonElement>) => {
        setAnchorEl(event.currentTarget);
    };

    const handleClose = () => {
        setAnchorEl(null);
    };

    const handleColorSelect = (color: string) => {
        handleClose();
        setSelectedColor(color);
        onColorChange(color);
    };

    return (
        <div>
            <ButtonBase
                onClick={handleClick}
                style={{
                    width: 48, height: 48, padding: 12, borderRadius: 18
                }}
            >
                <div style={{
                    width: 24, height: 24,
                    background: getBackgroundColor(selectedColor),
                    borderRadius: 6,
                    borderColor: theme.palette.text.secondary,
                    borderWidth: 1,
                    borderStyle: "solid"
                }} />

            </ButtonBase>
            <Menu
                anchorEl={anchorEl}
                open={anchorEl !== null}
                onClose={handleClose}
                anchorOrigin={{
                    vertical: 'bottom',
                    horizontal: 'right',
                }}
                transformOrigin={{
                    vertical: 'top',
                    horizontal: 'right',
                }}
            >
                <div style={{ display: "flex", flexFlow: "row wrap", width: 56 * 4 + 2 }}>
                    {colors.map((color) => {
                        let selected = color === selectedColor;
                        return (
                            <MenuItem key={color} onClick={() => handleColorSelect(color)}
                                style={{
                                    width: 48, height: 48, borderRadius: 6, margin: 4,
                                    borderStyle: "solid",
                                    borderWidth: selected ? 2 : 0.25,
                                    borderColor: color === selectedColor ?
                                        theme.palette.text.primary
                                        : theme.palette.text.secondary,
                                    backgroundColor: getBackgroundColor(color)
                                }}
                            ></MenuItem>
                        )
                    }
                    )}
                </div>
            </Menu>
        </div>
    );
};

export default ColorDropdownButton;