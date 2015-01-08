// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com
/*
* Copyright 2008 ZXing authors
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#include "barcode.h"
#include "include/BC_DataMatrixDecodedBitStreamParser.h"
#include "include/BC_CommonDecoderResult.h"
#include "include/BC_CommonBitSource.h"
const FX_CHAR CBC_DataMatrixDecodedBitStreamParser::C40_BASIC_SET_CHARS[] = {
    '*', '*', '*', ' ', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
    'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z'
};
const FX_CHAR CBC_DataMatrixDecodedBitStreamParser::C40_SHIFT2_SET_CHARS[] = {
    '!', '"', '#', '$', '%', '&', '\'', '(', ')', '*',  '+', ',', '-', '.',
    '/', ':', ';', '<', '=', '>', '?',  '@', '[', '\\', ']', '^', '_'
};
const FX_CHAR CBC_DataMatrixDecodedBitStreamParser::TEXT_BASIC_SET_CHARS[] = {
    '*', '*', '*', ' ', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
    'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z'
};
const FX_CHAR CBC_DataMatrixDecodedBitStreamParser::TEXT_SHIFT3_SET_CHARS[] = {
    '\'', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
    'O',  'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '{', '|', '}', '~', (FX_CHAR) 127
};
const FX_INT32 CBC_DataMatrixDecodedBitStreamParser::PAD_ENCODE = 0;
const FX_INT32 CBC_DataMatrixDecodedBitStreamParser::ASCII_ENCODE = 1;
const FX_INT32 CBC_DataMatrixDecodedBitStreamParser::C40_ENCODE = 2;
const FX_INT32 CBC_DataMatrixDecodedBitStreamParser::TEXT_ENCODE = 3;
const FX_INT32 CBC_DataMatrixDecodedBitStreamParser::ANSIX12_ENCODE = 4;
const FX_INT32 CBC_DataMatrixDecodedBitStreamParser::EDIFACT_ENCODE = 5;
const FX_INT32 CBC_DataMatrixDecodedBitStreamParser::BASE256_ENCODE = 6;
CBC_DataMatrixDecodedBitStreamParser::CBC_DataMatrixDecodedBitStreamParser()
{
}
CBC_DataMatrixDecodedBitStreamParser::~CBC_DataMatrixDecodedBitStreamParser()
{
}
CBC_CommonDecoderResult *CBC_DataMatrixDecodedBitStreamParser::Decode(CFX_ByteArray &bytes, FX_INT32 &e)
{
    CBC_CommonBitSource bits(&bytes);
    CFX_ByteString result;
    CFX_ByteString resultTrailer;
    CFX_Int32Array byteSegments;
    FX_INT32 mode = ASCII_ENCODE;
    do {
        if (mode == 1) {
            mode = DecodeAsciiSegment(&bits, result, resultTrailer, e);
            BC_EXCEPTION_CHECK_ReturnValue(e, NULL);
        } else {
            switch (mode) {
                case 2:
                    DecodeC40Segment(&bits, result, e);
                    BC_EXCEPTION_CHECK_ReturnValue(e, NULL);
                    break;
                case 3:
                    DecodeTextSegment(&bits, result, e);
                    BC_EXCEPTION_CHECK_ReturnValue(e, NULL);
                    break;
                case 4:
                    DecodeAnsiX12Segment(&bits, result, e);
                    BC_EXCEPTION_CHECK_ReturnValue(e, NULL);
                    break;
                case 5:
                    DecodeEdifactSegment(&bits, result, e);
                    BC_EXCEPTION_CHECK_ReturnValue(e, NULL);
                    break;
                case 6:
                    DecodeBase256Segment(&bits, result, byteSegments, e);
                    BC_EXCEPTION_CHECK_ReturnValue(e, NULL);
                    break;
                default:
                    NULL;
                    e = BCExceptionFormatException;
                    return NULL;
            }
            mode = ASCII_ENCODE;
        }
    } while (mode != PAD_ENCODE && bits.Available() > 0);
    if (resultTrailer.GetLength() > 0) {
        result += resultTrailer;
    }
    CBC_CommonDecoderResult *tempCp =  FX_NEW CBC_CommonDecoderResult();
    tempCp->Init(bytes, result, (byteSegments.GetSize() <= 0) ? CFX_Int32Array() : byteSegments, NULL, e);
    BC_EXCEPTION_CHECK_ReturnValue(e, NULL);
    return tempCp;
}
FX_INT32 CBC_DataMatrixDecodedBitStreamParser::DecodeAsciiSegment(CBC_CommonBitSource *bits, CFX_ByteString &result, CFX_ByteString &resultTrailer, FX_INT32 &e)
{
    FX_CHAR buffer[128];
    FX_BOOL upperShift = FALSE;
    do {
        FX_INT32 oneByte = bits->ReadBits(8, e);
        BC_EXCEPTION_CHECK_ReturnValue(e, 0);
        if (oneByte == 0) {
            e = BCExceptionFormatException;
            return 0;
        } else if (oneByte <= 128) {
            oneByte = upperShift ? oneByte + 128 : oneByte;
            upperShift = FALSE;
            result += ((FX_CHAR) (oneByte - 1));
            return ASCII_ENCODE;
        } else if (oneByte == 129) {
            return PAD_ENCODE;
        } else if (oneByte <= 229) {
            FX_INT32 value = oneByte - 130;
#if defined(_FX_WINAPI_PARTITION_APP_)
            memset(buffer, 0, sizeof(FX_CHAR) * 128);
            _itoa_s(value, buffer, 128, 10);
#else
            FXSYS_itoa(value, buffer, 10);
#endif
            if (value < 10) {
                result += '0';
                buffer[1] = '\0';
            } else {
                buffer[2] = '\0';
            }
            result += buffer;
        } else if (oneByte == 230) {
            return C40_ENCODE;
        } else if (oneByte == 231) {
            return BASE256_ENCODE;
        } else if (oneByte == 232 || oneByte == 233 || oneByte == 234) {
        } else if (oneByte == 235) {
            upperShift = TRUE;
        } else if (oneByte == 236) {
            result += "[)>";
            result += 0x1E;
            result += "05";
            result += 0x1D;
            resultTrailer.Insert(0, 0x1E);
            resultTrailer.Insert(0 + 1, 0x04);
        } else if (oneByte == 237) {
            result += "[)>";
            result += 0x1E;
            result += "06";
            result += 0x1D;
            resultTrailer.Insert(0, 0x1E);
            resultTrailer.Insert(0 + 1, 0x04);
        } else if (oneByte == 238) {
            return ANSIX12_ENCODE;
        } else if (oneByte == 239) {
            return TEXT_ENCODE;
        } else if (oneByte == 240) {
            return EDIFACT_ENCODE;
        } else if (oneByte == 241) {
        } else if (oneByte >= 242) {
            if (oneByte == 254 && bits->Available() == 0) {
            } else {
                e = BCExceptionFormatException;
                return 0;
            }
        }
    } while (bits->Available() > 0);
    return ASCII_ENCODE;
}
void CBC_DataMatrixDecodedBitStreamParser::DecodeC40Segment(CBC_CommonBitSource *bits, CFX_ByteString &result, FX_INT32 &e)
{
    FX_BOOL upperShift = FALSE;
    CFX_Int32Array cValues;
    cValues.SetSize(3);
    do {
        if (bits->Available() == 8) {
            return;
        }
        FX_INT32 firstByte = bits->ReadBits(8, e);
        BC_EXCEPTION_CHECK_ReturnVoid(e);
        if (firstByte == 254) {
            return;
        }
        FX_INT32 tempp = bits->ReadBits(8, e);
        BC_EXCEPTION_CHECK_ReturnVoid(e);
        ParseTwoBytes(firstByte, tempp, cValues);
        FX_INT32 shift = 0;
        FX_INT32 i;
        for (i = 0; i < 3; i++) {
            FX_INT32 cValue = cValues[i];
            switch (shift) {
                case 0:
                    if (cValue < 3) {
                        shift = cValue + 1;
                    } else if (cValue < 27) {
                        FX_CHAR c40char = C40_BASIC_SET_CHARS[cValue];
                        if (upperShift) {
                            result += (FX_CHAR) (c40char + 128);
                            upperShift = FALSE;
                        } else {
                            result += c40char;
                        }
                    } else {
                        e = BCExceptionFormatException;
                        return ;
                    }
                    break;
                case 1:
                    if (upperShift) {
                        result += (FX_CHAR) (cValue + 128);
                        upperShift = FALSE;
                    } else {
                        result += cValue;
                    }
                    shift = 0;
                    break;
                case 2:
                    if (cValue < 27) {
                        FX_CHAR c40char = C40_SHIFT2_SET_CHARS[cValue];
                        if (upperShift) {
                            result += (FX_CHAR) (c40char + 128);
                            upperShift = FALSE;
                        } else {
                            result += c40char;
                        }
                    } else if (cValue == 27) {
                        e = BCExceptionFormatException;
                        return;
                    } else if (cValue == 30) {
                        upperShift = TRUE;
                    } else {
                        e = BCExceptionFormatException;
                        return;
                    }
                    shift = 0;
                    break;
                case 3:
                    if (upperShift) {
                        result += (FX_CHAR) (cValue + 224);
                        upperShift = FALSE;
                    } else {
                        result += (FX_CHAR) (cValue + 96);
                    }
                    shift = 0;
                    break;
                default:
                    break;
                    e = BCExceptionFormatException;
                    return;
            }
        }
    } while (bits->Available() > 0);
}
void CBC_DataMatrixDecodedBitStreamParser::DecodeTextSegment(CBC_CommonBitSource *bits, CFX_ByteString &result, FX_INT32 &e)
{
    FX_BOOL upperShift = FALSE;
    CFX_Int32Array cValues;
    cValues.SetSize(3);
    FX_INT32 shift = 0;
    do {
        if (bits->Available() == 8) {
            return;
        }
        FX_INT32 firstByte = bits->ReadBits(8, e);
        BC_EXCEPTION_CHECK_ReturnVoid(e);
        if (firstByte == 254) {
            return;
        }
        FX_INT32 inTp = bits->ReadBits(8, e);
        BC_EXCEPTION_CHECK_ReturnVoid(e);
        ParseTwoBytes(firstByte, inTp, cValues);
        for (FX_INT32 i = 0; i < 3; i++) {
            FX_INT32 cValue = cValues[i];
            switch (shift) {
                case 0:
                    if (cValue < 3) {
                        shift = cValue + 1;
                    } else if (cValue < 40) {
                        FX_CHAR textChar = TEXT_BASIC_SET_CHARS[cValue];
                        if (upperShift) {
                            result += (FX_CHAR) (textChar + 128);
                            upperShift = FALSE;
                        } else {
                            result += textChar;
                        }
                    } else {
                        e = BCExceptionFormatException;
                        return;
                    }
                    break;
                case 1:
                    if (upperShift) {
                        result += (FX_CHAR) (cValue + 128);
                        upperShift = FALSE;
                    } else {
                        result += cValue;
                    }
                    shift = 0;
                    break;
                case 2:
                    if (cValue < 27) {
                        FX_CHAR c40char = C40_SHIFT2_SET_CHARS[cValue];
                        if (upperShift) {
                            result += (FX_CHAR) (c40char + 128);
                            upperShift = FALSE;
                        } else {
                            result += c40char;
                        }
                    } else if (cValue == 27) {
                        e = BCExceptionFormatException;
                        return;
                    } else if (cValue == 30) {
                        upperShift = TRUE;
                    } else {
                        e = BCExceptionFormatException;
                        return;
                    }
                    shift = 0;
                    break;
                case 3:
                    if (cValue < 19) {
                        FX_CHAR textChar = TEXT_SHIFT3_SET_CHARS[cValue];
                        if (upperShift) {
                            result += (FX_CHAR) (textChar + 128);
                            upperShift = FALSE;
                        } else {
                            result += textChar;
                        }
                        shift = 0;
                    } else {
                        e = BCExceptionFormatException;
                        return;
                    }
                    break;
                default:
                    break;
                    e = BCExceptionFormatException;
                    return;
            }
        }
    } while (bits->Available() > 0);
}
void CBC_DataMatrixDecodedBitStreamParser::DecodeAnsiX12Segment(CBC_CommonBitSource *bits, CFX_ByteString &result, FX_INT32 &e)
{
    CFX_Int32Array cValues;
    cValues.SetSize(3);
    do {
        if (bits->Available() == 8) {
            return;
        }
        FX_INT32 firstByte = bits->ReadBits(8, e);
        BC_EXCEPTION_CHECK_ReturnVoid(e);
        if (firstByte == 254) {
            return;
        }
        FX_INT32 iTemp1 = bits->ReadBits(8, e);
        BC_EXCEPTION_CHECK_ReturnVoid(e);
        ParseTwoBytes(firstByte, iTemp1, cValues);
        FX_INT32 i;
        for (i = 0; i < 3; i++) {
            FX_INT32 cValue = cValues[i];
            if (cValue == 0) {
                BC_FX_ByteString_Append(result, 1, '\r');
            } else if (cValue == 1) {
                BC_FX_ByteString_Append(result, 1, '*');
            } else if (cValue == 2) {
                BC_FX_ByteString_Append(result, 1, '>');
            } else if (cValue == 3) {
                BC_FX_ByteString_Append(result, 1, ' ');
            } else if (cValue < 14) {
                BC_FX_ByteString_Append(result, 1, (FX_CHAR) (cValue + 44));
            } else if (cValue < 40) {
                BC_FX_ByteString_Append(result, 1, (FX_CHAR) (cValue + 51));
            } else {
                e = BCExceptionFormatException;
                return;
            }
        }
    } while (bits->Available() > 0);
}
void CBC_DataMatrixDecodedBitStreamParser::ParseTwoBytes(FX_INT32 firstByte, FX_INT32 secondByte, CFX_Int32Array &result)
{
    FX_INT32 fullBitValue = (firstByte << 8) + secondByte - 1;
    FX_INT32 temp = fullBitValue / 1600;
    result[0] = temp;
    fullBitValue -= temp * 1600;
    temp = fullBitValue / 40;
    result[1] = temp;
    result[2] = fullBitValue - temp * 40;
}
void CBC_DataMatrixDecodedBitStreamParser::DecodeEdifactSegment(CBC_CommonBitSource *bits, CFX_ByteString &result, FX_INT32 &e)
{
    FX_CHAR buffer[128];
    FX_BOOL unlatch = FALSE;
    do {
        if (bits->Available() <= 16) {
            return;
        }
        FX_INT32 i;
        for (i = 0; i < 4; i++) {
            FX_INT32 edifactValue = bits->ReadBits(6, e);
            BC_EXCEPTION_CHECK_ReturnVoid(e);
            if (edifactValue == 0x1F) {
                unlatch = TRUE;
            }
            if (!unlatch) {
                if ((edifactValue & 32) == 0) {
                    edifactValue |= 64;
                }
#if defined(_FX_WINAPI_PARTITION_APP_)
                memset(buffer, 0, sizeof(FX_CHAR) * 128);
                _itoa_s(edifactValue, buffer, 128, 10);
                result += buffer;
#else
                result += FXSYS_itoa(edifactValue, buffer, 10);
#endif
            }
        }
    } while (!unlatch && bits->Available() > 0);
}
void CBC_DataMatrixDecodedBitStreamParser::DecodeBase256Segment(CBC_CommonBitSource *bits, CFX_ByteString &result, CFX_Int32Array &byteSegments, FX_INT32 &e)
{
    FX_INT32 codewordPosition = 1 + bits->getByteOffset();
    FX_INT32 iTmp =  bits->ReadBits(8, e);
    BC_EXCEPTION_CHECK_ReturnVoid(e);
    FX_INT32 d1 = Unrandomize255State(iTmp, codewordPosition++);
    FX_INT32 count;
    if (d1 == 0) {
        count = bits->Available() / 8;
    } else if (d1 < 250) {
        count = d1;
    } else {
        FX_INT32 iTmp3 = bits->ReadBits(8, e);
        BC_EXCEPTION_CHECK_ReturnVoid(e);
        count = 250 * (d1 - 249) + Unrandomize255State(iTmp3, codewordPosition++);
    }
    if (count < 0) {
        e = BCExceptionFormatException;
        return;
    }
    CFX_ByteArray *bytes = FX_NEW CFX_ByteArray();
    bytes->SetSize(count);
    FX_INT32 i;
    for (i = 0; i < count; i++) {
        if (bits->Available() < 8) {
            e  = BCExceptionFormatException;
            delete bytes;
            return;
        }
        FX_INT32 iTemp5 = bits->ReadBits(8, e);
        if (e != BCExceptionNO) {
            delete bytes;
            return;
        }
        bytes->SetAt(i, Unrandomize255State(iTemp5, codewordPosition++));
    }
    BC_FX_ByteString_Append(result, *bytes);
    delete bytes;
}
FX_BYTE CBC_DataMatrixDecodedBitStreamParser::Unrandomize255State(FX_INT32 randomizedBase256Codeword, FX_INT32 base256CodewordPosition)
{
    FX_INT32 pseudoRandomNumber = ((149 * base256CodewordPosition) % 255) + 1;
    FX_INT32 tempVariable = randomizedBase256Codeword - pseudoRandomNumber;
    return (FX_BYTE) (tempVariable >= 0 ? tempVariable : tempVariable + 256);
}
