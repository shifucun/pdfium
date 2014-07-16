// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
 
// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "../../../include/fxcrt/fx_ext.h"
#include "../../../include/fxge/fx_ge.h"
#if _FXM_PLATFORM_  == _FXM_PLATFORM_APPLE_
#include "../apple/apple_int.h"
#include "../../../include/fxge/fx_ge_apple.h"
#include "../ge/text_int.h"
#include "../dib/dib_int.h"
#include "../../../include/fxge/fx_freetype.h"

#if (_FXM_PLATFORM_  == _FXM_PLATFORM_APPLE_ && (!defined(_FPDFAPI_MINI_)) && defined(_SKIA_SUPPORT_))

void CFX_FaceCache::InitPlatform() {}
void CFX_FaceCache::DestroyPlatform() {}
CFX_GlyphBitmap* CFX_FaceCache::RenderGlyph_Nativetext(CFX_Font *				pFont,
        FX_DWORD					glyph_index,
        const CFX_AffineMatrix *	pMatrix,
        int						dest_width,
        int						anti_alias)
{
    return NULL;
}
static FX_BOOL _CGDrawGlyphRun(CGContextRef               pContext,
                               int                        nChars,
                               const FXTEXT_CHARPOS*      pCharPos,
                               CFX_Font*                  pFont,
                               CFX_FontCache*             pCache,
                               const CFX_AffineMatrix*    pObject2Device,
                               FX_FLOAT                   font_size,
                               FX_DWORD                   argb,
                               int                        alpha_flag,
                               void*                      pIccTransform)
{
    if (nChars == 0) {
        return TRUE;
    }
    CFX_AffineMatrix new_matrix;
    FX_BOOL bNegSize = font_size < 0;
    if (bNegSize) {
        font_size = -font_size;
    }
    FX_FLOAT ori_x = pCharPos[0].m_OriginX, ori_y = pCharPos[0].m_OriginY;
    new_matrix.Transform(ori_x, ori_y);
    if (pObject2Device) {
        new_matrix.Concat(*pObject2Device);
    }
    CQuartz2D& quartz2d = ((CApplePlatform *) CFX_GEModule::Get()->GetPlatformData())->_quartz2d;
    if (!pFont->m_pPlatformFont) {
        if (pFont->GetPsName() == CFX_WideString::FromLocal("DFHeiStd-W5")) {
            return FALSE;
        }
        pFont->m_pPlatformFont = quartz2d.CreateFont(pFont->m_pFontData, pFont->m_dwSize);
        if (NULL == pFont->m_pPlatformFont) {
            return FALSE;
        }
    }
    CFX_FixedBufGrow<FX_WORD, 32> glyph_indices(nChars);
    CFX_FixedBufGrow<CGPoint, 32> glyph_positions(nChars);
    for (int i = 0; i < nChars; i++ ) {
        glyph_indices[i] = pCharPos[i].m_ExtGID;
        if (bNegSize) {
            glyph_positions[i].x = -pCharPos[i].m_OriginX;
        } else {
            glyph_positions[i].x = pCharPos[i].m_OriginX;
        }
        glyph_positions[i].y = pCharPos[i].m_OriginY;
    }
    if (bNegSize) {
        new_matrix.a = -new_matrix.a;
    } else {
        new_matrix.b = -new_matrix.b;
        new_matrix.d = -new_matrix.d;
    }
    quartz2d.setGraphicsTextMatrix(pContext, &new_matrix);
    return quartz2d.drawGraphicsString(pContext,
                                       pFont->m_pPlatformFont,
                                       font_size,
                                       glyph_indices,
                                       glyph_positions,
                                       nChars,
                                       argb,
                                       NULL);
}
static void _DoNothing(void *info, const void *data, size_t size) {}

void CFX_Font::ReleasePlatformResource()
{
    if (m_pPlatformFont) {
        CQuartz2D & quartz2d = ((CApplePlatform *) CFX_GEModule::Get()->GetPlatformData())->_quartz2d;
        quartz2d.DestroyFont(m_pPlatformFont);
        m_pPlatformFont = NULL;
    }
}
#endif
#endif
