#include "qrcode.h"
#include <algorithm>
#include <climits>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <utility>
#include <QPainter>
#include <QColor>

namespace qrcodegen {

// ============================================================
// QR SEGMENT
// ============================================================

QrSegment::QrSegment(Mode md, int numCh, const std::vector<bool> &dt) :
    mode(md),
    numChars(numCh),
    data(dt) {
    if (numCh < 0)
        throw std::domain_error("Negative length");
}

QrSegment::QrSegment(Mode md, int numCh, std::vector<bool> &&dt) :
    mode(md),
    numChars(numCh),
    data(std::move(dt)) {
    if (numCh < 0)
        throw std::domain_error("Negative length");
}

QrSegment::Mode QrSegment::getMode() const { return mode; }
int QrSegment::getNumChars() const { return numChars; }
const std::vector<bool> &QrSegment::getData() const { return data; }

QrSegment QrSegment::makeBytes(const std::vector<uint8_t> &data) {
    if (data.size() > static_cast<size_t>(INT_MAX))
        throw std::length_error("Data too long");
    std::vector<bool> bb;
    for (uint8_t b : data) {
        for (int i = 7; i >= 0; i--)
            bb.push_back(((b >> i) & 1) != 0);
    }
    return QrSegment(Mode::BYTE, static_cast<int>(data.size()), std::move(bb));
}

QrSegment QrSegment::makeNumeric(const char *digits) {
    std::vector<bool> bb;
    int len = 0;
    while (digits[len] != '\0') {
        char c = digits[len];
        if (c < '0' || c > '9')
            throw std::invalid_argument("String contains non-numeric characters");
        len++;
    }
    for (int i = 0; i < len; ) {
        int n = std::min(len - i, 3);
        int val = 0;
        for (int j = 0; j < n; j++)
            val = val * 10 + (digits[i + j] - '0');
        int bitLen = n * 3 + 1;
        for (int j = bitLen - 1; j >= 0; j--)
            bb.push_back(((val >> j) & 1) != 0);
        i += n;
    }
    return QrSegment(Mode::NUMERIC, len, std::move(bb));
}

static const char *ALPHANUMERIC_CHARSET = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ $%*+-./:";

QrSegment QrSegment::makeAlphanumeric(const char *text) {
    std::vector<bool> bb;
    int len = 0;
    while (text[len] != '\0') {
        const char *p = std::strchr(ALPHANUMERIC_CHARSET, text[len]);
        if (!p)
            throw std::invalid_argument("String contains unencodable characters in alphanumeric mode");
        len++;
    }
    for (int i = 0; i < len; ) {
        int n = std::min(len - i, 2);
        int val = 0;
        for (int j = 0; j < n; j++) {
            val = val * 45 + static_cast<int>(std::strchr(ALPHANUMERIC_CHARSET, text[i + j]) - ALPHANUMERIC_CHARSET);
        }
        int bitLen = (n == 2) ? 11 : 6;
        for (int j = bitLen - 1; j >= 0; j--)
            bb.push_back(((val >> j) & 1) != 0);
        i += n;
    }
    return QrSegment(Mode::ALPHANUMERIC, len, std::move(bb));
}

QrSegment QrSegment::makeSegments(const char *text) {
    bool isNumeric = (text[0] != '\0');
    bool isAlpha = (text[0] != '\0');
    for (int i = 0; text[i] != '\0'; i++) {
        char c = text[i];
        if (c < '0' || c > '9') isNumeric = false;
        if (!std::strchr(ALPHANUMERIC_CHARSET, c)) isAlpha = false;
    }
    if (isNumeric) return makeNumeric(text);
    if (isAlpha) return makeAlphanumeric(text);

    std::vector<uint8_t> bytes;
    for (int i = 0; text[i] != '\0'; i++) {
        bytes.push_back(static_cast<uint8_t>(text[i]));
    }
    return makeBytes(bytes);
}

int QrSegment::getTotalBits(const std::vector<QrSegment> &segs, int version) {
    int result = 0;
    for (const QrSegment &seg : segs) {
        int ccbits;
        switch (seg.mode) {
            case Mode::NUMERIC:      ccbits = (version < 10) ? 10 : (version < 27) ? 12 : 14; break;
            case Mode::ALPHANUMERIC: ccbits = (version < 10) ? 9  : (version < 27) ? 11 : 13; break;
            case Mode::BYTE:         ccbits = (version < 10) ? 8  : 16; break;
            default:                 ccbits = 8; break;
        }
        if (seg.numChars >= (1 << ccbits))
            return -1;
        result += 4 + ccbits + static_cast<int>(seg.data.size());
    }
    return result;
}

// ============================================================
// ISO/IEC 18004 CONSTANTS AND TABLES
// ============================================================

static const int NUM_ERROR_CORRECTION_CODEWORDS[4][41] = {
    // Version: 0, 1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40
    {0,  7, 10, 15, 20, 26, 18, 20, 24, 30, 18, 20, 24, 26, 30, 22, 24, 28, 30, 28, 28, 28, 28, 30, 30, 26, 28, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30}, // Low
    {0, 10, 16, 26, 18, 24, 16, 18, 22, 22, 26, 30, 22, 22, 24, 24, 28, 28, 26, 26, 26, 26, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28}, // Medium
    {0, 13, 22, 18, 26, 18, 24, 18, 22, 20, 24, 28, 26, 24, 20, 30, 24, 28, 28, 26, 30, 28, 30, 30, 30, 30, 28, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30}, // Quartile
    {0, 17, 28, 22, 16, 22, 28, 26, 26, 24, 28, 24, 28, 22, 24, 24, 30, 28, 28, 26, 28, 30, 24, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30}  // High
};

static const int NUM_ERROR_CORRECTION_BLOCKS[4][41] = {
    {0, 1, 1, 1, 1, 1, 2, 2, 2, 2, 4, 4, 4, 4, 4, 6, 6, 6, 6, 7, 8, 8, 9, 9, 10, 12, 12, 12, 13, 14, 15, 16, 17, 18, 19, 19, 20, 21, 22, 24, 25},
    {0, 1, 1, 1, 2, 2, 4, 4, 4, 5, 5, 5, 8, 9, 9, 10, 10, 11, 13, 14, 16, 17, 17, 18, 20, 21, 23, 25, 26, 28, 29, 31, 33, 35, 37, 38, 40, 43, 45, 47, 49},
    {0, 1, 1, 2, 2, 4, 4, 6, 6, 8, 8, 8, 10, 12, 16, 12, 17, 16, 18, 21, 20, 23, 23, 25, 27, 29, 34, 34, 35, 38, 40, 43, 45, 48, 51, 53, 56, 59, 62, 65, 68},
    {0, 1, 1, 2, 4, 4, 4, 5, 6, 8, 8, 11, 11, 16, 16, 18, 16, 19, 21, 25, 25, 25, 34, 30, 32, 35, 37, 40, 42, 45, 48, 51, 54, 57, 60, 63, 66, 70, 74, 77, 81}
};

static const int NUM_RAW_DATA_MODULES[41] = {
    0, 208, 359, 567, 807, 1079, 1383, 1568, 1936, 2336, 2768, 3232, 3728, 4256, 4651, 5243, 5867, 6523, 7211, 7931,
    8683, 9252, 10068, 10916, 11796, 12708, 13652, 14628, 15371, 16411, 17483, 18587, 19723, 20891, 22091, 23008, 24272, 25568, 26896, 28256, 29648
};

static int getNumDataCodewords(int ver, QrCode::Ecc ecl) {
    return NUM_RAW_DATA_MODULES[ver] / 8 - NUM_ERROR_CORRECTION_CODEWORDS[static_cast<int>(ecl)][ver] * NUM_ERROR_CORRECTION_BLOCKS[static_cast<int>(ecl)][ver];
}

// ============================================================
// QR CODE GENERATOR
// ============================================================

QrCode QrCode::encodeText(const char *text, Ecc ecl) {
    std::vector<QrSegment> segs;
    segs.push_back(QrSegment::makeSegments(text));
    return encodeSegments(segs, ecl);
}

QrCode QrCode::encodeBinary(const std::vector<uint8_t> &data, Ecc ecl) {
    std::vector<QrSegment> segs;
    segs.push_back(QrSegment::makeBytes(data));
    return encodeSegments(segs, ecl);
}

QrCode QrCode::encodeSegments(const std::vector<QrSegment> &segs, Ecc ecl, int minVersion, int maxVersion, int mask, bool boostEcl) {
    int version, dataUsedBits = -1;
    for (version = minVersion; ; version++) {
        int dataCapacityBits = getNumDataCodewords(version, ecl) * 8;
        dataUsedBits = QrSegment::getTotalBits(segs, version);
        if (dataUsedBits != -1 && dataUsedBits <= dataCapacityBits)
            break;
        if (version >= maxVersion)
            throw std::length_error("Data too long for chosen QR Code version range");
    }

    if (boostEcl) {
        for (Ecc newEcl : {Ecc::MEDIUM, Ecc::QUARTILE, Ecc::HIGH}) {
            if (dataUsedBits <= getNumDataCodewords(version, newEcl) * 8)
                ecl = newEcl;
        }
    }

    std::vector<bool> bb;
    for (const QrSegment &seg : segs) {
        int modeBits;
        switch (seg.getMode()) {
            case QrSegment::Mode::NUMERIC:      modeBits = 0x1; break;
            case QrSegment::Mode::ALPHANUMERIC: modeBits = 0x2; break;
            case QrSegment::Mode::BYTE:         modeBits = 0x4; break;
            default:                            modeBits = 0x4; break;
        }
        for (int i = 3; i >= 0; i--)
            bb.push_back(((modeBits >> i) & 1) != 0);

        int ccbits;
        switch (seg.getMode()) {
            case QrSegment::Mode::NUMERIC:      ccbits = (version < 10) ? 10 : (version < 27) ? 12 : 14; break;
            case QrSegment::Mode::ALPHANUMERIC: ccbits = (version < 10) ? 9  : (version < 27) ? 11 : 13; break;
            case QrSegment::Mode::BYTE:         ccbits = (version < 10) ? 8  : 16; break;
            default:                            ccbits = 8; break;
        }
        for (int i = ccbits - 1; i >= 0; i--)
            bb.push_back(((seg.getNumChars() >> i) & 1) != 0);

        for (bool b : seg.getData())
            bb.push_back(b);
    }

    int capacityBits = getNumDataCodewords(version, ecl) * 8;
    for (int i = 0; i < 4 && static_cast<int>(bb.size()) < capacityBits; i++)
        bb.push_back(false);
    while (bb.size() % 8 != 0)
        bb.push_back(false);

    static const uint8_t PAD_BYTES[2] = {0xEC, 0x11};
    for (int i = 0; static_cast<int>(bb.size()) < capacityBits; i++) {
        for (int j = 7; j >= 0; j--)
            bb.push_back(((PAD_BYTES[i % 2] >> j) & 1) != 0);
    }

    std::vector<uint8_t> dataCodewords(bb.size() / 8);
    for (size_t i = 0; i < bb.size(); i++)
        dataCodewords[i / 8] |= (bb[i] ? 1 : 0) << (7 - (i % 8));

    return QrCode(version, ecl, dataCodewords, mask);
}

// Reed-Solomon error correction for GF(256) with generator poly 0x11D
static uint8_t reedSolomonMultiply(uint8_t x, uint8_t y) {
    int z = 0;
    for (int i = 7; i >= 0; i--) {
        z = (z << 1) ^ ((z >> 7) * 0x11D);
        z ^= ((y >> i) & 1) * x;
    }
    return static_cast<uint8_t>(z);
}

static std::vector<uint8_t> reedSolomonComputeDivisor(int degree) {
    std::vector<uint8_t> result(static_cast<size_t>(degree));
    result[result.size() - 1] = 1;
    uint8_t root = 1;
    for (int i = 0; i < degree; i++) {
        for (size_t j = 0; j < result.size(); j++) {
            result[j] = reedSolomonMultiply(result[j], root);
            if (j + 1 < result.size())
                result[j] ^= result[j + 1];
        }
        root = reedSolomonMultiply(root, 0x02);
    }
    return result;
}

static std::vector<uint8_t> reedSolomonComputeRemainder(const std::vector<uint8_t> &data, const std::vector<uint8_t> &divisor) {
    std::vector<uint8_t> result(divisor.size(), 0);
    for (uint8_t b : data) {
        uint8_t factor = b ^ result[0];
        result.erase(result.begin());
        result.push_back(0);
        for (size_t i = 0; i < divisor.size(); i++)
            result[i] ^= reedSolomonMultiply(divisor[i], factor);
    }
    return result;
}

QrCode::QrCode(int ver, Ecc ecl, const std::vector<uint8_t> &dataCodewords, int msk) :
    version(ver),
    size(ver * 4 + 17),
    errorCorrectionLevel(ecl),
    modules(size, std::vector<bool>(size)),
    isFunction(size, std::vector<bool>(size)) {

    if (ver < MIN_VERSION || ver > MAX_VERSION)
        throw std::domain_error("Version out of range");

    drawFunctionPatterns();
    std::vector<uint8_t> allCodewords;

    int numBlocks = NUM_ERROR_CORRECTION_BLOCKS[static_cast<int>(ecl)][ver];
    int blockEccLen = NUM_ERROR_CORRECTION_CODEWORDS[static_cast<int>(ecl)][ver];
    int rawCodewords = NUM_RAW_DATA_MODULES[ver] / 8;
    int numShortBlocks = numBlocks - rawCodewords % numBlocks;
    int shortBlockDataLen = rawCodewords / numBlocks - blockEccLen;

    std::vector<std::vector<uint8_t>> blocks;
    std::vector<uint8_t> rsDivisor = reedSolomonComputeDivisor(blockEccLen);
    for (int i = 0, k = 0; i < numBlocks; i++) {
        int datLen = shortBlockDataLen + (i < numShortBlocks ? 0 : 1);
        std::vector<uint8_t> dat(dataCodewords.begin() + k, dataCodewords.begin() + (k + datLen));
        k += datLen;
        std::vector<uint8_t> ecc = reedSolomonComputeRemainder(dat, rsDivisor);
        dat.insert(dat.end(), ecc.begin(), ecc.end());
        blocks.push_back(std::move(dat));
    }

    for (size_t i = 0; i < blocks[0].size(); i++) {
        for (size_t j = 0; j < blocks.size(); j++) {
            if (i < blocks[j].size())
                allCodewords.push_back(blocks[j][i]);
        }
    }

    drawCodewords(allCodewords);

    if (msk == -1) {
        long minPenalty = LONG_MAX;
        for (int i = 0; i < 8; i++) {
            applyMask(i);
            drawFormatBits(i);
            long penalty = getPenaltyScore();
            if (penalty < minPenalty) {
                minPenalty = penalty;
                mask = i;
            }
            applyMask(i);
        }
    } else {
        mask = msk;
    }
    applyMask(mask);
    drawFormatBits(mask);
    isFunction.clear();
    isFunction.shrink_to_fit();
}

int QrCode::getVersion() const { return version; }
int QrCode::getSize() const { return size; }
QrCode::Ecc QrCode::getErrorCorrectionLevel() const { return errorCorrectionLevel; }
int QrCode::getMask() const { return mask; }
bool QrCode::getModule(int x, int y) const { return (0 <= x && x < size && 0 <= y && y < size) ? modules[y][x] : false; }

void QrCode::drawFunctionPatterns() {
    for (int i = 0; i < size; i++) {
        setFunctionModule(6, i, i % 2 == 0);
        setFunctionModule(i, 6, i % 2 == 0);
    }
    drawFinderPattern(3, 3);
    drawFinderPattern(size - 4, 3);
    drawFinderPattern(3, size - 4);

    if (version >= 2) {
        int numAlign = version / 7 + 2;
        int step = (version == 32) ? 26 : (version * 4 + numAlign * 2 + 1) / (numAlign * 2 - 2) * 2;
        std::vector<int> pos(numAlign);
        pos[0] = 6;
        for (int i = numAlign - 1, p = size - 7; i >= 1; i--, p -= step)
            pos[i] = p;
        for (int i = 0; i < numAlign; i++) {
            for (int j = 0; j < numAlign; j++) {
                if ((i == 0 && j == 0) || (i == 0 && j == numAlign - 1) || (i == numAlign - 1 && j == 0))
                    continue;
                drawAlignmentPattern(pos[i], pos[j]);
            }
        }
    }
    drawFormatBits(0);
    drawVersion();
}

void QrCode::drawFinderPattern(int x, int y) {
    for (int dy = -4; dy <= 4; dy++) {
        for (int dx = -4; dx <= 4; dx++) {
            int dist = std::max(std::abs(dx), std::abs(dy));
            int xx = x + dx, yy = y + dy;
            if (xx >= 0 && xx < size && yy >= 0 && yy < size)
                setFunctionModule(xx, yy, dist != 2 && dist != 4);
        }
    }
}

void QrCode::drawAlignmentPattern(int x, int y) {
    for (int dy = -2; dy <= 2; dy++) {
        for (int dx = -2; dx <= 2; dx++)
            setFunctionModule(x + dx, y + dy, std::max(std::abs(dx), std::abs(dy)) != 1);
    }
}

void QrCode::setFunctionModule(int x, int y, bool isDark) {
    modules[y][x] = isDark;
    isFunction[y][x] = true;
}

void QrCode::drawFormatBits(int msk) {
    int data = (static_cast<int>(errorCorrectionLevel) ^ 1) << 3 | msk;
    int rem = data;
    for (int i = 0; i < 10; i++)
        rem = (rem << 1) ^ ((rem >> 9) * 0x537);
    int bits = (data << 10 | rem) ^ 0x5412;

    for (int i = 0; i <= 5; i++)
        setFunctionModule(8, i, getBit(bits, i));
    setFunctionModule(8, 7, getBit(bits, 6));
    setFunctionModule(8, 8, getBit(bits, 7));
    setFunctionModule(7, 8, getBit(bits, 8));
    for (int i = 9; i < 15; i++)
        setFunctionModule(14 - i, 8, getBit(bits, i));

    for (int i = 0; i < 8; i++)
        setFunctionModule(size - 1 - i, 8, getBit(bits, i));
    for (int i = 8; i < 15; i++)
        setFunctionModule(8, size - 15 + i, getBit(bits, i));
    setFunctionModule(8, size - 8, true);
}

void QrCode::drawVersion() {
    if (version < 7) return;
    int rem = version;
    for (int i = 0; i < 12; i++)
        rem = (rem << 1) ^ ((rem >> 11) * 0x1F25);
    int bits = version << 12 | rem;
    for (int i = 0; i < 18; i++) {
        bool bit = getBit(bits, i);
        int a = size - 11 + i % 3, b = i / 3;
        setFunctionModule(a, b, bit);
        setFunctionModule(b, a, bit);
    }
}

void QrCode::drawCodewords(const std::vector<uint8_t> &data) {
    size_t i = 0;
    for (int right = size - 1; right >= 1; right -= 2) {
        if (right == 6) right = 5;
        for (int vert = 0; vert < size; vert++) {
            for (int j = 0; j < 2; j++) {
                int x = right - j;
                bool upwards = ((right + 1) & 2) == 0;
                int y = upwards ? size - 1 - vert : vert;
                if (!isFunction[y][x] && i < data.size() * 8) {
                    modules[y][x] = getBit(data[i / 8], 7 - (i % 8));
                    i++;
                }
            }
        }
    }
}

void QrCode::applyMask(int msk) {
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            if (isFunction[y][x]) continue;
            bool invert;
            switch (msk) {
                case 0: invert = (x + y) % 2 == 0; break;
                case 1: invert = y % 2 == 0; break;
                case 2: invert = x % 3 == 0; break;
                case 3: invert = (x + y) % 3 == 0; break;
                case 4: invert = (x / 3 + y / 2) % 2 == 0; break;
                case 5: invert = (x * y) % 2 + (x * y) % 3 == 0; break;
                case 6: invert = ((x * y) % 2 + (x * y) % 3) % 2 == 0; break;
                case 7: invert = ((x + y) % 2 + (x * y) % 3) % 2 == 0; break;
                default: invert = false; break;
            }
            modules[y][x] = modules[y][x] ^ invert;
        }
    }
}

