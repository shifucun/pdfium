// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "../../../include/fxge/fx_ge.h"
#include "../../../include/fxcodec/fx_codec.h"
#include "fx_skia.h"
#include "fx_skia_blitter.h"
#include "fx_skia_driver.h"
#if (_FXM_PLATFORM_  == _FXM_PLATFORM_APPLE_)
#include "../apple/apple_int.h"
#endif


CFX_SkiaDriver::CFX_SkiaDriver(CFX_DIBitmap* pBitmap, int dither_bits, FX_BOOL bRgbByteOrder, CFX_DIBitmap* pOriDevice, FX_BOOL bGroupKnockout)
{
    m_pBitmap = pBitmap;
    m_DitherBits = dither_bits;
    m_pClipRgn = NULL;
    m_pPlatformBitmap = NULL;
    m_pPlatformGraphics = NULL;
    m_pDwRenderTartget = NULL;
    m_bRgbByteOrder = bRgbByteOrder;
    m_pOriDevice = pOriDevice;
    m_bGroupKnockout = bGroupKnockout;
    m_FillFlags = 0;
}

CFX_SkiaDriver::~CFX_SkiaDriver()
{
    if (m_pClipRgn) {
        delete m_pClipRgn;
    }
    for (int i = 0; i < m_StateStack.GetSize(); i++)
    if (m_StateStack[i]) {
        delete (CFX_ClipRgn*)m_StateStack[i];
    }
}

void CFX_SkiaDriver::InitPlatform()
{
#if (_FXM_PLATFORM_  == _FXM_PLATFORM_APPLE_ && (!defined(_FPDFAPI_MINI_)))
    CQuartz2D & quartz2d = ((CApplePlatform *)CFX_GEModule::Get()->GetPlatformData())->_quartz2d;
    m_pPlatformGraphics = quartz2d.createGraphics(m_pBitmap);
#endif
}

void CFX_SkiaDriver::DestroyPlatform()
{
#if (_FXM_PLATFORM_  == _FXM_PLATFORM_APPLE_ && (!defined(_FPDFAPI_MINI_)))
    CQuartz2D & quartz2d = ((CApplePlatform *)CFX_GEModule::Get()->GetPlatformData())->_quartz2d;
    if (m_pPlatformGraphics) {
        quartz2d.destroyGraphics(m_pPlatformGraphics);
        m_pPlatformGraphics = NULL;
    }
#endif
}

int CFX_SkiaDriver::GetDeviceCaps(int caps_id)
{
    switch (caps_id) {
    case FXDC_DEVICE_CLASS:
        return FXDC_DISPLAY;
    case FXDC_PIXEL_WIDTH:
        return m_pBitmap->GetWidth();
    case FXDC_PIXEL_HEIGHT:
        return m_pBitmap->GetHeight();
    case FXDC_BITS_PIXEL:
        return m_pBitmap->GetBPP();
    case FXDC_HORZ_SIZE:
    case FXDC_VERT_SIZE:
        return 0;
    case FXDC_RENDER_CAPS: {
                               int flags = FXRC_GET_BITS | FXRC_ALPHA_PATH | FXRC_ALPHA_IMAGE | FXRC_BLEND_MODE | FXRC_SOFT_CLIP;
                               if (m_pBitmap->HasAlpha()) {
                                   flags |= FXRC_ALPHA_OUTPUT;
                               }
                               else if (m_pBitmap->IsAlphaMask()) {
                                   if (m_pBitmap->GetBPP() == 1) {
                                       flags |= FXRC_BITMASK_OUTPUT;
                                   }
                                   else {
                                       flags |= FXRC_BYTEMASK_OUTPUT;
                                   }
                               }
                               if (m_pBitmap->IsCmykImage()) {
                                   flags |= FXRC_CMYK_OUTPUT;
                               }
                               return flags;
    }
    case FXDC_DITHER_BITS:
        return m_DitherBits;
    }
    return 0;
}

void CFX_SkiaDriver::SaveState()
{
    void* pClip = NULL;
    if (m_pClipRgn) {
        pClip = FX_NEW CFX_ClipRgn(*m_pClipRgn);
        if (!pClip) {
            return;
        }
    }
    m_StateStack.Add(pClip);
}

void CFX_SkiaDriver::RestoreState(FX_BOOL bKeepSaved)
{
    if (m_StateStack.GetSize() == 0) {
        if (m_pClipRgn) {
            delete m_pClipRgn;
            m_pClipRgn = NULL;
        }
        return;
    }
    CFX_ClipRgn* pSavedClip = (CFX_ClipRgn*)m_StateStack[m_StateStack.GetSize() - 1];
    if (m_pClipRgn) {
        delete m_pClipRgn;
        m_pClipRgn = NULL;
    }
    if (bKeepSaved) {
        if (pSavedClip) {
            m_pClipRgn = FX_NEW CFX_ClipRgn(*pSavedClip);
        }
    }
    else {
        m_StateStack.RemoveAt(m_StateStack.GetSize() - 1);
        m_pClipRgn = pSavedClip;
    }
}

