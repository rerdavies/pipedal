/*
 *   Copyright (c) Robin E.R. Davies
 *   All rights reserved.

 *   Permission is hereby granted, free of charge, to any person obtaining a copy
 *   of this software and associated documentation files (the "Software"), to deal
 *   in the Software without restriction, including without limitation the rights
 *   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *   copies of the Software, and to permit persons to whom the Software is
 *   furnished to do so, subject to the following conditions:

 *   The above copyright notice and this permission notice shall be included in all
 *   copies or substantial portions of the Software.
 *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *   SOFTWARE.
 */

import Button from "@mui/material/Button";
import Checkbox from "@mui/material/Checkbox";
import DialogActions from "@mui/material/DialogActions";
import DialogContent from "@mui/material/DialogContent";
import Paper from "@mui/material/Paper";
import Table from "@mui/material/Table";
import TableBody from "@mui/material/TableBody";
import TableCell from "@mui/material/TableCell";
import TableContainer from "@mui/material/TableContainer";
import TableHead from "@mui/material/TableHead";
import TableRow from "@mui/material/TableRow";
import React, { useEffect } from "react";

import DialogEx from "./DialogEx";
import { Model } from "./t3k/types.ts";
import Tone3000DownloadType from "./Tone3000DownloadType.tsx";
import Divider from "@mui/material/Divider";
import Toolbar from "@mui/material/Toolbar";
import Typography from "@mui/material/Typography";
import IconButtonEx from "./IconButtonEx.tsx";
import ArrowBackIcon from '@mui/icons-material/ArrowBack';
import DialogTitle from "@mui/material/DialogTitle";
import useWindowSize from "./UseWindowSize.tsx";

interface ModelSelection {
    selected: boolean;
    model: Model;
}

export interface ModelSelectionDialogParams {
    downloadType: Tone3000DownloadType;
    toneName: string;
    models: Model[];
    onCancel: () => void;
    onModelsSelected: (selectedModels: Model[]) => void;
};

export function ModelSelectionDialog(
    props: {
        open: boolean;
        params: ModelSelectionDialogParams;
        onClose: () => void;
    }) {

    const { models, toneName, onCancel, onModelsSelected } = props.params;
    const windowSize = useWindowSize();

    const [modelSelection, setModelSelection] = React.useState<ModelSelection[]>([]);

    useEffect(() => {
        setModelSelection(models.map((model) => ({
            selected: false,
            model,
        })));
    }, [models]);

    const selectedCount = modelSelection.filter((selection) => selection.selected).length;
    const allSelected = modelSelection.length > 0 && selectedCount === modelSelection.length;
    const partiallySelected = selectedCount > 0 && selectedCount < modelSelection.length;

    const toggleAll = () => {
        setModelSelection((currentSelection) => {
            const nextSelected = !allSelected;
            return currentSelection.map((selection) => ({
                ...selection,
                selected: nextSelected,
            }));
        });
    };

    const toggleModel = (modelId: number) => {
        setModelSelection((currentSelection) => currentSelection.map((selection) => {
            if (selection.model.id !== modelId) {
                return selection;
            }
            return {
                ...selection,
                selected: !selection.selected,
            };
        }));
    };

    const handleCancel = () => {
        onCancel();
        props.onClose();
    };

    const handleOk = () => {
        onModelsSelected(
            modelSelection
                .filter((selection) => selection.selected)
                .map((selection) => selection.model)
        );
        props.onClose();
    };

    let title = props.params.downloadType === Tone3000DownloadType.Nam
        ? "Select Models" :
        "Select IRs   ";

    let tableTitle = 
        "(" + selectedCount.toString() + " of " + models.length.toString() + " selected) - " + toneName
        
    return (
        <DialogEx tag="selectModels" open={props.open} onClose={() => { /* Do nothing */ }}
            fullWidth={true} maxWidth="md"
            fullScreen={windowSize[0].width < 450}


        >
            <DialogTitle style={{
                paddingBottom: 0,
                marginBottom: 0, paddingTop: 0, 
            }} >

                <Toolbar style={{ padding: 0 }}>
                    <IconButtonEx
                        tooltip="Back"
                        edge="start"
                        color="inherit"
                        aria-label="back"
                        style={{ opacity: 0.6 }}
                        onClick={() => { handleCancel(); }}
                    >
                        <ArrowBackIcon style={{ width: 24, height: 24 }} />
                    </IconButtonEx>
                    <Typography noWrap component="div" sx={{ flexGrow: 1 }}>
                        {title}
                    </Typography>
                </Toolbar>
            </DialogTitle>
            <DialogContent sx={{ marginLeft: 0, marginRight: 0, paddingLeft: 0, paddingRight: 0 }}>
                <TableContainer component={Paper} variant="outlined" sx={{ backgroundColor: "transparent", border: "0px" }}  >
                    <Table sx={{ backgroundColor: "transparent" }}>
                        <TableHead style={{ backgroundColor: "transparent" }}>
                            <TableRow sx={{ backgroundColor: "transparent" }} onClick={toggleAll}>
                                <TableCell padding="checkbox" sx={{ backgroundColor: "transparent" }}>
                                    <Checkbox
                                        checked={allSelected}
                                        indeterminate={partiallySelected}
                                        onChange={toggleAll}
                                        sx={{marginLeft: 1}}
                                        inputProps={{ 'aria-label': 'select all models' }}
                                    />
                                </TableCell>
                                <TableCell sx={{ backgroundColor: "transparent" }}>
                                    <Typography noWrap  variant="body1">{tableTitle}</Typography>
                                </TableCell>
                            </TableRow>
                        </TableHead>
                        <TableBody style={{ backgroundColor: "transparent" }}>
                            {modelSelection.map((selection) => (
                                <TableRow
                                    key={selection.model.id}
                                    onClick={() => toggleModel(selection.model.id)}
                                    sx={{ cursor: 'pointer', backgroundColor: 'transparent' }}
                                >
                                    <TableCell padding="checkbox" sx={{ backgroundColor: "transparent", paddingLeft: 3, borderBottom: "0px" }}>
                                        <Checkbox
                                            checked={selection.selected}
                                            onClick={(event) => { event.stopPropagation(); }}
                                            onChange={() => toggleModel(selection.model.id)}
                                            inputProps={{ 'aria-labelledby': `model-selection-${selection.model.id}` }}
                                        />
                                    </TableCell>
                                    <TableCell id={`model-selection-${selection.model.id}`} sx={{ backgroundColor: "transparent", borderBottom: "0px" }}>
                                        <Typography noWrap  variant="body2">
                                            {selection.model.name}
                                        </Typography>
                                    </TableCell>
                                </TableRow>
                            ))}
                        </TableBody>
                    </Table>
                </TableContainer>
            </DialogContent>
            <Divider />
            <DialogActions>
                <Button onClick={handleCancel} variant="dialogSecondary">
                    Cancel
                </Button>
                <Button onClick={handleOk} variant="dialogPrimary" disabled={selectedCount === 0}>
                    OK
                </Button>
            </DialogActions>
        </DialogEx>
    );
}