// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com
/*
* Copyright 2006-2007 Jeremias Maerki.
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
#include "include/BC_Encoder.h"
#include "include/BC_Dimension.h"
#include "include/BC_SymbolShapeHint.h"
#include "include/BC_SymbolInfo.h"
#include "include/BC_EncoderContext.h"
#include "include/BC_HighLevelEncoder.h"
#include "include/BC_ASCIIEncoder.h"
CBC_ASCIIEncoder::CBC_ASCIIEncoder()
{
}
CBC_ASCIIEncoder::~CBC_ASCIIEncoder()
{
}
FX_INT32 CBC_ASCIIEncoder::getEncodingMode()
{
    return ASCII_ENCODATION;
}
void CBC_ASCIIEncoder::Encode(CBC_EncoderContext &context, FX_INT32 &e)
{
    FX_INT32 n = CBC_HighLevelEncoder::determineConsecutiveDigitCount(context.m_msg, context.m_pos);
    if (n >= 2) {
        FX_WCHAR code = encodeASCIIDigits(context.m_msg.GetAt(context.m_pos), context.m_msg.GetAt(context.m_pos + 1), e);
        if (e != BCExceptionNO) {
            return;
        }
        context.writeCodeword(code);
        context.m_pos += 2;
    } else {
        FX_WCHAR c = context.getCurrentChar();
        FX_INT32 newMode = CBC_HighLevelEncoder::lookAheadTest(context.m_msg, context.m_pos, getEncodingMode());
        if (newMode != getEncodingMode()) {
            switch (newMode) {
                case BASE256_ENCODATION:
                    context.writeCodeword(CBC_HighLevelEncoder::LATCH_TO_BASE256);
                    context.signalEncoderChange(BASE256_ENCODATION);
                    return;
                case C40_ENCODATION:
                    context.writeCodeword(CBC_HighLevelEncoder::LATCH_TO_C40);
                    context.signalEncoderChange(C40_ENCODATION);
                    return;
                case X12_ENCODATION:
                    context.writeCodeword(CBC_HighLevelEncoder::LATCH_TO_ANSIX12);
                    context.signalEncoderChange(X12_ENCODATION);
                    break;
                case TEXT_ENCODATION:
                    context.writeCodeword(CBC_HighLevelEncoder::LATCH_TO_TEXT);
                    context.signalEncoderChange(TEXT_ENCODATION);
                    break;
                case EDIFACT_ENCODATION:
                    context.writeCodeword(CBC_HighLevelEncoder::LATCH_TO_EDIFACT);
                    context.signalEncoderChange(EDIFACT_ENCODATION);
                    break;
                default:
                    e = BCExceptionIllegalStateIllegalMode;
                    return;
            }
        } else if (CBC_HighLevelEncoder::isExtendedASCII(c)) {
            context.writeCodeword(CBC_HighLevelEncoder::UPPER_SHIFT);
            context.writeCodeword((FX_WCHAR) (c - 128 + 1));
            context.m_pos++;
        } else {
            context.writeCodeword((FX_WCHAR) (c + 1));
            context.m_pos++;
        }
    }
}
FX_WCHAR CBC_ASCIIEncoder::encodeASCIIDigits(FX_WCHAR digit1, FX_WCHAR digit2, FX_INT32 &e)
{
    if (CBC_HighLevelEncoder::isDigit(digit1) && CBC_HighLevelEncoder::isDigit(digit2)) {
        FX_INT32 num = (digit1 - 48) * 10 + (digit2 - 48);
        FX_WCHAR a = (FX_WCHAR) (num + 130);
        return (FX_WCHAR) (num + 130);
    }
    e = BCExceptionIllegalArgumentNotGigits;
    return 0;
}