FX_BOOL CFX_SkiaDriver::SetClip_PathFill(const CFX_PathData* pPathData,
    const CFX_AffineMatrix* pObject2Device,
    int fill_mode)
{
    if (m_pClipRgn == NULL) {
        m_pClipRgn = FX_NEW CFX_ClipRgn(GetDeviceCaps(FXDC_PIXEL_WIDTH), GetDeviceCaps(FXDC_PIXEL_HEIGHT));
    }
    if (!m_pClipRgn) {
        return FALSE;
    }
    if (pPathData->GetPointCount() == 5 || pPathData->GetPointCount() == 4) {
        CFX_FloatRect rectf;
        if (pPathData->IsRect(pObject2Device, &rectf)) {
            rectf.Intersect(CFX_FloatRect(0, 0, (FX_FLOAT)GetDeviceCaps(FXDC_PIXEL_WIDTH), (FX_FLOAT)GetDeviceCaps(FXDC_PIXEL_HEIGHT)));
            FX_RECT rect = rectf.GetOutterRect();
            m_pClipRgn->IntersectRect(rect);
            return TRUE;
        }
    }
    CSkia_PathData path_data;
    path_data.BuildPath(pPathData, pObject2Device);
    path_data.m_PathData.close();
    path_data.m_PathData.setFillType((fill_mode & 3) == FXFILL_WINDING ? SkPath::kWinding_FillType : SkPath::kEvenOdd_FillType);
    SkPaint spaint;
    spaint.setColor(0xffffffff);
    spaint.setAntiAlias(TRUE);
    spaint.setStyle(SkPaint::kFill_Style);
    SetClipMask(path_data.m_PathData, &spaint);
    return TRUE;
}

FX_BOOL CFX_SkiaDriver::SetClip_PathStroke(const CFX_PathData* pPathData,
        const CFX_AffineMatrix* pObject2Device,
        const CFX_GraphStateData* pGraphState)
{
    if (m_pClipRgn == NULL) {
        m_pClipRgn = FX_NEW CFX_ClipRgn(GetDeviceCaps(FXDC_PIXEL_WIDTH), GetDeviceCaps(FXDC_PIXEL_HEIGHT));
    }
    if (!m_pClipRgn) {
        return FALSE;
    }
    CSkia_PathData path_data;
    path_data.BuildPath(pPathData, NULL);
    path_data.m_PathData.setFillType(SkPath::kWinding_FillType);
    SkPaint spaint;
    spaint.setColor(0xffffffff);
    spaint.setStyle(SkPaint::kStroke_Style);
    spaint.setAntiAlias(TRUE);
    SkPath dst_path;
    SkRasterizeStroke(spaint, &dst_path, path_data.m_PathData, pObject2Device, pGraphState, 1, FALSE, 0);
    spaint.setStyle(SkPaint::kFill_Style);
    SetClipMask(dst_path, &spaint);
    return TRUE;
}

FX_BOOL	CFX_SkiaDriver::DrawPath(const CFX_PathData* pPathData,
                                       const CFX_AffineMatrix* pObject2Device,
                                       const CFX_GraphStateData* pGraphState,
                                       FX_DWORD fill_color,
                                       FX_DWORD stroke_color,
                                       int fill_mode,
                                       int alpha_flag,
                                       void* pIccTransform,
                                       int blend_type
                                      )
{
    if (blend_type != FXDIB_BLEND_NORMAL) {
        return FALSE;
    }
    if (GetBuffer() == NULL) {
        return TRUE;
    }
    SkIRect rect;
    rect.set(0, 0, GetDeviceCaps(FXDC_PIXEL_WIDTH), GetDeviceCaps(FXDC_PIXEL_HEIGHT));
    if ((fill_mode & 3) && fill_color) {
        CSkia_PathData path_data;
        path_data.BuildPath(pPathData, pObject2Device);
        path_data.m_PathData.setFillType((fill_mode & 3) == FXFILL_WINDING ? SkPath::kWinding_FillType : SkPath::kEvenOdd_FillType);
        SkPaint spaint;
        spaint.setAntiAlias(TRUE);
        spaint.setStyle(SkPaint::kFill_Style);
        spaint.setColor(fill_color);
        if (!RenderRasterizerSkia(path_data.m_PathData, spaint, rect, fill_color, fill_mode & FXFILL_FULLCOVER, FALSE, alpha_flag, pIccTransform)) {
            return FALSE;
        }
    }
    int stroke_alpha = FXGETFLAG_COLORTYPE(alpha_flag) ? FXGETFLAG_ALPHA_STROKE(alpha_flag) : FXARGB_A(stroke_color);
    if (pGraphState && stroke_alpha) {
        CFX_AffineMatrix matrix1, matrix2;
        if (pObject2Device) {
            matrix1.a = FXSYS_fabs(pObject2Device->a) > FXSYS_fabs(pObject2Device->b) ?
                        FXSYS_fabs(pObject2Device->a) : FXSYS_fabs(pObject2Device->b);
            matrix1.d = matrix1.a;
            matrix2.Set(pObject2Device->a / matrix1.a, pObject2Device->b / matrix1.a,
                        pObject2Device->c / matrix1.d, pObject2Device->d / matrix1.d,
                        pObject2Device->e, pObject2Device->f);
        }
        CSkia_PathData path_data;
        path_data.BuildPath(pPathData, &matrix1);
        path_data.m_PathData.setFillType(SkPath::kWinding_FillType);
        SkPaint spaint;
        spaint.setColor(stroke_color);
        spaint.setStyle(SkPaint::kStroke_Style);
        spaint.setAntiAlias(TRUE);
        SkPath dst_path;
        SkRasterizeStroke(spaint, &dst_path, path_data.m_PathData, &matrix2, pGraphState, matrix1.a, FALSE, 0);
        spaint.setStyle(SkPaint::kFill_Style);
        int fill_flag = FXGETFLAG_COLORTYPE(alpha_flag) << 8 | FXGETFLAG_ALPHA_STROKE(alpha_flag);
        if (!RenderRasterizerSkia(dst_path, spaint, rect, stroke_color, fill_mode & FXFILL_FULLCOVER, FALSE, fill_flag, pIccTransform, FALSE)) {
            return FALSE;
        }
    }
    return TRUE;
}

