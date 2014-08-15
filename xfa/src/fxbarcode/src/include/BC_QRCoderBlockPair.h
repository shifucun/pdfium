// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef _BC_QRCODERBLOCKPAIR_H_
#define _BC_QRCODERBLOCKPAIR_H_
class CBC_CommonByteArray;
class CBC_QRCoderBlockPair;
class CBC_QRCoderBlockPair : public CFX_Object
{
private:
    CBC_CommonByteArray* m_dataBytes;
    CBC_CommonByteArray* m_errorCorrectionBytes;
public:
    CBC_QRCoderBlockPair(CBC_CommonByteArray* data, CBC_CommonByteArray* errorCorrection);
    virtual ~CBC_QRCoderBlockPair();

    CBC_CommonByteArray* GetDataBytes();
    CBC_CommonByteArray* GetErrorCorrectionBytes();
};
#endif
