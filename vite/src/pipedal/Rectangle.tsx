

export default class Rectangle {
    top: number;
    left: number;
    width: number;
    height: number;

    constructor(top?: number, left?: number, width?: number, height?: number) {
        this.top = top ? top: 0;
        this.left = left ? left: 0;
        this.width = width ? width: 0;
        this.height = height ? height: 0;
    }

    get right(): number {
        return this.left + this.width;
    }

    get bottom(): number {
        return this.top + this.height;
    }

    set right(value: number) {
        this.width = value - this.left;
    }

    set bottom(value: number) {
        this.height = value - this.top;
    }
};