FX_BOOL CFX_SkiaDriver::SetPixel(int x, int y, FX_DWORD color, int alpha_flag, void* pIccTransform)
{
    if (m_pBitmap->GetBuffer() == NULL) {
        return TRUE;
    }
    if (!CFX_GEModule::Get()->GetCodecModule() || !CFX_GEModule::Get()->GetCodecModule()->GetIccModule()) {
        pIccTransform = NULL;
    }
    if (m_pClipRgn == NULL) {
        if (m_bRgbByteOrder) {
            RgbByteOrderSetPixel(m_pBitmap, x, y, color);
        }
        else {
            return _DibSetPixel(m_pBitmap, x, y, color, alpha_flag, pIccTransform);
        }
    }
    else if (m_pClipRgn->GetBox().Contains(x, y)) {
        if (m_pClipRgn->GetType() == CFX_ClipRgn::RectI) {
            if (m_bRgbByteOrder) {
                RgbByteOrderSetPixel(m_pBitmap, x, y, color);
            }
            else {
                return _DibSetPixel(m_pBitmap, x, y, color, alpha_flag, pIccTransform);
            }
        }
        else if (m_pClipRgn->GetType() == CFX_ClipRgn::MaskF) {
            const CFX_DIBitmap* pMask = m_pClipRgn->GetMask();
            FX_BOOL bCMYK = FXGETFLAG_COLORTYPE(alpha_flag);
            int new_alpha = bCMYK ? FXGETFLAG_ALPHA_FILL(alpha_flag) : FXARGB_A(color);
            new_alpha = new_alpha * pMask->GetScanline(y)[x] / 255;
            if (m_bRgbByteOrder) {
                RgbByteOrderSetPixel(m_pBitmap, x, y, (color & 0xffffff) | (new_alpha << 24));
                return TRUE;
            }
            if (bCMYK) {
                FXSETFLAG_ALPHA_FILL(alpha_flag, new_alpha);
            }
            else {
                color = (color & 0xffffff) | (new_alpha << 24);
            }
            return _DibSetPixel(m_pBitmap, x, y, color, alpha_flag, pIccTransform);
        }
    }
    return TRUE;
}

FX_BOOL CFX_SkiaDriver::FillRect(const FX_RECT* pRect, FX_DWORD fill_color, int alpha_flag, void* pIccTransform, int blend_type)
{
    if (blend_type != FXDIB_BLEND_NORMAL) {
        return FALSE;
    }
    if (m_pBitmap->GetBuffer() == NULL) {
        return TRUE;
    }
    FX_RECT clip_rect;
    GetClipBox(&clip_rect);
    FX_RECT draw_rect = clip_rect;
    if (pRect) {
        draw_rect.Intersect(*pRect);
    }
    if (draw_rect.IsEmpty()) {
        return TRUE;
    }
    if (m_pClipRgn == NULL || m_pClipRgn->GetType() == CFX_ClipRgn::RectI) {
        if (m_bRgbByteOrder) {
            RgbByteOrderCompositeRect(m_pBitmap, draw_rect.left, draw_rect.top, draw_rect.Width(), draw_rect.Height(), fill_color);
        }
        else {
            m_pBitmap->CompositeRect(draw_rect.left, draw_rect.top, draw_rect.Width(), draw_rect.Height(), fill_color, alpha_flag, pIccTransform);
        }
        return TRUE;
    }
    m_pBitmap->CompositeMask(draw_rect.left, draw_rect.top, draw_rect.Width(), draw_rect.Height(), (const CFX_DIBitmap*)m_pClipRgn->GetMask(),
        fill_color, draw_rect.left - clip_rect.left, draw_rect.top - clip_rect.top, FXDIB_BLEND_NORMAL, NULL, m_bRgbByteOrder, alpha_flag, pIccTransform);
    return TRUE;
}

FX_BOOL CFX_SkiaDriver::GetClipBox(FX_RECT* pRect)
{
    if (m_pClipRgn == NULL) {
        pRect->left = pRect->top = 0;
        pRect->right = GetDeviceCaps(FXDC_PIXEL_WIDTH);
        pRect->bottom = GetDeviceCaps(FXDC_PIXEL_HEIGHT);
        return TRUE;
    }
    *pRect = m_pClipRgn->GetBox();
    return TRUE;
}

FX_BOOL	CFX_SkiaDriver::GetDIBits(CFX_DIBitmap* pBitmap, int left, int top, void* pIccTransform, FX_BOOL bDEdge)
{
    if (m_pBitmap->GetBuffer() == NULL) {
        return TRUE;
    }
    if (bDEdge) {
        if (m_bRgbByteOrder) {
            RgbByteOrderTransferBitmap(pBitmap, 0, 0, pBitmap->GetWidth(), pBitmap->GetHeight(), m_pBitmap, left, top);
        }
        else {
            return pBitmap->TransferBitmap(0, 0, pBitmap->GetWidth(), pBitmap->GetHeight(), m_pBitmap, left, top, pIccTransform);
        }
        return TRUE;
    }
    FX_RECT rect(left, top, left + pBitmap->GetWidth(), top + pBitmap->GetHeight());
    CFX_DIBitmap *pBack = NULL;
    if (m_pOriDevice) {
        pBack = m_pOriDevice->Clone(&rect);
        if (!pBack) {
            return TRUE;
        }
        pBack->CompositeBitmap(0, 0, pBack->GetWidth(), pBack->GetHeight(), m_pBitmap, 0, 0);
    }
    else {
        pBack = m_pBitmap->Clone(&rect);
    }
    if (!pBack) {
        return TRUE;
    }
    FX_BOOL bRet = TRUE;
    left = left >= 0 ? 0 : left;
    top = top >= 0 ? 0 : top;
    if (m_bRgbByteOrder) {
        RgbByteOrderTransferBitmap(pBitmap, 0, 0, rect.Width(), rect.Height(), pBack, left, top);
    }
    else {
        bRet = pBitmap->TransferBitmap(0, 0, rect.Width(), rect.Height(), pBack, left, top, pIccTransform);
    }
    delete pBack;
    return bRet;
}

