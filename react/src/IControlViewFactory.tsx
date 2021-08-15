import React from 'react';
import { PiPedalModel } from "./PiPedalModel";
import { PedalBoardItem } from './PedalBoard';



interface IControlViewFactory  {
    uri: string;
    Create(model: PiPedalModel,pedalBoardItem: PedalBoardItem): React.ReactNode;

}

export default IControlViewFactory;
