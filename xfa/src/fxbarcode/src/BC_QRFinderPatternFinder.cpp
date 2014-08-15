// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "barcode.h"
#include "include/BC_ResultPoint.h"
#include "include/BC_QRFinderPatternFinder.h"
#include "include/BC_FinderPatternInfo.h"
#include "include/BC_CommonBitMatrix.h"
#include "include/BC_QRFinderPattern.h"
const FX_INT32 CBC_QRFinderPatternFinder::CENTER_QUORUM = 2;
const FX_INT32 CBC_QRFinderPatternFinder::MIN_SKIP = 3;
const FX_INT32 CBC_QRFinderPatternFinder::MAX_MODULES = 57;
const FX_INT32 CBC_QRFinderPatternFinder::INTEGER_MATH_SHIFT = 8;
CBC_QRFinderPatternFinder::CBC_QRFinderPatternFinder(CBC_CommonBitMatrix* image)
{
    m_image = image;
    m_crossCheckStateCount.SetSize(5);
    m_hasSkipped = FALSE;
}
CBC_QRFinderPatternFinder::~CBC_QRFinderPatternFinder()
{
    for(FX_INT32 i = 0; i < m_possibleCenters.GetSize(); i++) {
        delete (CBC_QRFinderPattern*)m_possibleCenters[i];
    }
    m_possibleCenters.RemoveAll();
}
class ClosestToAverageComparator
{
private:
    FX_FLOAT m_averageModuleSize;
public:
    ClosestToAverageComparator(FX_FLOAT averageModuleSize) : m_averageModuleSize(averageModuleSize)
    {
    }
    FX_INT32 operator()(FinderPattern *a, FinderPattern *b)
    {
        FX_FLOAT dA = (FX_FLOAT)fabs(a->GetEstimatedModuleSize() - m_averageModuleSize);
        FX_FLOAT dB = (FX_FLOAT)fabs(b->GetEstimatedModuleSize() - m_averageModuleSize);
        return dA < dB ? -1 : dA > dB ? 1 : 0;
    }
};
class CenterComparator
{
public:
    FX_INT32 operator()(FinderPattern *a, FinderPattern *b)
    {
        return b->GetCount() - a->GetCount();
    }
};
CBC_CommonBitMatrix *CBC_QRFinderPatternFinder::GetImage()
{
    return m_image;
}
CFX_Int32Array &CBC_QRFinderPatternFinder::GetCrossCheckStateCount()
{
    m_crossCheckStateCount[0] = 0;
    m_crossCheckStateCount[1] = 0;
    m_crossCheckStateCount[2] = 0;
    m_crossCheckStateCount[3] = 0;
    m_crossCheckStateCount[4] = 0;
    return m_crossCheckStateCount;
}
CFX_PtrArray *CBC_QRFinderPatternFinder::GetPossibleCenters()
{
    return &m_possibleCenters;
}
CBC_QRFinderPatternInfo* CBC_QRFinderPatternFinder::Find(FX_INT32 hint, FX_INT32 &e)
{
    FX_INT32 maxI = m_image->GetHeight();
    FX_INT32 maxJ = m_image->GetWidth();
    FX_INT32 iSkip = (3 * maxI) / (4 * MAX_MODULES);
    if(iSkip < MIN_SKIP || 0) {
        iSkip = MIN_SKIP;
    }
    FX_BOOL done = FALSE;
    CFX_Int32Array stateCount;
    stateCount.SetSize(5);
    for(FX_INT32 i = iSkip - 1; i < maxI && !done; i += iSkip) {
        stateCount[0] = 0;
        stateCount[1] = 0;
        stateCount[2] = 0;
        stateCount[3] = 0;
        stateCount[4] = 0;
        FX_INT32 currentState = 0;
        for(FX_INT32 j = 0; j < maxJ; j++) {
            if(m_image->Get(j, i)) {
                if((currentState & 1) == 1) {
                    currentState++;
                }
                stateCount[currentState]++;
            } else {
                if((currentState & 1) == 0) {
                    if(currentState == 4) {
                        if(FoundPatternCross(stateCount)) {
                            FX_BOOL confirmed = HandlePossibleCenter(stateCount, i, j);
                            if(confirmed) {
                                iSkip = 2;
                                if(m_hasSkipped) {
                                    done = HaveMultiplyConfirmedCenters();
                                } else {
                                    FX_INT32 rowSkip = FindRowSkip();
                                    if(rowSkip > stateCount[2]) {
                                        i += rowSkip - stateCount[2] - iSkip;
                                        j = maxJ - 1;
                                    }
                                }
                            } else {
                                do {
                                    j++;
                                } while (j < maxJ && !m_image->Get(j, i));
                                j--;
                            }
                            currentState = 0;
                            stateCount[0] = 0;
                            stateCount[1] = 0;
                            stateCount[2] = 0;
                            stateCount[3] = 0;
                            stateCount[4] = 0;
                        } else {
                            stateCount[0] = stateCount[2];
                            stateCount[1] = stateCount[3];
                            stateCount[2] = stateCount[4];
                            stateCount[3] = 1;
                            stateCount[4] = 0;
                            currentState = 3;
                        }
                    } else {
                        stateCount[++currentState]++;
                    }
                } else {
                    stateCount[currentState]++;
                }
            }
        }
        if(FoundPatternCross(stateCount)) {
            FX_BOOL confirmed = HandlePossibleCenter(stateCount, i, maxJ);
            if(confirmed) {
                iSkip = stateCount[0];
                if(m_hasSkipped) {
                    done = HaveMultiplyConfirmedCenters();
                }
            }
        }
    }
    CFX_PtrArray* ptr = SelectBestpatterns(e);
    BC_EXCEPTION_CHECK_ReturnValue(e, NULL);
    CBC_AutoPtr<CFX_PtrArray > patternInfo(ptr);
    OrderBestPatterns(patternInfo.get());
    return new CBC_QRFinderPatternInfo(patternInfo.get());
}
void CBC_QRFinderPatternFinder::OrderBestPatterns(CFX_PtrArray *patterns)
{
    FX_FLOAT abDistance = Distance((CBC_ResultPoint*)(*patterns)[0], (CBC_ResultPoint*)(*patterns)[1]);
    FX_FLOAT bcDistance = Distance((CBC_ResultPoint*)(*patterns)[1], (CBC_ResultPoint*)(*patterns)[2]);
    FX_FLOAT acDistance = Distance((CBC_ResultPoint*)(*patterns)[0], (CBC_ResultPoint*)(*patterns)[2]);
    CBC_QRFinderPattern *topLeft, *topRight, *bottomLeft;
    if (bcDistance >= abDistance && bcDistance >= acDistance) {
        topLeft = (CBC_QRFinderPattern *)(*patterns)[0];
        topRight = (CBC_QRFinderPattern *)(*patterns)[1];
        bottomLeft = (CBC_QRFinderPattern *)(*patterns)[2];
    } else if (acDistance >= bcDistance && acDistance >= abDistance) {
        topLeft = (CBC_QRFinderPattern *)(*patterns)[1];
        topRight = (CBC_QRFinderPattern *)(*patterns)[0];
        bottomLeft = (CBC_QRFinderPattern *)(*patterns)[2];
    } else {
        topLeft = (CBC_QRFinderPattern *)(*patterns)[2];
        topRight = (CBC_QRFinderPattern *)(*patterns)[0];
        bottomLeft = (CBC_QRFinderPattern *)(*patterns)[1];
    }
    if ((bottomLeft->GetY() - topLeft->GetY()) * (topRight->GetX() - topLeft->GetX()) < (bottomLeft->GetX()
            - topLeft->GetX()) * (topRight->GetY() - topLeft->GetY())) {
        CBC_QRFinderPattern* temp = topRight;
        topRight = bottomLeft;
        bottomLeft = temp;
    }
    (*patterns)[0] = bottomLeft;
    (*patterns)[1] = topLeft;
    (*patterns)[2] = topRight;
}
FX_FLOAT CBC_QRFinderPatternFinder::Distance(CBC_ResultPoint* point1, CBC_ResultPoint* point2)
{
    FX_FLOAT dx = point1->GetX() - point2->GetX();
    FX_FLOAT dy = point1->GetY() - point2->GetY();
    return (FX_FLOAT)FXSYS_sqrt(dx * dx + dy * dy);
}
FX_FLOAT CBC_QRFinderPatternFinder::CenterFromEnd(const CFX_Int32Array &stateCount, FX_INT32 end)
{
    return (FX_FLOAT)(end - stateCount[4] - stateCount[3]) - stateCount[2] / 2.0f;
}
FX_BOOL CBC_QRFinderPatternFinder::FoundPatternCross(const CFX_Int32Array &stateCount)
{
    FX_INT32 totalModuleSize = 0;
    for (FX_INT32 i = 0; i < 5; i++) {
        FX_INT32 count = stateCount[i];
        if (count == 0) {
            return FALSE;
        }
        totalModuleSize += count;
    }
    if (totalModuleSize < 7) {
        return FALSE;
    }
    FX_INT32 moduleSize = (totalModuleSize << INTEGER_MATH_SHIFT) / 7;
    FX_INT32 maxVariance = moduleSize / 2;
    return FXSYS_abs(moduleSize - (stateCount[0] << INTEGER_MATH_SHIFT)) < maxVariance &&
           FXSYS_abs(moduleSize - (stateCount[1] << INTEGER_MATH_SHIFT)) < maxVariance &&
           FXSYS_abs(3 * moduleSize - (stateCount[2] << INTEGER_MATH_SHIFT)) < 3 * maxVariance &&
           FXSYS_abs(moduleSize - (stateCount[3] << INTEGER_MATH_SHIFT)) < maxVariance &&
           FXSYS_abs(moduleSize - (stateCount[4] << INTEGER_MATH_SHIFT)) < maxVariance;
}
FX_FLOAT CBC_QRFinderPatternFinder::CrossCheckVertical(FX_INT32 startI, FX_INT32 centerJ, FX_INT32 maxCount,
        FX_INT32 originalStateCountTotal)
{
    CBC_CommonBitMatrix *image = m_image;
    FX_INT32 maxI = image->GetHeight();
    CFX_Int32Array &stateCount = GetCrossCheckStateCount();
    FX_INT32 i = startI;
    while(i >= 0 && image->Get(centerJ, i)) {
        stateCount[2]++;
        i--;
    }
    if(i < 0) {
        return FXSYS_nan();
    }
    while(i >= 0 && !image->Get(centerJ, i) && stateCount[1] <= maxCount) {
        stateCount[1]++;
        i--;
    }
    if(i < 0 || stateCount[1] > maxCount) {
        return FXSYS_nan();
    }
    while(i >= 0 && image->Get(centerJ, i) && stateCount[0] <= maxCount) {
        stateCount[0]++;
        i--;
    }
    if(stateCount[0] > maxCount) {
        return FXSYS_nan();
    }
    i = startI + 1;
    while(i < maxI && image->Get(centerJ, i)) {
        stateCount[2]++;
        i++;
    }
    if(i == maxI) {
        return FXSYS_nan();
    }
    while (i < maxI && !image->Get(centerJ, i) && stateCount[3] < maxCount) {
        stateCount[3]++;
        i++;
    }
    if (i == maxI || stateCount[3] >= maxCount) {
        return FXSYS_nan();
    }
    while (i < maxI && image->Get(centerJ, i) && stateCount[4] < maxCount) {
        stateCount[4]++;
        i++;
    }
    if (stateCount[4] >= maxCount) {
        return FXSYS_nan();
    }
    FX_INT32 stateCountTotal = stateCount[0] + stateCount[1] + stateCount[2] + stateCount[3] + stateCount[4];
    if (5 * FXSYS_abs(stateCountTotal - originalStateCountTotal) >= originalStateCountTotal) {
        return FXSYS_nan();
    }
    return FoundPatternCross(stateCount) ? CenterFromEnd(stateCount, i) : FXSYS_nan();
}
FX_FLOAT CBC_QRFinderPatternFinder::CrossCheckHorizontal(FX_INT32 startJ, FX_INT32 centerI, FX_INT32 maxCount, FX_INT32 originalStateCountTotal)
{
    CBC_CommonBitMatrix *image = m_image;
    FX_INT32 maxJ = image->GetWidth();
    CFX_Int32Array &stateCount = GetCrossCheckStateCount();
    FX_INT32 j = startJ;
    while (j >= 0 && image->Get(j, centerI)) {
        stateCount[2]++;
        j--;
    }
    if (j < 0) {
        return FXSYS_nan();
    }
    while (j >= 0 && !image->Get(j, centerI) && stateCount[1] <= maxCount) {
        stateCount[1]++;
        j--;
    }
    if (j < 0 || stateCount[1] > maxCount) {
        return FXSYS_nan();
    }
    while (j >= 0 && image->Get(j, centerI) && stateCount[0] <= maxCount) {
        stateCount[0]++;
        j--;
    }
    if (stateCount[0] > maxCount) {
        return FXSYS_nan();
    }
    j = startJ + 1;
    while (j < maxJ && image->Get(j, centerI)) {
        stateCount[2]++;
        j++;
    }
    if (j == maxJ) {
        return FXSYS_nan();
    }
    while (j < maxJ && !image->Get(j, centerI) && stateCount[3] < maxCount) {
        stateCount[3]++;
        j++;
    }
    if (j == maxJ || stateCount[3] >= maxCount) {
        return FXSYS_nan();
    }
    while (j < maxJ && image->Get(j, centerI) && stateCount[4] < maxCount) {
        stateCount[4]++;
        j++;
    }
    if (stateCount[4] >= maxCount) {
        return FXSYS_nan();
    }
    FX_INT32 stateCountTotal = stateCount[0] + stateCount[1] +
                               stateCount[2] + stateCount[3] +
                               stateCount[4];
    if (5 * FXSYS_abs(stateCountTotal - originalStateCountTotal) >= originalStateCountTotal) {
        return FXSYS_nan();
    }
    return FoundPatternCross(stateCount) ? CenterFromEnd(stateCount, j) : FXSYS_nan();
}
FX_BOOL CBC_QRFinderPatternFinder::HandlePossibleCenter(const CFX_Int32Array &stateCount, FX_INT32 i, FX_INT32 j)
{
    FX_INT32 stateCountTotal = stateCount[0] + stateCount[1] + stateCount[2] + stateCount[3] + stateCount[4];
    FX_FLOAT centerJ = CenterFromEnd(stateCount, j);
    FX_FLOAT centerI = CrossCheckVertical(i, (FX_INT32) centerJ, stateCount[2], stateCountTotal);
    if(!FXSYS_isnan(centerI)) {
        centerJ = CrossCheckHorizontal((FX_INT32) centerJ, (FX_INT32) centerI, stateCount[2], stateCountTotal);
        if(!FXSYS_isnan(centerJ)) {
            FX_FLOAT estimatedModuleSize = (FX_FLOAT) stateCountTotal / 7.0f;
            FX_BOOL found = FALSE;
            FX_INT32 max = m_possibleCenters.GetSize();
            for(FX_INT32 index = 0; index < max; index++) {
                CBC_QRFinderPattern *center = (CBC_QRFinderPattern*)(m_possibleCenters[index]);
                if(center->AboutEquals(estimatedModuleSize, centerI, centerJ)) {
                    center->IncrementCount();
                    found = TRUE;
                    break;
                }
            }
            if(!found) {
                m_possibleCenters.Add(FX_NEW CBC_QRFinderPattern(centerJ, centerI, estimatedModuleSize));
            }
            return TRUE;
        }
    }
    return FALSE;
}
FX_INT32 CBC_QRFinderPatternFinder::FindRowSkip()
{
    FX_INT32 max = m_possibleCenters.GetSize();
    if (max <= 1) {
        return 0;
    }
    FinderPattern *firstConfirmedCenter = NULL;
    for (FX_INT32 i = 0; i < max; i++) {
        CBC_QRFinderPattern *center = (CBC_QRFinderPattern*)m_possibleCenters[i];
        if (center->GetCount() >= CENTER_QUORUM) {
            if (firstConfirmedCenter == NULL) {
                firstConfirmedCenter = center;
            } else {
                m_hasSkipped = TRUE;
                return (FX_INT32) ((fabs(firstConfirmedCenter->GetX() - center->GetX()) -
                                    fabs(firstConfirmedCenter->GetY() - center->GetY())) / 2);
            }
        }
    }
    return 0;
}
FX_BOOL CBC_QRFinderPatternFinder::HaveMultiplyConfirmedCenters()
{
    FX_INT32 confirmedCount = 0;
    FX_FLOAT totalModuleSize = 0.0f;
    FX_INT32 max = m_possibleCenters.GetSize();
    FX_INT32 i;
    for (i = 0; i < max; i++) {
        CBC_QRFinderPattern *pattern = (CBC_QRFinderPattern*)m_possibleCenters[i];
        if (pattern->GetCount() >= CENTER_QUORUM) {
            confirmedCount++;
            totalModuleSize += pattern->GetEstimatedModuleSize();
        }
    }
    if (confirmedCount < 3) {
        return FALSE;
    }
    FX_FLOAT average = totalModuleSize / (FX_FLOAT) max;
    FX_FLOAT totalDeviation = 0.0f;
    for (i = 0; i < max; i++) {
        CBC_QRFinderPattern *pattern = (CBC_QRFinderPattern*)m_possibleCenters[i];
        totalDeviation += fabs(pattern->GetEstimatedModuleSize() - average);
    }
    return totalDeviation <= 0.05f * totalModuleSize;
}
inline FX_BOOL centerComparator(FX_LPVOID a, FX_LPVOID b)
{
    return ((CBC_QRFinderPattern*)b)->GetCount() < ((CBC_QRFinderPattern*)a)->GetCount();
}
CFX_PtrArray *CBC_QRFinderPatternFinder::SelectBestpatterns(FX_INT32 &e)
{
    FX_INT32 startSize = m_possibleCenters.GetSize();
    if(m_possibleCenters.GetSize() < 3) {
        e = BCExceptionRead;
        BC_EXCEPTION_CHECK_ReturnValue(e, NULL);
    }
    FX_FLOAT average = 0.0f;
    if(startSize > 3) {
        FX_FLOAT totalModuleSize = 0.0f;
        for(FX_INT32 i = 0; i < startSize; i++) {
            totalModuleSize += ((CBC_QRFinderPattern*)m_possibleCenters[i])->GetEstimatedModuleSize();
        }
        average = totalModuleSize / (FX_FLOAT)startSize;
        for(FX_INT32 j = 0; j < m_possibleCenters.GetSize() && m_possibleCenters.GetSize() > 3; j++) {
            CBC_QRFinderPattern *pattern = (CBC_QRFinderPattern*)m_possibleCenters[j];
            if(fabs(pattern->GetEstimatedModuleSize() - average) > 0.2f * average) {
                delete pattern;
                m_possibleCenters.RemoveAt(j);
                j--;
            }
        }
    }
    if(m_possibleCenters.GetSize() > 3) {
        BC_FX_PtrArray_Sort(m_possibleCenters, centerComparator);
    }
    CFX_PtrArray *vec = FX_NEW CFX_PtrArray();
    vec->SetSize(3);
    (*vec)[0] = ((CBC_QRFinderPattern*)m_possibleCenters[0])->Clone();
    (*vec)[1] = ((CBC_QRFinderPattern*)m_possibleCenters[1])->Clone();
    (*vec)[2] = ((CBC_QRFinderPattern*)m_possibleCenters[2])->Clone();
    return vec;
}
