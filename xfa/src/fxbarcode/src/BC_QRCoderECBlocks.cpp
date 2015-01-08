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
#include "include/BC_QRCoderECB.h"
#include "include/BC_QRCoderECBlocks.h"
CBC_QRCoderECBlocks::CBC_QRCoderECBlocks(FX_INT32 ecCodeWordsPerBlock, CBC_QRCoderECB* ecBlocks)
{
    m_ecCodeWordsPerBlock = ecCodeWordsPerBlock;
    m_ecBlocks.Add(ecBlocks);
}
CBC_QRCoderECBlocks::CBC_QRCoderECBlocks(FX_INT32 ecCodeWordsPerBlock,
        CBC_QRCoderECB* ecBlocks1,
        CBC_QRCoderECB* ecBlocks2)
{
    m_ecCodeWordsPerBlock = ecCodeWordsPerBlock;
    m_ecBlocks.Add(ecBlocks1);
    m_ecBlocks.Add(ecBlocks2);
}
CBC_QRCoderECBlocks::~CBC_QRCoderECBlocks()
{
    for (FX_INT32 i = 0 ; i < m_ecBlocks.GetSize(); i++) {
        delete ( (CBC_QRCoderECB*)(m_ecBlocks[i]) ) ;
    }
    m_ecBlocks.RemoveAll();
}
FX_INT32 CBC_QRCoderECBlocks::GetECCodeWordsPerBlock()
{
    return m_ecCodeWordsPerBlock;
}
FX_INT32 CBC_QRCoderECBlocks::GetNumBlocks()
{
    FX_INT32 total = 0;
    for(FX_INT32 i = 0; i < m_ecBlocks.GetSize(); i++) {
        total += ( (CBC_QRCoderECB*)(m_ecBlocks[i]) )->GetCount();
    }
    return total;
}
FX_INT32 CBC_QRCoderECBlocks::GetTotalECCodeWords()
{
    return m_ecCodeWordsPerBlock * GetNumBlocks();
}
CFX_PtrArray* CBC_QRCoderECBlocks::GetECBlocks()
{
    return &m_ecBlocks;
}
