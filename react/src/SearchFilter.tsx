

let escapedCharacters = /[^a-zA-Z0-9]/g;

function stringToRegex(text: string): RegExp
{
    text = text.replace(escapedCharacters,"\\$&");

    return new RegExp(text,"i");
}


class SearchFilter {
    regexes: RegExp[];

    constructor(text: string)
    {
        let regexes: RegExp[] = [];

        let bits = text.split(' ');
        for (let bit of bits)
        {
            if (bit.length !== 0)
            {
                regexes.push(stringToRegex(bit));
            }
        }


        this.regexes = regexes;
    }

    score(...strings: string[]): number {
        let score = 0;
        for (let r = 0; r < this.regexes.length; ++r)
        {
            let regex = this.regexes[r];
            regex.lastIndex = 0;
            let matched = false;
            for (let i = 0; i < strings.length; ++i)
            {
                let s = strings[i];
                let m = regex.exec(s);
                if (m) {
                    matched = true;
                    if (m.index === 0) 
                    {
                        if (r === 0 && i === 0) {
                            score += 16;
                        } else {
                            score += 8;
                        }
                    }
                    let end = m.index + m[0].length;
                    if (end === s.length || (end < s.length && s[end] === ' ')) // word boundary
                    {
                        score += 2;
                    } else {
                        score += 1;
                    }
                    break;
                }
            }
            if (!matched) return 0;
        }

        return score;

    }

};

export default SearchFilter;