// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef _FX_SKIA_DEVICE_DRIVER_
#define _FX_SKIA_DEVICE_DRIVER_
#if defined(_SKIA_SUPPORT_)
class CFX_SkiaDeviceDriver : public IFX_RenderDeviceDriver
{
public:
    CFX_SkiaDeviceDriver(CFX_DIBitmap* pBitmap, int dither_bits, FX_BOOL bRgbByteOrder, CFX_DIBitmap* pOriDevice, FX_BOOL bGroupKnockout);
    virtual ~CFX_SkiaDeviceDriver();

    virtual int			GetDeviceCaps(int caps_id);


    virtual void		SaveState();
    virtual void		RestoreState(FX_BOOL bKeepSaved);


    virtual FX_BOOL		SetClip_PathFill(const CFX_PathData* pPathData,
                                         const CFX_AffineMatrix* pObject2Device,
                                         int fill_mode
                                     );


    virtual FX_BOOL		SetClip_PathStroke(const CFX_PathData* pPathData,
                                           const CFX_AffineMatrix* pObject2Device,
                                           const CFX_GraphStateData* pGraphState
                                       );


    virtual FX_BOOL		DrawPath(const CFX_PathData* pPathData,
                                 const CFX_AffineMatrix* pObject2Device,
                                 const CFX_GraphStateData* pGraphState,
                                 FX_DWORD fill_color,
                                 FX_DWORD stroke_color,
                                 int fill_mode,
                                 int alpha_flag,
                                 void* pIccTransform,
                                 int blend_type
                             );

    virtual FX_BOOL		SetPixel(int x, int y, FX_DWORD color,
                                 int alpha_flag = 0, void* pIccTransform = NULL);

    virtual FX_BOOL		FillRect(const FX_RECT* pRect,
                                 FX_DWORD fill_color, int alpha_flag, void* pIccTransform, int blend_type);


    virtual FX_BOOL		DrawCosmeticLine(FX_FLOAT x1, FX_FLOAT y1, FX_FLOAT x2, FX_FLOAT y2, FX_DWORD color,
                                         int alpha_flag, void* pIccTransform, int blend_type)
    {
        return FALSE;
    }

    virtual FX_BOOL		GetClipBox(FX_RECT* pRect);


    virtual FX_BOOL		GetDIBits(CFX_DIBitmap* pBitmap, int left, int top, void* pIccTransform = NULL, FX_BOOL bDEdge = FALSE);
    virtual CFX_DIBitmap*   GetBackDrop()
    {
        return m_pAggDriver->GetBackDrop();
    }

    virtual FX_BOOL		SetDIBits(const CFX_DIBSource* pBitmap, FX_DWORD color, const FX_RECT* pSrcRect,
                                  int dest_left, int dest_top, int blend_type,
                                  int alpha_flag = 0, void* pIccTransform = NULL);
    virtual FX_BOOL		StretchDIBits(const CFX_DIBSource* pBitmap, FX_DWORD color, int dest_left, int dest_top,
                                      int dest_width, int dest_height, const FX_RECT* pClipRect, FX_DWORD flags,
                                      int alpha_flag = 0 , void* pIccTransform = NULL, int blend_type = FXDIB_BLEND_NORMAL);

    virtual FX_BOOL		StartDIBits(const CFX_DIBSource* pBitmap, int bitmap_alpha, FX_DWORD color,
                                    const CFX_AffineMatrix* pMatrix, FX_DWORD flags, FX_LPVOID& handle,
                                    int alpha_flag = 0, void* pIccTransform = NULL, int blend_type = FXDIB_BLEND_NORMAL);
    virtual FX_BOOL		ContinueDIBits(FX_LPVOID handle, IFX_Pause* pPause);
    virtual void		CancelDIBits(FX_LPVOID handle);

    virtual FX_BOOL     DrawDeviceText(int nChars, const FXTEXT_CHARPOS* pCharPos, CFX_Font* pFont,
                                       CFX_FontCache* pCache, const CFX_AffineMatrix* pObject2Device, FX_FLOAT font_size, FX_DWORD color,
                                       int alpha_flag = 0, void* pIccTransform = NULL);

    virtual FX_BOOL		RenderRasterizer(FX_NAMESPACE_DECLARE(agg, rasterizer_scanline_aa)& rasterizer, FX_DWORD color, FX_BOOL bFullCover, FX_BOOL bGroupKnockout,
                                         int alpha_flag, void* pIccTransform);
    virtual FX_BOOL		RenderRasterizerSkia(SkPath& skPath, const SkPaint& origPaint, SkIRect& rect, FX_DWORD color, FX_BOOL bFullCover, FX_BOOL bGroupKnockout,
            int alpha_flag, void* pIccTransform, FX_BOOL bFill = TRUE);
    void				SetClipMask(FX_NAMESPACE_DECLARE(agg, rasterizer_scanline_aa)& rasterizer);
    void				SetClipMask(SkPath& skPath, SkPaint* spaint);
    virtual	FX_LPBYTE	GetBuffer() const
    {
        return m_pAggDriver->GetBuffer();
    }
    CFX_AggDeviceDriver* m_pAggDriver;
};
#endif
#endif
