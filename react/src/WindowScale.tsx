import {isAndroidHosted} from './AndroidHost';


var validScales = [1.0,1.10,1.25,1.35,1.50,1.75,2.0,2.5];

export function canScaleWindow(): boolean {
    return getValidWindowScales().length > 1;
}

export function getValidWindowScales(): number[]
{
    if (isAndroidHosted())
    {
        return [1.0];
    }
    let result: number[] = [];
    let minDimension = Math.min(window.innerHeight,window.innerWidth);

    for (let validScale of validScales)
    {
        if (minDimension/validScale > 400)
        {
            result.push(validScale);
        }
    }
    return result;
}

export function setWindowScale(scale: number): void {
    localStorage.setItem("pipedalWindowScale", scale.toString());
}
export function getWindowScale(): number {
    const strvalue = localStorage.getItem("pipedalWindowScale");
    return strvalue ? parseFloat(strvalue) : 1.0;
}

export function getWindowScaleText(scale?: number)
{
    let value: number;
    if (scale)
    {
        value = scale;
    } else {
        value = getWindowScale();
    }
    let iValue = Math.round(value*100);
    return iValue.toString() + "%";
}