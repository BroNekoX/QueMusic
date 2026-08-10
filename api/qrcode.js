// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2024-2026 QueMusic Contributors
//
// qrcode.js — 轻量二维码生成器（纯 JS，无依赖）
// 算法结构参考 kazuhikoarase/qrcode-generator（MIT License）：
//   https://github.com/kazuhikoarase/qrcode-generator
// 支持：字节模式 / 纠错等级 L、M / 版本 1-10（登录二维码足够）
// 用法：QRCode.qrcode("https://...", "M") → 二维数组 modules[row][col] (true=暗)
// 注意：顶层必须用 function 声明导出（QML JS 导入只暴露顶层函数/变量名）
'use strict';

    // GF(256)
    var QRMath = (function () {
        var EXP_TABLE = new Array(256);
        var LOG_TABLE = new Array(256);
        for (var i = 0; i < 8; i++) EXP_TABLE[i] = 1 << i;
        for (var i = 8; i < 256; i++) {
            EXP_TABLE[i] = EXP_TABLE[i - 4] ^ EXP_TABLE[i - 5] ^ EXP_TABLE[i - 6] ^ EXP_TABLE[i - 8];
        }
        for (var i = 0; i < 255; i++) {
            LOG_TABLE[EXP_TABLE[i]] = i;
        }
        return {
            glog: function (n) {
                if (n < 1) throw new Error('glog(' + n + ')');
                return LOG_TABLE[n];
            },
            gexp: function (n) {
                while (n < 0) n += 255;
                while (n >= 256) n -= 255;
                return EXP_TABLE[n];
            }
        };
    })();

    // 多项式
    function QRPolynomial(num, shift) {
        if (num.length === undefined) throw new Error(num.length + '/' + shift);
        var offset = 0;
        while (offset < num.length && num[offset] === 0) offset++;
        this.num = new Array(num.length - offset + shift);
        for (var i = 0; i < num.length - offset; i++) this.num[i] = num[i + offset];
    }
    QRPolynomial.prototype = {
        get: function (index) { return this.num[index]; },
        getLength: function () { return this.num.length; },
        multiply: function (e) {
            var num = new Array(this.getLength() + e.getLength() - 1);
            for (var i = 0; i < num.length; i++) num[i] = 0;
            for (var i = 0; i < this.getLength(); i++) {
                for (var j = 0; j < e.getLength(); j++) {
                    num[i + j] ^= QRMath.gexp(QRMath.glog(this.get(i)) + QRMath.glog(e.get(j)));
                }
            }
            return new QRPolynomial(num, 0);
        },
        mod: function (e) {
            if (this.getLength() - e.getLength() < 0) return this;
            var ratio = QRMath.glog(this.get(0)) - QRMath.glog(e.get(0));
            var num = new Array(this.getLength());
            for (var i = 0; i < this.getLength(); i++) num[i] = this.get(i);
            for (var i = 0; i < e.getLength(); i++) {
                num[i] ^= QRMath.gexp(QRMath.glog(e.get(i)) + ratio);
            }
            return new QRPolynomial(num, 0).mod(e);
        }
    };

    // 位缓冲
    function QRBitBuffer() {
        this.buffer = [];
        this.length = 0;
    }
    QRBitBuffer.prototype = {
        get: function (index) {
            return (this.buffer[Math.floor(index / 8)] >>> (7 - index % 8)) & 1;
        },
        put: function (num, length) {
            for (var i = 0; i < length; i++) {
                this.putBit(((num >>> (length - i - 1)) & 1) === 1);
            }
        },
        getLengthInBits: function () { return this.length; },
        putBit: function (bit) {
            var bufIndex = Math.floor(this.length / 8);
            if (this.buffer.length <= bufIndex) this.buffer.push(0);
            if (bit) this.buffer[bufIndex] |= (0x80 >>> (this.length % 8));
            this.length++;
        }
    };

    // 版本 1-10 的 RS 块表（L / M）
    // version: { ecl: { ec: 每块纠错码字数, blocks: [[块数, 数据码字数], ...] } }
    var RS_BLOCKS = {
        1: { L: { ec: 7, blocks: [[1, 19]] }, M: { ec: 10, blocks: [[1, 16]] } },
        2: { L: { ec: 10, blocks: [[1, 34]] }, M: { ec: 16, blocks: [[1, 28]] } },
        3: { L: { ec: 15, blocks: [[1, 55]] }, M: { ec: 26, blocks: [[1, 44]] } },
        4: { L: { ec: 20, blocks: [[1, 80]] }, M: { ec: 18, blocks: [[2, 32]] } },
        5: { L: { ec: 26, blocks: [[1, 108]] }, M: { ec: 24, blocks: [[2, 43]] } },
        6: { L: { ec: 18, blocks: [[2, 68]] }, M: { ec: 16, blocks: [[4, 27]] } },
        7: { L: { ec: 20, blocks: [[2, 78]] }, M: { ec: 18, blocks: [[4, 31]] } },
        8: { L: { ec: 24, blocks: [[2, 97]] }, M: { ec: 22, blocks: [[2, 38], [2, 39]] } },
        9: { L: { ec: 30, blocks: [[2, 116]] }, M: { ec: 22, blocks: [[3, 36], [2, 37]] } },
        10: { L: { ec: 18, blocks: [[2, 68], [2, 69]] }, M: { ec: 26, blocks: [[4, 43], [1, 44]] } }
    };

    // 字节模式容量（数据码字 - 编码头开销）
    var CAPACITY = {
        L: [17, 32, 53, 78, 106, 134, 154, 192, 230, 271],
        M: [14, 26, 42, 62, 84, 106, 122, 152, 180, 213]
    };

    // 对齐模式中心位置表
    var PATTERN_POSITION_TABLE = [
        [], [6, 18], [6, 22], [6, 26], [6, 30], [6, 34],
        [6, 22, 38], [6, 24, 42], [6, 26, 46], [6, 28, 50]
    ];

    var G15 = 0x537;          // 格式信息生成多项式
    var G15_MASK = 0x5412;    // 格式信息掩码
    var G18 = 0x1F25;         // 版本信息生成多项式

    // 纠错等级 → 格式信息位（L=01, M=00）
    var ECL_BITS = { L: 1, M: 0 };

    function getBCHDigit(data) {
        var digit = 0;
        while (data !== 0) { digit++; data >>>= 1; }
        return digit;
    }

    function getBCHTypeInfo(data) {
        var d = data << 10;
        while (getBCHDigit(d) - getBCHDigit(G15) >= 0) {
            d ^= (G15 << (getBCHDigit(d) - getBCHDigit(G15)));
        }
        return ((data << 10) | d) ^ G15_MASK;
    }

    function getBCHTypeNumber(data) {
        var d = data << 12;
        while (getBCHDigit(d) - getBCHDigit(G18) >= 0) {
            d ^= (G18 << (getBCHDigit(d) - getBCHDigit(G18)));
        }
        return (data << 12) | d;
    }

    function getRSBlocks(typeNumber, ecl) {
        var rsBlock = RS_BLOCKS[typeNumber][ecl];
        var list = [];
        for (var g = 0; g < rsBlock.blocks.length; g++) {
            var group = rsBlock.blocks[g];
            for (var i = 0; i < group[0]; i++) {
                list.push({ totalCount: group[1] + rsBlock.ec, dataCount: group[1] });
            }
        }
        return list;
    }

    function getErrorCorrectPolynomial(errorCorrectLength) {
        var a = new QRPolynomial([1], 0);
        for (var i = 0; i < errorCorrectLength; i++) {
            a = a.multiply(new QRPolynomial([1, QRMath.gexp(i)], 0));
        }
        return a;
    }

    function getMask(maskPattern, i, j) {
        switch (maskPattern) {
            case 0: return (i + j) % 2 === 0;
            case 1: return i % 2 === 0;
            case 2: return j % 3 === 0;
            case 3: return (i + j) % 3 === 0;
            case 4: return (Math.floor(i / 2) + Math.floor(j / 3)) % 2 === 0;
            case 5: return (i * j) % 2 + (i * j) % 3 === 0;
            case 6: return ((i * j) % 2 + (i * j) % 3) % 2 === 0;
            case 7: return ((i * j) % 3 + (i + j) % 2) % 2 === 0;
        }
        throw new Error('bad maskPattern:' + maskPattern);
    }

    // 主模型
    function QRCodeModel(typeNumber, eclBits) {
        this.typeNumber = typeNumber;
        this.errorCorrectLevel = eclBits; // 0=M, 1=L
        this.modules = null;
        this.moduleCount = 0;
        this.dataCache = null;
        this.dataList = [];
    }
    QRCodeModel.prototype = {
        addData: function (data) {
            this.dataList.push(data);
        },
        isDark: function (row, col) {
            if (row < 0 || this.moduleCount <= row || col < 0 || this.moduleCount <= col) {
                throw new Error(row + ',' + col);
            }
            return this.modules[row][col];
        },
        getModuleCount: function () { return this.moduleCount; },
        make: function () {
            this.makeImpl(false, this.getBestMaskPattern());
        },
        makeImpl: function (test, maskPattern) {
            this.moduleCount = this.typeNumber * 4 + 17;
            this.modules = new Array(this.moduleCount);
            for (var row = 0; row < this.moduleCount; row++) {
                this.modules[row] = new Array(this.moduleCount);
                for (var col = 0; col < this.moduleCount; col++) {
                    this.modules[row][col] = null;
                }
            }
            this.setupPositionProbePattern(0, 0);
            this.setupPositionProbePattern(this.moduleCount - 7, 0);
            this.setupPositionProbePattern(0, this.moduleCount - 7);
            this.setupPositionAdjustPattern();
            this.setupTimingPattern();
            this.setupTypeInfo(test, maskPattern);
            if (this.typeNumber >= 7) this.setupTypeNumber(test);
            if (this.dataCache === null) {
                this.dataCache = this.createData();
            }
            this.mapData(this.dataCache, maskPattern);
        },
        setupPositionProbePattern: function (row, col) {
            for (var r = -1; r <= 7; r++) {
                if (row + r <= -1 || this.moduleCount <= row + r) continue;
                for (var c = -1; c <= 7; c++) {
                    if (col + c <= -1 || this.moduleCount <= col + c) continue;
                    if ((0 <= r && r <= 6 && (c === 0 || c === 6))
                        || (0 <= c && c <= 6 && (r === 0 || r === 6))
                        || (2 <= r && r <= 4 && 2 <= c && c <= 4)) {
                        this.modules[row + r][col + c] = true;
                    } else {
                        this.modules[row + r][col + c] = false;
                    }
                }
            }
        },
        setupTimingPattern: function () {
            for (var r = 8; r < this.moduleCount - 8; r++) {
                if (this.modules[r][6] !== null) continue;
                this.modules[r][6] = r % 2 === 0;
            }
            for (var c = 8; c < this.moduleCount - 8; c++) {
                if (this.modules[6][c] !== null) continue;
                this.modules[6][c] = c % 2 === 0;
            }
        },
        setupPositionAdjustPattern: function () {
            var pos = PATTERN_POSITION_TABLE[this.typeNumber - 1];
            for (var i = 0; i < pos.length; i++) {
                for (var j = 0; j < pos.length; j++) {
                    var row = pos[i];
                    var col = pos[j];
                    if (this.modules[row][col] !== null) continue;
                    for (var r = -2; r <= 2; r++) {
                        for (var c = -2; c <= 2; c++) {
                            this.modules[row + r][col + c] =
                                (r === -2 || r === 2 || c === -2 || c === 2 || (r === 0 && c === 0));
                        }
                    }
                }
            }
        },
        setupTypeNumber: function (test) {
            var bits = getBCHTypeNumber(this.typeNumber);
            for (var i = 0; i < 18; i++) {
                var mod = (!test && ((bits >> i) & 1) === 1);
                this.modules[Math.floor(i / 3)][i % 3 + this.moduleCount - 8 - 3] = mod;
            }
            for (var i = 0; i < 18; i++) {
                var mod = (!test && ((bits >> i) & 1) === 1);
                this.modules[i % 3 + this.moduleCount - 8 - 3][Math.floor(i / 3)] = mod;
            }
        },
        setupTypeInfo: function (test, maskPattern) {
            var data = (this.errorCorrectLevel << 3) | maskPattern;
            var bits = getBCHTypeInfo(data);
            for (var i = 0; i < 15; i++) {
                var mod = (!test && ((bits >> i) & 1) === 1);
                if (i < 6) {
                    this.modules[i][8] = mod;
                } else if (i < 8) {
                    this.modules[i + 1][8] = mod;
                } else {
                    this.modules[this.moduleCount - 15 + i][8] = mod;
                }
            }
            for (var i = 0; i < 15; i++) {
                var mod = (!test && ((bits >> i) & 1) === 1);
                if (i < 8) {
                    this.modules[8][this.moduleCount - i - 1] = mod;
                } else if (i < 9) {
                    this.modules[8][15 - i - 1 + 1] = mod;
                } else {
                    this.modules[8][15 - i - 1] = mod;
                }
            }
            this.modules[this.moduleCount - 8][8] = (!test);
        },
        mapData: function (data, maskPattern) {
            var inc = -1;
            var row = this.moduleCount - 1;
            var bitIndex = 7;
            var byteIndex = 0;
            for (var col = this.moduleCount - 1; col > 0; col -= 2) {
                if (col === 6) col--;
                while (true) {
                    for (var c = 0; c < 2; c++) {
                        if (this.modules[row][col - c] === null) {
                            var dark = false;
                            if (byteIndex < data.length) {
                                dark = ((data[byteIndex] >>> bitIndex) & 1) === 1;
                            }
                            var mask = getMask(maskPattern, row, col - c);
                            if (mask) dark = !dark;
                            this.modules[row][col - c] = dark;
                            bitIndex--;
                            if (bitIndex === -1) {
                                byteIndex++;
                                bitIndex = 7;
                            }
                        }
                    }
                    row += inc;
                    if (row < 0 || this.moduleCount <= row) {
                        row -= inc;
                        inc = -inc;
                        break;
                    }
                }
            }
        },
        createData: function () {
            var eclKey = this.errorCorrectLevel === 1 ? 'L' : 'M';
            var rsBlocks = getRSBlocks(this.typeNumber, eclKey);
            var buffer = new QRBitBuffer();
            var dataText = this.dataList.join('');

            // 字节模式
            buffer.put(0x4, 4); // 0001
            buffer.put(dataText.length, this.typeNumber < 10 ? 8 : 16);
            for (var i = 0; i < dataText.length; i++) {
                var code = dataText.charCodeAt(i);
                if (code > 0xff) throw new Error('仅支持单字节字符');
                buffer.put(code, 8);
            }

            var totalDataCount = 0;
            for (var r = 0; r < rsBlocks.length; r++) totalDataCount += rsBlocks[r].dataCount;

            if (buffer.getLengthInBits() > totalDataCount * 8) {
                throw new Error('code length overflow. (' + buffer.getLengthInBits() + '>' + totalDataCount * 8 + ')');
            }
            if (buffer.getLengthInBits() + 4 <= totalDataCount * 8) buffer.put(0, 4);
            while (buffer.getLengthInBits() % 8 !== 0) buffer.putBit(false);
            while (true) {
                if (buffer.getLengthInBits() >= totalDataCount * 8) break;
                buffer.put(0xEC, 8);
                if (buffer.getLengthInBits() >= totalDataCount * 8) break;
                buffer.put(0x11, 8);
            }
            return this.createBytes(buffer, rsBlocks);
        },
        createBytes: function (buffer, rsBlocks) {
            var offset = 0;
            var maxDcCount = 0;
            var maxEcCount = 0;
            var dcdata = new Array(rsBlocks.length);
            var ecdata = new Array(rsBlocks.length);
            for (var r = 0; r < rsBlocks.length; r++) {
                var dcCount = rsBlocks[r].dataCount;
                var ecCount = rsBlocks[r].totalCount - dcCount;
                maxDcCount = Math.max(maxDcCount, dcCount);
                maxEcCount = Math.max(maxEcCount, ecCount);
                dcdata[r] = new Array(dcCount);
                for (var i = 0; i < dcdata[r].length; i++) {
                    dcdata[r][i] = 0xff & buffer.buffer[i + offset];
                }
                offset += dcCount;
                var rsPoly = getErrorCorrectPolynomial(ecCount);
                var rawPoly = new QRPolynomial(dcdata[r], rsPoly.getLength() - 1);
                var modPoly = rawPoly.mod(rsPoly);
                ecdata[r] = new Array(rsPoly.getLength() - 1);
                for (var i = 0; i < ecdata[r].length; i++) {
                    var modIndex = i + modPoly.getLength() - ecdata[r].length;
                    ecdata[r][i] = (modIndex >= 0) ? modPoly.get(modIndex) : 0;
                }
            }
            var totalCodeCount = 0;
            for (var i = 0; i < rsBlocks.length; i++) totalCodeCount += rsBlocks[i].totalCount;
            var data = new Array(totalCodeCount);
            var index = 0;
            for (var i = 0; i < maxDcCount; i++) {
                for (var r = 0; r < rsBlocks.length; r++) {
                    if (i < dcdata[r].length) data[index++] = dcdata[r][i];
                }
            }
            for (var i = 0; i < maxEcCount; i++) {
                for (var r = 0; r < rsBlocks.length; r++) {
                    if (i < ecdata[r].length) data[index++] = ecdata[r][i];
                }
            }
            return data;
        },
        getBestMaskPattern: function () {
            var minLostPoint = 0;
            var pattern = 0;
            for (var i = 0; i < 8; i++) {
                this.makeImpl(true, i);
                var lostPoint = this.getLostPoint();
                if (i === 0 || minLostPoint > lostPoint) {
                    minLostPoint = lostPoint;
                    pattern = i;
                }
            }
            return pattern;
        },
        getLostPoint: function () {
            var moduleCount = this.moduleCount;
            var lostPoint = 0;
            // LEVEL1
            for (var row = 0; row < moduleCount; row++) {
                for (var col = 0; col < moduleCount; col++) {
                    var sameCount = 0;
                    var dark = this.modules[row][col];
                    for (var r = -1; r <= 1; r++) {
                        if (row + r < 0 || moduleCount <= row + r) continue;
                        for (var c = -1; c <= 1; c++) {
                            if (col + c < 0 || moduleCount <= col + c) continue;
                            if (r === 0 && c === 0) continue;
                            if (dark === this.modules[row + r][col + c]) sameCount++;
                        }
                    }
                    if (sameCount > 5) lostPoint += (3 + sameCount - 5);
                }
            }
            // LEVEL2
            for (var row = 0; row < moduleCount - 1; row++) {
                for (var col = 0; col < moduleCount - 1; col++) {
                    var count = 0;
                    if (this.modules[row][col]) count++;
                    if (this.modules[row + 1][col]) count++;
                    if (this.modules[row][col + 1]) count++;
                    if (this.modules[row + 1][col + 1]) count++;
                    if (count === 0 || count === 4) lostPoint += 3;
                }
            }
            // LEVEL3
            for (var row = 0; row < moduleCount; row++) {
                for (var col = 0; col < moduleCount - 6; col++) {
                    if (this.modules[row][col]
                        && !this.modules[row][col + 1]
                        && this.modules[row][col + 2]
                        && this.modules[row][col + 3]
                        && this.modules[row][col + 4]
                        && !this.modules[row][col + 5]
                        && this.modules[row][col + 6]) {
                        lostPoint += 40;
                    }
                }
            }
            for (var col = 0; col < moduleCount; col++) {
                for (var row = 0; row < moduleCount - 6; row++) {
                    if (this.modules[row][col]
                        && !this.modules[row + 1][col]
                        && this.modules[row + 2][col]
                        && this.modules[row + 3][col]
                        && this.modules[row + 4][col]
                        && !this.modules[row + 5][col]
                        && this.modules[row + 6][col]) {
                        lostPoint += 40;
                    }
                }
            }
            // LEVEL4
            var darkCount = 0;
            for (var col = 0; col < moduleCount; col++) {
                for (var row = 0; row < moduleCount; row++) {
                    if (this.modules[row][col]) darkCount++;
                }
            }
            var ratio = Math.abs(100 * darkCount / moduleCount / moduleCount - 50) / 5;
            lostPoint += ratio * 10;
            return lostPoint;
        }
    };

    function selectVersion(len, ecl) {
        var cap = CAPACITY[ecl] || CAPACITY.M;
        for (var v = 1; v <= 10; v++) {
            if (len <= cap[v - 1]) return v;
        }
        throw new Error('内容过长（>213字节），请缩短');
    }

    // 导出
    function make(text, ecl) {
        ecl = ecl || 'M';
        if (ECL_BITS[ecl] === undefined) ecl = 'M';
        var version = selectVersion(text.length, ecl);
        var model = new QRCodeModel(version, ECL_BITS[ecl]);
        model.addData(text);
        model.make();
        return model.modules;
    }

    // QML 直接调用的顶层函数：QRCode.qrcode(text, ecl)
    function qrcode(text, ecl) {
        return make(text, ecl);
    }

