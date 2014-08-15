// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "../../../foxitlib.h"
#include "fde_gedevice.h"
#include "fde_geobject.h"
#include "fde_devbasic.h"
#ifndef _FDEPLUS
#ifdef _cplusplus
exten "C" {
#endif
    FX_BOOL FDE_GetStockHatchMask(FX_INT32 iHatchStyle, CFX_DIBitmap & hatchMask)
    {
        FDE_LPCHATCHDATA pData = FDE_DEVGetHatchData(iHatchStyle);
        if (!pData) {
            return FALSE;
        }
        hatchMask.Create(pData->iWidth, pData->iHeight, FXDIB_1bppMask);
        FXSYS_memcpy(hatchMask.GetBuffer(), pData->MaskBits, hatchMask.GetPitch() * pData->iHeight);
        return TRUE;
    }
#ifdef _cplusplus
}
#endif
IFDE_RenderDevice*	IFDE_RenderDevice::Create(CFX_DIBitmap *pBitmap, FX_BOOL bRgbByteOrder )
{
    if (pBitmap == NULL) {
        return NULL;
    }
    CFX_FxgeDevice *pDevice = FX_NEW CFX_FxgeDevice;
    if (pDevice == NULL) {
        return NULL;
    }
    pDevice->Attach(pBitmap, 0, bRgbByteOrder);
    return FDE_New CFDE_FxgeDevice(pDevice, TRUE);
}
IFDE_RenderDevice* IFDE_RenderDevice::Create(CFX_RenderDevice *pDevice)
{
    if (pDevice == NULL) {
        return NULL;
    }
    return FDE_New CFDE_FxgeDevice(pDevice, FALSE);
}
CFDE_FxgeDevice::CFDE_FxgeDevice(CFX_RenderDevice *pDevice, FX_BOOL bOwnerDevice)
    : m_pDevice(pDevice)
    , m_bOwnerDevice(bOwnerDevice)
    , m_pCharPos(NULL)
    , m_iCharCount(0)
{
    FXSYS_assert(pDevice != NULL);
    FX_RECT rt = m_pDevice->GetClipBox();
    m_rtClip.Set((FX_FLOAT)rt.left, (FX_FLOAT)rt.top, (FX_FLOAT)rt.Width(), (FX_FLOAT)rt.Height());
}
CFDE_FxgeDevice::~CFDE_FxgeDevice()
{
    if (m_pCharPos != NULL) {
        FDE_Free(m_pCharPos);
    }
    if (m_bOwnerDevice && m_pDevice) {
        delete m_pDevice;
    }
}
FX_INT32 CFDE_FxgeDevice::GetWidth() const
{
    return m_pDevice->GetWidth();
}
FX_INT32 CFDE_FxgeDevice::GetHeight() const
{
    return m_pDevice->GetHeight();
}
FDE_HDEVICESTATE CFDE_FxgeDevice::SaveState()
{
    m_pDevice->SaveState();
    return NULL;
}
void CFDE_FxgeDevice::RestoreState(FDE_HDEVICESTATE hState)
{
    m_pDevice->RestoreState();
    const FX_RECT &rt = m_pDevice->GetClipBox();
    m_rtClip.Set((FX_FLOAT)rt.left, (FX_FLOAT)rt.top, (FX_FLOAT)rt.Width(), (FX_FLOAT)rt.Height());
}
FX_BOOL CFDE_FxgeDevice::SetClipRect(const CFX_RectF &rtClip)
{
    m_rtClip = rtClip;
    FX_RECT rt((FX_INT32)FXSYS_floor(rtClip.left), (FX_INT32)FXSYS_floor(rtClip.top),
               (FX_INT32)FXSYS_ceil(rtClip.right()), (FX_INT32)FXSYS_ceil(rtClip.bottom()));
    return m_pDevice->SetClip_Rect(&rt);
}
const CFX_RectF& CFDE_FxgeDevice::GetClipRect()
{
    return m_rtClip;
}
FX_BOOL CFDE_FxgeDevice::SetClipPath(const IFDE_Path *pClip)
{
    return FALSE;
}
IFDE_Path* CFDE_FxgeDevice::GetClipPath() const
{
    return NULL;
}
FX_FLOAT CFDE_FxgeDevice::GetDpiX() const
{
    return 96;
}
FX_FLOAT CFDE_FxgeDevice::GetDpiY() const
{
    return 96;
}
FX_BOOL CFDE_FxgeDevice::DrawImage(CFX_DIBSource *pDib, const CFX_RectF *pSrcRect, const CFX_RectF &dstRect, const CFX_Matrix *pImgMatrix, const CFX_Matrix *pDevMatrix)
{
    FXSYS_assert(pDib != NULL);
    CFX_RectF srcRect;
    if (pSrcRect) {
        srcRect = *pSrcRect;
    } else {
        srcRect.Set(0, 0, (FX_FLOAT)pDib->GetWidth(), (FX_FLOAT)pDib->GetHeight());
    }
    if (srcRect.IsEmpty()) {
        return FALSE;
    }
    CFX_Matrix dib2fxdev;
    if (pImgMatrix) {
        dib2fxdev = *pImgMatrix;
    } else {
        dib2fxdev.Reset();
    }
    dib2fxdev.a = dstRect.width;
    dib2fxdev.d = -dstRect.height;
    dib2fxdev.e = dstRect.left;
    dib2fxdev.f = dstRect.bottom();
    if (pDevMatrix) {
        dib2fxdev.Concat(*pDevMatrix);
    }
    FX_LPVOID handle = NULL;
    m_pDevice->StartDIBits(pDib, 255, 0, (const CFX_AffineMatrix*)&dib2fxdev, 0, handle);
    while (m_pDevice->ContinueDIBits(handle, NULL)) { }
    m_pDevice->CancelDIBits(handle);
    return handle != NULL;
}
FX_BOOL CFDE_FxgeDevice::DrawString(IFDE_Brush *pBrush, IFX_Font *pFont, const FXTEXT_CHARPOS *pCharPos, FX_INT32 iCount, FX_FLOAT fFontSize, const CFX_Matrix *pMatrix)
{
    FXSYS_assert(pBrush != NULL && pFont != NULL && pCharPos != NULL && iCount > 0);
    CFX_FontCache *pCache = CFX_GEModule::Get()->GetFontCache();
    CFX_Font *pFxFont = (CFX_Font*)pFont->GetDevFont();
    switch (pBrush->GetType()) {
        case FDE_BRUSHTYPE_Solid: {
#if _FX_OS_ == _FX_WIN32_DESKTOP_ || _FX_OS_ == _FX_WIN32_MOBILE_ || _FX_OS_ == _FX_WIN64_ || _FX_OS_ == _FX_MACOSX_
                FX_ARGB argb = ((IFDE_SolidBrush*)pBrush)->GetColor();
                if ((pFont->GetFontStyles() & FX_FONTSTYLE_Italic) != 0 && !pFxFont->IsItalic()) {
                    FXTEXT_CHARPOS *pCP = (FXTEXT_CHARPOS*)pCharPos;
                    FX_FLOAT *pAM;
                    for (FX_INT32 i = 0; i < iCount; ++i) {
                        static const FX_FLOAT  mc = 0.267949f;
                        pAM = pCP->m_AdjustMatrix;
                        pAM[2] = mc * pAM[0] + pAM[2];
                        pAM[3] = mc * pAM[1] + pAM[3];
                        pCP ++;
                    }
                }
                FXTEXT_CHARPOS *pCP = (FXTEXT_CHARPOS*)pCharPos;
                IFX_Font *pCurFont = NULL;
                IFX_Font *pSTFont = NULL;
                FXTEXT_CHARPOS *pCurCP = NULL;
                FX_INT32 iCurCount = 0;
                for (FX_INT32 i = 0; i < iCount; ++i) {
                    pSTFont = pFont->GetSubstFont((FX_INT32)pCP->m_GlyphIndex);
                    pCP->m_GlyphIndex &= 0x00FFFFFF;
                    pCP->m_bFontStyle = FALSE;
                    if (pCurFont != pSTFont) {
                        if (pCurFont != NULL) {
                            pFxFont = (CFX_Font*)pCurFont->GetDevFont();
                            m_pDevice->DrawNormalText(iCurCount, pCurCP, pFxFont, pCache, -fFontSize, (const CFX_AffineMatrix*)pMatrix, argb, FXTEXT_CLEARTYPE);
                        }
                        pCurFont = pSTFont;
                        pCurCP = pCP;
                        iCurCount = 1;
                    } else {
                        iCurCount ++;
                    }
                    pCP ++;
                }
                if (pCurFont != NULL && iCurCount) {
                    pFxFont = (CFX_Font*)pCurFont->GetDevFont();
                    return m_pDevice->DrawNormalText(iCurCount, pCurCP, pFxFont, pCache, -fFontSize, (const CFX_AffineMatrix*)pMatrix, argb, FXTEXT_CLEARTYPE);
                }
                return TRUE;
#else
                FX_ARGB argb = ((IFDE_SolidBrush*)pBrush)->GetColor();
                if ((pFont->GetFontStyles() & FX_FONTSTYLE_Italic) != 0 && !pFxFont->IsItalic()) {
                    FXTEXT_CHARPOS *pCP = (FXTEXT_CHARPOS*)pCharPos;
                    FX_FLOAT *pAM;
                    for (FX_INT32 i = 0; i < iCount; ++i) {
                        static const FX_FLOAT  mc = 0.267949f;
                        pAM = pCP->m_AdjustMatrix;
                        pAM[2] = mc * pAM[0] + pAM[2];
                        pAM[3] = mc * pAM[1] + pAM[3];
                        pCP ++;
                    }
                }
                return m_pDevice->DrawNormalText(iCount, pCharPos, pFxFont, pCache, -fFontSize, (const CFX_AffineMatrix*)pMatrix, argb, FXTEXT_CLEARTYPE);
#endif
            }
            break;
        default:
            return FALSE;
    }
}
FX_BOOL CFDE_FxgeDevice::DrawBezier(IFDE_Pen *pPen, FX_FLOAT fPenWidth, const CFX_PointF &pt1, const CFX_PointF &pt2, const CFX_PointF &pt3, const CFX_PointF &pt4, const CFX_Matrix *pMatrix )
{
    CFX_PointsF points;
    points.Add(pt1);
    points.Add(pt2);
    points.Add(pt3);
    points.Add(pt4);
    CFDE_Path path;
    path.AddBezier(points);
    return DrawPath(pPen, fPenWidth, &path, pMatrix);
}
FX_BOOL CFDE_FxgeDevice::DrawCurve(IFDE_Pen *pPen, FX_FLOAT fPenWidth, const CFX_PointsF &points, FX_BOOL bClosed, FX_FLOAT fTension , const CFX_Matrix *pMatrix )
{
    CFDE_Path path;
    path.AddCurve(points, bClosed, fTension);
    return DrawPath(pPen, fPenWidth, &path, pMatrix);
}
FX_BOOL CFDE_FxgeDevice::DrawEllipse(IFDE_Pen *pPen, FX_FLOAT fPenWidth, const CFX_RectF &rect, const CFX_Matrix *pMatrix )
{
    CFDE_Path path;
    path.AddEllipse(rect);
    return DrawPath(pPen, fPenWidth, &path, pMatrix);
}
FX_BOOL CFDE_FxgeDevice::DrawLines(IFDE_Pen *pPen, FX_FLOAT fPenWidth, const CFX_PointsF &points, const CFX_Matrix *pMatrix )
{
    CFDE_Path path;
    path.AddLines(points);
    return DrawPath(pPen, fPenWidth, &path, pMatrix);
}
FX_BOOL CFDE_FxgeDevice::DrawLine(IFDE_Pen *pPen, FX_FLOAT fPenWidth, const CFX_PointF &pt1, const CFX_PointF &pt2, const CFX_Matrix *pMatrix )
{
    CFDE_Path path;
    path.AddLine(pt1, pt2);
    return DrawPath(pPen, fPenWidth, &path, pMatrix);
}
FX_BOOL CFDE_FxgeDevice::DrawPath(IFDE_Pen *pPen, FX_FLOAT fPenWidth, const IFDE_Path *pPath, const CFX_Matrix *pMatrix )
{
    CFDE_Path *pGePath = (CFDE_Path*)pPath;
    if (pGePath == NULL) {
        return FALSE;
    }
    CFX_GraphStateData graphState;
    if (!CreatePen(pPen, fPenWidth, graphState)) {
        return FALSE;
    }
    return m_pDevice->DrawPath(&pGePath->m_Path, (const CFX_AffineMatrix*)pMatrix, &graphState, 0, pPen->GetColor(), 0);
}
FX_BOOL CFDE_FxgeDevice::DrawPolygon(IFDE_Pen *pPen, FX_FLOAT fPenWidth, const CFX_PointsF &points, const CFX_Matrix *pMatrix )
{
    CFDE_Path path;
    path.AddPolygon(points);
    return DrawPath(pPen, fPenWidth, &path, pMatrix);
}
FX_BOOL CFDE_FxgeDevice::DrawRectangle(IFDE_Pen *pPen, FX_FLOAT fPenWidth, const CFX_RectF &rect, const CFX_Matrix *pMatrix )
{
    CFDE_Path path;
    path.AddRectangle(rect);
    return DrawPath(pPen, fPenWidth, &path, pMatrix);
}
FX_BOOL CFDE_FxgeDevice::FillClosedCurve(IFDE_Brush *pBrush, const CFX_PointsF &points, FX_FLOAT fTension , const CFX_Matrix *pMatrix )
{
    CFDE_Path path;
    path.AddCurve(points, TRUE, fTension);
    return FillPath(pBrush, &path, pMatrix);
}
FX_BOOL CFDE_FxgeDevice::FillEllipse(IFDE_Brush* pBrush, const CFX_RectF& rect, const CFX_Matrix *pMatrix )
{
    CFDE_Path path;
    path.AddEllipse(rect);
    return FillPath(pBrush, &path, pMatrix);
}
FX_BOOL CFDE_FxgeDevice::FillPolygon(IFDE_Brush *pBrush, const CFX_PointsF &points, const CFX_Matrix *pMatrix )
{
    CFDE_Path path;
    path.AddPolygon(points);
    return FillPath(pBrush, &path, pMatrix);
}
FX_BOOL CFDE_FxgeDevice::FillRectangle(IFDE_Brush *pBrush, const CFX_RectF &rect, const CFX_Matrix *pMatrix )
{
    CFDE_Path path;
    path.AddRectangle(rect);
    return FillPath(pBrush, &path, pMatrix);
}
FX_BOOL CFDE_FxgeDevice::CreatePen(IFDE_Pen *pPen, FX_FLOAT fPenWidth, CFX_GraphStateData &graphState)
{
    if (pPen == NULL) {
        return FALSE;
    }
    graphState.m_LineCap = (CFX_GraphStateData::LineCap)pPen->GetLineCap();
    graphState.m_LineJoin = (CFX_GraphStateData::LineJoin)pPen->GetLineJoin();
    graphState.m_LineWidth = fPenWidth;
    graphState.m_MiterLimit = pPen->GetMiterLimit();
    graphState.m_DashPhase = pPen->GetDashPhase();
    CFX_FloatArray dashArray;
    switch (pPen->GetDashStyle()) {
        case FDE_DASHSTYLE_Dash:
            dashArray.Add(3);
            dashArray.Add(1);
            break;
        case FDE_DASHSTYLE_Dot:
            dashArray.Add(1);
            dashArray.Add(1);
            break;
        case FDE_DASHSTYLE_DashDot:
            dashArray.Add(3);
            dashArray.Add(1);
            dashArray.Add(1);
            dashArray.Add(1);
            break;
        case FDE_DASHSTYLE_DashDotDot:
            dashArray.Add(3);
            dashArray.Add(1);
            dashArray.Add(1);
            dashArray.Add(1);
            dashArray.Add(1);
            dashArray.Add(1);
            break;
        case FDE_DASHSTYLE_Customized:
            pPen->GetDashArray(dashArray);
            break;
    }
    FX_INT32 iDashCount = dashArray.GetSize();
    if (iDashCount > 0) {
        graphState.SetDashCount(iDashCount);
        for (FX_INT32 i = 0; i < iDashCount; ++i) {
            graphState.m_DashArray[i] = dashArray[i] * fPenWidth;
        }
    }
    return TRUE;
}
typedef FX_BOOL (CFDE_FxgeDevice::*pfFillPath)(IFDE_Brush *pBrush, const CFX_PathData *pPath, const CFX_Matrix *pMatrix);
static const pfFillPath gs_FillPath[] = {
    &CFDE_FxgeDevice::FillSolidPath,
    &CFDE_FxgeDevice::FillHatchPath,
    &CFDE_FxgeDevice::FillTexturePath,
    &CFDE_FxgeDevice::FillLinearGradientPath,
};
FX_BOOL CFDE_FxgeDevice::FillPath(IFDE_Brush *pBrush, const IFDE_Path *pPath, const CFX_Matrix *pMatrix)
{
    CFDE_Path *pGePath = (CFDE_Path*)pPath;
    if (pGePath == NULL) {
        return FALSE;
    }
    if (pBrush == NULL) {
        return FALSE;
    }
    FX_INT32 iType = pBrush->GetType();
    if (iType < 0 || iType > FDE_BRUSHTYPE_MAX) {
        return FALSE;
    }
    return (this->*gs_FillPath[iType])(pBrush, &pGePath->m_Path, pMatrix);
}
FX_BOOL CFDE_FxgeDevice::FillSolidPath(IFDE_Brush *pBrush, const CFX_PathData *pPath, const CFX_Matrix *pMatrix)
{
    FXSYS_assert(pPath && pBrush && pBrush->GetType() == FDE_BRUSHTYPE_Solid);
    IFDE_SolidBrush *pSolidBrush = (IFDE_SolidBrush*)pBrush;
    return m_pDevice->DrawPath(pPath, (const CFX_AffineMatrix*)pMatrix, NULL, pSolidBrush->GetColor(), 0, FXFILL_WINDING);
}
FX_BOOL CFDE_FxgeDevice::FillHatchPath(IFDE_Brush *pBrush, const CFX_PathData *pPath, const CFX_Matrix *pMatrix)
{
    FXSYS_assert(pPath && pBrush && pBrush->GetType() == FDE_BRUSHTYPE_Hatch);
    IFDE_HatchBrush *pHatchBrush = (IFDE_HatchBrush*)pBrush;
    FX_INT32 iStyle = pHatchBrush->GetHatchStyle();
    if (iStyle < FDE_HATCHSTYLE_Min || iStyle > FDE_HATCHSTYLE_Max) {
        return FALSE;
    }
    CFX_DIBitmap mask;
    if (!FDE_GetStockHatchMask(iStyle, mask)) {
        return FALSE;
    }
    FX_ARGB dwForeColor = pHatchBrush->GetColor(TRUE);
    FX_ARGB dwBackColor = pHatchBrush->GetColor(FALSE);
    CFX_FloatRect rectf = pPath->GetBoundingBox();
    if (pMatrix) {
        rectf.Transform((const CFX_AffineMatrix*)pMatrix);
    }
    FX_RECT rect(FXSYS_round(rectf.left), FXSYS_round(rectf.top),
                 FXSYS_round(rectf.right), FXSYS_round(rectf.bottom));
    m_pDevice->SaveState();
    m_pDevice->StartRendering();
    m_pDevice->SetClip_PathFill(pPath, (const CFX_AffineMatrix*)pMatrix, FXFILL_WINDING);
    m_pDevice->FillRect(&rect, dwBackColor);
    for (FX_INT32 j = rect.bottom; j < rect.top; j += mask.GetHeight())
        for (FX_INT32 i = rect.left; i < rect.right; i += mask.GetWidth()) {
            m_pDevice->SetBitMask(&mask, i, j, dwForeColor);
        }
    m_pDevice->EndRendering();
    m_pDevice->RestoreState();
    return TRUE;
}
FX_BOOL CFDE_FxgeDevice::FillTexturePath(IFDE_Brush *pBrush, const CFX_PathData *pPath, const CFX_Matrix *pMatrix)
{
    FXSYS_assert(pPath && pBrush && pBrush->GetType() == FDE_BRUSHTYPE_Texture);
    IFDE_TextureBrush *pTextureBrush = (IFDE_TextureBrush*)pBrush;
    IFDE_Image *pImage = (IFDE_Image*)pTextureBrush->GetImage();
    if (pImage == NULL) {
        return FALSE;
    }
    CFX_Size size;
    size.Set(pImage->GetImageWidth(), pImage->GetImageHeight());
    CFX_DIBitmap bmp;
    bmp.Create(size.x, size.y, FXDIB_Argb);
    if (!pImage->StartLoadImage(&bmp, 0, 0, size.x, size.y, 0, 0, size.x, size.y)) {
        return FALSE;
    }
    if (pImage->DoLoadImage() < 100) {
        return FALSE;
    }
    pImage->StopLoadImage();
    return WrapTexture(pTextureBrush->GetWrapMode(), &bmp, pPath, pMatrix);
}
FX_BOOL CFDE_FxgeDevice::WrapTexture(FX_INT32 iWrapMode, const CFX_DIBitmap *pBitmap, const CFX_PathData *pPath, const CFX_Matrix *pMatrix)
{
    CFX_FloatRect rectf = pPath->GetBoundingBox();
    if (pMatrix) {
        rectf.Transform((const CFX_AffineMatrix*)pMatrix);
    }
    FX_RECT rect(FXSYS_round(rectf.left), FXSYS_round(rectf.top),
                 FXSYS_round(rectf.right), FXSYS_round(rectf.bottom));
    rect.Normalize();
    if (rect.IsEmpty()) {
        return FALSE;
    }
    m_pDevice->SaveState();
    m_pDevice->StartRendering();
    m_pDevice->SetClip_PathFill(pPath, (const CFX_AffineMatrix*)pMatrix, FXFILL_WINDING);
    switch (iWrapMode) {
        case FDE_WRAPMODE_Tile:
        case FDE_WRAPMODE_TileFlipX:
        case FDE_WRAPMODE_TileFlipY:
        case FDE_WRAPMODE_TileFlipXY: {
                FX_BOOL bFlipX = iWrapMode == FDE_WRAPMODE_TileFlipXY || iWrapMode == FDE_WRAPMODE_TileFlipX;
                FX_BOOL bFlipY = iWrapMode == FDE_WRAPMODE_TileFlipXY || iWrapMode == FDE_WRAPMODE_TileFlipY;
                const CFX_DIBitmap *pFlip[2][2];
                pFlip[0][0] = pBitmap;
                pFlip[0][1] = bFlipX ? pBitmap->FlipImage(TRUE, FALSE) : pBitmap;
                pFlip[1][0] = bFlipY ? pBitmap->FlipImage(FALSE, TRUE) : pBitmap;
                pFlip[1][1] = (bFlipX || bFlipY) ? pBitmap->FlipImage(bFlipX, bFlipY) : pBitmap;
                FX_INT32 iCounterY = 0;
                for (FX_INT32 j = rect.top; j < rect.bottom; j += pBitmap->GetHeight()) {
                    FX_INT32 indexY = iCounterY++ % 2;
                    FX_INT32 iCounterX = 0;
                    for (FX_INT32 i = rect.left; i < rect.right; i += pBitmap->GetWidth()) {
                        FX_INT32 indexX = iCounterX++ % 2;
                        m_pDevice->SetDIBits(pFlip[indexY][indexX], i, j);
                    }
                }
                if (pFlip[0][1] != pFlip[0][0]) {
                    delete pFlip[0][1];
                }
                if (pFlip[1][0] != pFlip[0][0]) {
                    delete pFlip[1][0];
                }
                if (pFlip[1][1] != pFlip[0][0]) {
                    delete pFlip[1][1];
                }
            }
            break;
        case FDE_WRAPMODE_Clamp: {
                m_pDevice->SetDIBits(pBitmap, rect.left, rect.bottom);
            }
            break;
    }
    m_pDevice->EndRendering();
    m_pDevice->RestoreState();
    return TRUE;
}
FX_BOOL CFDE_FxgeDevice::FillLinearGradientPath(IFDE_Brush *pBrush, const CFX_PathData *pPath, const CFX_Matrix *pMatrix)
{
    FXSYS_assert(pPath && pBrush && pBrush->GetType() == FDE_BRUSHTYPE_LinearGradient);
    IFDE_LinearGradientBrush *pLinearBrush = (IFDE_LinearGradientBrush*)pBrush;
    CFX_PointF pt0, pt1;
    pLinearBrush->GetLinearPoints(pt0, pt1);
    CFX_VectorF fDiagonal;
    fDiagonal.Set(pt0, pt1);
    FX_FLOAT fTheta = FXSYS_atan2(fDiagonal.y, fDiagonal.x);
    FX_FLOAT fLength = fDiagonal.Length();
    FX_FLOAT fTotalX = fLength / FXSYS_cos(fTheta);
    FX_FLOAT fTotalY = fLength / FXSYS_cos(FX_PI / 2 - fTheta);
    FX_FLOAT fSteps = FX_MAX(fTotalX, fTotalY);
    FX_FLOAT dx = fTotalX / fSteps;
    FX_FLOAT dy = fTotalY / fSteps;
    FX_ARGB cr0, cr1;
    pLinearBrush->GetLinearColors(cr0, cr1);
    FX_FLOAT a0 = FXARGB_A(cr0);
    FX_FLOAT r0 = FXARGB_R(cr0);
    FX_FLOAT g0 = FXARGB_G(cr0);
    FX_FLOAT b0 = FXARGB_B(cr0);
    FX_FLOAT da = (FXARGB_A(cr1) - a0) / fSteps;
    FX_FLOAT dr = (FXARGB_R(cr1) - r0) / fSteps;
    FX_FLOAT dg = (FXARGB_G(cr1) - g0) / fSteps;
    FX_FLOAT db = (FXARGB_B(cr1) - b0) / fSteps;
    CFX_DIBitmap bmp;
    bmp.Create(FXSYS_round(FXSYS_fabs(fDiagonal.x)), FXSYS_round(FXSYS_fabs(fDiagonal.y)), FXDIB_Argb);
    CFX_FxgeDevice dev;
    dev.Attach(&bmp);
    pt1 = pt0;
    FX_INT32 iSteps = FXSYS_round(FXSYS_ceil(fSteps));
    while (--iSteps >= 0) {
        cr0 = ArgbEncode(FXSYS_round(a0), FXSYS_round(r0), FXSYS_round(g0), FXSYS_round(b0));
        dev.DrawCosmeticLine(pt0.x, pt0.y, pt1.x, pt1.y, cr0);
        pt1.x += dx;
        pt0.y += dy;
        a0 += da;
        r0 += dr;
        g0 += dg;
        b0 += db;
    }
    return WrapTexture(pLinearBrush->GetWrapMode(), &bmp, pPath, pMatrix);
}
#endif
