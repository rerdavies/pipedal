import StringBuilder from './StringBuilder';

class SvgPathBuilder {
    _sb: StringBuilder = new StringBuilder();

    moveTo(x: number, y: number): SvgPathBuilder
    {
        this._sb.append('M').append(x).append(',').append(y).append(' ');
        return this;
    }
    lineTo(x: number, y: number): SvgPathBuilder
    {
        this._sb.append('L').append(x).append(',').append(y).append(' ');
        return this;
    }
    bezierTo(x1: number, y1: number, x2: number, y2: number, x: number, y: number)
    {
        this._sb.append('C').append(x1).append(',').append(y1)
            .append(' ').append(x2).append(',').append(y2)
            .append(' ').append(x).append(',').append(y).append(' ');

    }
    closePath(): SvgPathBuilder {
        this._sb.append("Z ");
        return this;

    }

    toString(): string { return this._sb.toString(); }
}

export default SvgPathBuilder;