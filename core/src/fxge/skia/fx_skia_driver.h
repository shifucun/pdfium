// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef _FX_SKIA_DRIVER_
#define _FX_SKIA_DRIVER_

#include "fx_skia.h"

class CFX_SkiaDriver : public IFX_RenderDeviceDriver
{
public:
    CFX_SkiaDriver(CFX_DIBitmap* pBitmap, int dither_bits, FX_BOOL bRgbByteOrder, CFX_DIBitmap* pOriDevice, FX_BOOL bGroupKnockout);

    virtual ~CFX_SkiaDriver();
    
    void                InitPlatform();

    void                DestroyPlatform();

    virtual int         GetDeviceCaps(int caps_id);

    virtual void        SaveState();

    virtual void        RestoreState(FX_BOOL bKeepSaved);

    virtual FX_BOOL     SetClip_PathFill(const CFX_PathData* pPathData,
                                         const CFX_AffineMatrix* pObject2Device,
                                         int fill_mode
                                     );

    virtual FX_BOOL     SetClip_PathStroke(const CFX_PathData* pPathData,
                                           const CFX_AffineMatrix* pObject2Device,
                                           const CFX_GraphStateData* pGraphState
                                       );

    virtual FX_BOOL     DrawPath(const CFX_PathData* pPathData,
                                 const CFX_AffineMatrix* pObject2Device,
                                 const CFX_GraphStateData* pGraphState,
                                 FX_DWORD fill_color,
                                 FX_DWORD stroke_color,
                                 int fill_mode,
                                 int alpha_flag,
                                 void* pIccTransform,
                                 int blend_type
                             );

    virtual FX_BOOL     SetPixel(int x, int y, FX_DWORD color,
                                 int alpha_flag = 0, void* pIccTransform = NULL);

    virtual FX_BOOL     FillRect(const FX_RECT* pRect,
                                 FX_DWORD fill_color, int alpha_flag, void* pIccTransform, int blend_type);

    virtual FX_BOOL     DrawCosmeticLine(FX_FLOAT x1, FX_FLOAT y1, FX_FLOAT x2, FX_FLOAT y2, FX_DWORD color,
                                         int alpha_flag, void* pIccTransform, int blend_type)
    {
        return FALSE;
    }

    virtual FX_BOOL     GetClipBox(FX_RECT* pRect);

    virtual FX_BOOL     GetDIBits(CFX_DIBitmap* pBitmap, int left, int top, void* pIccTransform = NULL, FX_BOOL bDEdge = FALSE);

    virtual CFX_DIBitmap*   GetBackDrop()
    {
        return m_pOriDevice;
    }

    virtual FX_BOOL     SetDIBits(const CFX_DIBSource* pBitmap, FX_DWORD color, const FX_RECT* pSrcRect,
                                  int dest_left, int dest_top, int blend_type,
                                  int alpha_flag = 0, void* pIccTransform = NULL);
    virtual FX_BOOL     StretchDIBits(const CFX_DIBSource* pBitmap, FX_DWORD color, int dest_left, int dest_top,
                                      int dest_width, int dest_height, const FX_RECT* pClipRect, FX_DWORD flags,
                                      int alpha_flag = 0 , void* pIccTransform = NULL, int blend_type = FXDIB_BLEND_NORMAL);

    virtual FX_BOOL     StartDIBits(const CFX_DIBSource* pBitmap, int bitmap_alpha, FX_DWORD color,
                                    const CFX_AffineMatrix* pMatrix, FX_DWORD flags, FX_LPVOID& handle,
                                    int alpha_flag = 0, void* pIccTransform = NULL, int blend_type = FXDIB_BLEND_NORMAL);

    virtual FX_BOOL     ContinueDIBits(FX_LPVOID handle, IFX_Pause* pPause);

    virtual void        CancelDIBits(FX_LPVOID handle);

    virtual FX_BOOL     DrawDeviceText(int nChars, const FXTEXT_CHARPOS* pCharPos, CFX_Font* pFont,
                                       CFX_FontCache* pCache, const CFX_AffineMatrix* pObject2Device, FX_FLOAT font_size, FX_DWORD color,
                                       int alpha_flag = 0, void* pIccTransform = NULL);

    virtual FX_BOOL     RenderRasterizerSkia(SkPath& skPath, const SkPaint& origPaint, SkIRect& rect, FX_DWORD color, FX_BOOL bFullCover, FX_BOOL bGroupKnockout,
            int alpha_flag, void* pIccTransform, FX_BOOL bFill = TRUE);

    void                SetClipMask(SkPath& skPath, SkPaint* spaint);

    virtual FX_LPBYTE   GetBuffer() const
    {
        return m_pBitmap->GetBuffer();
    }

private:
    CFX_DIBitmap*       m_pBitmap;
    CFX_ClipRgn*        m_pClipRgn;
    CFX_PtrArray        m_StateStack;
    void*               m_pPlatformGraphics;
    void*               m_pPlatformBitmap;
    void*               m_pDwRenderTartget;
    int                 m_FillFlags;
    int                 m_DitherBits;
    FX_BOOL             m_bRgbByteOrder;
    CFX_DIBitmap*       m_pOriDevice;
    FX_BOOL             m_bGroupKnockout;
};

class SuperBlitter_skia
{
public:
    static void DrawPath(const SkPath& srcPath, SkBlitter* blitter, const SkRasterClip& rect, const SkPaint& origPaint);
};

class CSkia_PathData : public CFX_Object
{
public:
    CSkia_PathData() {}
    ~CSkia_PathData() {}
    SkPath			m_PathData;
    void			BuildPath(const CFX_PathData* pPathData, const CFX_AffineMatrix* pObject2Device);
};

#endif

FX_BOOL FxSkDrawTreatAsHairline(const SkPaint& paint, SkScalar* coverage);

static void SkRasterizeStroke(SkPaint& spaint, SkPath* dstPathData, SkPath& path_data,
    const CFX_AffineMatrix* pObject2Device,
    const CFX_GraphStateData* pGraphState, FX_FLOAT scale = 1.0f,
    FX_BOOL bStrokeAdjust = FALSE, FX_BOOL bTextMode = FALSE);

void RgbByteOrderSetPixel(CFX_DIBitmap* pBitmap, int x, int y, FX_DWORD argb);

FX_ARGB _DefaultCMYK2ARGB(FX_CMYK cmyk, FX_BYTE alpha);

FX_BOOL _DibSetPixel(CFX_DIBitmap* pDevice, int x, int y, FX_DWORD color, int alpha_flag, void* pIccTransform);

void RgbByteOrderCompositeRect(CFX_DIBitmap* pBitmap, int left, int top, int width, int height, FX_ARGB argb);

void RgbByteOrderTransferBitmap(CFX_DIBitmap* pBitmap, int dest_left, int dest_top, int width, int height,
    const CFX_DIBSource* pSrcBitmap, int src_left, int src_top);