long QrCode::getPenaltyScore() const {
    long result = 0;
    for (int y = 0; y < size; y++) {
        bool runColor = false;
        int runX = 0;
        for (int x = 0; x < size; x++) {
            if (modules[y][x] == runColor) {
                runX++;
                if (runX == 5) result += 3;
                else if (runX > 5) result++;
            } else {
                runColor = modules[y][x];
                runX = 1;
            }
        }
    }
    for (int x = 0; x < size; x++) {
        bool runColor = false;
        int runY = 0;
        for (int y = 0; y < size; y++) {
            if (modules[y][x] == runColor) {
                runY++;
                if (runY == 5) result += 3;
                else if (runY > 5) result++;
            } else {
                runColor = modules[y][x];
                runY = 1;
            }
        }
    }
    return result;
}

bool QrCode::getBit(int x, int i) {
    return ((x >> i) & 1) != 0;
}

// Helper de conversion directe vers QImage avec marge de contraste blanche
QImage QrCode::toQImage(int scale, int border) const {
    if (scale <= 0 || border < 0) return QImage();
    int imgSize = (size + border * 2) * scale;
    QImage image(imgSize, imgSize, QImage::Format_RGB32);
    image.fill(Qt::white);

    QPainter painter(&image);
    painter.setPen(Qt::NoPen);
    painter.setBrush(Qt::black);

    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            if (getModule(x, y)) {
                painter.drawRect((x + border) * scale, (y + border) * scale, scale, scale);
            }
        }
    }
    return image;
}

QImage QrCode::generateQrImage(const QString &text, int targetSizePx, int border, Ecc ecl) {
    if (text.isEmpty()) return QImage();
    try {
        QrCode qr = encodeText(text.toUtf8().constData(), ecl);
        int totalModules = qr.getSize() + border * 2;
        int scale = std::max(4, targetSizePx / totalModules);
        return qr.toQImage(scale, border);
    } catch (...) {
        return QImage();
    }
}

} // namespace qrcodegen