FX_BOOL	CFX_SkiaDriver::SetDIBits(const CFX_DIBSource* pBitmap, FX_DWORD argb, const FX_RECT* pSrcRect, int left, int top, int blend_type,
    int alpha_flag, void* pIccTransform)
{
    if (m_pBitmap->GetBuffer() == NULL) {
        return TRUE;
    }
    if (pBitmap->IsAlphaMask())
        return m_pBitmap->CompositeMask(left, top, pSrcRect->Width(), pSrcRect->Height(), pBitmap, argb,
        pSrcRect->left, pSrcRect->top, blend_type, m_pClipRgn, m_bRgbByteOrder, alpha_flag, pIccTransform);
    return m_pBitmap->CompositeBitmap(left, top, pSrcRect->Width(), pSrcRect->Height(), pBitmap,
        pSrcRect->left, pSrcRect->top, blend_type, m_pClipRgn, m_bRgbByteOrder, pIccTransform);
}

FX_BOOL	CFX_SkiaDriver::StretchDIBits(const CFX_DIBSource* pSource, FX_DWORD argb, int dest_left, int dest_top,
    int dest_width, int dest_height, const FX_RECT* pClipRect, FX_DWORD flags,
    int alpha_flag, void* pIccTransform, int blend_type)
{
    if (m_pBitmap->GetBuffer() == NULL) {
        return TRUE;
    }
    if (dest_width == pSource->GetWidth() && dest_height == pSource->GetHeight()) {
        FX_RECT rect(0, 0, dest_width, dest_height);
        return SetDIBits(pSource, argb, &rect, dest_left, dest_top, blend_type, alpha_flag, pIccTransform);
    }
    FX_RECT dest_rect(dest_left, dest_top, dest_left + dest_width, dest_top + dest_height);
    dest_rect.Normalize();
    FX_RECT dest_clip = dest_rect;
    dest_clip.Intersect(*pClipRect);
    CFX_BitmapComposer composer;
    composer.Compose(m_pBitmap, m_pClipRgn, 255, argb, dest_clip, FALSE, FALSE, FALSE, m_bRgbByteOrder, alpha_flag, pIccTransform, blend_type);
    dest_clip.Offset(-dest_rect.left, -dest_rect.top);
    CFX_ImageStretcher stretcher;
    if (stretcher.Start(&composer, pSource, dest_width, dest_height, dest_clip, flags)) {
        stretcher.Continue(NULL);
    }
    return TRUE;
}

FX_BOOL	CFX_SkiaDriver::StartDIBits(const CFX_DIBSource* pSource, int bitmap_alpha, FX_DWORD argb,
    const CFX_AffineMatrix* pMatrix, FX_DWORD render_flags, FX_LPVOID& handle,
    int alpha_flag, void* pIccTransform, int blend_type)
{
    if (m_pBitmap->GetBuffer() == NULL) {
        return TRUE;
    }
    CFX_ImageRenderer* pRenderer = FX_NEW CFX_ImageRenderer;
    if (!pRenderer) {
        return FALSE;
    }
    pRenderer->Start(m_pBitmap, m_pClipRgn, pSource, bitmap_alpha, argb, pMatrix, render_flags, m_bRgbByteOrder, alpha_flag, pIccTransform);
    handle = pRenderer;
    return TRUE;
}

FX_BOOL	CFX_SkiaDriver::ContinueDIBits(FX_LPVOID pHandle, IFX_Pause* pPause)
{
    if (m_pBitmap->GetBuffer() == NULL) {
        return TRUE;
    }
    return ((CFX_ImageRenderer*)pHandle)->Continue(pPause);
}

void CFX_SkiaDriver::CancelDIBits(FX_LPVOID pHandle)
{
    if (m_pBitmap->GetBuffer() == NULL) {
        return;
    }
    delete (CFX_ImageRenderer*)pHandle;
}

FX_BOOL CFX_SkiaDriver::DrawDeviceText(int nChars, const FXTEXT_CHARPOS* pCharPos, CFX_Font* pFont,
    CFX_FontCache* pCache, const CFX_AffineMatrix* pObject2Device, FX_FLOAT font_size, FX_DWORD color,
    int alpha_flag, void* pIccTransform)
{
#if _FX_OS_ == _FX_WIN32_DESKTOP_ || _FX_OS_ == _FX_WIN64_  || _FXM_PLATFORM_ == _FXM_PLATFORM_LINUX_
    return FALSE;
#endif

#if (_FXM_PLATFORM_  == _FXM_PLATFORM_APPLE_ && (!defined(_FPDFAPI_MINI_)))
    if (!pFont) {
        return FALSE;
    }
    FX_BOOL bBold = pFont->IsBold();
    if (!bBold && pFont->GetSubstFont() &&
        pFont->GetSubstFont()->m_Weight >= 500 &&
        pFont->GetSubstFont()->m_Weight <= 600) {
        return FALSE;
    }
    for (int i = 0; i < nChars; i++) {
        if (pCharPos[i].m_bGlyphAdjust) {
            return FALSE;
        }
    }
    CGContextRef ctx = CGContextRef(m_pPlatformGraphics);
    if (NULL == ctx) {
        return FALSE;
    }
    CGContextSaveGState(ctx);
    CGContextSetTextDrawingMode(ctx, kCGTextFillClip);
    CGRect rect_cg;
    CGImageRef pImageCG = NULL;
    if (m_pClipRgn) {
        rect_cg = CGRectMake(m_pClipRgn->GetBox().left, m_pClipRgn->GetBox().top, m_pClipRgn->GetBox().Width(), m_pClipRgn->GetBox().Height());
        const CFX_DIBitmap*	pClipMask = m_pClipRgn->GetMask();
        if (pClipMask) {
            CGDataProviderRef pClipMaskDataProvider = CGDataProviderCreateWithData(NULL,
                pClipMask->GetBuffer(),
                pClipMask->GetPitch() * pClipMask->GetHeight(),
                _DoNothing);
            CGFloat decode_f[2] = { 255.f, 0.f };
            pImageCG = CGImageMaskCreate(pClipMask->GetWidth(), pClipMask->GetHeight(),
                8, 8, pClipMask->GetPitch(), pClipMaskDataProvider,
                decode_f, FALSE);
            CGDataProviderRelease(pClipMaskDataProvider);
        }
    }
    else {
        rect_cg = CGRectMake(0, 0, m_pBitmap->GetWidth(), m_pBitmap->GetHeight());
    }
    rect_cg = CGContextConvertRectToDeviceSpace(ctx, rect_cg);
    if (pImageCG) {
        CGContextClipToMask(ctx, rect_cg, pImageCG);
    }
    else {
        CGContextClipToRect(ctx, rect_cg);
    }
    FX_BOOL ret = _CGDrawGlyphRun(ctx, nChars, pCharPos, pFont, pCache, pObject2Device, font_size, argb, alpha_flag, pIccTransform);
    if (pImageCG) {
        CGImageRelease(pImageCG);
    }
    CGContextRestoreGState(ctx);
    return ret;
#endif

}

