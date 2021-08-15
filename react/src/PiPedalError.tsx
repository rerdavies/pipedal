
export class PiPedalError extends Error {
    constructor(message: string)
    {
        super(message);
        this.name = "PiPedalError";
    }
}

export class PiPedalArgumentError extends PiPedalError {
    constructor(message: string)
    {
        super(message);
        this.name = "PiPedalArgumentError";
    }
}
export class PiPedalStateError extends PiPedalError {
    constructor(message: string)
    {
        super(message);
        this.name = "PiPedalStateError";
    }
}

export default PiPedalError;

