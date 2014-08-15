// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "../fgas_base.h"
#include "fx_unicode.h"
#include "fx_rtfbreak.h"
extern const FX_DWORD gs_FX_TextLayout_CodeProperties[65536];
extern const FX_WCHAR gs_FX_TextLayout_VerticalMirror[64];
extern const FX_WCHAR gs_FX_TextLayout_BidiMirror[512];
extern const FX_LINEBREAKTYPE gs_FX_LineBreak_PairTable[64][32];
IFX_RTFBreak* IFX_RTFBreak::Create(FX_DWORD dwPolicies)
{
    return FX_NEW CFX_RTFBreak(dwPolicies);
}
CFX_RTFBreak::CFX_RTFBreak(FX_DWORD dwPolicies)
    : m_dwPolicies(dwPolicies)
    , m_pArabicChar(NULL)
    , m_iLineStart(0)
    , m_iLineEnd(2000000)
    , m_dwLayoutStyles(0)
    , m_bPagination(FALSE)
    , m_bVertical(FALSE)
    , m_bSingleLine(FALSE)
    , m_bCharCode(FALSE)
    , m_pFont(NULL)
    , m_iFontHeight(240)
    , m_iFontSize(240)
    , m_iTabWidth(720000)
    , m_PositionedTabs()
    , m_bOrphanLine(FALSE)
    , m_wDefChar(0xFEFF)
    , m_iDefChar(0)
    , m_wLineBreakChar(L'\n')
    , m_iHorizontalScale(100)
    , m_iVerticalScale(100)
    , m_iLineRotation(0)
    , m_iCharRotation(0)
    , m_iRotation(0)
    , m_iCharSpace(0)
    , m_bWordSpace(FALSE)
    , m_iWordSpace(0)
    , m_bRTL(FALSE)
    , m_iAlignment(FX_RTFLINEALIGNMENT_Left)
    , m_pUserData(NULL)
    , m_dwCharType(0)
    , m_dwIdentity(0)
    , m_RTFLine1()
    , m_RTFLine2()
    , m_pCurLine(NULL)
    , m_iReady(0)
    , m_iTolerance(0)
{
    m_pArabicChar = IFX_ArabicChar::Create();
    m_pCurLine = &m_RTFLine1;
}
CFX_RTFBreak::~CFX_RTFBreak()
{
    Reset();
    m_PositionedTabs.RemoveAll();
    m_pArabicChar->Release();
    if (m_pUserData != NULL) {
        m_pUserData->Release();
    }
}
void CFX_RTFBreak::SetLineWidth(FX_FLOAT fLineStart, FX_FLOAT fLineEnd)
{
    m_iLineStart = FXSYS_round(fLineStart * 20000.0f);
    m_iLineEnd = FXSYS_round(fLineEnd * 20000.0f);
    FXSYS_assert(m_iLineEnd >= m_iLineStart);
    if (m_pCurLine->m_iStart < m_iLineStart) {
        m_pCurLine->m_iStart = m_iLineStart;
    }
}
void CFX_RTFBreak::SetLinePos(FX_FLOAT fLinePos)
{
    FX_INT32 iLinePos = FXSYS_round(fLinePos * 20000.0f);
    if (iLinePos > m_iLineEnd) {
        iLinePos = m_iLineEnd;
    }
    m_pCurLine->m_iStart = iLinePos;
}
void CFX_RTFBreak::SetLayoutStyles(FX_DWORD dwLayoutStyles)
{
    if (m_dwLayoutStyles == dwLayoutStyles) {
        return;
    }
    SetBreakStatus();
    m_dwLayoutStyles = dwLayoutStyles;
    m_bPagination = (m_dwLayoutStyles & FX_RTFLAYOUTSTYLE_Pagination) != 0;
    m_bVertical = (m_dwLayoutStyles & FX_RTFLAYOUTSTYLE_VerticalChars) != 0;
    m_bSingleLine = (m_dwLayoutStyles & FX_RTFLAYOUTSTYLE_SingleLine) != 0;
    m_bCharCode = (m_dwLayoutStyles & FX_RTFLAYOUTSTYLE_MBCSCode) != 0;
    m_iLineRotation = GetLineRotation(m_dwLayoutStyles);
    m_iRotation = m_iLineRotation + m_iCharRotation;
    m_iRotation %= 4;
}
void CFX_RTFBreak::SetFont(IFX_Font *pFont)
{
    if (pFont == NULL) {
        return;
    }
    if (m_pFont == pFont) {
        return;
    }
    SetBreakStatus();
    m_pFont = pFont;
    m_iDefChar = 0;
    if (m_pFont != NULL) {
        m_iFontHeight = m_iFontSize;
        if (m_wDefChar != 0xFEFF) {
            m_pFont->GetCharWidth(m_wDefChar, m_iDefChar, FALSE);
            m_iDefChar *= m_iFontSize;
        }
    }
}
void CFX_RTFBreak::SetFontSize(FX_FLOAT fFontSize)
{
    FX_INT32 iFontSize = FXSYS_round(fFontSize * 20.0f);
    if (m_iFontSize == iFontSize) {
        return;
    }
    SetBreakStatus();
    m_iFontSize = iFontSize;
    m_iDefChar = 0;
    if (m_pFont != NULL) {
        m_iFontHeight = m_iFontSize;
        if (m_wDefChar != 0xFEFF) {
            m_pFont->GetCharWidth(m_wDefChar, m_iDefChar, FALSE);
            m_iDefChar *= m_iFontSize;
        }
    }
}
void CFX_RTFBreak::SetTabWidth(FX_FLOAT fTabWidth)
{
    m_iTabWidth = FXSYS_round(fTabWidth * 20000.0f);
}
void CFX_RTFBreak::AddPositionedTab(FX_FLOAT fTabPos)
{
    FX_INT32 iLineEnd = m_iLineEnd;
    FX_INT32 iTabPos = FXSYS_round(fTabPos * 20000.0f) + m_iLineStart;
    if (iTabPos > iLineEnd) {
        iTabPos = iLineEnd;
    }
    if (m_PositionedTabs.Find(iTabPos, 0) > -1) {
        return;
    }
    FX_INT32 iCount = m_PositionedTabs.GetSize();
    FX_INT32 iFind = 0;
    for (; iFind < iCount; iFind ++) {
        if (m_PositionedTabs[iFind] > iTabPos) {
            break;
        }
    }
    m_PositionedTabs.InsertAt(iFind, iTabPos);
    if (m_dwPolicies & FX_RTFBREAKPOLICY_OrphanPositionedTab) {
        m_bOrphanLine = GetLastPositionedTab() >= iLineEnd;
    } else {
        m_bOrphanLine = FALSE;
    }
}
void CFX_RTFBreak::SetPositionedTabs(const CFX_FloatArray &tabs)
{
    m_PositionedTabs.RemoveAll();
    FX_INT32 iCount = tabs.GetSize();
    m_PositionedTabs.SetSize(iCount);
    FX_INT32 iLineEnd = m_iLineEnd;
    FX_INT32 iTabPos;
    for (FX_INT32 i = 0; i < iCount; i ++) {
        iTabPos = FXSYS_round(tabs[i] * 20000.0f) + m_iLineStart;
        if (iTabPos > iLineEnd) {
            iTabPos = iLineEnd;
        }
        m_PositionedTabs[i] = iTabPos;
    }
    if (m_dwPolicies & FX_RTFBREAKPOLICY_OrphanPositionedTab) {
        m_bOrphanLine = GetLastPositionedTab() >= iLineEnd;
    } else {
        m_bOrphanLine = FALSE;
    }
}
void CFX_RTFBreak::ClearPositionedTabs()
{
    m_PositionedTabs.RemoveAll();
    m_bOrphanLine = FALSE;
}
void CFX_RTFBreak::SetDefaultChar(FX_WCHAR wch)
{
    m_wDefChar = wch;
    m_iDefChar = 0;
    if (m_wDefChar != 0xFEFF && m_pFont != NULL) {
        m_pFont->GetCharWidth(m_wDefChar, m_iDefChar, FALSE);
        if (m_iDefChar < 0) {
            m_iDefChar = 0;
        } else {
            m_iDefChar *= m_iFontSize;
        }
    }
}
void CFX_RTFBreak::SetLineBreakChar(FX_WCHAR wch)
{
    if (wch != L'\r' && wch != L'\n') {
        return;
    }
    m_wLineBreakChar = wch;
}
void CFX_RTFBreak::SetLineBreakTolerance(FX_FLOAT fTolerance)
{
    m_iTolerance = FXSYS_round(fTolerance * 20000.0f);
}
void CFX_RTFBreak::SetHorizontalScale(FX_INT32 iScale)
{
    if (iScale < 0) {
        iScale = 0;
    }
    if (m_iHorizontalScale == iScale) {
        return;
    }
    SetBreakStatus();
    m_iHorizontalScale = iScale;
}
void CFX_RTFBreak::SetVerticalScale(FX_INT32 iScale)
{
    if (iScale < 0) {
        iScale = 0;
    }
    if (m_iVerticalScale == iScale) {
        return;
    }
    SetBreakStatus();
    m_iVerticalScale = iScale;
}
void CFX_RTFBreak::SetCharRotation(FX_INT32 iCharRotation)
{
    if (iCharRotation < 0) {
        iCharRotation += (-iCharRotation / 4 + 1) * 4;
    } else if (iCharRotation > 3) {
        iCharRotation -= (iCharRotation / 4) * 4;
    }
    if (m_iCharRotation == iCharRotation) {
        return;
    }
    SetBreakStatus();
    m_iCharRotation = iCharRotation;
    m_iRotation = m_iLineRotation + m_iCharRotation;
    m_iRotation %= 4;
}
void CFX_RTFBreak::SetCharSpace(FX_FLOAT fCharSpace)
{
    m_iCharSpace = FXSYS_round(fCharSpace * 20000.0f);
}
void CFX_RTFBreak::SetWordSpace(FX_BOOL bDefault, FX_FLOAT fWordSpace)
{
    m_bWordSpace = !bDefault;
    m_iWordSpace = FXSYS_round(fWordSpace * 20000.0f);
}
void CFX_RTFBreak::SetReadingOrder(FX_BOOL bRTL)
{
    m_bRTL = bRTL;
}
void CFX_RTFBreak::SetAlignment(FX_INT32 iAlignment)
{
    FXSYS_assert(iAlignment >= FX_RTFLINEALIGNMENT_Left && iAlignment <= FX_RTFLINEALIGNMENT_Distributed);
    m_iAlignment = iAlignment;
}
void CFX_RTFBreak::SetUserData(IFX_Unknown *pUserData)
{
    if (m_pUserData == pUserData) {
        return;
    }
    SetBreakStatus();
    if (m_pUserData != NULL) {
        m_pUserData->Release();
    }
    m_pUserData = pUserData;
    if (m_pUserData != NULL) {
        m_pUserData->AddRef();
    }
}
static const FX_INT32 gs_FX_RTFLineRotations[8] = {0, 3, 1, 0, 2, 1, 3, 2};
FX_INT32 CFX_RTFBreak::GetLineRotation(FX_DWORD dwStyles) const
{
    return gs_FX_RTFLineRotations[(dwStyles & 0x0E) >> 1];
}
void CFX_RTFBreak::SetBreakStatus()
{
    m_dwIdentity ++;
    FX_INT32 iCount = m_pCurLine->CountChars();
    if (iCount < 1) {
        return;
    }
    CFX_RTFChar &tc = m_pCurLine->GetChar(iCount - 1);
    if (tc.m_dwStatus == 0) {
        tc.m_dwStatus = FX_RTFBREAK_PieceBreak;
    }
}
CFX_RTFChar* CFX_RTFBreak::GetLastChar(FX_INT32 index) const
{
    CFX_RTFCharArray &tca = m_pCurLine->m_LineChars;
    FX_INT32 iCount = tca.GetSize();
    if (index < 0 || index >= iCount) {
        return NULL;
    }
    CFX_RTFChar *pTC;
    FX_INT32 iStart = iCount - 1;
    while (iStart > -1) {
        pTC = tca.GetDataPtr(iStart --);
        if (pTC->m_iCharWidth >= 0 || pTC->GetCharType() != FX_CHARTYPE_Combination) {
            if (--index < 0) {
                return pTC;
            }
        }
    }
    return NULL;
}
CFX_RTFLine* CFX_RTFBreak::GetRTFLine(FX_BOOL bReady) const
{
    if (bReady) {
        if (m_iReady == 1) {
            return (CFX_RTFLine*)&m_RTFLine1;
        } else if (m_iReady == 2) {
            return (CFX_RTFLine*)&m_RTFLine2;
        } else {
            return NULL;
        }
    }
    FXSYS_assert(m_pCurLine != NULL);
    return m_pCurLine;
}
CFX_RTFPieceArray* CFX_RTFBreak::GetRTFPieces(FX_BOOL bReady) const
{
    CFX_RTFLine *pRTFLine = GetRTFLine(bReady);
    if (pRTFLine == NULL) {
        return NULL;
    }
    return &pRTFLine->m_LinePieces;
}
inline FX_DWORD CFX_RTFBreak::GetUnifiedCharType(FX_DWORD dwType) const
{
    return dwType >= FX_CHARTYPE_ArabicAlef ? FX_CHARTYPE_Arabic : dwType;
}
FX_INT32 CFX_RTFBreak::GetLastPositionedTab() const
{
    FX_INT32 iCount = m_PositionedTabs.GetSize();
    if (iCount < 1) {
        return m_iLineStart;
    }
    return m_PositionedTabs[iCount - 1];
}
FX_BOOL CFX_RTFBreak::GetPositionedTab(FX_INT32 &iTabPos) const
{
    FX_INT32 iCount = m_PositionedTabs.GetSize();
    for (FX_INT32 i = 0; i < iCount; i ++) {
        if (m_PositionedTabs[i] > iTabPos) {
            iTabPos = m_PositionedTabs[i];
            return TRUE;
        }
    }
    return FALSE;
}
typedef FX_DWORD (CFX_RTFBreak::*FX_RTFBreak_LPFAppendChar)(CFX_RTFChar *pCurChar, FX_INT32 iRotation);
static const FX_RTFBreak_LPFAppendChar g_FX_RTFBreak_lpfAppendChar[16] = {
    &CFX_RTFBreak::AppendChar_Others,
    &CFX_RTFBreak::AppendChar_Tab,
    &CFX_RTFBreak::AppendChar_Others,
    &CFX_RTFBreak::AppendChar_Control,
    &CFX_RTFBreak::AppendChar_Combination,
    &CFX_RTFBreak::AppendChar_Others,
    &CFX_RTFBreak::AppendChar_Others,
    &CFX_RTFBreak::AppendChar_Arabic,
    &CFX_RTFBreak::AppendChar_Arabic,
    &CFX_RTFBreak::AppendChar_Arabic,
    &CFX_RTFBreak::AppendChar_Arabic,
    &CFX_RTFBreak::AppendChar_Arabic,
    &CFX_RTFBreak::AppendChar_Arabic,
    &CFX_RTFBreak::AppendChar_Others,
    &CFX_RTFBreak::AppendChar_Others,
    &CFX_RTFBreak::AppendChar_Others,
};
FX_DWORD CFX_RTFBreak::AppendChar(FX_WCHAR wch)
{
    FXSYS_assert(m_pFont != NULL && m_pCurLine != NULL && m_pArabicChar != NULL);
    if (m_bCharCode) {
        return AppendChar_CharCode(wch);
    }
    FX_DWORD dwProps = gs_FX_TextLayout_CodeProperties[(FX_WORD)wch];
    FX_DWORD dwType = (dwProps & FX_CHARTYPEBITSMASK);
    CFX_RTFCharArray &tca = m_pCurLine->m_LineChars;
    CFX_RTFChar *pCurChar = tca.AddSpace();
    pCurChar->m_dwStatus = 0;
    pCurChar->m_wCharCode = wch;
    pCurChar->m_dwCharProps = dwProps;
    pCurChar->m_dwCharStyles = 0;
    pCurChar->m_dwLayoutStyles = 0;
    pCurChar->m_iFontSize = m_iFontSize;
    pCurChar->m_iFontHeight = m_iFontHeight;
    pCurChar->m_iHorizontalScale = m_iHorizontalScale;
    pCurChar->m_iVertialScale = m_iVerticalScale;
    pCurChar->m_nRotation = m_iCharRotation;
    pCurChar->m_iCharWidth = 0;
    pCurChar->m_dwIdentity = m_dwIdentity;
    if (m_pUserData != NULL) {
        m_pUserData->AddRef();
    }
    pCurChar->m_pUserData = m_pUserData;
    FX_DWORD dwRet1 = FX_RTFBREAK_None;
    if (dwType != FX_CHARTYPE_Combination && GetUnifiedCharType(m_dwCharType) != GetUnifiedCharType(dwType)) {
        if (!m_bSingleLine && !m_bOrphanLine && m_dwCharType > 0 && m_pCurLine->GetLineEnd() > m_iLineEnd + m_iTolerance) {
            if (m_dwCharType != FX_CHARTYPE_Space || dwType != FX_CHARTYPE_Control) {
                dwRet1 = EndBreak(FX_RTFBREAK_LineBreak);
                FX_INT32 iCount = m_pCurLine->CountChars();
                if (iCount > 0) {
                    pCurChar = m_pCurLine->m_LineChars.GetDataPtr(iCount - 1);
                }
            }
        }
    }
    FX_INT32 iRotation = m_iRotation;
    if (m_bVertical && (dwProps & 0x8000) != 0) {
        iRotation = (iRotation + 1) % 4;
    }
    FX_DWORD dwRet2 = (this->*g_FX_RTFBreak_lpfAppendChar[dwType >> FX_CHARTYPEBITS])(pCurChar, iRotation);
    m_dwCharType = dwType;
    return FX_MAX(dwRet1, dwRet2);
}
FX_DWORD CFX_RTFBreak::AppendChar_CharCode(FX_WCHAR wch)
{
    FXSYS_assert(m_pFont != NULL && m_pCurLine != NULL);
    FXSYS_assert(m_bCharCode);
    m_pCurLine->m_iMBCSChars ++;
    CFX_RTFCharArray &tca = m_pCurLine->m_LineChars;
    CFX_RTFChar *pCurChar = tca.AddSpace();
    pCurChar->m_dwStatus = 0;
    pCurChar->m_wCharCode = wch;
    pCurChar->m_dwCharProps = 0;
    pCurChar->m_dwCharStyles = 0;
    pCurChar->m_dwLayoutStyles = m_dwLayoutStyles;
    pCurChar->m_iFontSize = m_iFontSize;
    pCurChar->m_iFontHeight = m_iFontHeight;
    pCurChar->m_iHorizontalScale = m_iHorizontalScale;
    pCurChar->m_iVertialScale = m_iVerticalScale;
    pCurChar->m_nRotation = m_iCharRotation;
    pCurChar->m_iCharWidth = 0;
    pCurChar->m_dwIdentity = m_dwIdentity;
    if (m_pUserData != NULL) {
        m_pUserData->AddRef();
    }
    pCurChar->m_pUserData = m_pUserData;
    FX_INT32 iCharWidth = 0;
    if (m_bVertical != FX_IsOdd(m_iRotation)) {
        iCharWidth = 1000;
    } else {
        if (!m_pFont->GetCharWidth(wch, iCharWidth, TRUE)) {
            iCharWidth = m_iDefChar;
        }
    }
    iCharWidth *= m_iFontSize;
    iCharWidth = iCharWidth * m_iHorizontalScale / 100;
    iCharWidth += m_iCharSpace;
    pCurChar->m_iCharWidth = iCharWidth;
    m_pCurLine->m_iWidth += iCharWidth;
    m_dwCharType = 0;
    if (!m_bSingleLine && m_pCurLine->GetLineEnd() > m_iLineEnd + m_iTolerance) {
        return EndBreak(FX_RTFBREAK_LineBreak);
    }
    return FX_RTFBREAK_None;
}
FX_DWORD CFX_RTFBreak::AppendChar_Combination(CFX_RTFChar *pCurChar, FX_INT32 iRotation)
{
    FX_INT32 iCharWidth = 0;
    if (m_bVertical != FX_IsOdd(iRotation)) {
        iCharWidth = 1000;
    } else {
        if (!m_pFont->GetCharWidth(pCurChar->m_wCharCode, iCharWidth, m_bCharCode)) {
            iCharWidth = 0;
        }
    }
    iCharWidth *= m_iFontSize;
    iCharWidth = iCharWidth * m_iHorizontalScale / 100;
    CFX_RTFChar *pLastChar = GetLastChar(0);
    if (pLastChar != NULL && pLastChar->GetCharType() > FX_CHARTYPE_Combination) {
        iCharWidth = -iCharWidth;
    } else {
        m_dwCharType = FX_CHARTYPE_Combination;
    }
    pCurChar->m_iCharWidth = iCharWidth;
    if (iCharWidth > 0) {
        m_pCurLine->m_iWidth += iCharWidth;
    }
    return FX_RTFBREAK_None;
}
FX_DWORD CFX_RTFBreak::AppendChar_Tab(CFX_RTFChar *pCurChar, FX_INT32 iRotation)
{
    if (m_dwLayoutStyles & FX_RTFLAYOUTSTYLE_ExpandTab) {
        FX_BOOL bBreak = FALSE;
        if ((m_dwPolicies & FX_RTFBREAKPOLICY_TabBreak) != 0) {
            bBreak = (m_pCurLine->GetLineEnd() > m_iLineEnd + m_iTolerance);
        }
        FX_INT32 &iLineWidth = m_pCurLine->m_iWidth;
        FX_INT32 iCharWidth = iLineWidth;
        if (GetPositionedTab(iCharWidth)) {
            iCharWidth -= iLineWidth;
        } else {
            iCharWidth = m_iTabWidth * (iLineWidth / m_iTabWidth + 1) - iLineWidth;
        }
        pCurChar->m_iCharWidth = iCharWidth;
        iLineWidth += iCharWidth;
        if (!m_bSingleLine && !m_bOrphanLine && bBreak) {
            return EndBreak(FX_RTFBREAK_LineBreak);
        }
    }
    return FX_RTFBREAK_None;
}
FX_DWORD CFX_RTFBreak::AppendChar_Control(CFX_RTFChar *pCurChar, FX_INT32 iRotation)
{
    FX_DWORD dwRet2 = FX_RTFBREAK_None;
    if (!m_bSingleLine) {
        switch (pCurChar->m_wCharCode) {
            case L'\v':
            case 0x2028:
                dwRet2 = FX_RTFBREAK_LineBreak;
                break;
            case L'\f':
                dwRet2 = FX_RTFBREAK_PageBreak;
                break;
            case 0x2029:
                dwRet2 = FX_RTFBREAK_ParagraphBreak;
                break;
            default:
                if (pCurChar->m_wCharCode == m_wLineBreakChar) {
                    dwRet2 = FX_RTFBREAK_ParagraphBreak;
                }
                break;
        }
        if (dwRet2 != FX_RTFBREAK_None) {
            dwRet2 = EndBreak(dwRet2);
        }
    }
    return dwRet2;
}
FX_DWORD CFX_RTFBreak::AppendChar_Arabic(CFX_RTFChar *pCurChar, FX_INT32 iRotation)
{
    CFX_RTFChar *pLastChar = NULL;
    FX_INT32 &iLineWidth = m_pCurLine->m_iWidth;
    FX_INT32 iCharWidth = 0;
    FX_WCHAR wForm;
    FX_BOOL bAlef = FALSE;
    if (m_dwCharType >= FX_CHARTYPE_ArabicAlef && m_dwCharType <= FX_CHARTYPE_ArabicDistortion) {
        pLastChar = GetLastChar(1);
        if (pLastChar != NULL) {
            iLineWidth -= pLastChar->m_iCharWidth;
            CFX_RTFChar *pPrevChar = GetLastChar(2);
            wForm = m_pArabicChar->GetFormChar(pLastChar, pPrevChar, pCurChar);
            bAlef = (wForm == 0xFEFF && pLastChar->GetCharType() == FX_CHARTYPE_ArabicAlef);
            FX_INT32 iLastRotation = pLastChar->m_nRotation + m_iLineRotation;
            if (m_bVertical && (pLastChar->m_dwCharProps & 0x8000) != 0) {
                iLastRotation ++;
            }
            if (m_bVertical != FX_IsOdd(iLastRotation)) {
                iCharWidth = 1000;
            } else {
                if (!m_pFont->GetCharWidth(wForm, iCharWidth, m_bCharCode))
                    if (!m_pFont->GetCharWidth(pLastChar->m_wCharCode, iCharWidth, m_bCharCode)) {
                        iCharWidth = m_iDefChar;
                    }
            }
            iCharWidth *= m_iFontSize;
            iCharWidth = iCharWidth * m_iHorizontalScale / 100;
            pLastChar->m_iCharWidth = iCharWidth;
            iLineWidth += iCharWidth;
            iCharWidth = 0;
        }
    }
    wForm = m_pArabicChar->GetFormChar(pCurChar, (bAlef ? NULL : pLastChar), NULL);
    if (m_bVertical != FX_IsOdd(iRotation)) {
        iCharWidth = 1000;
    } else {
        if (!m_pFont->GetCharWidth(wForm, iCharWidth, m_bCharCode))
            if (!m_pFont->GetCharWidth(pCurChar->m_wCharCode, iCharWidth, m_bCharCode)) {
                iCharWidth = m_iDefChar;
            }
    }
    iCharWidth *= m_iFontSize;
    iCharWidth = iCharWidth * m_iHorizontalScale / 100;
    pCurChar->m_iCharWidth = iCharWidth;
    iLineWidth += iCharWidth;
    m_pCurLine->m_iArabicChars ++;
    if (!m_bSingleLine && !m_bOrphanLine && m_pCurLine->GetLineEnd() > m_iLineEnd + m_iTolerance) {
        return EndBreak(FX_RTFBREAK_LineBreak);
    }
    return FX_RTFBREAK_None;
}
FX_DWORD CFX_RTFBreak::AppendChar_Others(CFX_RTFChar *pCurChar, FX_INT32 iRotation)
{
    FX_DWORD dwType = (pCurChar->m_dwCharProps & FX_CHARTYPEBITSMASK);
    FX_WCHAR wForm;
    if (dwType == FX_CHARTYPE_Numeric) {
        if (m_dwLayoutStyles & FX_RTFLAYOUTSTYLE_ArabicNumber) {
            wForm = pCurChar->m_wCharCode + 0x0630;
        } else {
            wForm = pCurChar->m_wCharCode;
        }
    } else if (m_bRTL || m_bVertical) {
        wForm = FX_GetMirrorChar(pCurChar->m_wCharCode, pCurChar->m_dwCharProps, m_bRTL, m_bVertical);
    } else {
        wForm = pCurChar->m_wCharCode;
    }
    FX_INT32 iCharWidth = 0;
    if (m_bVertical != FX_IsOdd(iRotation)) {
        iCharWidth = 1000;
    } else {
        if (!m_pFont->GetCharWidth(wForm, iCharWidth, m_bCharCode)) {
            iCharWidth = m_iDefChar;
        }
    }
    iCharWidth *= m_iFontSize;
    iCharWidth = iCharWidth * m_iHorizontalScale / 100;
    iCharWidth += m_iCharSpace;
    if (dwType == FX_CHARTYPE_Space && m_bWordSpace) {
        iCharWidth += m_iWordSpace;
    }
    pCurChar->m_iCharWidth = iCharWidth;
    m_pCurLine->m_iWidth += iCharWidth;
    FX_BOOL bBreak = (dwType != FX_CHARTYPE_Space || (m_dwPolicies & FX_RTFBREAKPOLICY_SpaceBreak) != 0);
    if (!m_bSingleLine && !m_bOrphanLine && bBreak && m_pCurLine->GetLineEnd() > m_iLineEnd + m_iTolerance) {
        return EndBreak(FX_RTFBREAK_LineBreak);
    }
    return FX_RTFBREAK_None;
}
FX_DWORD CFX_RTFBreak::EndBreak(FX_DWORD dwStatus)
{
    FXSYS_assert(dwStatus >= FX_RTFBREAK_PieceBreak && dwStatus <= FX_RTFBREAK_PageBreak);
    m_dwIdentity ++;
    CFX_RTFPieceArray *pCurPieces = &m_pCurLine->m_LinePieces;
    FX_INT32 iCount = pCurPieces->GetSize();
    if (iCount > 0) {
        CFX_RTFPiece *pLastPiece = pCurPieces->GetPtrAt(-- iCount);
        if (dwStatus > FX_RTFBREAK_PieceBreak) {
            pLastPiece->m_dwStatus = dwStatus;
        } else {
            dwStatus = pLastPiece->m_dwStatus;
        }
        return dwStatus;
    } else {
        CFX_RTFLine *pLastLine = GetRTFLine(TRUE);
        if (pLastLine != NULL) {
            pCurPieces = &pLastLine->m_LinePieces;
            iCount = pCurPieces->GetSize();
            if (iCount -- > 0) {
                CFX_RTFPiece *pLastPiece = pCurPieces->GetPtrAt(iCount);
                if (dwStatus > FX_RTFBREAK_PieceBreak) {
                    pLastPiece->m_dwStatus = dwStatus;
                } else {
                    dwStatus = pLastPiece->m_dwStatus;
                }
                return dwStatus;
            }
            return FX_RTFBREAK_None;
        }
        iCount = m_pCurLine->CountChars();
        if (iCount < 1) {
            return FX_RTFBREAK_None;
        }
        CFX_RTFChar &tc = m_pCurLine->GetChar(iCount - 1);
        tc.m_dwStatus = dwStatus;
        if (dwStatus <= FX_RTFBREAK_PieceBreak) {
            return dwStatus;
        }
    }
    m_iReady = (m_pCurLine == &m_RTFLine1) ? 1 : 2;
    CFX_RTFLine *pNextLine = (m_pCurLine == &m_RTFLine1) ? &m_RTFLine2 : &m_RTFLine1;
    FX_BOOL bAllChars = (m_iAlignment > FX_RTFLINEALIGNMENT_Right);
    CFX_TPOArray tpos;
    if (EndBreak_SplitLine(pNextLine, bAllChars, dwStatus)) {
        goto EndBreak_Ret;
    }
    if (!m_bCharCode) {
        EndBreak_BidiLine(tpos, dwStatus);
    }
    if (!m_bPagination && m_iAlignment > FX_RTFLINEALIGNMENT_Left) {
        EndBreak_Alignment(tpos, bAllChars, dwStatus);
    }
EndBreak_Ret:
    m_pCurLine = pNextLine;
    m_pCurLine->m_iStart = m_iLineStart;
    CFX_RTFChar *pTC = GetLastChar(0);
    m_dwCharType = pTC == NULL ? 0 : pTC->GetCharType();
    return dwStatus;
}
FX_BOOL CFX_RTFBreak::EndBreak_SplitLine(CFX_RTFLine *pNextLine, FX_BOOL bAllChars, FX_DWORD dwStatus)
{
    FX_BOOL bDone = FALSE;
    if (!m_bSingleLine && !m_bOrphanLine && m_pCurLine->GetLineEnd() > m_iLineEnd + m_iTolerance) {
        CFX_RTFChar &tc = m_pCurLine->GetChar(m_pCurLine->CountChars() - 1);
        switch (tc.GetCharType()) {
            case FX_CHARTYPE_Tab:
                if ((m_dwPolicies & FX_RTFBREAKPOLICY_TabBreak) != 0) {
                    SplitTextLine(m_pCurLine, pNextLine, !m_bPagination && bAllChars);
                    bDone = TRUE;
                }
                break;
            case FX_CHARTYPE_Control:
                break;
            case FX_CHARTYPE_Space:
                if ((m_dwPolicies & FX_RTFBREAKPOLICY_SpaceBreak) != 0) {
                    SplitTextLine(m_pCurLine, pNextLine, !m_bPagination && bAllChars);
                    bDone = TRUE;
                }
                break;
            default:
                SplitTextLine(m_pCurLine, pNextLine, !m_bPagination && bAllChars);
                bDone = TRUE;
                break;
        }
    }
    if (m_bPagination || m_pCurLine->m_iMBCSChars > 0) {
        const CFX_RTFChar *pCurChars = m_pCurLine->m_LineChars.GetData();
        const CFX_RTFChar *pTC;
        CFX_RTFPieceArray *pCurPieces = &m_pCurLine->m_LinePieces;
        CFX_RTFPiece tp;
        tp.m_pChars = &m_pCurLine->m_LineChars;
        FX_BOOL bNew = TRUE;
        FX_DWORD dwIdentity = (FX_DWORD) - 1;
        FX_INT32 iLast = m_pCurLine->CountChars() - 1, j = 0;
        for (FX_INT32 i = 0; i <= iLast;) {
            pTC = pCurChars + i;
            if (bNew) {
                tp.m_iStartChar = i;
                tp.m_iStartPos += tp.m_iWidth;
                tp.m_iWidth = 0;
                tp.m_dwStatus = pTC->m_dwStatus;
                tp.m_iFontSize = pTC->m_iFontSize;
                tp.m_iFontHeight = pTC->m_iFontHeight;
                tp.m_iHorizontalScale = pTC->m_iHorizontalScale;
                tp.m_iVerticalScale = pTC->m_iVertialScale;
                tp.m_dwLayoutStyles = pTC->m_dwLayoutStyles;
                dwIdentity = pTC->m_dwIdentity;
                tp.m_dwIdentity = dwIdentity;
                tp.m_pUserData = pTC->m_pUserData;
                j = i;
                bNew = FALSE;
            }
            if (i == iLast || pTC->m_dwStatus != FX_RTFBREAK_None || pTC->m_dwIdentity != dwIdentity) {
                tp.m_iChars = i - j;
                if (pTC->m_dwIdentity == dwIdentity) {
                    tp.m_dwStatus = pTC->m_dwStatus;
                    tp.m_iWidth += pTC->m_iCharWidth;
                    tp.m_iChars += 1;
                    i ++;
                }
                pCurPieces->Add(tp);
                bNew = TRUE;
            } else {
                tp.m_iWidth += pTC->m_iCharWidth;
                i ++;
            }
        }
        return TRUE;
    }
    if (bAllChars && !bDone) {
        FX_INT32 iEndPos = m_pCurLine->GetLineEnd();
        GetBreakPos(m_pCurLine->m_LineChars, iEndPos, bAllChars, TRUE);
    }
    return FALSE;
}
void CFX_RTFBreak::EndBreak_BidiLine(CFX_TPOArray &tpos, FX_DWORD dwStatus)
{
    FX_TPO tpo;
    CFX_RTFPiece tp;
    CFX_RTFChar *pTC;
    FX_INT32 i, j;
    CFX_RTFCharArray &chars = m_pCurLine->m_LineChars;
    FX_INT32 iCount = m_pCurLine->CountChars();
    FX_BOOL bDone = (!m_bPagination && !m_bCharCode && (m_pCurLine->m_iArabicChars > 0 || m_bRTL));
    if (bDone) {
        FX_INT32 iBidiNum = 0;
        for (i = 0; i < iCount; i ++) {
            pTC = chars.GetDataPtr(i);
            pTC->m_iBidiPos = i;
            if (pTC->GetCharType() != FX_CHARTYPE_Control) {
                iBidiNum = i;
            }
            if (i == 0) {
                pTC->m_iBidiLevel = 1;
            }
        }
        FX_BidiLine(chars, iBidiNum + 1, m_bRTL ? 1 : 0);
    } else {
        for (i = 0; i < iCount; i ++) {
            pTC = chars.GetDataPtr(i);
            pTC->m_iBidiLevel = 0;
            pTC->m_iBidiPos = 0;
            pTC->m_iBidiOrder = 0;
        }
    }
    tp.m_dwStatus = FX_RTFBREAK_PieceBreak;
    tp.m_iStartPos = m_pCurLine->m_iStart;
    tp.m_pChars = &chars;
    CFX_RTFPieceArray *pCurPieces = &m_pCurLine->m_LinePieces;
    FX_INT32 iBidiLevel = -1, iCharWidth;
    FX_DWORD dwIdentity = (FX_DWORD) - 1;
    i = j = 0;
    while (i < iCount) {
        pTC = chars.GetDataPtr(i);
        if (iBidiLevel < 0) {
            iBidiLevel = pTC->m_iBidiLevel;
            iCharWidth = pTC->m_iCharWidth;
            if (iCharWidth < 1) {
                tp.m_iWidth = 0;
            } else {
                tp.m_iWidth = iCharWidth;
            }
            tp.m_iBidiLevel = iBidiLevel;
            tp.m_iBidiPos = pTC->m_iBidiOrder;
            tp.m_iFontSize = pTC->m_iFontSize;
            tp.m_iFontHeight = pTC->m_iFontHeight;
            tp.m_iHorizontalScale = pTC->m_iHorizontalScale;
            tp.m_iVerticalScale = pTC->m_iVertialScale;
            dwIdentity = pTC->m_dwIdentity;
            tp.m_dwIdentity = dwIdentity;
            tp.m_pUserData = pTC->m_pUserData;
            tp.m_dwStatus = FX_RTFBREAK_PieceBreak;
            i ++;
        } else if (iBidiLevel != pTC->m_iBidiLevel || pTC->m_dwIdentity != dwIdentity) {
            tp.m_iChars = i - tp.m_iStartChar;
            pCurPieces->Add(tp);
            tp.m_iStartPos += tp.m_iWidth;
            tp.m_iStartChar = i;
            tpo.index = j ++;
            tpo.pos = tp.m_iBidiPos;
            tpos.Add(tpo);
            iBidiLevel = -1;
        } else {
            iCharWidth = pTC->m_iCharWidth;
            if (iCharWidth > 0) {
                tp.m_iWidth += iCharWidth;
            }
            i ++;
        }
    }
    if (i > tp.m_iStartChar) {
        tp.m_dwStatus = dwStatus;
        tp.m_iChars = i - tp.m_iStartChar;
        pCurPieces->Add(tp);
        tpo.index = j;
        tpo.pos = tp.m_iBidiPos;
        tpos.Add(tpo);
    }
    if (!m_bCharCode) {
        j = tpos.GetSize() - 1;
        FX_TEXTLAYOUT_PieceSort(tpos, 0, j);
        FX_INT32 iStartPos = m_pCurLine->m_iStart;
        for (i = 0; i <= j; i ++) {
            tpo = tpos.GetAt(i);
            CFX_RTFPiece &ttp = pCurPieces->GetAt(tpo.index);
            ttp.m_iStartPos = iStartPos;
            iStartPos += ttp.m_iWidth;
        }
    }
}
void CFX_RTFBreak::EndBreak_Alignment(CFX_TPOArray &tpos, FX_BOOL bAllChars, FX_DWORD dwStatus)
{
    CFX_RTFPieceArray *pCurPieces = &m_pCurLine->m_LinePieces;
    FX_INT32 iNetWidth = m_pCurLine->m_iWidth, iGapChars = 0, iCharWidth;
    FX_INT32 iCount = pCurPieces->GetSize();
    FX_BOOL bFind = FALSE;
    FX_DWORD dwCharType;
    FX_INT32 i, j;
    FX_TPO tpo;
    for (i = iCount - 1; i > -1; i --) {
        tpo = tpos.GetAt(i);
        CFX_RTFPiece &ttp = pCurPieces->GetAt(tpo.index);
        if (!bFind) {
            iNetWidth = ttp.GetEndPos();
        }
        FX_BOOL bArabic = FX_IsOdd(ttp.m_iBidiLevel);
        j = bArabic ? 0 : ttp.m_iChars - 1;
        while (j > -1 && j < ttp.m_iChars) {
            const CFX_RTFChar &tc = ttp.GetChar(j);
            if (tc.m_nBreakType == FX_LBT_DIRECT_BRK) {
                iGapChars ++;
            }
            if (!bFind || !bAllChars) {
                dwCharType = tc.GetCharType();
                if (dwCharType == FX_CHARTYPE_Space || dwCharType == FX_CHARTYPE_Control) {
                    if (!bFind) {
                        iCharWidth = tc.m_iCharWidth;
                        if (bAllChars && iCharWidth > 0) {
                            iNetWidth -= iCharWidth;
                        }
                    }
                } else {
                    bFind = TRUE;
                    if (!bAllChars) {
                        break;
                    }
                }
            }
            j += bArabic ? 1 : -1;
        }
        if (!bAllChars && bFind) {
            break;
        }
    }
    FX_INT32 iOffset = m_iLineEnd - iNetWidth;
    FX_INT32 iLowerAlignment = (m_iAlignment & FX_RTFLINEALIGNMENT_LowerMask);
    FX_INT32 iHigherAlignment = (m_iAlignment & FX_RTFLINEALIGNMENT_HigherMask);
    if (iGapChars > 0 && (iHigherAlignment == FX_RTFLINEALIGNMENT_Distributed || (iHigherAlignment == FX_RTFLINEALIGNMENT_Justified && dwStatus != FX_RTFBREAK_ParagraphBreak))) {
        FX_INT32 iStart = -1;
        for (i = 0; i < iCount; i ++) {
            tpo = tpos.GetAt(i);
            CFX_RTFPiece &ttp = pCurPieces->GetAt(tpo.index);
            if (iStart < 0) {
                iStart = ttp.m_iStartPos;
            } else {
                ttp.m_iStartPos = iStart;
            }
            FX_INT32 k;
            for (j = 0; j < ttp.m_iChars; j ++) {
                CFX_RTFChar &tc = ttp.GetChar(j);
                if (tc.m_nBreakType != FX_LBT_DIRECT_BRK || tc.m_iCharWidth < 0) {
                    continue;
                }
                k = iOffset / iGapChars;
                tc.m_iCharWidth += k;
                ttp.m_iWidth += k;
                iOffset -= k;
                iGapChars --;
                if (iGapChars < 1) {
                    break;
                }
            }
            iStart += ttp.m_iWidth;
        }
    } else if (iLowerAlignment > FX_RTFLINEALIGNMENT_Left) {
        if (iLowerAlignment == FX_RTFLINEALIGNMENT_Center) {
            iOffset /= 2;
        }
        if (iOffset > 0) {
            for (i = 0; i < iCount; i ++) {
                CFX_RTFPiece &ttp = pCurPieces->GetAt(i);
                ttp.m_iStartPos += iOffset;
            }
        }
    }
}
FX_INT32 CFX_RTFBreak::GetBreakPos(CFX_RTFCharArray &tca, FX_INT32 &iEndPos, FX_BOOL bAllChars, FX_BOOL bOnlyBrk)
{
    FX_INT32 iLength = tca.GetSize() - 1;
    if (iLength < 1) {
        return iLength;
    }
    FX_INT32 iBreak = -1, iBreakPos = -1, iIndirect = -1, iIndirectPos = -1, iLast = -1, iLastPos = -1;
    if (m_bSingleLine || m_bOrphanLine || iEndPos <= m_iLineEnd) {
        if (!bAllChars || m_bCharCode) {
            return iLength;
        }
        iBreak = iLength;
        iBreakPos = iEndPos;
    }
    CFX_RTFChar *pCharArray = tca.GetData();
    if (m_bCharCode) {
        const CFX_RTFChar *pChar;
        FX_INT32 iCharWidth;
        while (iLength > 0) {
            if (iEndPos <= m_iLineEnd) {
                break;
            }
            pChar = pCharArray + iLength--;
            iCharWidth = pChar->m_iCharWidth;
            if (iCharWidth > 0) {
                iEndPos -= iCharWidth;
            }
        }
        return iLength;
    }
    FX_BOOL bSpaceBreak = (m_dwPolicies & FX_RTFBREAKPOLICY_SpaceBreak) != 0;
    FX_BOOL bTabBreak = (m_dwPolicies & FX_RTFBREAKPOLICY_TabBreak) != 0;
    FX_BOOL bNumberBreak = (m_dwPolicies & FX_RTFBREAKPOLICY_NumberBreak) != 0;
    FX_BOOL bInfixBreak = (m_dwPolicies & FX_RTFBREAKPOLICY_InfixBreak) != 0;
    FX_LINEBREAKTYPE eType;
    FX_DWORD nCodeProp, nCur, nNext;
    CFX_RTFChar *pCur = pCharArray + iLength--;
    if (bAllChars) {
        pCur->m_nBreakType = FX_LBT_UNKNOWN;
    }
    nCodeProp = pCur->m_dwCharProps;
    nNext = nCodeProp & 0x003F;
    FX_INT32 iCharWidth = pCur->m_iCharWidth;
    if (iCharWidth > 0) {
        iEndPos -= iCharWidth;
    }
    while (iLength >= 0) {
        pCur = pCharArray + iLength;
        nCodeProp = pCur->m_dwCharProps;
        nCur = nCodeProp & 0x003F;
        FX_BOOL bNeedBreak = FALSE;
        if (nCur == FX_CBP_SP) {
            bNeedBreak = !bSpaceBreak;
            if (nNext == FX_CBP_SP) {
                eType = bSpaceBreak ? FX_LBT_DIRECT_BRK : FX_LBT_PROHIBITED_BRK;
            } else {
                eType = *((const FX_LINEBREAKTYPE *)gs_FX_LineBreak_PairTable + (nCur << 5) + nNext);
            }
        } else if (nCur == FX_CBP_TB) {
            bNeedBreak = !bTabBreak;
            if (nNext == FX_CBP_TB) {
                eType = bTabBreak ? FX_LBT_DIRECT_BRK : FX_LBT_PROHIBITED_BRK;
            } else {
                eType = *((const FX_LINEBREAKTYPE *)gs_FX_LineBreak_PairTable + (nCur << 5) + nNext);
            }
        } else if (bNumberBreak && nCur == FX_CBP_NU && nNext == FX_CBP_NU) {
            eType = FX_LBT_DIRECT_BRK;
        } else if (bInfixBreak && nCur == FX_CBP_IS && nNext == FX_CBP_IS) {
            eType = FX_LBT_DIRECT_BRK;
        } else {
            if (nNext == FX_CBP_SP) {
                eType = FX_LBT_PROHIBITED_BRK;
            } else {
                eType = *((const FX_LINEBREAKTYPE *)gs_FX_LineBreak_PairTable + (nCur << 5) + nNext);
            }
        }
        if (bAllChars) {
            pCur->m_nBreakType = eType;
        }
        if (!bOnlyBrk) {
            iCharWidth = pCur->m_iCharWidth;
            FX_BOOL bBreak = FALSE;
            if (nCur == FX_CBP_TB && bTabBreak) {
                bBreak = iCharWidth > 0 && iEndPos - iCharWidth <= m_iLineEnd;
            } else {
                bBreak = iEndPos <= m_iLineEnd;
            }
            if (m_bSingleLine || m_bOrphanLine || bBreak || bNeedBreak) {
                if (eType == FX_LBT_DIRECT_BRK && iBreak < 0) {
                    iBreak = iLength;
                    iBreakPos = iEndPos;
                    if (!bAllChars) {
                        return iLength;
                    }
                } else if (eType == FX_LBT_INDIRECT_BRK && iIndirect < 0) {
                    iIndirect = iLength;
                    iIndirectPos = iEndPos;
                }
                if (iLast < 0) {
                    iLast = iLength;
                    iLastPos = iEndPos;
                }
            }
            if (iCharWidth > 0) {
                iEndPos -= iCharWidth;
            }
        }
        nNext = nCodeProp & 0x003F;
        iLength --;
    }
    if (bOnlyBrk) {
        return 0;
    }
    if (iBreak > -1) {
        iEndPos = iBreakPos;
        return iBreak;
    }
    if (iIndirect > -1) {
        iEndPos = iIndirectPos;
        return iIndirect;
    }
    if (iLast > -1) {
        iEndPos = iLastPos;
        return iLast;
    }
    return 0;
}
void CFX_RTFBreak::SplitTextLine(CFX_RTFLine *pCurLine, CFX_RTFLine *pNextLine, FX_BOOL bAllChars)
{
    FXSYS_assert(pCurLine != NULL && pNextLine != NULL);
    FX_INT32 iCount = pCurLine->CountChars();
    if (iCount < 2) {
        return;
    }
    FX_INT32 iEndPos = pCurLine->GetLineEnd();
    CFX_RTFCharArray &curChars = pCurLine->m_LineChars;
    FX_INT32 iCharPos = GetBreakPos(curChars, iEndPos, bAllChars, FALSE);
    if (iCharPos < 0) {
        iCharPos = 0;
    }
    iCharPos ++;
    if (iCharPos >= iCount) {
        pNextLine->RemoveAll(TRUE);
        CFX_Char *pTC = curChars.GetDataPtr(iCharPos - 1);
        pTC->m_nBreakType = FX_LBT_UNKNOWN;
        return;
    }
    CFX_RTFCharArray &nextChars = pNextLine->m_LineChars;
    int cur_size = curChars.GetSize();
    nextChars.SetSize(cur_size - iCharPos);
    FXSYS_memcpy(nextChars.GetData(), curChars.GetDataPtr(iCharPos), (cur_size - iCharPos) * sizeof(CFX_RTFChar));
    iCount -= iCharPos;
    cur_size = curChars.GetSize();
    curChars.RemoveAt(cur_size - iCount, iCount);
    pNextLine->m_iStart = pCurLine->m_iStart;
    pNextLine->m_iWidth = pCurLine->GetLineEnd() - iEndPos;
    pCurLine->m_iWidth = iEndPos;
    CFX_RTFChar* tc = curChars.GetDataPtr(iCharPos - 1);
    tc->m_nBreakType = FX_LBT_UNKNOWN;
    iCount = nextChars.GetSize();
    CFX_RTFChar *pNextChars = nextChars.GetData();
    for (FX_INT32 i = 0; i < iCount; i ++) {
        CFX_RTFChar* tc = pNextChars + i;
        if (tc->GetCharType() >= FX_CHARTYPE_ArabicAlef) {
            pCurLine->m_iArabicChars --;
            pNextLine->m_iArabicChars ++;
        }
        if (tc->m_dwLayoutStyles & FX_RTFLAYOUTSTYLE_MBCSCode) {
            pCurLine->m_iMBCSChars --;
            pNextLine->m_iMBCSChars ++;
        }
        tc->m_dwStatus = 0;
    }
}
FX_INT32 CFX_RTFBreak::CountBreakPieces() const
{
    CFX_RTFPieceArray *pRTFPieces = GetRTFPieces(TRUE);
    if (pRTFPieces == NULL) {
        return 0;
    }
    return pRTFPieces->GetSize();
}
const CFX_RTFPiece* CFX_RTFBreak::GetBreakPiece(FX_INT32 index) const
{
    CFX_RTFPieceArray *pRTFPieces = GetRTFPieces(TRUE);
    if (pRTFPieces == NULL) {
        return NULL;
    }
    if (index < 0 || index >= pRTFPieces->GetSize()) {
        return NULL;
    }
    return pRTFPieces->GetPtrAt(index);
}
void CFX_RTFBreak::GetLineRect(CFX_RectF &rect) const
{
    rect.top = 0;
    CFX_RTFLine *pRTFLine = GetRTFLine(TRUE);
    if (pRTFLine == NULL) {
        rect.left = ((FX_FLOAT)m_iLineStart) / 20000.0f;
        rect.width = rect.height = 0;
        return;
    }
    rect.left = ((FX_FLOAT)pRTFLine->m_iStart) / 20000.0f;
    rect.width = ((FX_FLOAT)pRTFLine->m_iWidth) / 20000.0f;
    CFX_RTFPieceArray &rtfPieces = pRTFLine->m_LinePieces;
    FX_INT32 iCount = rtfPieces.GetSize();
    if (iCount < 1) {
        rect.width = 0;
        return;
    }
    CFX_RTFPiece *pBreakPiece;
    FX_INT32 iLineHeight = 0, iMax;
    for (FX_INT32 i = 0; i < iCount; i ++) {
        pBreakPiece = rtfPieces.GetPtrAt(i);
        FX_INT32 iFontHeight = FXSYS_round(pBreakPiece->m_iFontHeight * pBreakPiece->m_iVerticalScale / 100.0f);
        iMax = FX_MAX(pBreakPiece->m_iFontSize, iFontHeight);
        if (i == 0) {
            iLineHeight = iMax;
        } else if (iLineHeight < iMax) {
            iLineHeight = iMax;
        }
    }
    rect.height = ((FX_FLOAT)iLineHeight) / 20.0f;
}
void CFX_RTFBreak::ClearBreakPieces()
{
    CFX_RTFLine *pRTFLine = GetRTFLine(TRUE);
    if (pRTFLine != NULL) {
        pRTFLine->RemoveAll(TRUE);
    }
    m_iReady = 0;
}
void CFX_RTFBreak::Reset()
{
    m_dwCharType = 0;
    m_RTFLine1.RemoveAll(TRUE);
    m_RTFLine2.RemoveAll(TRUE);
}
FX_INT32 CFX_RTFBreak::GetDisplayPos(FX_LPCRTFTEXTOBJ pText, FXTEXT_CHARPOS *pCharPos, FX_BOOL bCharCode , CFX_WideString *pWSForms , FX_AdjustCharDisplayPos pAdjustPos ) const
{
    if (pText == NULL || pText->iLength < 1) {
        return 0;
    }
    FXSYS_assert(pText->pStr != NULL && pText->pWidths != NULL && pText->pFont != NULL && pText->pRect != NULL);
    FX_LPCWSTR pStr = pText->pStr;
    FX_INT32 *pWidths = pText->pWidths;
    FX_INT32 iLength = pText->iLength - 1;
    IFX_Font *pFont = pText->pFont;
    FX_DWORD dwStyles = pText->dwLayoutStyles;
    CFX_RectF rtText(*pText->pRect);
    FX_BOOL bRTLPiece = FX_IsOdd(pText->iBidiLevel);
    FX_FLOAT fFontSize = pText->fFontSize;
    FX_INT32 iFontSize = FXSYS_round(fFontSize * 20.0f);
    FX_INT32 iAscent = pFont->GetAscent();
    FX_INT32 iDescent = pFont->GetDescent();
    FX_INT32 iMaxHeight = iAscent - iDescent;
    FX_FLOAT fFontHeight = fFontSize;
    FX_FLOAT fAscent = fFontHeight * (FX_FLOAT)iAscent / (FX_FLOAT)iMaxHeight;
    FX_FLOAT fDescent = fFontHeight * (FX_FLOAT)iDescent / (FX_FLOAT)iMaxHeight;
    FX_BOOL bVerticalDoc = (dwStyles & FX_RTFLAYOUTSTYLE_VerticalLayout) != 0;
    FX_BOOL bVerticalChar = (dwStyles & FX_RTFLAYOUTSTYLE_VerticalChars) != 0;
    FX_BOOL bArabicNumber = (dwStyles & FX_RTFLAYOUTSTYLE_ArabicNumber) != 0;
    FX_BOOL bMBCSCode = (dwStyles & FX_RTFLAYOUTSTYLE_MBCSCode) != 0;
    FX_INT32 iRotation = GetLineRotation(dwStyles) + pText->iCharRotation;
    FX_INT32 iCharRotation;
    FX_WCHAR wch, wPrev = 0xFEFF, wNext, wForm;
    FX_INT32 iWidth, iCharWidth, iCharHeight;
    FX_FLOAT fX, fY, fCharWidth, fCharHeight;
    FX_INT32 iHorScale = pText->iHorizontalScale;
    FX_INT32 iVerScale = pText->iVerticalScale;
    FX_BOOL bEmptyChar;
    FX_DWORD dwProps, dwCharType;
    fX = rtText.left;
    fY = rtText.top;
    if (bVerticalDoc) {
        fX += (rtText.width - fFontSize) / 2.0f;
        if (bRTLPiece) {
            fY = rtText.bottom();
        }
    } else {
        if (bRTLPiece) {
            fX = rtText.right();
        }
        fY += fAscent;
    }
    FX_INT32 iCount = 0;
    for (FX_INT32 i = 0; i <= iLength; i ++) {
        wch = *pStr ++;
        iWidth = *pWidths ++;
        if (!bMBCSCode) {
            dwProps = FX_GetUnicodeProperties(wch);
            dwCharType = (dwProps & FX_CHARTYPEBITSMASK);
            if (dwCharType == FX_CHARTYPE_ArabicAlef && iWidth == 0) {
                wPrev = 0xFEFF;
                continue;
            }
        } else {
            dwProps = 0;
            dwCharType = 0;
        }
        if (iWidth != 0) {
            iCharWidth = iWidth;
            if (iCharWidth < 0) {
                iCharWidth = -iCharWidth;
            }
            if (!bMBCSCode) {
                bEmptyChar = (dwCharType >= FX_CHARTYPE_Tab && dwCharType <= FX_CHARTYPE_Control);
            } else {
                bEmptyChar = FALSE;
            }
            if (!bEmptyChar) {
                iCount ++;
            }
            if (pCharPos != NULL) {
                iCharWidth /= iFontSize;
                wForm = wch;
                if (!bMBCSCode) {
                    if (dwCharType >= FX_CHARTYPE_ArabicAlef) {
                        if (i < iLength) {
                            wNext = *pStr;
                            if (*pWidths < 0) {
                                if (i + 1 < iLength) {
                                    wNext = pStr[1];
                                }
                            }
                        } else {
                            wNext = 0xFEFF;
                        }
                        wForm = m_pArabicChar->GetFormChar(wch, wPrev, wNext);
                    } else if (bRTLPiece || bVerticalChar) {
                        wForm = FX_GetMirrorChar(wch, dwProps, bRTLPiece, bVerticalChar);
                    } else if (dwCharType == FX_CHARTYPE_Numeric && bArabicNumber) {
                        wForm = wch + 0x0630;
                    }
                    dwProps = FX_GetUnicodeProperties(wForm);
                }
                iCharRotation = iRotation;
                if (!bMBCSCode && bVerticalChar && (dwProps & 0x8000) != 0) {
                    iCharRotation ++;
                    iCharRotation %= 4;
                }
                if (!bEmptyChar) {
                    if (bCharCode) {
                        pCharPos->m_GlyphIndex = wch;
                    } else {
                        pCharPos->m_GlyphIndex = pFont->GetGlyphIndex(wForm, bMBCSCode);
                        if(pCharPos->m_GlyphIndex == 0xFFFF) {
                            pCharPos->m_GlyphIndex = pFont->GetGlyphIndex(wch, bMBCSCode);
                        }
                    }
                    pCharPos->m_ExtGID = pCharPos->m_GlyphIndex;
                    pCharPos->m_FontCharWidth = iCharWidth;
                    if (pWSForms) {
                        *pWSForms += wForm;
                    }
                }
                if (bVerticalDoc) {
                    iCharHeight = iCharWidth;
                    iCharWidth = 1000;
                } else {
                    iCharHeight = 1000;
                }
                fCharWidth = fFontSize * iCharWidth / 1000.0f;
                fCharHeight = fFontSize * iCharHeight / 1000.0f;
                if (!bMBCSCode && bRTLPiece && dwCharType != FX_CHARTYPE_Combination) {
                    if (bVerticalDoc) {
                        fY -= fCharHeight;
                    } else {
                        fX -= fCharWidth;
                    }
                }
                if (!bEmptyChar) {
                    CFX_PointF ptOffset;
                    ptOffset.Reset();
                    FX_BOOL bAdjusted = FALSE;
                    if (pAdjustPos) {
                        bAdjusted = pAdjustPos(wForm, bMBCSCode, pFont, fFontSize, bVerticalChar, ptOffset);
                    }
                    if (!pAdjustPos && bVerticalChar && (dwProps & 0x00010000) != 0) {
                        CFX_Rect rtBBox;
                        rtBBox.Reset();
                        if (pFont->GetCharBBox(wForm, rtBBox, bMBCSCode)) {
                            ptOffset.x = fFontSize * (850 - rtBBox.right()) / 1000.0f;
                            ptOffset.y = fFontSize * (1000 - rtBBox.height) / 2000.0f;
                        }
                    }
                    pCharPos->m_OriginX = fX + ptOffset.x;
                    pCharPos->m_OriginY = fY - ptOffset.y;
                }
                if (!bRTLPiece && dwCharType != FX_CHARTYPE_Combination) {
                    if (bVerticalDoc) {
                        fY += fCharHeight;
                    } else {
                        fX += fCharWidth;
                    }
                }
                if (!bEmptyChar) {
                    pCharPos->m_bGlyphAdjust = TRUE;
                    if (bVerticalDoc) {
                        if (iCharRotation == 0) {
                            pCharPos->m_AdjustMatrix[0] = -1;
                            pCharPos->m_AdjustMatrix[1] = 0;
                            pCharPos->m_AdjustMatrix[2] = 0;
                            pCharPos->m_AdjustMatrix[3] = 1;
                            pCharPos->m_OriginY += fAscent * iVerScale / 100.0f;
                        } else if (iCharRotation == 1) {
                            pCharPos->m_AdjustMatrix[0] = 0;
                            pCharPos->m_AdjustMatrix[1] = -1;
                            pCharPos->m_AdjustMatrix[2] = -1;
                            pCharPos->m_AdjustMatrix[3] = 0;
                            pCharPos->m_OriginX -= fDescent + fAscent * iVerScale / 100.0f - fAscent;
                        } else if (iCharRotation == 2) {
                            pCharPos->m_AdjustMatrix[0] = 1;
                            pCharPos->m_AdjustMatrix[1] = 0;
                            pCharPos->m_AdjustMatrix[2] = 0;
                            pCharPos->m_AdjustMatrix[3] = -1;
                            pCharPos->m_OriginX += fCharWidth;
                            pCharPos->m_OriginY += fAscent;
                        } else {
                            pCharPos->m_AdjustMatrix[0] = 0;
                            pCharPos->m_AdjustMatrix[1] = 1;
                            pCharPos->m_AdjustMatrix[2] = 1;
                            pCharPos->m_AdjustMatrix[3] = 0;
                            pCharPos->m_OriginX += fAscent;
                            pCharPos->m_OriginY += fCharWidth;
                        }
                    } else {
                        if (iCharRotation == 0) {
                            pCharPos->m_AdjustMatrix[0] = -1;
                            pCharPos->m_AdjustMatrix[1] = 0;
                            pCharPos->m_AdjustMatrix[2] = 0;
                            pCharPos->m_AdjustMatrix[3] = 1;
                            pCharPos->m_OriginY += fAscent * iVerScale / 100.0f - fAscent;
                        } else if (iCharRotation == 1) {
                            pCharPos->m_AdjustMatrix[0] = 0;
                            pCharPos->m_AdjustMatrix[1] = -1;
                            pCharPos->m_AdjustMatrix[2] = -1;
                            pCharPos->m_AdjustMatrix[3] = 0;
                            pCharPos->m_OriginX -= fDescent;
                            pCharPos->m_OriginY -= fAscent + fDescent;
                        } else if (iCharRotation == 2) {
                            pCharPos->m_AdjustMatrix[0] = 1;
                            pCharPos->m_AdjustMatrix[1] = 0;
                            pCharPos->m_AdjustMatrix[2] = 0;
                            pCharPos->m_AdjustMatrix[3] = -1;
                            pCharPos->m_OriginX += fCharWidth;
                            pCharPos->m_OriginY -= fAscent;
                        } else {
                            pCharPos->m_AdjustMatrix[0] = 0;
                            pCharPos->m_AdjustMatrix[1] = 1;
                            pCharPos->m_AdjustMatrix[2] = 1;
                            pCharPos->m_AdjustMatrix[3] = 0;
                            pCharPos->m_OriginX += fAscent * iVerScale / 100.0f;
                        }
                    }
                    if (iHorScale != 100 || iVerScale != 100) {
                        pCharPos->m_AdjustMatrix[0] = pCharPos->m_AdjustMatrix[0] * iHorScale / 100.0f;
                        pCharPos->m_AdjustMatrix[1] = pCharPos->m_AdjustMatrix[1] * iHorScale / 100.0f;
                        pCharPos->m_AdjustMatrix[2] = pCharPos->m_AdjustMatrix[2] * iVerScale / 100.0f;
                        pCharPos->m_AdjustMatrix[3] = pCharPos->m_AdjustMatrix[3] * iVerScale / 100.0f;
                    }
                    pCharPos ++;
                }
            }
        }
        if (iWidth > 0) {
            wPrev = wch;
        }
    }
    return iCount;
}
FX_INT32 CFX_RTFBreak::GetCharRects(FX_LPCRTFTEXTOBJ pText, CFX_RectFArray &rtArray, FX_BOOL bCharBBox ) const
{
    if (pText == NULL || pText->iLength < 1) {
        return 0;
    }
    FXSYS_assert(pText->pStr != NULL && pText->pWidths != NULL && pText->pFont != NULL && pText->pRect != NULL);
    FX_LPCWSTR pStr = pText->pStr;
    FX_INT32 *pWidths = pText->pWidths;
    FX_INT32 iLength = pText->iLength;
    CFX_RectF rect(*pText->pRect);
    FX_BOOL bRTLPiece = FX_IsOdd(pText->iBidiLevel);
    FX_FLOAT fFontSize = pText->fFontSize;
    FX_INT32 iFontSize = FXSYS_round(fFontSize * 20.0f);
    FX_FLOAT fScale = fFontSize / 1000.0f;
    IFX_Font *pFont = pText->pFont;
    if (pFont == NULL) {
        bCharBBox = FALSE;
    }
    CFX_Rect bbox;
    bbox.Set(0, 0, 0, 0);
    if (bCharBBox) {
        bCharBBox = pFont->GetBBox(bbox);
    }
    FX_FLOAT fLeft = FX_MAX(0, bbox.left * fScale);
    FX_FLOAT fHeight = FXSYS_fabs(bbox.height * fScale);
    rtArray.RemoveAll();
    rtArray.SetSize(iLength);
    FX_DWORD dwStyles = pText->dwLayoutStyles;
    FX_BOOL bVertical = (dwStyles & FX_RTFLAYOUTSTYLE_VerticalLayout) != 0;
    FX_BOOL bMBCSCode = (dwStyles & FX_RTFLAYOUTSTYLE_MBCSCode) != 0;
    FX_BOOL bSingleLine = (dwStyles & FX_RTFLAYOUTSTYLE_SingleLine) != 0;
    FX_BOOL bCombText = (dwStyles & FX_TXTLAYOUTSTYLE_CombText) != 0;
    FX_WCHAR wch, wLineBreakChar = pText->wLineBreakChar;
    FX_INT32 iCharSize;
    FX_FLOAT fCharSize, fStart;
    if (bVertical) {
        fStart = bRTLPiece ? rect.bottom() : rect.top;
    } else {
        fStart = bRTLPiece ? rect.right() : rect.left;
    }
    for (FX_INT32 i = 0; i < iLength; i ++) {
        wch = *pStr ++;
        iCharSize = *pWidths ++;
        fCharSize = (FX_FLOAT)iCharSize / 20000.0f;
        FX_BOOL bRet = (!bSingleLine && FX_IsCtrlCode(wch));
        if (!(wch == L'\v' || wch == L'\f' || wch == 0x2028 || wch == 0x2029 || (wLineBreakChar != 0xFEFF && wch == wLineBreakChar))) {
            bRet = FALSE;
        }
        if (bRet) {
            iCharSize = iFontSize * 500;
            fCharSize = fFontSize / 2.0f;
        }
        if (bVertical) {
            rect.top = fStart;
            if (bRTLPiece) {
                rect.top -= fCharSize;
                fStart -= fCharSize;
            } else {
                fStart += fCharSize;
            }
            rect.height = fCharSize;
        } else {
            rect.left = fStart;
            if (bRTLPiece) {
                rect.left -= fCharSize;
                fStart -= fCharSize;
            } else {
                fStart += fCharSize;
            }
            rect.width = fCharSize;
        }
        if (bCharBBox && !bRet) {
            FX_INT32 iCharWidth = 1000;
            pFont->GetCharWidth(wch, iCharWidth);
            FX_FLOAT fRTLeft = 0, fCharWidth = 0;
            if (iCharWidth > 0) {
                fCharWidth = iCharWidth * fScale;
                fRTLeft = fLeft;
                if (bCombText) {
                    fRTLeft = (rect.width - fCharWidth) / 2.0f;
                }
            }
            CFX_RectF rtBBoxF;
            if (bVertical) {
                rtBBoxF.top = rect.left + fRTLeft;
                rtBBoxF.left = rect.top + (rect.height - fHeight) / 2.0f;
                rtBBoxF.height = fCharWidth;
                rtBBoxF.width = fHeight;
                rtBBoxF.left = FX_MAX(rtBBoxF.left, 0);
            } else {
                rtBBoxF.left = rect.left + fRTLeft;
                rtBBoxF.top = rect.top + (rect.height - fHeight) / 2.0f;
                rtBBoxF.width = fCharWidth;
                rtBBoxF.height = fHeight;
                rtBBoxF.top = FX_MAX(rtBBoxF.top, 0);
            }
            rtArray.SetAt(i, rtBBoxF);
            continue;
        }
        rtArray.SetAt(i, rect);
    }
    return iLength;
}
