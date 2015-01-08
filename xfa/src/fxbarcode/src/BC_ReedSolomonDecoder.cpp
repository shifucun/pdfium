// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com
/*
 * Copyright 2007 ZXing authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "barcode.h"
#include "include/BC_ReedSolomonGF256.h"
#include "include/BC_ReedSolomonGF256Poly.h"
#include "include/BC_ReedSolomonDecoder.h"
CBC_ReedSolomonDecoder::CBC_ReedSolomonDecoder(CBC_ReedSolomonGF256* field)
{
    m_field = field;
}
CBC_ReedSolomonDecoder::~CBC_ReedSolomonDecoder()
{
}
void CBC_ReedSolomonDecoder::Decode(CFX_Int32Array* received, FX_INT32 twoS, FX_INT32 &e)
{
    CBC_ReedSolomonGF256Poly poly;
    poly.Init(m_field, received, e);
    BC_EXCEPTION_CHECK_ReturnVoid(e);
    CFX_Int32Array syndromeCoefficients;
    syndromeCoefficients.SetSize(twoS);
    FX_BOOL dataMatrix = FALSE;
    FX_BOOL noError = TRUE;
    for (FX_INT32 i = 0; i < twoS; i++) {
        FX_INT32 eval = poly.EvaluateAt(m_field->Exp(dataMatrix ? i + 1 : i));
        syndromeCoefficients[twoS - 1 - i] = eval;
        if (eval != 0) {
            noError = FALSE;
        }
    }
    if(noError) {
        return;
    }
    CBC_ReedSolomonGF256Poly syndrome;
    syndrome.Init(m_field, &syndromeCoefficients, e);
    BC_EXCEPTION_CHECK_ReturnVoid(e);
    CBC_ReedSolomonGF256Poly* rsg = m_field->BuildMonomial(twoS, 1, e);
    BC_EXCEPTION_CHECK_ReturnVoid(e);
    CBC_AutoPtr<CBC_ReedSolomonGF256Poly> temp(rsg);
    CFX_PtrArray* pa = RunEuclideanAlgorithm(temp.get(), &syndrome, twoS, e);
    BC_EXCEPTION_CHECK_ReturnVoid(e);
    CBC_AutoPtr<CFX_PtrArray > sigmaOmega(pa);
    CBC_AutoPtr<CBC_ReedSolomonGF256Poly> sigma((CBC_ReedSolomonGF256Poly*)(*sigmaOmega)[0]);
    CBC_AutoPtr<CBC_ReedSolomonGF256Poly> omega((CBC_ReedSolomonGF256Poly*)(*sigmaOmega)[1]);
    CFX_Int32Array* ia1 = FindErrorLocations(sigma.get(), e);
    BC_EXCEPTION_CHECK_ReturnVoid(e);
    CBC_AutoPtr<CFX_Int32Array > errorLocations(ia1);
    CFX_Int32Array* ia2 = FindErrorMagnitudes(omega.get(), errorLocations.get(), dataMatrix, e);
    BC_EXCEPTION_CHECK_ReturnVoid(e);
    CBC_AutoPtr<CFX_Int32Array > errorMagnitudes(ia2);
    for (FX_INT32 k = 0; k < errorLocations->GetSize(); k++) {
        FX_INT32 position = received->GetSize() - 1 - m_field->Log((*errorLocations)[k], e);
        BC_EXCEPTION_CHECK_ReturnVoid(e);
        if(position < 0) {
            e = BCExceptionBadErrorLocation;
            BC_EXCEPTION_CHECK_ReturnVoid(e);
        }
        (*received)[position] = CBC_ReedSolomonGF256::AddOrSubtract((*received)[position], (*errorMagnitudes)[k]);
    }
}
CFX_PtrArray *CBC_ReedSolomonDecoder::RunEuclideanAlgorithm(CBC_ReedSolomonGF256Poly* a, CBC_ReedSolomonGF256Poly* b, FX_INT32 R, FX_INT32 &e)
{
    if (a->GetDegree() < b->GetDegree()) {
        CBC_ReedSolomonGF256Poly* temp = a;
        a = b;
        b = temp;
    }
    CBC_ReedSolomonGF256Poly* rsg1 = a->Clone(e);
    BC_EXCEPTION_CHECK_ReturnValue(e, NULL);
    CBC_AutoPtr<CBC_ReedSolomonGF256Poly> rLast(rsg1);
    CBC_ReedSolomonGF256Poly* rsg2 = b->Clone(e);
    BC_EXCEPTION_CHECK_ReturnValue(e, NULL);
    CBC_AutoPtr<CBC_ReedSolomonGF256Poly> r(rsg2);
    CBC_ReedSolomonGF256Poly* rsg3 = m_field->GetOne()->Clone(e);
    BC_EXCEPTION_CHECK_ReturnValue(e, NULL);
    CBC_AutoPtr<CBC_ReedSolomonGF256Poly> sLast(rsg3);
    CBC_ReedSolomonGF256Poly* rsg4 = m_field->GetZero()->Clone(e);
    BC_EXCEPTION_CHECK_ReturnValue(e, NULL);
    CBC_AutoPtr<CBC_ReedSolomonGF256Poly> s(rsg4);
    CBC_ReedSolomonGF256Poly* rsg5 = m_field->GetZero()->Clone(e);
    BC_EXCEPTION_CHECK_ReturnValue(e, NULL);
    CBC_AutoPtr<CBC_ReedSolomonGF256Poly> tLast(rsg5);
    CBC_ReedSolomonGF256Poly* rsg6 = m_field->GetOne()->Clone(e);
    BC_EXCEPTION_CHECK_ReturnValue(e, NULL);
    CBC_AutoPtr<CBC_ReedSolomonGF256Poly> t(rsg6);
    while (r->GetDegree() >= R / 2) {
        CBC_AutoPtr<CBC_ReedSolomonGF256Poly> rLastLast = rLast;
        CBC_AutoPtr<CBC_ReedSolomonGF256Poly> sLastLast = sLast;
        CBC_AutoPtr<CBC_ReedSolomonGF256Poly> tLastlast = tLast;
        rLast = r;
        sLast = s;
        tLast = t;
        if (rLast->IsZero()) {
            e = BCExceptionR_I_1IsZero;
            BC_EXCEPTION_CHECK_ReturnValue(e, NULL);
        }
        CBC_ReedSolomonGF256Poly* rsg7 =  rLastLast->Clone(e);
        BC_EXCEPTION_CHECK_ReturnValue(e, NULL);
        CBC_AutoPtr<CBC_ReedSolomonGF256Poly> rTemp(rsg7);
        r = rTemp;
        CBC_ReedSolomonGF256Poly* rsg8 =  m_field->GetZero()->Clone(e);
        BC_EXCEPTION_CHECK_ReturnValue(e, NULL);
        CBC_AutoPtr<CBC_ReedSolomonGF256Poly> q(rsg8);
        FX_INT32 denominatorLeadingTerm = rLast->GetCoefficients(rLast->GetDegree());
        FX_INT32 dltInverse = m_field->Inverse(denominatorLeadingTerm, e);
        BC_EXCEPTION_CHECK_ReturnValue(e, NULL);
        while (r->GetDegree() >= rLast->GetDegree() && !(r->IsZero())) {
            FX_INT32 degreeDiff = r->GetDegree() - rLast->GetDegree();
            FX_INT32 scale = m_field->Multiply(r->GetCoefficients(r->GetDegree()), dltInverse);
            CBC_ReedSolomonGF256Poly* rsgp1 = m_field->BuildMonomial(degreeDiff, scale, e);
            BC_EXCEPTION_CHECK_ReturnValue(e, NULL);
            CBC_AutoPtr<CBC_ReedSolomonGF256Poly> build(rsgp1);
            CBC_ReedSolomonGF256Poly* rsgp2 = q->AddOrSubtract(build.get(), e);
            BC_EXCEPTION_CHECK_ReturnValue(e, NULL);
            CBC_AutoPtr<CBC_ReedSolomonGF256Poly> temp(rsgp2);
            q = temp;
            CBC_ReedSolomonGF256Poly* rsgp3 = rLast->MultiplyByMonomial(degreeDiff, scale, e);
            BC_EXCEPTION_CHECK_ReturnValue(e, NULL);
            CBC_AutoPtr<CBC_ReedSolomonGF256Poly> multiply(rsgp3);
            CBC_ReedSolomonGF256Poly* rsgp4 = r->AddOrSubtract(multiply.get(), e);
            BC_EXCEPTION_CHECK_ReturnValue(e, NULL);
            CBC_AutoPtr<CBC_ReedSolomonGF256Poly> temp3(rsgp4);
            r = temp3;
        }
        CBC_ReedSolomonGF256Poly* rsg9 = q->Multiply(sLast.get(), e);
        BC_EXCEPTION_CHECK_ReturnValue(e, NULL);
        CBC_AutoPtr<CBC_ReedSolomonGF256Poly> temp1(rsg9);
        CBC_ReedSolomonGF256Poly* rsg10 = temp1->AddOrSubtract(sLastLast.get(), e);
        BC_EXCEPTION_CHECK_ReturnValue(e, NULL);
        CBC_AutoPtr<CBC_ReedSolomonGF256Poly> temp2(rsg10);
        s = temp2;
        CBC_ReedSolomonGF256Poly* rsg11 = q->Multiply(tLast.get(), e);
        BC_EXCEPTION_CHECK_ReturnValue(e, NULL);
        CBC_AutoPtr<CBC_ReedSolomonGF256Poly> temp5(rsg11);
        CBC_ReedSolomonGF256Poly* rsg12 = temp5->AddOrSubtract(tLastlast.get(), e);
        BC_EXCEPTION_CHECK_ReturnValue(e, NULL);
        CBC_AutoPtr<CBC_ReedSolomonGF256Poly> temp6(rsg12);
        t = temp6;
    }
    FX_INT32 sigmaTildeAtZero = t->GetCoefficients(0);
    if (sigmaTildeAtZero == 0) {
        e = BCExceptionIsZero;
        BC_EXCEPTION_CHECK_ReturnValue(e, NULL);
    }
    FX_INT32 inverse = m_field->Inverse(sigmaTildeAtZero, e);
    BC_EXCEPTION_CHECK_ReturnValue(e, NULL);
    CBC_ReedSolomonGF256Poly* rsg13 = t->Multiply(inverse, e);
    BC_EXCEPTION_CHECK_ReturnValue(e, NULL);
    CBC_AutoPtr<CBC_ReedSolomonGF256Poly> sigma(rsg13);
    CBC_ReedSolomonGF256Poly* rsg14 = r->Multiply(inverse, e);
    BC_EXCEPTION_CHECK_ReturnValue(e, NULL);
    CBC_AutoPtr<CBC_ReedSolomonGF256Poly> omega(rsg14);
    CFX_PtrArray *temp = FX_NEW CFX_PtrArray;
    temp->Add(sigma.release());
    temp->Add(omega.release());
    return temp;
}
CFX_Int32Array *CBC_ReedSolomonDecoder::FindErrorLocations(CBC_ReedSolomonGF256Poly* errorLocator, FX_INT32 &e)
{
    FX_INT32 numErrors = errorLocator->GetDegree();
    if (numErrors == 1) {
        CBC_AutoPtr<CFX_Int32Array > temp(FX_NEW CFX_Int32Array);
        temp->Add(errorLocator->GetCoefficients(1));
        return temp.release();
    }
    CFX_Int32Array *tempT = FX_NEW CFX_Int32Array;
    tempT->SetSize(numErrors);
    CBC_AutoPtr<CFX_Int32Array > result(tempT);
    FX_INT32 ie = 0;
    for (FX_INT32 i = 1; i < 256 && ie < numErrors; i++) {
        if(errorLocator->EvaluateAt(i) == 0) {
            (*result)[ie] = m_field->Inverse(i, ie);
            BC_EXCEPTION_CHECK_ReturnValue(e, NULL);
            ie++;
        }
    }
    if (ie != numErrors) {
        e = BCExceptionDegreeNotMatchRoots;
        BC_EXCEPTION_CHECK_ReturnValue(e, NULL);
    }
    return result.release();
}
CFX_Int32Array *CBC_ReedSolomonDecoder::FindErrorMagnitudes(CBC_ReedSolomonGF256Poly* errorEvaluator, CFX_Int32Array* errorLocations, FX_BOOL dataMatrix, FX_INT32 &e)
{
    FX_INT32 s = errorLocations->GetSize();
    CFX_Int32Array * temp = FX_NEW CFX_Int32Array;
    temp->SetSize(s);
    CBC_AutoPtr<CFX_Int32Array > result(temp);
    for (FX_INT32 i = 0; i < s; i++) {
        FX_INT32 xiInverse = m_field->Inverse(errorLocations->operator [](i), e);
        BC_EXCEPTION_CHECK_ReturnValue(e, NULL);
        FX_INT32 denominator = 1;
        for(FX_INT32 j = 0; j < s; j++) {
            if(i != j) {
                denominator = m_field->Multiply(denominator,
                                                CBC_ReedSolomonGF256::AddOrSubtract(1, m_field->Multiply(errorLocations->operator [](j), xiInverse)));
            }
        }
        FX_INT32 temp = m_field->Inverse(denominator, temp);
        BC_EXCEPTION_CHECK_ReturnValue(e, NULL);
        (*result)[i] = m_field->Multiply(errorEvaluator->EvaluateAt(xiInverse),
                                         temp);
    }
    return result.release();
}