FX_BOOL	CFX_SkiaDriver::RenderRasterizerSkia(SkPath& skPath, const SkPaint& origPaint, SkIRect& rect, FX_DWORD color, FX_BOOL bFullCover, FX_BOOL bGroupKnockout,
        int alpha_flag, void* pIccTransform, FX_BOOL bFill)
{
    CFX_DIBitmap* pt = bGroupKnockout ? this->GetBackDrop() : NULL;
    CFX_SkiaRenderer render;
    if (!render.Init(m_pBitmap, pt, m_pClipRgn, color, bFullCover, m_bRgbByteOrder, alpha_flag, pIccTransform)) {
        return FALSE;
    }
    SkRasterClip rasterClip(rect);
    SuperBlitter_skia::DrawPath(skPath, (SkBlitter*)&render,  rasterClip, origPaint);
    return TRUE;
}

void CFX_SkiaDriver::SetClipMask(SkPath& skPath, SkPaint* spaint)
{
    SkIRect clip_box;
    clip_box.set(0, 0, (FX_FLOAT)(GetDeviceCaps(FXDC_PIXEL_WIDTH)), (FX_FLOAT)(GetDeviceCaps(FXDC_PIXEL_HEIGHT)));
    clip_box.intersect(m_pClipRgn->GetBox().left, m_pClipRgn->GetBox().top,
        m_pClipRgn->GetBox().right, m_pClipRgn->GetBox().bottom);
    SkPath* pathPtr = &skPath;
    SkRect path_rect = skPath.getBounds();
    clip_box.intersect(FXSYS_floor(path_rect.fLeft), FXSYS_floor(path_rect.fTop), FXSYS_floor(path_rect.fRight) + 1, FXSYS_floor(path_rect.fBottom) + 1);
    CFX_DIBitmapRef mask;
    CFX_DIBitmap* pThisLayer = mask.New();
    if (!pThisLayer) {
        return;
    }
    pThisLayer->Create(clip_box.width(), clip_box.height(), FXDIB_8bppMask);
    pThisLayer->Clear(0);
    CFX_SkiaA8Renderer render;
    render.Init(pThisLayer, clip_box.fLeft, clip_box.fTop);
    SkRasterClip rasterClip(clip_box);
    SuperBlitter_skia::DrawPath(skPath, (SkBlitter*)&render, rasterClip, *spaint);
    m_pClipRgn->IntersectMaskF(clip_box.fLeft, clip_box.fTop, mask);
}


void SuperBlitter_skia::DrawPath(const SkPath& srcPath, SkBlitter* blitter, const SkRasterClip& rect, const SkPaint& origPaint)
{
    SkPath*		pathPtr = (SkPath*)&srcPath;
    bool		doFill = true;
    SkPath		tmpPath;
    SkTCopyOnFirstWrite<SkPaint> paint(origPaint);
    {
        SkScalar coverage;
        if (FxSkDrawTreatAsHairline(origPaint, &coverage)) {
            if (FXSYS_fabs(SK_Scalar1 - coverage) < 0.000001f) {
                paint.writable()->setStrokeWidth(0);
            }
            else if (1) {
                U8CPU newAlpha;
                int scale = (int)SkScalarMul(coverage, 256);
                newAlpha = origPaint.getAlpha() * scale >> 8;
                SkPaint* writablePaint = paint.writable();
                writablePaint->setStrokeWidth(0);
                writablePaint->setAlpha(newAlpha);
            }
        }
    }
    if (paint->getPathEffect() || paint->getStyle() != SkPaint::kFill_Style) {
        SkIRect devBounds = rect.getBounds();
        devBounds.outset(1, 1);
        SkRect cullRect = SkRect::Make(devBounds);
        doFill = paint->getFillPath(*pathPtr, &tmpPath, &cullRect);
        pathPtr = &tmpPath;
    }
    SkPath* devPathPtr = pathPtr;
    void(*proc)(const SkPath&, const SkRasterClip&, SkBlitter*);
    if (doFill) {
        if (paint->isAntiAlias()) {
            proc = SkScan::AntiFillPath;
        }
        else {
            proc = SkScan::FillPath;
        }
    }
    else {
        if (paint->isAntiAlias()) {
            proc = SkScan::AntiHairPath;
        }
        else {
            proc = SkScan::HairPath;
        }
    }
    proc(*devPathPtr, rect, blitter);
}

