// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "barcode.h"
#include "include/BC_PDF417Compaction.h"
#include "include/BC_UtilCodingConvert.h"
#include "include/BC_PDF417HighLevelEncoder.h"
#define  SUBMODE_ALPHA  0
#define  SUBMODE_LOWER  1
#define  SUBMODE_MIXED  2
FX_INT32 CBC_PDF417HighLevelEncoder::TEXT_COMPACTION = 0;
FX_INT32 CBC_PDF417HighLevelEncoder::BYTE_COMPACTION = 1;
FX_INT32 CBC_PDF417HighLevelEncoder::NUMERIC_COMPACTION = 2;
FX_INT32 CBC_PDF417HighLevelEncoder::SUBMODE_PUNCTUATION = 3;
FX_INT32 CBC_PDF417HighLevelEncoder::LATCH_TO_TEXT = 900;
FX_INT32 CBC_PDF417HighLevelEncoder::LATCH_TO_BYTE_PADDED = 901;
FX_INT32 CBC_PDF417HighLevelEncoder::LATCH_TO_NUMERIC = 902;
FX_INT32 CBC_PDF417HighLevelEncoder::SHIFT_TO_BYTE = 913;
FX_INT32 CBC_PDF417HighLevelEncoder::LATCH_TO_BYTE = 924;
FX_BYTE CBC_PDF417HighLevelEncoder::TEXT_MIXED_RAW[] = {48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 38, 13, 9, 44, 58,
                                                        35, 45, 46, 36, 47, 43, 37, 42, 61, 94, 0, 32, 0, 0, 0
                                                       };
FX_BYTE CBC_PDF417HighLevelEncoder::TEXT_PUNCTUATION_RAW[] = {59, 60, 62, 64, 91, 92, 93, 95, 96, 126, 33, 13, 9, 44, 58,
                                                              10, 45, 46, 36, 47, 34, 124, 42, 40, 41, 63, 123, 125, 39, 0
                                                             };
