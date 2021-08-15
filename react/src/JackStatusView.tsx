import React from 'react';
import { PiPedalModel, PiPedalModelFactory,State } from './PiPedalModel';
import JackHostStatus from './JackHostStatus';




interface JackStatusViewProps {

};

interface JackStatusViewState {
    jackStatus?: JackHostStatus;
}


export default class JackStatusView extends React.Component<JackStatusViewProps, JackStatusViewState>
{
    model: PiPedalModel;

    constructor(props: JackStatusViewProps) {
        super(props);
        this.state = {
            jackStatus: undefined
        };
        this.model = PiPedalModelFactory.getInstance();
        this.tick = this.tick.bind(this);
    }

    tick() {
        if (this.model.state.get() === State.Ready) {
            this.model.getJackStatus()
                .then(jackStatus => {
                    this.setState({jackStatus: jackStatus});
                })
                .catch(error => { /* ignore*/ });
        }
    }

    timerHandle?: NodeJS.Timeout;
    componentDidMount() {
        this.timerHandle = setInterval(this.tick, 1000);
    }
    componentWillUnmount() {
        if (this.timerHandle) {
            clearTimeout(this.timerHandle);
            this.timerHandle = undefined;
        }
    }

    render() {
        return (
            <div style={{
                position: "absolute", right: 30, bottom: 0, height: 30, width: 200,
                paddingRight: 20, paddingBottom: 6,
                textAlign: "right", opacity: 0.7, whiteSpace: "nowrap", fontSize: 12, zIndex: 10, fontWeight: 900
            }}>
                {JackHostStatus.getDisplayView("",this.state.jackStatus) }
            </div>
        );

    }
}