// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com
/*
 * Copyright 2007 ZXing authors
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
#include "include/BC_CommonBitMatrix.h"
#include "include/BC_ResultPoint.h"
#include "include/BC_PDF417DetectorResult.h"
CBC_PDF417DetectorResult::CBC_PDF417DetectorResult(CBC_CommonBitMatrix* bits, CFX_PtrArray* points)
{
    m_bits = bits;
    m_points = points;
}
CBC_PDF417DetectorResult::~CBC_PDF417DetectorResult()
{
    for (FX_INT32 i = 0; i < m_points->GetSize(); i++) {
        CFX_PtrArray* temp = (CFX_PtrArray*)m_points->GetAt(i);
        for (FX_INT32 j = 0; j < temp->GetSize(); j++) {
            delete (CBC_ResultPoint*)temp->GetAt(j);
        }
        temp->RemoveAll();
        delete temp;
    }
    m_points->RemoveAll();
    delete m_points;
}
CBC_CommonBitMatrix* CBC_PDF417DetectorResult::getBits()
{
    return m_bits;
}

CFX_PtrArray* CBC_PDF417DetectorResult::getPoints()
{
    return m_points;
}
