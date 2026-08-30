#ifndef QRCODE_H
#define QRCODE_H

#include <array>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>
#include <QImage>
#include <QString>

namespace qrcodegen {

class QrSegment final {
public:
    enum class Mode {
        NUMERIC,
        ALPHANUMERIC,
        BYTE,
        KANJI,
        ECI
    };

    static QrSegment makeBytes(const std::vector<uint8_t> &data);
    static QrSegment makeNumeric(const char *digits);
    static QrSegment makeAlphanumeric(const char *text);
    static QrSegment makeSegments(const char *text);

    QrSegment(Mode md, int numCh, const std::vector<bool> &dt);
    QrSegment(Mode md, int numCh, std::vector<bool> &&dt);

    Mode getMode() const;
    int getNumChars() const;
    const std::vector<bool> &getData() const;

    static int getTotalBits(const std::vector<QrSegment> &segs, int version);

private:
    Mode mode;
    int numChars;
    std::vector<bool> data;
};

class QrCode final {
public:
    enum class Ecc {
        LOW = 0,
        MEDIUM,
        QUARTILE,
        HIGH
    };

    static constexpr int MIN_VERSION = 1;
    static constexpr int MAX_VERSION = 40;

    static QrCode encodeText(const char *text, Ecc ecl);
    static QrCode encodeBinary(const std::vector<uint8_t> &data, Ecc ecl);
    static QrCode encodeSegments(const std::vector<QrSegment> &segs, Ecc ecl,
        int minVersion = 1, int maxVersion = 40, int mask = -1, bool boostEcl = true);

    QrCode(int ver, Ecc ecl, const std::vector<uint8_t> &dataCodewords, int msk);

    int getVersion() const;
    int getSize() const;
    Ecc getErrorCorrectionLevel() const;
    int getMask() const;
    bool getModule(int x, int y) const;

    // Helper Qt pour générer un QImage haute résolution avec marge blanche
    QImage toQImage(int scale = 8, int border = 2) const;
    static QImage generateQrImage(const QString &text, int targetSizePx = 200, int border = 2, Ecc ecl = Ecc::MEDIUM);

private:
    int version;
    int size;
    Ecc errorCorrectionLevel;
    int mask;
    std::vector<std::vector<bool>> modules;
    std::vector<std::vector<bool>> isFunction;

    void drawFunctionPatterns();
    void drawFormatBits(int msk);
    void drawVersion();
    void drawFinderPattern(int x, int y);
    void drawAlignmentPattern(int x, int y);
    void setFunctionModule(int x, int y, bool isDark);
    void drawCodewords(const std::vector<uint8_t> &data);
    void applyMask(int msk);
    long getPenaltyScore() const;

    static bool getBit(int x, int i);
};

} // namespace qrcodegen

#endif // QRCODE_H