void CSkia_PathData::BuildPath(const CFX_PathData* pPathData, const CFX_AffineMatrix* pObject2Device)
{
    const CFX_PathData* pFPath = pPathData;
    int nPoints = pFPath->GetPointCount();
    FX_PATHPOINT* pPoints = pFPath->GetPoints();
    for (int i = 0; i < nPoints; i++) {
        FX_FLOAT x = pPoints[i].m_PointX, y = pPoints[i].m_PointY;
        if (pObject2Device) {
            pObject2Device->Transform(x, y);
        }
        int point_type = pPoints[i].m_Flag & FXPT_TYPE;
        if (point_type == FXPT_MOVETO) {
            m_PathData.moveTo(x, y);
        }
        else if (point_type == FXPT_LINETO) {
            if (pPoints[i - 1].m_Flag == FXPT_MOVETO && (i == nPoints - 1 || pPoints[i + 1].m_Flag == FXPT_MOVETO) &&
                FXSYS_abs(pPoints[i].m_PointX - pPoints[i - 1].m_PointX) < 0.4f && FXSYS_abs(pPoints[i].m_PointY - pPoints[i - 1].m_PointY) < 0.4f) {
                x += 0.4f;
            }
            m_PathData.lineTo(x, y);
        }
        else if (point_type == FXPT_BEZIERTO) {
            FX_FLOAT x2 = pPoints[i + 1].m_PointX, y2 = pPoints[i + 1].m_PointY;
            FX_FLOAT x3 = pPoints[i + 2].m_PointX, y3 = pPoints[i + 2].m_PointY;
            if (pObject2Device) {
                pObject2Device->Transform(x2, y2);
                pObject2Device->Transform(x3, y3);
            }
            m_PathData.cubicTo(x, y, x2, y2, x3, y3);
            i += 2;
        }
        if (pPoints[i].m_Flag & FXPT_CLOSEFIGURE) {
            m_PathData.close();
        }
    }
}

FX_BOOL FxSkDrawTreatAsHairline(const SkPaint& paint, SkScalar* coverage)
{
    if (SkPaint::kStroke_Style != paint.getStyle()) {
        return FALSE;
    }
    FXSYS_assert(coverage);
    SkScalar strokeWidth = paint.getStrokeWidth();
    if (strokeWidth < 0.000001f) {
        *coverage = SK_Scalar1;
        return TRUE;
    }
    if (!paint.isAntiAlias()) {
        return FALSE;
    }
    if (strokeWidth <= SK_Scalar1) {
        *coverage = strokeWidth;
        return TRUE;
    }
    return FALSE;
}

static void SkRasterizeStroke(SkPaint& spaint, SkPath* dstPathData, SkPath& path_data,
    const CFX_AffineMatrix* pObject2Device,
    const CFX_GraphStateData* pGraphState, FX_FLOAT scale,
    FX_BOOL bStrokeAdjust, FX_BOOL bTextMode)
{
    SkPaint::Cap cap;
    switch (pGraphState->m_LineCap) {
    case CFX_GraphStateData::LineCapRound:
        cap = SkPaint::Cap::kRound_Cap;
        break;
    case CFX_GraphStateData::LineCapSquare:
        cap = SkPaint::Cap::kSquare_Cap;
        break;
    default:
        cap = SkPaint::Cap::kButt_Cap;
        break;
    }
    SkPaint::Join join;
    switch (pGraphState->m_LineJoin) {
    case CFX_GraphStateData::LineJoinRound:
        join = SkPaint::Join::kRound_Join;
        break;
    case CFX_GraphStateData::LineJoinBevel:
        join = SkPaint::Join::kBevel_Join;
        break;
    default:
        join = SkPaint::Join::kMiter_Join;
        break;
    }
    FX_FLOAT width = pGraphState->m_LineWidth * scale;
    FX_FLOAT unit = FXSYS_Div(1.0f, (pObject2Device->GetXUnit() + pObject2Device->GetYUnit()) / 2);
    if (width <= unit) {
        width = unit;
    }
    if (pGraphState->m_DashArray == NULL) {
        SkStroke stroker;
        stroker.setCap(cap);
        stroker.setJoin(join);
        stroker.setMiterLimit(pGraphState->m_MiterLimit);
        stroker.setWidth(width);
        stroker.setDoFill(FALSE);
        stroker.strokePath(path_data, dstPathData);
        SkMatrix smatrix;
        smatrix.setAll(pObject2Device->a, pObject2Device->c, pObject2Device->e, pObject2Device->b, pObject2Device->d, pObject2Device->f, 0, 0, 1);
        dstPathData->transform(smatrix);
    }
    else {
        int count = (pGraphState->m_DashCount + 1) / 2;
        SkScalar* intervals = FX_Alloc(SkScalar, count * sizeof (SkScalar));
        if (!intervals) {
            return;
        }
        for (int i = 0; i < count; i++) {
            FX_FLOAT on = pGraphState->m_DashArray[i * 2];
            if (on <= 0.000001f) {
                on = 1.0f / 10;
            }
            FX_FLOAT off = i * 2 + 1 == pGraphState->m_DashCount ? on :
                pGraphState->m_DashArray[i * 2 + 1];
            if (off < 0) {
                off = 0;
            }
            intervals[i * 2] = on * scale;
            intervals[i * 2 + 1] = off * scale;
        }
        SkDashPathEffect* pEffect = SkDashPathEffect::Create(intervals, count * 2, pGraphState->m_DashPhase * scale);
        if (!pEffect) {
            FX_Free(intervals);
            return;
        }
        spaint.setPathEffect(pEffect)->unref();
        spaint.setStrokeWidth(width);
        spaint.setStrokeMiter(pGraphState->m_MiterLimit);
        spaint.setStrokeCap(cap);
        spaint.setStrokeJoin(join);
        spaint.getFillPath(path_data, dstPathData);
        SkMatrix smatrix;
        smatrix.setAll(pObject2Device->a, pObject2Device->c, pObject2Device->e, pObject2Device->b, pObject2Device->d, pObject2Device->f, 0, 0, 1);
        dstPathData->transform(smatrix);
        FX_Free(intervals);
    }
}

