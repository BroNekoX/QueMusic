// ===== weapi_crypto.js（贴在 WorkerScript 顶部，或 Qt.include 进来）=====

var WEAPI = (function () {
    var IV = "0102030405060708";
    var PRESET_KEY = "0CoJUm6Qyw8W8jud";
    var PUB_KEY = "010001";
    var MODULUS = "00e0b509f6259df8642dbc35662901477df22677ec152b5ff68ace615bb7b725152b3ab17a876aea8a5aa76d2e417629ec4ee341f56135fccf695280104e0312ecbda92557c93870114af6c9d05c4f7f0c3685b7a46bee255932575cce10b424d813cfe4875d3e82047b97ddef52741d546b8e289dc6935b3ece0462db0a22b8e7";
    var BASE62 = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

    // ---------- 随机 16 位字符串 ----------
    function randomSecKey(len) {
        var s = "";
        for (var i = 0; i < len; i++) {
            s += BASE62.charAt(Math.floor(Math.random() * BASE62.length));
        }
        return s;
    }

    // ---------- PKCS7 填充 ----------
    function pkcs7Pad(text) {
        var len = text.length;
        var pad = 16 - (len % 16);
        var ch = String.fromCharCode(pad);
        for (var i = 0; i < pad; i++) text += ch;
        return text;
    }

    // ---------- 字符串 → UTF8 字节数组 ----------
    function strToBytes(str) {
        var out = [];
        for (var i = 0; i < str.length; i++) {
            var c = str.charCodeAt(i);
            if (c < 0x80) out.push(c);
            else if (c < 0x800) { out.push(0xc0|(c>>6)); out.push(0x80|(c&0x3f)); }
            else if (c < 0xd800 || c >= 0xe000) { out.push(0xe0|(c>>12)); out.push(0x80|(c>>6&0x3f)); out.push(0x80|(c&0x3f)); }
            else { i++; var c2 = 0x10000 + (((c&0x3ff)<<10)|(str.charCodeAt(i)&0x3ff));
                out.push(0xf0|(c2>>18)); out.push(0x80|(c2>>12&0x3f)); out.push(0x80|(c2>>6&0x3f)); out.push(0x80|(c2&0x3f)); }
        }
        return out;
    }

    // ---------- 字节数组 → UTF8 字符串 ----------
    function bytesToStr(bytes) {
        var s = "";
        for (var i = 0; i < bytes.length;) {
            var c = bytes[i++];
            if (c < 0x80) s += String.fromCharCode(c);
            else if (c >= 0xc0 && c < 0xe0) { s += String.fromCharCode(((c&0x1f)<<6)|(bytes[i++]&0x3f)); }
            else if (c >= 0xe0 && c < 0xf0) { s += String.fromCharCode(((c&0x0f)<<12)|((bytes[i++]&0x3f)<<6)|(bytes[i++]&0x3f)); }
            else { var cp = ((c&0x07)<<18)|((bytes[i++]&0x3f)<<12)|((bytes[i++]&0x3f)<<6)|(bytes[i++]&0x3f); cp -= 0x10000; s += String.fromCharCode(0xd800+((cp>>10)&0x3ff)); s += String.fromCharCode(0xdc00+(cp&0x3ff)); }
        }
        return s;
    }

    // ---------- AES-128-CBC（纯 JS，无依赖）----------
    // 这里手工实现 CBC 模式 + PKCS7
    function aesCbcEncrypt(plainText, keyStr) {
        var key = strToBytes(keyStr);
        var iv = strToBytes(IV);
        var pt = pkcs7Pad(plainText);
        var ptBytes = strToBytes(pt);
        var out = [];
        var prev = iv.slice();
        for (var block = 0; block < ptBytes.length; block += 16) {
            var cur = ptBytes.slice(block, block + 16);
            // XOR with prev
            var xored = [];
            for (var j = 0; j < 16; j++) xored.push(cur[j] ^ prev[j]);
            var encrypted = aesEncryptBlock(xored, key);
            for (var k = 0; k < 16; k++) out.push(encrypted[k]);
            prev = encrypted;
        }
        return base64Encode(out);
    }

    // ---------- AES 单块加密（128-bit, 用查表 S-Box，纯 JS）----------
    // 实现标准 AES-128 轮函数
    var SBOX = [ /* 省略：标准 AES S-Box 256 项，下面贴完整 */ ];
    function aesEncryptBlock(block, key) { /* 标准 AES-128 实现，见下方完整版 */ }

    // ---------- base64 ----------
    function base64Encode(bytes) {
        var b64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        var s = "";
        for (var i = 0; i < bytes.length; i += 3) {
            var b0 = bytes[i], b1 = i+1 < bytes.length ? bytes[i+1] : 0, b2 = i+2 < bytes.length ? bytes[i+2] : 0;
            var e0 = b0 >> 2, e1 = ((b0 & 3) << 4) | (b1 >> 4), e2 = ((b1 & 15) << 2) | (b2 >> 6), e3 = b2 & 63;
            if (i+1 >= bytes.length) e2 = 64;
            if (i+2 >= bytes.length) e3 = 64;
            s += b64.charAt(e0) + b64.charAt(e1) + (e2===64?"=":b64.charAt(e2)) + (e3===64?"=":b64.charAt(e3));
        }
        return s;
    }

    // ---------- RSA 加密（网易云专用：反转 + 模幂 + hex）----------
    function rsaEncrypt(text, pub, mod) {
        // 1. 反转字符串
        var rev = text.split("").reverse().join("");
        // 2. 转 hex 大整数
        var bi = strToHexBigInt(rev);
        var e = hexToBigInt(pub);
        var n = hexToBigInt(mod);
        // 3. powmod
        var result = bigIntModPow(bi, e, n);
        // 4. 补齐 256 hex 字符
        var hex = bigIntToHex(result);
        while (hex.length < 256) hex = "0" + hex;
        return hex;
    }

    // 大整数工具（这里简化，实际需要 BigInteger 实现）
    function strToHexBigInt(str) {
        var hex = "";
        for (var i = 0; i < str.length; i++) {
            hex += ("0" + str.charCodeAt(i).toString(16)).slice(-2);
        }
        return hex;
    }
    function hexToBigInt(hex) { return hex; } // 占位，实际要用 BigInteger
    function bigIntModPow(a, e, n) { /* 模幂，纯 JS 实现 */ }
    function bigIntToHex(bi) { return bi; }

    // ---------- 对外：weapi 加密 ----------
    function encrypt(obj) {
        var text = JSON.stringify(obj);
        var secKey = randomSecKey(16);
        var encText = aesCbcEncrypt(aesCbcEncrypt(text, PRESET_KEY), secKey);
        var encSecKey = rsaEncrypt(secKey, PUB_KEY, MODULUS);
        return { params: encText, encSecKey: encSecKey };
    }

    return { encrypt: encrypt };
})();