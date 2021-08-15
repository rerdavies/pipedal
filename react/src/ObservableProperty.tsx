

export type OnChangedHandler<VALUE_TYPE> = (newValue: VALUE_TYPE) => void;


export class ObservableProperty<VALUE_TYPE> {

    _value_type: VALUE_TYPE;
    _on_changed_handlers: OnChangedHandler<VALUE_TYPE>[] = [];

    constructor(default_value: VALUE_TYPE)
    {
        this._value_type = default_value;
    }

    addOnChangedHandler(handler: OnChangedHandler<VALUE_TYPE> ) : void
    {
        this._on_changed_handlers.push(handler);
        handler(this._value_type);
    }
    removeOnChangedHandler(handler: OnChangedHandler<VALUE_TYPE> ) : void
    {
        let oldArray = this._on_changed_handlers;

        let newArray: OnChangedHandler<VALUE_TYPE>[] = [];

        let outIx = 0;
        for (let i = 0; i < oldArray.length; ++i)
        {
            if (oldArray[i] !== handler)
            {
                newArray[outIx++]  = oldArray[i];
            }
        }
        this._on_changed_handlers = newArray;
    }

    get() : VALUE_TYPE {
        return this._value_type;
    }

    set(value: VALUE_TYPE) : void
    {
        this._value_type = value;

        let t = this._on_changed_handlers; // take an copy in case removes happen while iteratiing.
        t.forEach(c => c(value));
    }

};