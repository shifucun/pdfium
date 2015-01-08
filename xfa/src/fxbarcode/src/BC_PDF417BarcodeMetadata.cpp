// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com
/*
* Copyright 2013 ZXing authors
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
#include "include/BC_PDF417BarcodeMetadata.h"
CBC_BarcodeMetadata::CBC_BarcodeMetadata(FX_INT32 columnCount, FX_INT32 rowCountUpperPart, FX_INT32 rowCountLowerPart, FX_INT32 errorCorrectionLevel)
{
    m_columnCount = columnCount;
    m_rowCountUpperPart = rowCountUpperPart;
    m_rowCountLowerPart = rowCountLowerPart;
    m_errorCorrectionLevel = errorCorrectionLevel;
    m_rowCount = m_rowCountUpperPart + m_rowCountLowerPart;
}
CBC_BarcodeMetadata::~CBC_BarcodeMetadata()
{
}
FX_INT32 CBC_BarcodeMetadata::getColumnCount()
{
    return m_columnCount;
}
FX_INT32 CBC_BarcodeMetadata::getErrorCorrectionLevel()
{
    return m_errorCorrectionLevel;
}
FX_INT32 CBC_BarcodeMetadata::getRowCount()
{
    return m_rowCount;
}
FX_INT32 CBC_BarcodeMetadata::getRowCountUpperPart()
{
    return m_rowCountUpperPart;
}
FX_INT32 CBC_BarcodeMetadata::getRowCountLowerPart()
{
    return m_rowCountLowerPart;
}
