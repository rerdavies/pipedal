export function safeFilenameDecode(filename: string): string {
    let originalFilename = filename;
    let pos = filename.indexOf('%');
    if (pos < 0) {
        return filename;
    }
    let result = "";
    while (true) {
        if (pos < 0) {
            result += filename;
            return result;
        }
        result += filename.substring(0, pos);
        if (pos+3 >= filename.length) {
            // invalid encoding. just use the original
            return originalFilename; 
        }
        let hex = filename.charAt(pos+1) + filename.charAt(pos+2);
        let byte = parseInt(hex, 16);

        // Handle UTF-8 to UTF-16 conversion
        if (byte < 0x80) {
            // Single-byte character (ASCII)
            result += String.fromCharCode(byte);
            filename = filename.substring(pos+3);
        } else if ((byte & 0xE0) === 0xC0) {
            // 2-byte sequence
            if (pos+6 > filename.length || filename.charAt(pos+3) !== '%') {
                return originalFilename; // invalid encoding
            }
            let byte2 = parseInt(filename.charAt(pos+4) + filename.charAt(pos+5), 16);
            let codePoint = ((byte & 0x1F) << 6) | (byte2 & 0x3F);
            result += String.fromCharCode(codePoint);
            filename = filename.substring(pos+6);
        } else if ((byte & 0xF0) === 0xE0) {
            // 3-byte sequence
            if (pos+9 > filename.length || filename.charAt(pos+3) !== '%' || filename.charAt(pos+6) !== '%') {
                return originalFilename; // invalid encoding
            }
            let byte2 = parseInt(filename.charAt(pos+4) + filename.charAt(pos+5), 16);
            let byte3 = parseInt(filename.charAt(pos+7) + filename.charAt(pos+8), 16);
            let codePoint = ((byte & 0x0F) << 12) | ((byte2 & 0x3F) << 6) | (byte3 & 0x3F);
            result += String.fromCharCode(codePoint);
            filename = filename.substring(pos+9);
        } else if ((byte & 0xF8) === 0xF0) {
            // 4-byte sequence (surrogate pair needed)
            if (pos+12 > filename.length || filename.charAt(pos+3) !== '%' || filename.charAt(pos+6) !== '%' || filename.charAt(pos+9) !== '%') {
                return originalFilename; // invalid encoding
            }
            let byte2 = parseInt(filename.charAt(pos+4) + filename.charAt(pos+5), 16);
            let byte3 = parseInt(filename.charAt(pos+7) + filename.charAt(pos+8), 16);
            let byte4 = parseInt(filename.charAt(pos+10) + filename.charAt(pos+11), 16);
            let codePoint = ((byte & 0x07) << 18) | ((byte2 & 0x3F) << 12) | ((byte3 & 0x3F) << 6) | (byte4 & 0x3F);
            // Convert to UTF-16 surrogate pair
            codePoint -= 0x10000;
            result += String.fromCharCode(0xD800 + (codePoint >> 10));
            result += String.fromCharCode(0xDC00 + (codePoint & 0x3FF));
            filename = filename.substring(pos+12);
        } else {
            // Invalid UTF-8 start byte
            return originalFilename;
        }
        pos = filename.indexOf('%');
    }
}

export function safeFilenameEncode(filename: string): string {
    let result = "";
    
    for (let i = 0; i < filename.length; i++) {
        let charCode = filename.charCodeAt(i);
        
        // Handle UTF-16 surrogate pairs
        if (charCode >= 0xD800 && charCode <= 0xDBFF) {
            // High surrogate - must be followed by low surrogate
            if (i + 1 < filename.length) {
                let lowSurrogate = filename.charCodeAt(i + 1);
                if (lowSurrogate >= 0xDC00 && lowSurrogate <= 0xDFFF) {
                    // Combine surrogates to get the code point
                    let codePoint = 0x10000 + ((charCode & 0x3FF) << 10) + (lowSurrogate & 0x3FF);
                    
                    // Encode as 4-byte UTF-8
                    let byte1 = 0xF0 | ((codePoint >> 18) & 0x07);
                    let byte2 = 0x80 | ((codePoint >> 12) & 0x3F);
                    let byte3 = 0x80 | ((codePoint >> 6) & 0x3F);
                    let byte4 = 0x80 | (codePoint & 0x3F);
                    
                    result += '%' + byte1.toString(16).padStart(2, '0');
                    result += '%' + byte2.toString(16).padStart(2, '0');
                    result += '%' + byte3.toString(16).padStart(2, '0');
                    result += '%' + byte4.toString(16).padStart(2, '0');
                    
                    i++; // Skip the low surrogate
                    continue;
                }
            }
            // Invalid surrogate pair - encode as-is (shouldn't happen in valid strings)
        }
        
        // Handle control characters (< 0x20)
        if (charCode < 0x20) {
            result += '%' + charCode.toString(16).padStart(2, '0');
        }
        // Handle ASCII printable characters (0x20-0x7F), space is NOT encoded
        else if (charCode <= 0x7F) {
            result += filename.charAt(i);
        }
        // Handle 2-byte UTF-8 (0x80-0x7FF)
        else if (charCode <= 0x7FF) {
            let byte1 = 0xC0 | ((charCode >> 6) & 0x1F);
            let byte2 = 0x80 | (charCode & 0x3F);
            result += '%' + byte1.toString(16).padStart(2, '0');
            result += '%' + byte2.toString(16).padStart(2, '0');
        }
        // Handle 3-byte UTF-8 (0x800-0xFFFF)
        else {
            let byte1 = 0xE0 | ((charCode >> 12) & 0x0F);
            let byte2 = 0x80 | ((charCode >> 6) & 0x3F);
            let byte3 = 0x80 | (charCode & 0x3F);
            result += '%' + byte1.toString(16).padStart(2, '0');
            result += '%' + byte2.toString(16).padStart(2, '0');
            result += '%' + byte3.toString(16).padStart(2, '0');
        }
    }
    
    return result;
}
