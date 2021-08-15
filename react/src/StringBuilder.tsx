

class StringBuilder 
{
    _array: string[] = [];

    append(value: any) {
        if (typeof(value) === "string")
        {
            this._array.push(value);
        } else {
            this._array.push(value+"");
        }
        return this;
    }
    toString(): string {
        return this._array.join('');
    }
}

export default StringBuilder;