void RgbByteOrderSetPixel(CFX_DIBitmap* pBitmap, int x, int y, FX_DWORD argb)
{
    if (x < 0 || x >= pBitmap->GetWidth() || y < 0 || y >= pBitmap->GetHeight()) {
        return;
    }
    FX_LPBYTE pos = (FX_BYTE*)pBitmap->GetBuffer() + y * pBitmap->GetPitch() + x * pBitmap->GetBPP() / 8;
    if (pBitmap->GetFormat() == FXDIB_Argb) {
        FXARGB_SETRGBORDERDIB(pos, ArgbGamma(argb));
    }
    else {
        int alpha = FXARGB_A(argb);
        pos[0] = (FXARGB_R(argb) * alpha + pos[0] * (255 - alpha)) / 255;
        pos[1] = (FXARGB_G(argb) * alpha + pos[1] * (255 - alpha)) / 255;
        pos[2] = (FXARGB_B(argb) * alpha + pos[2] * (255 - alpha)) / 255;
    }
}

FX_ARGB _DefaultCMYK2ARGB(FX_CMYK cmyk, FX_BYTE alpha)
{
    FX_BYTE r, g, b;
    AdobeCMYK_to_sRGB1(FXSYS_GetCValue(cmyk), FXSYS_GetMValue(cmyk), FXSYS_GetYValue(cmyk), FXSYS_GetKValue(cmyk),
        r, g, b);
    return ArgbEncode(alpha, r, g, b);
}

FX_BOOL _DibSetPixel(CFX_DIBitmap* pDevice, int x, int y, FX_DWORD color, int alpha_flag, void* pIccTransform)
{
    FX_BOOL bObjCMYK = FXGETFLAG_COLORTYPE(alpha_flag);
    int alpha = bObjCMYK ? FXGETFLAG_ALPHA_FILL(alpha_flag) : FXARGB_A(color);
    if (pIccTransform) {
        ICodec_IccModule* pIccModule = CFX_GEModule::Get()->GetCodecModule()->GetIccModule();
        color = bObjCMYK ? FXCMYK_TODIB(color) : FXARGB_TODIB(color);
        pIccModule->TranslateScanline(pIccTransform, (FX_LPBYTE)&color, (FX_LPBYTE)&color, 1);
        color = bObjCMYK ? FXCMYK_TODIB(color) : FXARGB_TODIB(color);
        if (!pDevice->IsCmykImage()) {
            color = (color & 0xffffff) | (alpha << 24);
        }
    }
    else {
        if (pDevice->IsCmykImage()) {
            if (!bObjCMYK) {
                return FALSE;
            }
        }
        else {
            if (bObjCMYK) {
                color = _DefaultCMYK2ARGB(color, alpha);
            }
        }
    }
    pDevice->SetPixel(x, y, color);
    if (pDevice->m_pAlphaMask) {
        pDevice->m_pAlphaMask->SetPixel(x, y, alpha << 24);
    }
    return TRUE;
}

void RgbByteOrderCompositeRect(CFX_DIBitmap* pBitmap, int left, int top, int width, int height, FX_ARGB argb)
{
    int src_alpha = FXARGB_A(argb);
    if (src_alpha == 0) {
        return;
    }
    FX_RECT rect(left, top, left + width, top + height);
    rect.Intersect(0, 0, pBitmap->GetWidth(), pBitmap->GetHeight());
    width = rect.Width();
    int src_r = FXARGB_R(argb), src_g = FXARGB_G(argb), src_b = FXARGB_B(argb);
    int Bpp = pBitmap->GetBPP() / 8;
    FX_BOOL bAlpha = pBitmap->HasAlpha();
    int dib_argb = FXARGB_TOBGRORDERDIB(argb);
    FX_BYTE* pBuffer = pBitmap->GetBuffer();
    if (src_alpha == 255) {
        for (int row = rect.top; row < rect.bottom; row++) {
            FX_LPBYTE dest_scan = pBuffer + row * pBitmap->GetPitch() + rect.left * Bpp;
            if (Bpp == 4) {
                FX_DWORD* scan = (FX_DWORD*)dest_scan;
                for (int col = 0; col < width; col++) {
                    *scan++ = dib_argb;
                }
            }
            else {
                for (int col = 0; col < width; col++) {
                    *dest_scan++ = src_r;
                    *dest_scan++ = src_g;
                    *dest_scan++ = src_b;
                }
            }
        }
        return;
    }
    src_r = FX_GAMMA(src_r);
    src_g = FX_GAMMA(src_g);
    src_b = FX_GAMMA(src_b);
    for (int row = rect.top; row < rect.bottom; row++) {
        FX_LPBYTE dest_scan = pBuffer + row * pBitmap->GetPitch() + rect.left * Bpp;
        if (bAlpha) {
            for (int col = 0; col < width; col++) {
                FX_BYTE back_alpha = dest_scan[3];
                if (back_alpha == 0) {
                    FXARGB_SETRGBORDERDIB(dest_scan, FXARGB_MAKE(src_alpha, src_r, src_g, src_b));
                    dest_scan += 4;
                    continue;
                }
                FX_BYTE dest_alpha = back_alpha + src_alpha - back_alpha * src_alpha / 255;
                dest_scan[3] = dest_alpha;
                int alpha_ratio = src_alpha * 255 / dest_alpha;
                *dest_scan = FXDIB_ALPHA_MERGE(*dest_scan, src_r, alpha_ratio);
                dest_scan++;
                *dest_scan = FXDIB_ALPHA_MERGE(*dest_scan, src_g, alpha_ratio);
                dest_scan++;
                *dest_scan = FXDIB_ALPHA_MERGE(*dest_scan, src_b, alpha_ratio);
                dest_scan += 2;
            }
        }
        else {
            for (int col = 0; col < width; col++) {
                *dest_scan = FX_GAMMA_INVERSE(FXDIB_ALPHA_MERGE(FX_GAMMA(*dest_scan), src_r, src_alpha));
                dest_scan++;
                *dest_scan = FX_GAMMA_INVERSE(FXDIB_ALPHA_MERGE(FX_GAMMA(*dest_scan), src_g, src_alpha));
                dest_scan++;
                *dest_scan = FX_GAMMA_INVERSE(FXDIB_ALPHA_MERGE(FX_GAMMA(*dest_scan), src_b, src_alpha));
                dest_scan++;
                if (Bpp == 4) {
                    dest_scan++;
                }
            }
        }
    }
}

