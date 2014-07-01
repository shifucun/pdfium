// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "../../../include/fxge/fx_ge.h"
#if defined(_SKIA_SUPPORT_)
#include "../../../include/fxcodec/fx_codec.h"
#include "fx_skia.h"
#include "fx_skia_blitter_new.h"
#include "../agg/include/fx_agg_driver.h"
#include "fx_skia_device.h"
class SuperBlitter_skia
{
public:
    static void DrawPath(const SkPath& srcPath, SkBlitter* blitter,  const SkRasterClip& rect, const SkPaint& origPaint);
};
FX_BOOL FxSkDrawTreatAsHairline(const SkPaint& paint, SkScalar* coverage)
{
    if (SkPaint::kStroke_Style != paint.getStyle()) {
        return FALSE;
    }
    FXSYS_assert(coverage);
    SkScalar strokeWidth = paint.getStrokeWidth();
    if (0 == strokeWidth) {
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
void SuperBlitter_skia::DrawPath(const SkPath& srcPath, SkBlitter* blitter, const SkRasterClip& rect, const SkPaint& origPaint)
{
    SkPath*		pathPtr = (SkPath*)&srcPath;
    bool		doFill = true;
    SkPath		tmpPath;
    SkTCopyOnFirstWrite<SkPaint> paint(origPaint);
    {
        SkScalar coverage;
        if (FxSkDrawTreatAsHairline(origPaint, &coverage)) {
            if (SK_Scalar1 == coverage) {
                paint.writable()->setStrokeWidth(0);
            } else if (1) {
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
    void (*proc)(const SkPath&, const SkRasterClip&, SkBlitter*);
    if (doFill) {
        if (paint->isAntiAlias()) {
            proc = SkScan::AntiFillPath;
        } else {
            proc = SkScan::FillPath;
        }
    } else {
        if (paint->isAntiAlias()) {
            proc = SkScan::AntiHairPath;
        } else {
            proc = SkScan::HairPath;
        }
    }
    proc(*devPathPtr, rect, blitter);
}
class CSkia_PathData : public CFX_Object
{
public:
    CSkia_PathData() {}
    ~CSkia_PathData() {}
    SkPath			m_PathData;
    void			BuildPath(const CFX_PathData* pPathData, const CFX_AffineMatrix* pObject2Device);
};
void CSkia_PathData::BuildPath(const CFX_PathData* pPathData, const CFX_AffineMatrix* pObject2Device)
{
    const CFX_PathData* pFPath = pPathData;
    int nPoints = pFPath->GetPointCount();
    FX_PATHPOINT* pPoints = pFPath->GetPoints();
    for (int i = 0; i < nPoints; i ++) {
        FX_FLOAT x = pPoints[i].m_PointX, y = pPoints[i].m_PointY;
        if (pObject2Device) {
            pObject2Device->Transform(x, y);
        }
        int point_type = pPoints[i].m_Flag & FXPT_TYPE;
        if (point_type == FXPT_MOVETO) {
            m_PathData.moveTo(x, y);
        } else if (point_type == FXPT_LINETO) {
            if (pPoints[i - 1].m_Flag == FXPT_MOVETO && (i == nPoints - 1 || pPoints[i + 1].m_Flag == FXPT_MOVETO) &&
                    FXSYS_abs(pPoints[i].m_PointX - pPoints[i - 1].m_PointX) < 0.4f && FXSYS_abs(pPoints[i].m_PointY - pPoints[i - 1].m_PointY) < 0.4f) {
                x += 0.4f;
            }
            m_PathData.lineTo(x, y);
        } else if (point_type == FXPT_BEZIERTO) {
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
static void SkRasterizeStroke(SkPaint& spaint, SkPath* dstPathData, SkPath& path_data,
                              const CFX_AffineMatrix* pObject2Device,
                              const CFX_GraphStateData* pGraphState, FX_FLOAT scale = 1.0f,
                              FX_BOOL bStrokeAdjust = FALSE, FX_BOOL bTextMode = FALSE)
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
    } else {
        int count = (pGraphState->m_DashCount + 1) / 2;
        SkScalar* intervals = FX_Alloc(SkScalar, count * sizeof (SkScalar));
        if (!intervals) {
            return;
        }
        for (int i = 0; i < count; i ++) {
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
CFX_SkiaDeviceDriver::CFX_SkiaDeviceDriver(CFX_DIBitmap* pBitmap, int dither_bits, FX_BOOL bRgbByteOrder, CFX_DIBitmap* pOriDevice, FX_BOOL bGroupKnockout)
{
    m_pAggDriver = FX_NEW CFX_AggDeviceDriver(pBitmap, dither_bits, bRgbByteOrder, pOriDevice, bGroupKnockout);
}
CFX_SkiaDeviceDriver::~CFX_SkiaDeviceDriver()
{
    if (m_pAggDriver) {
        delete m_pAggDriver;
    }
}
FX_BOOL CFX_SkiaDeviceDriver::DrawDeviceText(int nChars, const FXTEXT_CHARPOS* pCharPos, CFX_Font* pFont,
        CFX_FontCache* pCache, const CFX_AffineMatrix* pObject2Device, FX_FLOAT font_size, FX_DWORD color,
        int alpha_flag, void* pIccTransform)
{
    return m_pAggDriver->DrawDeviceText(nChars, pCharPos, pFont, pCache, pObject2Device, font_size, color,
                                        alpha_flag, pIccTransform);
}
int CFX_SkiaDeviceDriver::GetDeviceCaps(int caps_id)
{
    return m_pAggDriver->GetDeviceCaps(caps_id);
}
void CFX_SkiaDeviceDriver::SaveState()
{
    m_pAggDriver->SaveState();
}
void CFX_SkiaDeviceDriver::RestoreState(FX_BOOL bKeepSaved)
{
    m_pAggDriver->RestoreState(bKeepSaved);
}
void CFX_SkiaDeviceDriver::SetClipMask(FX_NAMESPACE_DECLARE(agg, rasterizer_scanline_aa)& rasterizer)
{
    m_pAggDriver->SetClipMask(rasterizer);
}
void CFX_SkiaDeviceDriver::SetClipMask(SkPath& skPath, SkPaint* spaint)
{
    SkIRect clip_box;
    clip_box.set(0, 0, (FX_FLOAT)(GetDeviceCaps(FXDC_PIXEL_WIDTH)), (FX_FLOAT)(GetDeviceCaps(FXDC_PIXEL_HEIGHT)));
    clip_box.intersect(m_pAggDriver->m_pClipRgn->GetBox().left, m_pAggDriver->m_pClipRgn->GetBox().top,
                       m_pAggDriver->m_pClipRgn->GetBox().right, m_pAggDriver->m_pClipRgn->GetBox().bottom);
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
    m_pAggDriver->m_pClipRgn->IntersectMaskF(clip_box.fLeft, clip_box.fTop, mask);
}
FX_BOOL CFX_SkiaDeviceDriver::SetClip_PathFill(const CFX_PathData* pPathData,
        const CFX_AffineMatrix* pObject2Device,
        int fill_mode
                                              )
{
    if (m_pAggDriver->m_pClipRgn == NULL) {
        m_pAggDriver->m_pClipRgn = FX_NEW CFX_ClipRgn(GetDeviceCaps(FXDC_PIXEL_WIDTH), GetDeviceCaps(FXDC_PIXEL_HEIGHT));
    }
    if (!m_pAggDriver->m_pClipRgn) {
        return FALSE;
    }
    if (pPathData->GetPointCount() == 5 || pPathData->GetPointCount() == 4) {
        CFX_FloatRect rectf;
        if (pPathData->IsRect(pObject2Device, &rectf)) {
            rectf.Intersect(CFX_FloatRect(0, 0, (FX_FLOAT)GetDeviceCaps(FXDC_PIXEL_WIDTH), (FX_FLOAT)GetDeviceCaps(FXDC_PIXEL_HEIGHT)));
            FX_RECT rect = rectf.GetOutterRect();
            m_pAggDriver->m_pClipRgn->IntersectRect(rect);
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
FX_BOOL CFX_SkiaDeviceDriver::SetClip_PathStroke(const CFX_PathData* pPathData,
        const CFX_AffineMatrix* pObject2Device,
        const CFX_GraphStateData* pGraphState
                                                )
{
    if (m_pAggDriver->m_pClipRgn == NULL) {
        m_pAggDriver->m_pClipRgn = FX_NEW CFX_ClipRgn(GetDeviceCaps(FXDC_PIXEL_WIDTH), GetDeviceCaps(FXDC_PIXEL_HEIGHT));
    }
    if (!m_pAggDriver->m_pClipRgn) {
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
FX_BOOL CFX_SkiaDeviceDriver::RenderRasterizer(FX_NAMESPACE_DECLARE(agg, rasterizer_scanline_aa)& rasterizer, FX_DWORD color, FX_BOOL bFullCover, FX_BOOL bGroupKnockout,
        int alpha_flag, void* pIccTransform)
{
    return m_pAggDriver->RenderRasterizer(rasterizer, color, bFullCover, bGroupKnockout, alpha_flag, pIccTransform);
}
FX_BOOL	CFX_SkiaDeviceDriver::RenderRasterizerSkia(SkPath& skPath, const SkPaint& origPaint, SkIRect& rect, FX_DWORD color, FX_BOOL bFullCover, FX_BOOL bGroupKnockout,
        int alpha_flag, void* pIccTransform, FX_BOOL bFill)
{
    CFX_DIBitmap* pt = bGroupKnockout ? m_pAggDriver->GetBackDrop() : NULL;
    CFX_SkiaRenderer render;
    if (!render.Init(m_pAggDriver->m_pBitmap, pt, m_pAggDriver->m_pClipRgn, color, bFullCover, m_pAggDriver->m_bRgbByteOrder, alpha_flag, pIccTransform)) {
        return FALSE;
    }
    SkRasterClip rasterClip(rect);
    SuperBlitter_skia::DrawPath(skPath, (SkBlitter*)&render,  rasterClip, origPaint);
    return TRUE;
}
FX_BOOL	CFX_SkiaDeviceDriver::DrawPath(const CFX_PathData* pPathData,
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
FX_BOOL CFX_SkiaDeviceDriver::SetPixel(int x, int y, FX_DWORD color, int alpha_flag, void* pIccTransform)
{
    return m_pAggDriver->SetPixel(x, y, color, alpha_flag, pIccTransform);
}
FX_BOOL CFX_SkiaDeviceDriver::FillRect(const FX_RECT* pRect, FX_DWORD fill_color, int alpha_flag, void* pIccTransform, int blend_type)
{
    return m_pAggDriver->FillRect(pRect, fill_color, alpha_flag, pIccTransform, blend_type);
}
FX_BOOL CFX_SkiaDeviceDriver::GetClipBox(FX_RECT* pRect)
{
    return m_pAggDriver->GetClipBox(pRect);
}
FX_BOOL	CFX_SkiaDeviceDriver::GetDIBits(CFX_DIBitmap* pBitmap, int left, int top, void* pIccTransform, FX_BOOL bDEdge)
{
    return m_pAggDriver->GetDIBits(pBitmap, left, top, pIccTransform, bDEdge);
}
FX_BOOL	CFX_SkiaDeviceDriver::SetDIBits(const CFX_DIBSource* pBitmap, FX_DWORD argb, const FX_RECT* pSrcRect, int left, int top, int blend_type,
                                        int alpha_flag, void* pIccTransform)
{
    return m_pAggDriver->SetDIBits(pBitmap, argb, pSrcRect, left, top, blend_type, alpha_flag, pIccTransform);
}
FX_BOOL	CFX_SkiaDeviceDriver::StretchDIBits(const CFX_DIBSource* pSource, FX_DWORD argb, int dest_left, int dest_top,
        int dest_width, int dest_height, const FX_RECT* pClipRect, FX_DWORD flags,
        int alpha_flag, void* pIccTransform, int blend_type)
{
    return m_pAggDriver->StretchDIBits(pSource, argb, dest_left, dest_top,
                                       dest_width, dest_height, pClipRect, flags,
                                       alpha_flag, pIccTransform, blend_type);
}
FX_BOOL	CFX_SkiaDeviceDriver::StartDIBits(const CFX_DIBSource* pSource, int bitmap_alpha, FX_DWORD argb,
        const CFX_AffineMatrix* pMatrix, FX_DWORD render_flags, FX_LPVOID& handle,
        int alpha_flag, void* pIccTransform, int blend_type)
{
    return m_pAggDriver->StartDIBits(pSource, bitmap_alpha, argb,
                                     pMatrix, render_flags, handle, alpha_flag, pIccTransform, blend_type);
}
FX_BOOL	CFX_SkiaDeviceDriver::ContinueDIBits(FX_LPVOID pHandle, IFX_Pause* pPause)
{
    return m_pAggDriver->ContinueDIBits(pHandle, pPause);
}
void CFX_SkiaDeviceDriver::CancelDIBits(FX_LPVOID pHandle)
{
    m_pAggDriver->CancelDIBits(pHandle);
}
CFX_SkiaDevice::CFX_SkiaDevice()
{
    m_bOwnedBitmap = FALSE;
}
FX_BOOL CFX_SkiaDevice::Attach(CFX_DIBitmap* pBitmap, int dither_bits, FX_BOOL bRgbByteOrder, CFX_DIBitmap* pOriDevice, FX_BOOL bGroupKnockout)
{
    if (pBitmap == NULL) {
        return FALSE;
    }
    SetBitmap(pBitmap);
    CFX_SkiaDeviceDriver* pDriver = FX_NEW CFX_SkiaDeviceDriver(pBitmap, dither_bits, bRgbByteOrder, pOriDevice, bGroupKnockout);
    if (!pDriver) {
        return FALSE;
    }
    SetDeviceDriver(pDriver);
    return TRUE;
}
FX_BOOL CFX_SkiaDevice::Create(int width, int height, FXDIB_Format format, int dither_bits, CFX_DIBitmap* pOriDevice)
{
    m_bOwnedBitmap = TRUE;
    CFX_DIBitmap* pBitmap = FX_NEW CFX_DIBitmap;
    if (!pBitmap) {
        return FALSE;
    }
    if (!pBitmap->Create(width, height, format)) {
        delete pBitmap;
        return FALSE;
    }
    SetBitmap(pBitmap);
    CFX_SkiaDeviceDriver* pDriver =  FX_NEW CFX_SkiaDeviceDriver(pBitmap, dither_bits, FALSE, pOriDevice, FALSE);
    if (!pDriver) {
        return FALSE;
    }
    SetDeviceDriver(pDriver);
    return TRUE;
}
CFX_SkiaDevice::~CFX_SkiaDevice()
{
    if (m_bOwnedBitmap && GetBitmap()) {
        delete GetBitmap();
    }
}
#endif
