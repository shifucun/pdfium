// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com
/*
 * Copyright 2009 ZXing authors
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
#include "include/BC_UtilRSS.h"
CBC_UtilRSS::CBC_UtilRSS()
{
}
CBC_UtilRSS::~CBC_UtilRSS()
{
}
CFX_Int32Array *CBC_UtilRSS::GetRssWidths(FX_INT32 val, FX_INT32 n, FX_INT32 elements, FX_INT32 maxWidth, FX_BOOL noNarrow)
{
    CFX_Int32Array *iTemp =  FX_NEW CFX_Int32Array;
    iTemp->SetSize(elements);
    CBC_AutoPtr<CFX_Int32Array > widths(iTemp);
    FX_INT32 bar;
    FX_INT32 narrowMask = 0;
    for (bar = 0; bar < elements - 1; bar++) {
        narrowMask |= (1 << bar);
        FX_INT32 elmWidth = 1;
        FX_INT32 subVal;
        while (TRUE) {
            subVal = Combins(n - elmWidth - 1, elements - bar - 2);
            if (noNarrow && (narrowMask == 0) &&
                    (n - elmWidth - (elements - bar - 1) >= elements - bar - 1)) {
                subVal -= Combins(n - elmWidth - (elements - bar), elements - bar - 2);
            }
            if (elements - bar - 1 > 1) {
                FX_INT32 lessVal = 0;
                for (FX_INT32 mxwElement = n - elmWidth - (elements - bar - 2);
                        mxwElement > maxWidth;
                        mxwElement--) {
                    lessVal += Combins(n - elmWidth - mxwElement - 1, elements - bar - 3);
                }
                subVal -= lessVal * (elements - 1 - bar);
            } else if (n - elmWidth > maxWidth) {
                subVal--;
            }
            val -= subVal;
            if (val < 0) {
                break;
            }
            elmWidth++;
            narrowMask &= ~(1 << bar);
        }
        val += subVal;
        n -= elmWidth;
        (*widths)[bar] = elmWidth;
    }
    (*widths)[bar] = n;
    return widths.release();
}
FX_INT32 CBC_UtilRSS::GetRSSvalue(CFX_Int32Array &widths, FX_INT32 maxWidth, FX_BOOL noNarrow)
{
    FX_INT32 elements = widths.GetSize();
    FX_INT32 n = 0;
    for (FX_INT32 i = 0; i < elements; i++) {
        n += widths[i];
    }
    FX_INT32 val = 0;
    FX_INT32 narrowMask = 0;
    for (FX_INT32 bar = 0; bar < elements - 1; bar++) {
        FX_INT32 elmWidth;
        for (elmWidth = 1, narrowMask |= (1 << bar);
                elmWidth < widths[bar];
                elmWidth++, narrowMask &= ~(1 << bar)) {
            FX_INT32 subVal = Combins(n - elmWidth - 1, elements - bar - 2);
            if (noNarrow && (narrowMask == 0) &&
                    (n - elmWidth - (elements - bar - 1) >= elements - bar - 1)) {
                subVal -= Combins(n - elmWidth - (elements - bar),
                                  elements - bar - 2);
            }
            if (elements - bar - 1 > 1) {
                FX_INT32 lessVal = 0;
                for (FX_INT32 mxwElement = n - elmWidth - (elements - bar - 2);
                        mxwElement > maxWidth; mxwElement--) {
                    lessVal += Combins(n - elmWidth - mxwElement - 1,
                                       elements - bar - 3);
                }
                subVal -= lessVal * (elements - 1 - bar);
            } else if (n - elmWidth > maxWidth) {
                subVal--;
            }
            val += subVal;
        }
        n -= elmWidth;
    }
    return val;
}
FX_INT32 CBC_UtilRSS::Combins(FX_INT32 n, FX_INT32 r)
{
    FX_INT32 maxDenom;
    FX_INT32 minDenom;
    if (n - r > r) {
        minDenom = r;
        maxDenom = n - r;
    } else {
        minDenom = n - r;
        maxDenom = r;
    }
    FX_INT32 val = 1;
    FX_INT32 j = 1;
    for (FX_INT32 i = n; i > maxDenom; i--) {
        val *= i;
        if (j <= minDenom) {
            val /= j;
            j++;
        }
    }
    while (j <= minDenom) {
        val /= j;
        j++;
    }
    return val;
}
CFX_Int32Array *CBC_UtilRSS::Elements(CFX_Int32Array &eDist, FX_INT32 N, FX_INT32 K)
{
    CFX_Int32Array *widths = FX_NEW CFX_Int32Array;
    widths->SetSize(eDist.GetSize() + 2);
    FX_INT32 twoK = K << 1;
    (*widths)[0] = 1;
    FX_INT32 i;
    FX_INT32 minEven = 10;
    FX_INT32 barSum = 1;
    for (i = 1; i < twoK - 2; i += 2) {
        (*widths)[i] = eDist[i - 1] - (*widths)[i - 1];
        (*widths)[i + 1] = eDist[i] - (*widths)[i];
        barSum += (*widths)[i] + (*widths)[i + 1];
        if ((*widths)[i] < minEven) {
            minEven = (*widths)[i];
        }
    }
    (*widths)[twoK - 1] = N - barSum;
    if ((*widths)[twoK - 1] < minEven) {
        minEven = (*widths)[twoK - 1];
    }
    if (minEven > 1) {
        for (i = 0; i < twoK; i += 2) {
            (*widths)[i] += minEven - 1;
            (*widths)[i + 1] -= minEven - 1;
        }
    }
    return widths;
}