FX_INT32 CBC_PDF417HighLevelEncoder::MIXED[128] = {0};
FX_INT32 CBC_PDF417HighLevelEncoder::PUNCTUATION[128] = {0};
void CBC_PDF417HighLevelEncoder::Initialize()
{
    Inverse();
}
void CBC_PDF417HighLevelEncoder::Finalize()
{
}
CBC_PDF417HighLevelEncoder::CBC_PDF417HighLevelEncoder()
{
}
CBC_PDF417HighLevelEncoder::~CBC_PDF417HighLevelEncoder()
{
}
CFX_WideString CBC_PDF417HighLevelEncoder::encodeHighLevel(CFX_WideString wideMsg, Compaction compaction, FX_INT32 &e)
{
    CFX_ByteString bytes;
    CBC_UtilCodingConvert::UnicodeToUTF8(wideMsg, bytes);
    CFX_WideString msg;
    FX_INT32 len = bytes.GetLength();
    for (FX_INT32 i = 0; i < len; i++) {
        FX_WCHAR ch =  (FX_WCHAR)(bytes.GetAt(i) & 0xff);
        if (ch == '?' && bytes.GetAt(i) != '?') {
            e = BCExceptionCharactersOutsideISO88591Encoding;
            return (FX_LPWSTR)"";
        }
        msg += ch;
    }
    CFX_ByteArray byteArr;
    for (FX_INT32 k = 0; k < bytes.GetLength(); k++) {
        byteArr.Add(bytes.GetAt(k));
    }
    CFX_WideString sb;
    len = msg.GetLength();
    FX_INT32 p = 0;
    FX_INT32 textSubMode = SUBMODE_ALPHA;
    if (compaction == TEXT) {
        encodeText(msg, p, len, sb, textSubMode);
    } else if (compaction == BYTES) {
        encodeBinary(&byteArr, p, byteArr.GetSize(), BYTE_COMPACTION, sb);
    } else if (compaction == NUMERIC) {
        sb += (FX_WCHAR) LATCH_TO_NUMERIC;
        encodeNumeric(msg, p, len, sb);
    } else {
        FX_INT32 encodingMode = LATCH_TO_TEXT;
        while (p < len) {
            FX_INT32 n = determineConsecutiveDigitCount(msg, p);
            if (n >= 13) {
                sb += (FX_WCHAR) LATCH_TO_NUMERIC;
                encodingMode = NUMERIC_COMPACTION;
                textSubMode = SUBMODE_ALPHA;
                encodeNumeric(msg, p, n, sb);
                p += n;
            } else {
                FX_INT32 t = determineConsecutiveTextCount(msg, p);
                if (t >= 5 || n == len) {
                    if (encodingMode != TEXT_COMPACTION) {
                        sb += (FX_WCHAR) LATCH_TO_TEXT;
                        encodingMode = TEXT_COMPACTION;
                        textSubMode = SUBMODE_ALPHA;
                    }
                    textSubMode = encodeText(msg, p, t, sb, textSubMode);
                    p += t;
                } else {
                    FX_INT32 b = determineConsecutiveBinaryCount(msg, &byteArr, p, e);
                    BC_EXCEPTION_CHECK_ReturnValue(e,  (FX_WCHAR)' ');
                    if (b == 0) {
                        b = 1;
                    }
                    if (b == 1 && encodingMode == TEXT_COMPACTION) {
                        encodeBinary(&byteArr, p, 1, TEXT_COMPACTION, sb);
                    } else {
                        encodeBinary(&byteArr, p, b, encodingMode, sb);
                        encodingMode = BYTE_COMPACTION;
                        textSubMode = SUBMODE_ALPHA;
                    }
                    p += b;
                }
            }
        }
    }
    return sb;
}
void CBC_PDF417HighLevelEncoder::Inverse()
{
    FX_BYTE i = 0;
    FX_INT32 l = 0;
    for (l = 0; l < sizeof(MIXED) / sizeof(MIXED[0]); l++) {
        MIXED[l] = -1;
    }
    for (i = 0; i < sizeof(TEXT_MIXED_RAW) / sizeof(TEXT_MIXED_RAW[0]); i++) {
        FX_BYTE b = TEXT_MIXED_RAW[i];
        if (b > 0) {
            MIXED[b] = i;
        }
    }
    for (l = 0; l < sizeof(PUNCTUATION) / sizeof(PUNCTUATION[0]); l++) {
        PUNCTUATION[l] = -1;
    }
    for (i = 0; i < sizeof(TEXT_PUNCTUATION_RAW) / sizeof(TEXT_PUNCTUATION_RAW[0]); i++) {
        FX_BYTE b = TEXT_PUNCTUATION_RAW[i];
        if (b > 0) {
            PUNCTUATION[b] = i;
        }
    }
}
CFX_ByteArray* CBC_PDF417HighLevelEncoder::getBytesForMessage(CFX_WideString msg)
{
    CFX_ByteString bytestring;
    CBC_UtilCodingConvert::UnicodeToUTF8(msg, bytestring);
    CFX_ByteArray* bytearray = FX_NEW CFX_ByteArray;
    for (FX_INT32 i = 0; i < bytestring.GetLength(); i++) {
        bytearray->Add(bytestring.GetAt(i));
    }
    return bytearray;
}
FX_INT32 CBC_PDF417HighLevelEncoder::encodeText(CFX_WideString msg, FX_INT32 startpos, FX_INT32 count, CFX_WideString &sb, FX_INT32 initialSubmode)
{
    CFX_WideString tmp;
    FX_INT32 submode = initialSubmode;
    FX_INT32 idx = 0;
    while (TRUE) {
        FX_WCHAR ch = msg.GetAt(startpos + idx);
        switch (submode) {
            case SUBMODE_ALPHA:
                if (isAlphaUpper(ch)) {
                    if (ch == ' ') {
                        tmp += (FX_WCHAR) 26;
                    } else {
                        tmp += (FX_WCHAR) (ch - 65);
                    }
                } else {
                    if (isAlphaLower(ch)) {
                        submode = SUBMODE_LOWER;
                        tmp += (FX_WCHAR) 27;
                        continue;
                    } else if (isMixed(ch)) {
                        submode = SUBMODE_MIXED;
                        tmp += (FX_WCHAR) 28;
                        continue;
                    } else {
                        tmp += (FX_WCHAR) 29;
                        tmp += PUNCTUATION[ch];
                        break;
                    }
                }
                break;
            case SUBMODE_LOWER:
                if (isAlphaLower(ch)) {
                    if (ch == ' ') {
                        tmp += (FX_WCHAR) 26;
                    } else {
                        tmp += (FX_WCHAR) (ch - 97);
                    }
                } else {
                    if (isAlphaUpper(ch)) {
                        tmp += (FX_WCHAR) 27;
                        tmp += (FX_WCHAR) (ch - 65);
                        break;
                    } else if (isMixed(ch)) {
                        submode = SUBMODE_MIXED;
                        tmp += (FX_WCHAR) 28;
                        continue;
                    } else {
                        tmp += (FX_WCHAR) 29;
                        tmp += PUNCTUATION[ch];
                        break;
                    }
                }
                break;
            case SUBMODE_MIXED:
                if (isMixed(ch)) {
                    FX_WCHAR a =  MIXED[ch];
                    FX_INT32 b = (FX_INT32)a;
                    tmp += MIXED[ch];
                } else {
                    if (isAlphaUpper(ch)) {
                        submode = SUBMODE_ALPHA;
                        tmp += (FX_WCHAR) 28;
                        continue;
                    } else if (isAlphaLower(ch)) {
                        submode = SUBMODE_LOWER;
                        tmp += (FX_WCHAR) 27;
                        continue;
                    } else {
                        if (startpos + idx + 1 < count) {
                            FX_WCHAR next = msg.GetAt(startpos + idx + 1);
                            if (isPunctuation(next)) {
                                submode = SUBMODE_PUNCTUATION;
                                tmp += (FX_WCHAR) 25;
                                continue;
                            }
                        }
                        tmp += (FX_WCHAR) 29;
                        tmp += PUNCTUATION[ch];
                    }
                }
                break;
            default:
                if (isPunctuation(ch)) {
                    tmp += PUNCTUATION[ch];
                } else {
                    submode = SUBMODE_ALPHA;
                    tmp += (FX_WCHAR) 29;
                    continue;
                }
        }
        idx++;
        if (idx >= count) {
            break;
        }
    }
    FX_WCHAR h = 0;
    FX_INT32 len = tmp.GetLength();
    for (FX_INT32 i = 0; i < len; i++) {
        FX_BOOL odd = (i % 2) != 0;
        if (odd) {
            h = (FX_WCHAR) ((h * 30) + tmp.GetAt(i));
            sb += h;
        } else {
            h = tmp.GetAt(i);
        }
    }
    if ((len % 2) != 0) {
        sb += (FX_WCHAR) ((h * 30) + 29);
    }
    return submode;
}
void CBC_PDF417HighLevelEncoder::encodeBinary(CFX_ByteArray* bytes, FX_INT32 startpos, FX_INT32 count, FX_INT32 startmode, CFX_WideString &sb)
{
    if (count == 1 && startmode == TEXT_COMPACTION) {
        sb += (FX_WCHAR) SHIFT_TO_BYTE;
    }
    FX_INT32 idx = startpos;
    FX_INT32 i = 0;
    if (count >= 6) {
        sb += (FX_WCHAR) LATCH_TO_BYTE;
        FX_WCHAR chars[5];
        while ((startpos + count - idx) >= 6) {
            FX_INT64 t = 0;
            for (i = 0; i < 6; i++) {
                t <<= 8;
                t += bytes->GetAt(idx + i) & 0xff;
            }
            for (i = 0; i < 5; i++) {
                chars[i] = (FX_WCHAR) (t % 900);
                t /= 900;
            }
            for (i = 4; i >= 0; i--) {
                sb += (chars[i]);
            }
            idx += 6;
        }
    }
    if (idx < startpos + count) {
        sb += (FX_WCHAR) LATCH_TO_BYTE_PADDED;
    }
    for (i = idx; i < startpos + count; i++) {
        FX_INT32 ch = bytes->GetAt(i) & 0xff;
        sb += (FX_WCHAR) ch;
    }
}
void CBC_PDF417HighLevelEncoder::encodeNumeric(CFX_WideString msg, FX_INT32 startpos, FX_INT32 count, CFX_WideString &sb)
{
    FX_INT32 idx = 0;
    CFX_WideString tmp;
    BigInteger num900 = 900;
    BigInteger num0 = 0;
    while (idx < count - 1) {
        FX_INT32 len = 44 < count - idx ? 44 : count - idx;
        CFX_ByteString part = ((FX_WCHAR)'1' + msg.Mid(startpos + idx, len)).UTF8Encode();
        BigInteger bigint = stringToBigInteger(part.c_str());
        do {
            BigInteger c = bigint % num900;
            tmp += (FX_WCHAR)(std::stoi(bigIntegerToString(c)));
            bigint = bigint / num900;
        } while (bigint != num0);
        for (FX_INT32 i = tmp.GetLength() - 1; i >= 0; i--) {
            sb += tmp.GetAt(i);
        }
        idx += len;
    }
}
FX_BOOL CBC_PDF417HighLevelEncoder::isDigit(FX_WCHAR ch)
{
    return ch >= '0' && ch <= '9';
}
FX_BOOL CBC_PDF417HighLevelEncoder::isAlphaUpper(FX_WCHAR ch)
{
    return ch == ' ' || (ch >= 'A' && ch <= 'Z');
}
FX_BOOL CBC_PDF417HighLevelEncoder::isAlphaLower(FX_WCHAR ch)
{
    return ch == ' ' || (ch >= 'a' && ch <= 'z');
}
FX_BOOL CBC_PDF417HighLevelEncoder::isMixed(FX_WCHAR ch)
{
    return MIXED[ch] != -1;
}
FX_BOOL CBC_PDF417HighLevelEncoder::isPunctuation(FX_WCHAR ch)
{
    return PUNCTUATION[ch] != -1;
}
FX_BOOL CBC_PDF417HighLevelEncoder::isText(FX_WCHAR ch)
{
    return ch == '\t' || ch == '\n' || ch == '\r' || (ch >= 32 && ch <= 126);
}
FX_INT32 CBC_PDF417HighLevelEncoder::determineConsecutiveDigitCount(CFX_WideString msg, FX_INT32 startpos)
{
    FX_INT32 count = 0;
    FX_INT32 len = msg.GetLength();
    FX_INT32 idx = startpos;
    if (idx < len) {
        FX_WCHAR ch = msg.GetAt(idx);
        while (isDigit(ch) && idx < len) {
            count++;
            idx++;
            if (idx < len) {
                ch = msg.GetAt(idx);
            }
        }
    }
    return count;
}
FX_INT32 CBC_PDF417HighLevelEncoder::determineConsecutiveTextCount(CFX_WideString msg, FX_INT32 startpos)
{
    FX_INT32 len = msg.GetLength();
    FX_INT32 idx = startpos;
    while (idx < len) {
        FX_WCHAR ch = msg.GetAt(idx);
        FX_INT32 numericCount = 0;
        while (numericCount < 13 && isDigit(ch) && idx < len) {
            numericCount++;
            idx++;
            if (idx < len) {
                ch = msg.GetAt(idx);
            }
        }
        if (numericCount >= 13) {
            return idx - startpos - numericCount;
        }
        if (numericCount > 0) {
            continue;
        }
        ch = msg.GetAt(idx);
        if (!isText(ch)) {
            break;
        }
        idx++;
    }
    return idx - startpos;
}
FX_INT32 CBC_PDF417HighLevelEncoder::determineConsecutiveBinaryCount(CFX_WideString msg, CFX_ByteArray* bytes, FX_INT32 startpos, FX_INT32 &e)
{
    FX_INT32 len = msg.GetLength();
    FX_INT32 idx = startpos;
    while (idx < len) {
        FX_WCHAR ch = msg.GetAt(idx);
        FX_INT32 numericCount = 0;
        while (numericCount < 13 && isDigit(ch)) {
            numericCount++;
            FX_INT32 i = idx + numericCount;
            if (i >= len) {
                break;
            }
            ch = msg.GetAt(i);
        }
        if (numericCount >= 13) {
            return idx - startpos;
        }
        FX_INT32 textCount = 0;
        while (textCount < 5 && isText(ch)) {
            textCount++;
            FX_INT32 i = idx + textCount;
            if (i >= len) {
                break;
            }
            ch = msg.GetAt(i);
        }
        if (textCount >= 5) {
            return idx - startpos;
        }
        ch = msg.GetAt(idx);
        if (bytes->GetAt(idx) == 63 && ch != '?') {
            e = BCExceptionNonEncodableCharacterDetected;
            return -1;
        }
        idx++;
    }
    return idx - startpos;
}