void RgbByteOrderTransferBitmap(CFX_DIBitmap* pBitmap, int dest_left, int dest_top, int width, int height,
    const CFX_DIBSource* pSrcBitmap, int src_left, int src_top)
{
    if (pBitmap == NULL) {
        return;
    }
    pBitmap->GetOverlapRect(dest_left, dest_top, width, height, pSrcBitmap->GetWidth(), pSrcBitmap->GetHeight(), src_left, src_top, NULL);
    if (width == 0 || height == 0) {
        return;
    }
    int Bpp = pBitmap->GetBPP() / 8;
    FXDIB_Format dest_format = pBitmap->GetFormat();
    FXDIB_Format src_format = pSrcBitmap->GetFormat();
    int pitch = pBitmap->GetPitch();
    FX_BYTE* buffer = pBitmap->GetBuffer();
    if (dest_format == src_format) {
        for (int row = 0; row < height; row++) {
            FX_LPBYTE dest_scan = buffer + (dest_top + row) * pitch + dest_left * Bpp;
            FX_LPBYTE src_scan = (FX_LPBYTE)pSrcBitmap->GetScanline(src_top + row) + src_left * Bpp;
            if (Bpp == 4) {
                for (int col = 0; col < width; col++) {
                    FXARGB_SETDIB(dest_scan, FXARGB_MAKE(src_scan[3], src_scan[0], src_scan[1], src_scan[2]));
                    dest_scan += 4;
                    src_scan += 4;
                }
            }
            else {
                for (int col = 0; col < width; col++) {
                    *dest_scan++ = src_scan[2];
                    *dest_scan++ = src_scan[1];
                    *dest_scan++ = src_scan[0];
                    src_scan += 3;
                }
            }
        }
        return;
    }
    int src_pitch = pSrcBitmap->GetPitch();
    FX_ARGB* src_pal = pSrcBitmap->GetPalette();
    FX_LPBYTE dest_buf = buffer + dest_top * pitch + dest_left * Bpp;
    if (dest_format == FXDIB_Rgb) {
        if (src_format == FXDIB_Rgb32) {
            for (int row = 0; row < height; row++) {
                FX_LPBYTE dest_scan = dest_buf + row * pitch;
                FX_LPBYTE src_scan = (FX_BYTE*)pSrcBitmap->GetScanline(src_top + row) + src_left * 4;
                for (int col = 0; col < width; col++) {
                    *dest_scan++ = src_scan[2];
                    *dest_scan++ = src_scan[1];
                    *dest_scan++ = src_scan[0];
                    src_scan += 4;
                }
            }
        }
        else {
            ASSERT(FALSE);
        }
    }
    else if (dest_format == FXDIB_Argb || dest_format == FXDIB_Rgb32) {
        if (src_format == FXDIB_Rgb) {
            for (int row = 0; row < height; row++) {
                FX_BYTE* dest_scan = (FX_BYTE*)(dest_buf + row * pitch);
                FX_LPBYTE src_scan = (FX_BYTE*)pSrcBitmap->GetScanline(src_top + row) + src_left * 3;
                if (src_format == FXDIB_Argb) {
                    for (int col = 0; col < width; col++) {
                        FXARGB_SETDIB(dest_scan, FXARGB_MAKE(0xff, FX_GAMMA(src_scan[0]), FX_GAMMA(src_scan[1]), FX_GAMMA(src_scan[2])));
                        dest_scan += 4;
                        src_scan += 3;
                    }
                }
                else {
                    for (int col = 0; col < width; col++) {
                        FXARGB_SETDIB(dest_scan, FXARGB_MAKE(0xff, src_scan[0], src_scan[1], src_scan[2]));
                        dest_scan += 4;
                        src_scan += 3;
                    }
                }
            }
        }
        else if (src_format == FXDIB_Rgb32) {
            ASSERT(dest_format == FXDIB_Argb);
            for (int row = 0; row < height; row++) {
                FX_LPBYTE dest_scan = dest_buf + row * pitch;
                FX_LPBYTE src_scan = (FX_LPBYTE)(pSrcBitmap->GetScanline(src_top + row) + src_left * 4);
                for (int col = 0; col < width; col++) {
                    FXARGB_SETDIB(dest_scan, FXARGB_MAKE(0xff, src_scan[0], src_scan[1], src_scan[2]));
                    src_scan += 4;
                    dest_scan += 4;
                }
            }
        }
    }
    else {
        ASSERT(FALSE);
    }
}

