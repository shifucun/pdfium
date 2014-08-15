// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "barcode.h"
#include "include/BC_CommonPerspectiveTransform.h"
#include "include/BC_CommonBitMatrix.h"
#include "include/BC_QRGridSampler.h"
CBC_QRGridSampler CBC_QRGridSampler::m_gridSampler;
CBC_QRGridSampler::CBC_QRGridSampler()
{
}
CBC_QRGridSampler::~CBC_QRGridSampler()
{
}
CBC_QRGridSampler &CBC_QRGridSampler::GetInstance()
{
    return m_gridSampler;
}
void CBC_QRGridSampler::CheckAndNudgePoints(CBC_CommonBitMatrix *image, CFX_FloatArray *points, FX_INT32 &e)
{
    FX_INT32 width = image->GetWidth();
    FX_INT32 height = image->GetHeight();
    FX_BOOL nudged = TRUE;
    FX_INT32 offset;
    for (offset = 0; offset < points->GetSize() && nudged; offset += 2) {
        FX_INT32 x = (FX_INT32) (*points)[offset];
        FX_INT32 y = (FX_INT32) (*points)[offset + 1];
        if (x < -1 || x > width || y < -1 || y > height) {
            e = BCExceptionRead;
            BC_EXCEPTION_CHECK_ReturnVoid(e);
        }
        nudged = FALSE;
        if (x == -1) {
            (*points)[offset] = 0.0f;
            nudged = TRUE;
        } else if (x == width) {
            (*points)[offset] = (FX_FLOAT)(width - 1);
            nudged = TRUE;
        }
        if (y == -1) {
            (*points)[offset + 1] = 0.0f;
            nudged = TRUE;
        } else if (y == height) {
            (*points)[offset + 1] = (FX_FLOAT)(height - 1);
            nudged = TRUE;
        }
    }
    nudged = TRUE;
    for (offset = (*points).GetSize() - 2; offset >= 0 && nudged; offset -= 2) {
        FX_INT32 x = (FX_INT32) (*points)[offset];
        FX_INT32 y = (FX_INT32) (*points)[offset + 1];
        if (x < -1 || x > width || y < -1 || y > height) {
            e = BCExceptionRead;
            BC_EXCEPTION_CHECK_ReturnVoid(e);
        }
        nudged = FALSE;
        if (x == -1) {
            (*points)[offset] = 0.0f;
            nudged = TRUE;
        } else if (x == width) {
            (*points)[offset] = (FX_FLOAT)(width - 1);
            nudged = TRUE;
        }
        if (y == -1) {
            (*points)[offset + 1] = 0.0f;
            nudged = TRUE;
        } else if (y == height) {
            (*points)[offset + 1] = (FX_FLOAT)(height - 1);
            nudged = TRUE;
        }
    }
}
CBC_CommonBitMatrix *CBC_QRGridSampler::SampleGrid(CBC_CommonBitMatrix *image, FX_INT32 dimensionX, FX_INT32 dimensionY,
        FX_FLOAT p1ToX, FX_FLOAT p1ToY,
        FX_FLOAT p2ToX, FX_FLOAT p2ToY,
        FX_FLOAT p3ToX, FX_FLOAT p3ToY,
        FX_FLOAT p4ToX, FX_FLOAT p4ToY,
        FX_FLOAT p1FromX, FX_FLOAT p1FromY,
        FX_FLOAT p2FromX, FX_FLOAT p2FromY,
        FX_FLOAT p3FromX, FX_FLOAT p3FromY,
        FX_FLOAT p4FromX, FX_FLOAT p4FromY, FX_INT32 &e)
{
    CBC_AutoPtr<CBC_CommonPerspectiveTransform> transform(CBC_CommonPerspectiveTransform::QuadrilateralToQuadrilateral(
                p1ToX, p1ToY, p2ToX, p2ToY, p3ToX, p3ToY, p4ToX, p4ToY,
                p1FromX, p1FromY, p2FromX, p2FromY, p3FromX, p3FromY, p4FromX, p4FromY));
    CBC_CommonBitMatrix *tempBitM = FX_NEW CBC_CommonBitMatrix();
    tempBitM->Init(dimensionX, dimensionY);
    CBC_AutoPtr<CBC_CommonBitMatrix> bits(tempBitM);
    CFX_FloatArray points;
    points.SetSize(dimensionX << 1);
    for (FX_INT32 y = 0; y < dimensionY; y++) {
        FX_INT32 max = points.GetSize();
        FX_FLOAT iValue = (FX_FLOAT) (y + 0.5f);
        FX_INT32 x;
        for (x = 0; x < max; x += 2) {
            points[x] = (FX_FLOAT) ((x >> 1) + 0.5f);
            points[x + 1] = iValue;
        }
        transform->TransformPoints(&points);
        CheckAndNudgePoints(image, &points, e);
        BC_EXCEPTION_CHECK_ReturnValue(e, NULL);
        for (x = 0; x < max; x += 2) {
            if (image->Get((FX_INT32) points[x], (FX_INT32) points[x + 1])) {
                bits->Set(x >> 1, y);
            }
        }
    }
    return bits.release();
}
