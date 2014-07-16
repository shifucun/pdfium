// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
 
// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "../../../include/fxge/fx_ge.h"
#include "../skia/fx_skia.h"
#include "../skia/fx_skia_device.h"


#if _FX_OS_ == _FX_WIN32_DESKTOP_ || _FX_OS_ == _FX_WIN64_
FX_BOOL CFX_SkiaDeviceDriver::DrawDeviceText(int nChars, const FXTEXT_CHARPOS* pCharPos, CFX_Font* pFont,
    CFX_FontCache* pCache, const CFX_AffineMatrix* pObject2Device, FX_FLOAT font_size, FX_DWORD color,
    int alpha_flag, void* pIccTransform)
{
    return FALSE;
}
#endif