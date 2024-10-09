/*
* MIT License
* 
* Copyright (c) 2024 Robin E. R. Davies
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

import SnapshotButton from './SnapshotButton';
import { Pedalboard, Snapshot } from './Pedalboard';
import { PiPedalModel, PiPedalModelFactory } from './PiPedalModel';
import ResizeResponsiveComponent from './ResizeResponsiveComponent';

import SnapshotPropertiesDialog from './SnapshotPropertiesDialog';



export interface SnapshotPanelProps {
    panelHeight?: string,
    onEdit?: (snapshotIndex: number) => boolean
};

export interface SnapshotPanelState {
    snapshots: (Snapshot | null)[],
    portraitOrientation: boolean,
    collapseButtons: boolean,
    largeText: boolean,

    snapshotPropertiesDialogOpen: boolean,
    editTitle: string,
    editColor: string,
    editing: boolean,
    editId: number,
    selectedSnapshot: number

};

export default class SnapshotPanel extends ResizeResponsiveComponent<SnapshotPanelProps, SnapshotPanelState> {
    private model: PiPedalModel;

    constructor(props: SnapshotPanelProps) {
        super(props);
        this.model = PiPedalModelFactory.getInstance();
        this.state = {
            snapshots: this.getCurrentSnapshots(),
            selectedSnapshot: this.getSelectedSnapshot(),
            portraitOrientation: this.getPortraitOrientation(),
            collapseButtons: this.getCollapseButtons(),
            largeText: this.getLargeText(),

            snapshotPropertiesDialogOpen: false,
            editTitle: "",
            editColor: "",
            editing: false,
            editId: 0,

        };
        this.onPedalboardChanged = this.onPedalboardChanged.bind(this);
        this.onSelectedSnapshotChanged = this.onSelectedSnapshotChanged.bind(this);
    }

    getCurrentSnapshots() {
        let result: (Snapshot | null)[] = [];
        let currentSnapshots = this.model.pedalboard.get().snapshots;
        for (let i = 0; i < 6; ++i) {
            let mySnapshot = null;
            if (i < currentSnapshots.length) {
                if (currentSnapshots[i] !== null) {
                    mySnapshot = new Snapshot().deserialize(currentSnapshots[i]); // a deep clone.
                }
            }
            result.push(mySnapshot);
        }
        return result;
    }
    getSelectedSnapshot() {
        return this.model.selectedSnapshot.get();
    }

    onSelectedSnapshotChanged(value: number) {
        this.setState({ selectedSnapshot: value })
    }
    onPedalboardChanged(value: Pedalboard) {
        // boofs our current edit, oh well. we can't track property across a structural change.
        this.setState({
            snapshots: this.getCurrentSnapshots(),
            selectedSnapshot: this.getSelectedSnapshot()
        });
    }
    componentDidMount(): void {
        super.componentDidMount();
        this.model.pedalboard.addOnChangedHandler(this.onPedalboardChanged);
        this.model.selectedSnapshot.addOnChangedHandler(this.onSelectedSnapshotChanged);
        this.setState({
            snapshots: this.getCurrentSnapshots(),
            selectedSnapshot: this.getSelectedSnapshot()
        });
    }

    componentWillUnmount(): void {
        super.componentWillUnmount();
        this.model.selectedSnapshot.removeOnChangedHandler(this.onSelectedSnapshotChanged);
        this.model.pedalboard.removeOnChangedHandler(this.onPedalboardChanged)

    }

    getCollapseButtons() {
        if (this.getPortraitOrientation())
        {
            return window.innerWidth < 550;
        } else {
            return window.innerWidth < 800;
        }
    }
    getPortraitOrientation() {
        return window.innerWidth * 2 < window.innerHeight * 3;
    }

    getLargeText() {
        return window.innerHeight > 700 && window.innerWidth > 1000;
    }
    onSaveSnapshot(index: number) {
        let snapshot = this.state.snapshots[index];
        if (snapshot) {
            // no dialog required. just save it.
            this.handleSnapshotPropertyOk(
                index,
                snapshot.name,
                snapshot.color,
                false);
        } else {
            this.setState({
                editTitle: "",
                editColor: "",
                editId: index,
                snapshotPropertiesDialogOpen: true,
                editing: false
            });

        }

    }
    onEditSnapshot(index: number) {
        if (this.props.onEdit) {
            if (this.props.onEdit(index)) {
                return;
            }
        }
        let snapshot = this.state.snapshots[index];
        if (snapshot) {
            this.setState({
                editTitle: snapshot.name,
                editColor: snapshot.color,
                editId: index,
                snapshotPropertiesDialogOpen: true,
                editing: true
            });
        }
    }
    onWindowSizeChanged(width: number, height: number): void {
        this.setState(
            {
                portraitOrientation: this.getPortraitOrientation(),
                collapseButtons: this.getCollapseButtons(),
                largeText: this.getLargeText()
            }
        );
    }

    selectSnapshot(index: number) {
        this.model.selectSnapshot(index);
    }
    renderSnapshot(snapshot: Snapshot | null, index: number) {
        return (
            <SnapshotButton key={"s"+index}
                snapshot={snapshot}
                snapshotIndex={index}
                collapseButtons={this.state.collapseButtons}
                selected={this.state.selectedSnapshot === index}
                largeText={this.state.largeText}
                onEditSnapshot={(index)=>{this.onEditSnapshot(index)}}
                onSaveSnapshot={(index)=>{ this.onSaveSnapshot(index); }}
                onSelectSnapshot={(index)=>{this.model.selectSnapshot(index);}}
            />
        );
    }




    handleSnapshotPropertyOk(index: number, name: string, color: string, editing: boolean) {
        if (editing) {
            let snapshots = Snapshot.cloneSnapshots(this.state.snapshots);
            let snapshot = snapshots[index];
            if (snapshot) {
                snapshot.name = name;
                snapshot.color = color;
            }
            this.setState({
                snapshots: snapshots,
                snapshotPropertiesDialogOpen: false
            });
            this.model.setSnapshots(snapshots, -1);

        } else {
            let pedalboard = this.model.pedalboard.get();
            let newSnapshot = pedalboard.makeSnapshot();
            newSnapshot.name = name;
            newSnapshot.color = color;
            let snapshots = Snapshot.cloneSnapshots(this.state.snapshots);
            snapshots[index] = newSnapshot;
            this.setState({
                snapshots: snapshots,
                snapshotPropertiesDialogOpen: false
            });
            this.model.setSnapshots(snapshots, index);



        }
    }

    bg: undefined;

    render() {
        let snapshots = this.state.snapshots;
        return (
            <div style={{
                flex: "1 1 auto",
                height: this.props.panelHeight,
                overflow: "hidden", margin: 0,
                paddingLeft: 16, paddingRight: 16, paddingTop: 0, paddingBottom: 24,
                background: this.bg,
                display: "flex", flexFlow: "column nowrap", alignContent: "stretch", justifyContent: "stretch"
            }}>

                <div style={{
                    flexGrow: 1, flexShrink: 1, flexBasis: 1,

                    display: "flex",
                    flexFlow: this.state.portraitOrientation ?
                        "row nowrap" : "column nowrap",
                    alignItems: "stretch", gap: 8
                }}>
                    <div style={{
                        flexGrow: 1, flexShrink: 1, flexBasis: 1,
                        display: "flex",
                        flexFlow: this.state.portraitOrientation ?
                            "column nowrap"
                            : "row nowrap",
                        alignItems: "stretch", gap: 8
                    }}>
                        {this.renderSnapshot(snapshots[0], 0)}
                        {this.renderSnapshot(snapshots[1], 1)}
                        {this.renderSnapshot(snapshots[2], 2)}
                    </div>
                    <div style={{
                        flexGrow: 1, flexShrink: 1, flexBasis: 1,
                        display: "flex",
                        flexFlow: this.state.portraitOrientation ? "column nowrap" : "row nowrap", alignItems: "stretch", gap: 8
                    }}>
                        {this.renderSnapshot(snapshots[3], 3)}
                        {this.renderSnapshot(snapshots[4], 4)}
                        {this.renderSnapshot(snapshots[5], 5)}
                    </div>
                </div>
                {
                    this.state.snapshotPropertiesDialogOpen && (
                        <SnapshotPropertiesDialog
                            open={this.state.snapshotPropertiesDialogOpen}
                            name={this.state.editTitle}
                            color={this.state.editColor}
                            editing={this.state.editing}
                            snapshotIndex={this.state.editId}
                            onClose={() => { this.setState({ snapshotPropertiesDialogOpen: false }); }}
                            onOk={(snapshotId, name, color, editing) => {
                                this.handleSnapshotPropertyOk(snapshotId, name, color, editing);
                            }}
                        />
                    )
                }
            </div>
        );
    }
}