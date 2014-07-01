//----------------------------------------------------------------------------
// Anti-Grain Geometry - Version 2.3
// Copyright (C) 2002-2005 Maxim Shemanarev (http://www.antigrain.com)
//
// Permission to copy, use, modify, sell and distribute this software
// is granted provided this copyright notice appears in all copies.
// This software is provided "as is" without express or implied
// warranty, and with no claim as to its suitability for any purpose.
//
//----------------------------------------------------------------------------
// Contact: mcseem@antigrain.com
//          mcseemagg@yahoo.com
//          http://www.antigrain.com
//----------------------------------------------------------------------------
//
// Liang-Barsky clipping
//
//----------------------------------------------------------------------------
#ifndef AGG_CLIP_LIANG_BARSKY_INCLUDED
#define AGG_CLIP_LIANG_BARSKY_INCLUDED
#include "../../../include/fxcrt/fx_coordinates.h"

inline unsigned clip_liang_barsky(FX_FLOAT x1, FX_FLOAT y1, FX_FLOAT x2, FX_FLOAT y2,
                                  const CFX_FloatRect& clip_box,
                                  FX_FLOAT* x, FX_FLOAT* y)
{
    const FX_FLOAT nearzero = 1e-30f;
    FX_FLOAT deltax = (FX_FLOAT)(x2 - x1);
    FX_FLOAT deltay = (FX_FLOAT)(y2 - y1);
    unsigned np = 0;
    if(deltax == 0) {
        deltax = (x1 > clip_box.left) ? -nearzero : nearzero;
    }
    FX_FLOAT xin, xout;
    if(deltax > 0) {
        xin  = (FX_FLOAT)clip_box.left;
        xout = (FX_FLOAT)clip_box.right;
    } else {
        xin  = (FX_FLOAT)clip_box.right;
        xout = (FX_FLOAT)clip_box.left;
    }
    FX_FLOAT tinx = FXSYS_Div(xin - x1, deltax);
    if(deltay == 0) {
        deltay = (y1 > clip_box.top) ? -nearzero : nearzero;
    }
    FX_FLOAT yin, yout;
    if(deltay > 0) {
        yin  = (FX_FLOAT)clip_box.top;
        yout = (FX_FLOAT)clip_box.bottom;
    } else {
        yin  = (FX_FLOAT)clip_box.bottom;
        yout = (FX_FLOAT)clip_box.top;
    }
    FX_FLOAT tiny = FXSYS_Div(yin - y1, deltay);
    FX_FLOAT tin1, tin2;
    if (tinx < tiny) {
        tin1 = tinx;
        tin2 = tiny;
    } else {
        tin1 = tiny;
        tin2 = tinx;
    }
    if(tin1 <= 1.0f) {
        if(0 < tin1) {
            *x++ = xin;
            *y++ = yin;
            ++np;
        }
        if(tin2 <= 1.0f) {
            FX_FLOAT toutx = FXSYS_Div(xout - x1, deltax);
            FX_FLOAT touty = FXSYS_Div(yout - y1, deltay);
            FX_FLOAT tout1 = (toutx < touty) ? toutx : touty;
            if(tin2 > 0 || tout1 > 0) {
                if(tin2 <= tout1) {
                    if(tin2 > 0) {
                        if(tinx > tiny) {
                            *x++ = xin;
                            *y++ = (y1 + FXSYS_Mul(deltay, tinx));
                        } else {
                            *x++ = (x1 + FXSYS_Mul(deltax, tiny));
                            *y++ = yin;
                        }
                        ++np;
                    }
                    if(tout1 < 1.0f) {
                        if(toutx < touty) {
                            *x++ = xout;
                            *y++ = (y1 + FXSYS_Mul(deltay, toutx));
                        } else {
                            *x++ = (x1 + FXSYS_Mul(deltax, touty));
                            *y++ = yout;
                        }
                    } else {
                        *x++ = x2;
                        *y++ = y2;
                    }
                    ++np;
                } else {
                    if(tinx > tiny) {
                        *x++ = xin;
                        *y++ = yout;
                    } else {
                        *x++ = xout;
                        *y++ = yin;
                    }
                    ++np;
                }
            }
        }
    }
    return np;
}

#endif
