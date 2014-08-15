// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "../fgas_base.h"
#include "fx_stdfontmgr.h"
#include "fx_fontutils.h"
IFX_FontMgr* IFX_FontMgr::Create(FX_LPEnumAllFonts pEnumerator, FX_LPMatchFont pMatcher , FX_LPVOID pUserData )
{
    return FX_NEW CFX_StdFontMgrImp(pEnumerator, pMatcher, pUserData);
}
CFX_StdFontMgrImp::CFX_StdFontMgrImp(FX_LPEnumAllFonts pEnumerator, FX_LPMatchFont pMatcher, FX_LPVOID pUserData)
    : m_pMatcher(pMatcher)
    , m_pEnumerator(pEnumerator)
    , m_FontFaces()
    , m_Fonts()
    , m_CPFonts(8)
    , m_FamilyFonts(16)
    , m_UnicodeFonts(16)
    , m_BufferFonts(4)
    , m_FileFonts(4)
    , m_StreamFonts(4)
    , m_DeriveFonts(4)
    , m_pUserData(pUserData)
{
    if (m_pEnumerator != NULL) {
        m_pEnumerator(m_FontFaces, m_pUserData, NULL, 0xFEFF);
    }
    if (m_pMatcher == NULL) {
        m_pMatcher = FX_DefFontMatcher;
    }
    FXSYS_assert(m_pMatcher != NULL);
}
CFX_StdFontMgrImp::~CFX_StdFontMgrImp()
{
    for (FX_INT32 i = 0; i < m_FontFaces.GetSize(); i++) {
        IFX_FontHandler* pFontHandler = m_FontFaces.GetPtrAt(i)->pFontHandler;
        if (pFontHandler == NULL) {
            break;
        }
        pFontHandler->Release();
    }
    m_FontFaces.RemoveAll();
    FX_POSITION ps = m_CachedFace.GetStartPosition();
    while (ps) {
        FX_LPVOID pKey;
        FXFT_Face ftFace;
        m_CachedFace.GetNextAssoc(ps, pKey, (void*&)ftFace);
        FXFT_Done_Face(ftFace);
    }
    m_CachedFace.RemoveAll();
    m_CPFonts.RemoveAll();
    m_FamilyFonts.RemoveAll();
    m_UnicodeFonts.RemoveAll();
    m_BufferFonts.RemoveAll();
    m_FileFonts.RemoveAll();
    m_StreamFonts.RemoveAll();
    m_DeriveFonts.RemoveAll();
    for (FX_INT32 i = m_Fonts.GetUpperBound(); i >= 0; i--) {
        IFX_Font *pFont = (IFX_Font*)m_Fonts[i];
        if (pFont != NULL) {
            pFont->Release();
        }
    }
    m_Fonts.RemoveAll();
}
IFX_Font* CFX_StdFontMgrImp::GetDefFontByCodePage(FX_WORD wCodePage, FX_DWORD dwFontStyles, FX_LPCWSTR pszFontFamily)
{
    FX_DWORD dwHash = FGAS_GetFontHashCode(wCodePage, dwFontStyles);
    IFX_Font *pFont = NULL;
    if (m_CPFonts.Lookup((void*)(FX_UINTPTR)dwHash, (void*&)pFont)) {
        return pFont ? LoadFont(pFont, dwFontStyles, wCodePage) : NULL;
    }
    FX_LPCFONTDESCRIPTOR pFD;
    if ((pFD = FindFont(pszFontFamily, dwFontStyles, TRUE, wCodePage)) == NULL)
        if ((pFD = FindFont(NULL, dwFontStyles, TRUE, wCodePage)) == NULL)
            if ((pFD = FindFont(NULL, dwFontStyles, FALSE, wCodePage)) == NULL) {
                return NULL;
            }
    FXSYS_assert(pFD != NULL);
    pFont = IFX_Font::LoadFont(pFD->wsFontFace, dwFontStyles, wCodePage, this);
    if (pFont != NULL) {
        m_Fonts.Add(pFont);
        m_CPFonts.SetAt((void*)(FX_UINTPTR)dwHash, (void*)pFont);
        dwHash = FGAS_GetFontFamilyHash(pFD->wsFontFace, dwFontStyles, wCodePage);
        m_FamilyFonts.SetAt((void*)(FX_UINTPTR)dwHash, (void*)pFont);
        return LoadFont(pFont, dwFontStyles, wCodePage);
    }
    return NULL;
}
IFX_Font* CFX_StdFontMgrImp::GetDefFontByCharset(FX_BYTE nCharset, FX_DWORD dwFontStyles, FX_LPCWSTR pszFontFamily)
{
    return GetDefFontByCodePage(FX_GetCodePageFromCharset(nCharset), dwFontStyles, pszFontFamily);
}
IFX_Font* CFX_StdFontMgrImp::GetFont(FX_LPCFONTDESCRIPTOR pFD, FX_DWORD dwFontStyles)
{
    IFX_Font *pFont = NULL;
    FX_WORD wCodePage = FX_GetCodePageFromCharset(pFD->uCharSet);
    FX_LPCWSTR pFontFace = pFD->wsFontFace;
    if (pFD->pFontHandler) {
        FX_DWORD dwCacheHash = FGAS_GetFontFamilyHash(pFontFace, 0, wCodePage);
        FXFT_Face ftFace = (FXFT_Face)m_CachedFace.GetValueAt((void*)(FX_UINTPTR)dwCacheHash);
        if (ftFace == NULL) {
            ftFace = pFD->pFontHandler->CreateFace();
            if (ftFace == NULL) {
                return NULL;
            }
            m_CachedFace.SetAt((void*)(FX_UINTPTR)dwCacheHash, ftFace);
        }
        CFX_Font* pFXFont = FX_NEW CFX_Font;
        if (pFXFont == NULL) {
            return NULL;
        }
        pFXFont->m_Face = ftFace;
        pFXFont->m_pFontData = FXFT_Get_Face_Stream_Base(ftFace);
        pFXFont->m_dwSize = FXFT_Get_Face_Stream_Size(ftFace);
        pFont = IFX_Font::LoadFont(pFXFont, this);
    } else {
        pFont = IFX_Font::LoadFont(pFontFace, dwFontStyles, wCodePage, this);
    }
    return pFont;
}
#define _FX_USEGASFONTMGR_
IFX_Font* CFX_StdFontMgrImp::GetDefFontByUnicode(FX_WCHAR wUnicode, FX_DWORD dwFontStyles, FX_LPCWSTR pszFontFamily)
{
    FGAS_LPCFONTUSB pRet = FGAS_GetUnicodeBitField(wUnicode);
    if (pRet->wBitField == 999) {
        return NULL;
    }
    FX_DWORD dwHash = FGAS_GetFontFamilyHash(pszFontFamily, dwFontStyles, pRet->wBitField);
    IFX_Font *pFont = NULL;
    if (m_UnicodeFonts.Lookup((void*)(FX_UINTPTR)dwHash, (void*&)pFont)) {
        return pFont ? LoadFont(pFont, dwFontStyles, pRet->wCodePage) : NULL;
    }
#ifdef _FX_USEGASFONTMGR_
    FX_LPCFONTDESCRIPTOR pFD = FindFont(pszFontFamily, dwFontStyles, FALSE, pRet->wCodePage, pRet->wBitField, wUnicode);
    if (pFD == NULL && pszFontFamily) {
        pFD = FindFont(NULL, dwFontStyles, FALSE, pRet->wCodePage, pRet->wBitField, wUnicode);
    }
    if (pFD == NULL) {
        return NULL;
    }
    FXSYS_assert(pFD);
    FX_WORD wCodePage = FX_GetCodePageFromCharset(pFD->uCharSet);
    FX_LPCWSTR pFontFace = pFD->wsFontFace;
    pFont = GetFont(pFD, dwFontStyles);
#else
    CFX_FontMapper* pBuiltinMapper = CFX_GEModule::Get()->GetFontMgr()->m_pBuiltinMapper;
    if (pBuiltinMapper == NULL) {
        return NULL;
    }
    FX_INT32 iWeight = (dwFontStyles & FX_FONTSTYLE_Bold) ? FXFONT_FW_BOLD : FXFONT_FW_NORMAL;
    int italic_angle = 0;
    FXFT_Face ftFace = pBuiltinMapper->FindSubstFontByUnicode(wUnicode, dwFontStyles, iWeight, italic_angle);
    if (ftFace == NULL) {
        return NULL;
    }
    CFX_Font* pFXFont = FX_NEW CFX_Font;
    if (pFXFont == NULL) {
        return NULL;
    }
    pFXFont->m_Face = ftFace;
    pFXFont->m_pFontData = FXFT_Get_Face_Stream_Base(ftFace);
    pFXFont->m_dwSize = FXFT_Get_Face_Stream_Size(ftFace);
    pFont = IFX_Font::LoadFont(pFXFont, this);
    FX_WORD wCodePage = pRet->wCodePage;
    CFX_WideString wsPsName = pFXFont->GetPsName();
    FX_LPCWSTR pFontFace = wsPsName;
#endif
    if (pFont != NULL) {
        m_Fonts.Add(pFont);
        m_UnicodeFonts.SetAt((void*)(FX_UINTPTR)dwHash, (void*)pFont);
        dwHash = FGAS_GetFontHashCode(wCodePage, dwFontStyles);
        m_CPFonts.SetAt((void*)(FX_UINTPTR)dwHash, (void*)pFont);
        dwHash = FGAS_GetFontFamilyHash(pFontFace, dwFontStyles, wCodePage);
        m_FamilyFonts.SetAt((void*)(FX_UINTPTR)dwHash, (void*)pFont);
        return LoadFont(pFont, dwFontStyles, wCodePage);
    }
    return NULL;
}
IFX_Font* CFX_StdFontMgrImp::GetDefFontByLanguage(FX_WORD wLanguage, FX_DWORD dwFontStyles, FX_LPCWSTR pszFontFamily)
{
    return GetDefFontByCodePage(FX_GetDefCodePageByLanguage(wLanguage), dwFontStyles, pszFontFamily);
}
IFX_Font* CFX_StdFontMgrImp::LoadFont(FX_LPCWSTR pszFontFamily, FX_DWORD dwFontStyles, FX_WORD wCodePage)
{
    FX_DWORD dwHash = FGAS_GetFontFamilyHash(pszFontFamily, dwFontStyles, wCodePage);
    IFX_Font *pFont = NULL;
    if (m_FamilyFonts.Lookup((void*)(FX_UINTPTR)dwHash, (void*&)pFont)) {
        return pFont ? LoadFont(pFont, dwFontStyles, wCodePage) : NULL;
    }
    FX_LPCFONTDESCRIPTOR pFD = NULL;
    if ((pFD = FindFont(pszFontFamily, dwFontStyles, TRUE, wCodePage)) == NULL)
        if ((pFD = FindFont(pszFontFamily, dwFontStyles, FALSE, wCodePage)) == NULL) {
            return NULL;
        }
    FXSYS_assert(pFD != NULL);
    if (wCodePage == 0xFFFF) {
        wCodePage = FX_GetCodePageFromCharset(pFD->uCharSet);
    }
    pFont = GetFont(pFD, dwFontStyles);;
    if (pFont != NULL) {
        m_Fonts.Add(pFont);
        m_FamilyFonts.SetAt((void*)(FX_UINTPTR)dwHash, (void*)pFont);
        dwHash = FGAS_GetFontHashCode(wCodePage, dwFontStyles);
        m_CPFonts.SetAt((void*)(FX_UINTPTR)dwHash, (void*)pFont);
        return LoadFont(pFont, dwFontStyles, wCodePage);
    }
    return NULL;
}
IFX_Font* CFX_StdFontMgrImp::LoadFont(FX_LPCBYTE pBuffer, FX_INT32 iLength)
{
    FXSYS_assert(pBuffer != NULL && iLength > 0);
    IFX_Font *pFont = NULL;
    if (m_BufferFonts.Lookup((void*)pBuffer, (void*&)pFont)) {
        if (pFont != NULL) {
            return pFont->Retain();
        }
    }
    pFont = IFX_Font::LoadFont(pBuffer, iLength, this);
    if (pFont != NULL) {
        m_Fonts.Add(pFont);
        m_BufferFonts.SetAt((void*)pBuffer, pFont);
        return pFont->Retain();
    }
    return NULL;
}
IFX_Font* CFX_StdFontMgrImp::LoadFont(FX_LPCWSTR pszFileName)
{
    FXSYS_assert(pszFileName != NULL);
    FX_DWORD dwHash = FX_HashCode_String_GetW(pszFileName, -1);
    IFX_Font *pFont = NULL;
    if (m_FileFonts.Lookup((void*)(FX_UINTPTR)dwHash, (void*&)pFont)) {
        if (pFont != NULL) {
            return pFont->Retain();
        }
    }
    pFont = IFX_Font::LoadFont(pszFileName, NULL);
    if (pFont != NULL) {
        m_Fonts.Add(pFont);
        m_FileFonts.SetAt((void*)(FX_UINTPTR)dwHash, (void*)pFont);
        return pFont->Retain();
    }
    return NULL;
}
IFX_Font* CFX_StdFontMgrImp::LoadFont(IFX_Stream *pFontStream, FX_LPCWSTR pszFontAlias , FX_DWORD dwFontStyles , FX_WORD wCodePage , FX_BOOL bSaveStream )
{
    FXSYS_assert(pFontStream != NULL && pFontStream->GetLength() > 0);
    IFX_Font *pFont = NULL;
    if (m_StreamFonts.Lookup((void*)pFontStream, (void*&)pFont)) {
        if (pFont != NULL) {
            if (pszFontAlias != NULL) {
                FX_DWORD dwHash = FGAS_GetFontFamilyHash(pszFontAlias, dwFontStyles, wCodePage);
                m_FamilyFonts.SetAt((void*)(FX_UINTPTR)dwHash, (void*)pFont);
            }
            return LoadFont(pFont, dwFontStyles, wCodePage);
        }
    }
    pFont = IFX_Font::LoadFont(pFontStream, this, bSaveStream);
    if (pFont != NULL) {
        m_Fonts.Add(pFont);
        m_StreamFonts.SetAt((void*)pFontStream, (void*)pFont);
        if (pszFontAlias != NULL) {
            FX_DWORD dwHash = FGAS_GetFontFamilyHash(pszFontAlias, dwFontStyles, wCodePage);
            m_FamilyFonts.SetAt((void*)(FX_UINTPTR)dwHash, (void*)pFont);
        }
        return LoadFont(pFont, dwFontStyles, wCodePage);
    }
    return NULL;
}
IFX_Font* CFX_StdFontMgrImp::LoadFont(IFX_Font *pSrcFont, FX_DWORD dwFontStyles, FX_WORD wCodePage)
{
    FXSYS_assert(pSrcFont != NULL);
    if (pSrcFont->GetFontStyles() == dwFontStyles) {
        return pSrcFont->Retain();
    }
    void* buffer[3] = {pSrcFont, (void*)(FX_UINTPTR)dwFontStyles, (void*)(FX_UINTPTR)wCodePage};
    FX_DWORD dwHash = FX_HashCode_String_GetA((FX_LPCSTR)buffer, 3 * sizeof(void*));
    IFX_Font *pFont = NULL;
    if (m_DeriveFonts.GetCount() > 0) {
        m_DeriveFonts.Lookup((void*)(FX_UINTPTR)dwHash, (void*&)pFont);
        if (pFont != NULL) {
            return pFont->Retain();
        }
    }
    pFont = pSrcFont->Derive(dwFontStyles, wCodePage);
    if (pFont != NULL) {
        m_DeriveFonts.SetAt((void*)(FX_UINTPTR)dwHash, (void*)pFont);
        FX_INT32 index = m_Fonts.Find(pFont);
        if (index < 0) {
            m_Fonts.Add(pFont);
            pFont->Retain();
        }
        return pFont;
    }
    return NULL;
}
void CFX_StdFontMgrImp::ClearFontCache()
{
    FX_INT32 iCount = m_Fonts.GetSize();
    for (FX_INT32 i = 0; i < iCount; i ++) {
        IFX_Font *pFont = (IFX_Font*)m_Fonts[i];
        if (pFont != NULL) {
            pFont->Reset();
        }
    }
}
void CFX_StdFontMgrImp::RemoveFont(CFX_MapPtrToPtr &fontMap, IFX_Font *pFont)
{
    FX_POSITION pos = fontMap.GetStartPosition();
    FX_LPVOID pKey;
    FX_LPVOID pFind;
    while (pos != NULL) {
        pFind = NULL;
        fontMap.GetNextAssoc(pos, pKey, pFind);
        if (pFind != (void*)pFont) {
            continue;
        }
        fontMap.RemoveKey(pKey);
        break;
    }
}
void CFX_StdFontMgrImp::RemoveFont(IFX_Font *pFont)
{
    RemoveFont(m_CPFonts, pFont);
    RemoveFont(m_FamilyFonts, pFont);
    RemoveFont(m_UnicodeFonts, pFont);
    RemoveFont(m_BufferFonts, pFont);
    RemoveFont(m_FileFonts, pFont);
    RemoveFont(m_StreamFonts, pFont);
    RemoveFont(m_DeriveFonts, pFont);
    FX_INT32 iFind = m_Fonts.Find(pFont);
    if (iFind > -1) {
        m_Fonts.RemoveAt(iFind, 1);
    }
}
FX_LPCFONTDESCRIPTOR CFX_StdFontMgrImp::FindFont(FX_LPCWSTR pszFontFamily, FX_DWORD dwFontStyles, FX_DWORD dwMatchFlags, FX_WORD wCodePage, FX_DWORD dwUSB , FX_WCHAR wUnicode )
{
    if (m_pMatcher == NULL) {
        return NULL;
    }
    FX_FONTMATCHPARAMS params;
    FX_memset(&params, 0, sizeof(params));
    params.dwUSB = dwUSB;
    params.wUnicode = wUnicode;
    params.wCodePage = wCodePage;
    params.pwsFamily = pszFontFamily;
    params.dwFontStyles = dwFontStyles;
    params.dwMatchFlags = dwMatchFlags;
    FX_LPCFONTDESCRIPTOR pDesc = m_pMatcher(&params, m_FontFaces, m_pUserData);
    if (pDesc) {
        return pDesc;
    }
#if _FXM_PLATFORM_ == _FXM_PLATFORM_WINDOWS_
    if (pszFontFamily && m_pEnumerator) {
        CFX_FontDescriptors namedFonts;
        m_pEnumerator(namedFonts, m_pUserData, pszFontFamily, wUnicode);
        params.pwsFamily = NULL;
        pDesc = m_pMatcher(&params, namedFonts, m_pUserData);
        if (pDesc == NULL) {
            return NULL;
        }
        for (FX_INT32 i = m_FontFaces.GetSize() - 1; i >= 0; i--) {
            FX_LPCFONTDESCRIPTOR pMatch = m_FontFaces.GetPtrAt(i);
            if (*pMatch == *pDesc) {
                return pMatch;
            }
        }
        int index = m_FontFaces.Add(*pDesc);
        return m_FontFaces.GetPtrAt(index);
    }
#endif
    return NULL;
}
FX_LPCFONTDESCRIPTOR FX_DefFontMatcher(FX_LPFONTMATCHPARAMS pParams, const CFX_FontDescriptors &fonts, FX_LPVOID pUserData)
{
    FX_LPCFONTDESCRIPTOR pBestFont = NULL;
    FX_INT32 iBestSimilar = 0;
    FX_BOOL bMatchStyle = (pParams->dwMatchFlags & FX_FONTMATCHPARA_MacthStyle) > 0;
    FX_INT32 iCount = fonts.GetSize();
    for (FX_INT32 i = 0; i < iCount; ++i) {
        FX_LPCFONTDESCRIPTOR pFont = fonts.GetPtrAt(i);
        if ((pFont->dwFontStyles & FX_FONTSTYLE_BoldItalic) == FX_FONTSTYLE_BoldItalic) {
            continue;
        }
        if (pParams->pwsFamily) {
            if (FXSYS_wcsicmp(pParams->pwsFamily, pFont->wsFontFace)) {
                continue;
            }
            if (pFont->uCharSet == FX_CHARSET_Symbol) {
                return pFont;
            }
        }
        if (pFont->uCharSet == FX_CHARSET_Symbol) {
            continue;
        }
        if (pParams->wCodePage != 0xFFFF) {
            if (FX_GetCodePageFromCharset(pFont->uCharSet) != pParams->wCodePage) {
                continue;
            }
        } else {
            if (pParams->dwUSB < 128) {
                FX_DWORD dwByte = pParams->dwUSB / 32;
                FX_DWORD dwUSB = 1 << (pParams->dwUSB % 32);
                if ((pFont->FontSignature.fsUsb[dwByte] & dwUSB) == 0) {
                    continue;
                }
            }
        }
        if (bMatchStyle) {
            if ((pFont->dwFontStyles & 0x0F) == (pParams->dwFontStyles & 0x0F)) {
                return pFont;
            } else {
                continue;
            }
        }
        if (pParams->pwsFamily != NULL) {
            if (FXSYS_wcsicmp(pParams->pwsFamily, pFont->wsFontFace) == 0) {
                return pFont;
            }
        }
        FX_INT32 iSimilarValue = FX_GetSimilarValue(pFont, pParams->dwFontStyles);
        if (iBestSimilar < iSimilarValue) {
            iBestSimilar = iSimilarValue;
            pBestFont = pFont;
        }
    }
    return iBestSimilar < 1 ? NULL : pBestFont;
}
FX_INT32 FX_GetSimilarValue(FX_LPCFONTDESCRIPTOR pFont, FX_DWORD dwFontStyles)
{
    FX_INT32 iValue = 0;
    if ((dwFontStyles & FX_FONTSTYLE_Symbolic) == (pFont->dwFontStyles & FX_FONTSTYLE_Symbolic)) {
        iValue += 64;
    }
    if ((dwFontStyles & FX_FONTSTYLE_FixedPitch) == (pFont->dwFontStyles & FX_FONTSTYLE_FixedPitch)) {
        iValue += 32;
    }
    if ((dwFontStyles & FX_FONTSTYLE_Serif) == (pFont->dwFontStyles & FX_FONTSTYLE_Serif)) {
        iValue += 16;
    }
    if ((dwFontStyles & FX_FONTSTYLE_Script) == (pFont->dwFontStyles & FX_FONTSTYLE_Script)) {
        iValue += 8;
    }
    return iValue;
}
FX_LPMatchFont FX_GetDefFontMatchor()
{
    return FX_DefFontMatcher;
}
#if  _FXM_PLATFORM_ == _FXM_PLATFORM_WINDOWS_
FX_DWORD FX_GetGdiFontStyles(const LOGFONTW &lf)
{
    FX_DWORD dwStyles = 0;
    if ((lf.lfPitchAndFamily & 0x03) == FIXED_PITCH) {
        dwStyles |= FX_FONTSTYLE_FixedPitch;
    }
    FX_BYTE nFamilies = lf.lfPitchAndFamily & 0xF0;
    if (nFamilies == FF_ROMAN) {
        dwStyles |= FX_FONTSTYLE_Serif;
    }
    if (nFamilies == FF_SCRIPT) {
        dwStyles |= FX_FONTSTYLE_Script;
    }
    if (lf.lfCharSet == SYMBOL_CHARSET) {
        dwStyles |= FX_FONTSTYLE_Symbolic;
    }
    return dwStyles;
}
static FX_INT32 CALLBACK FX_GdiFontEnumProc(ENUMLOGFONTEX *lpelfe, NEWTEXTMETRICEX *lpntme, DWORD dwFontType, LPARAM lParam)
{
    if (dwFontType != TRUETYPE_FONTTYPE) {
        return 1;
    }
    const LOGFONTW &lf = ((LPENUMLOGFONTEXW)lpelfe)->elfLogFont;
    if (lf.lfFaceName[0] == L'@') {
        return 1;
    }
    FX_LPFONTDESCRIPTOR pFont = FX_Alloc(FX_FONTDESCRIPTOR, 1);
    if (NULL == pFont) {
        return 0;
    }
    FXSYS_memset(pFont, 0, sizeof(FX_FONTDESCRIPTOR));
    pFont->uCharSet = lf.lfCharSet;
    pFont->dwFontStyles = FX_GetGdiFontStyles(lf);
    FXSYS_wcsncpy(pFont->wsFontFace, (FX_LPCWSTR)lf.lfFaceName, 31);
    pFont->wsFontFace[31] = 0;
    FX_memcpy(&pFont->FontSignature, &lpntme->ntmFontSig, sizeof(lpntme->ntmFontSig));
    ((CFX_FontDescriptors*)lParam)->Add(*pFont);
    FX_Free(pFont);
    return 1;
}
static void FX_EnumGdiFonts(CFX_FontDescriptors &fonts, FX_LPVOID pUserData, FX_LPCWSTR pwsFaceName, FX_WCHAR wUnicode)
{
    HDC hDC = ::GetDC(NULL);
    LOGFONTW lfFind;
    FX_memset(&lfFind, 0, sizeof(lfFind));
    lfFind.lfCharSet = DEFAULT_CHARSET;
    if (pwsFaceName) {
        FXSYS_wcsncpy((FX_LPWSTR)lfFind.lfFaceName, pwsFaceName, 31);
        lfFind.lfFaceName[31] = 0;
    }
    EnumFontFamiliesExW(hDC, (LPLOGFONTW)&lfFind, (FONTENUMPROCW)FX_GdiFontEnumProc, (LPARAM)&fonts, 0);
    ::ReleaseDC(NULL, hDC);
}
FX_LPEnumAllFonts FX_GetDefFontEnumerator()
{
    return FX_EnumGdiFonts;
}
#else
FX_LPCSTR g_FontFolders[] = {
#if _FXM_PLATFORM_ == _FXM_PLATFORM_LINUX_
    (FX_LPCSTR)"/usr/share/fonts",
    (FX_LPCSTR)"/usr/share/X11/fonts/Type1",
    (FX_LPCSTR)"/usr/share/X11/fonts/TTF",
    (FX_LPCSTR)"/usr/local/share/fonts",
#elif _FXM_PLATFORM_ == _FXM_PLATFORM_APPLE_
    (FX_LPCSTR)"~/Library/Fonts",
    (FX_LPCSTR)"/Library/Fonts",
    (FX_LPCSTR)"/System/Library/Fonts",
#elif _FXM_PLATFORM_ == _FXM_PLATFORM_ANDROID_
    (FX_LPCSTR)"/system/fonts",
#endif
};
static void FX_EnumFileFonts(CFX_FontDescriptors &fonts, FX_LPVOID pUserData, FX_LPCWSTR pwsFaceName, FX_WCHAR wUnicode)
{
    CFX_FileFontEnumerator FFE;
    for (FX_INT32 i = 0; i < sizeof(g_FontFolders) / sizeof(FX_LPCSTR); i++) {
        FFE.AddFolder(g_FontFolders[i]);
    }
    FFE.EnumFonts(fonts);
}
FX_LPEnumAllFonts FX_GetDefFontEnumerator()
{
    return FX_EnumFileFonts;
}
void CFX_FileFontEnumerator::AddFolder( FX_LPCSTR path )
{
    m_FolderPaths.Add(path);
}
FX_BOOL CFX_FileFontEnumerator::EnumFonts( CFX_FontDescriptors &Fonts )
{
    FXFT_Library& library = CFX_GEModule::Get()->GetFontMgr()->m_FTLibrary;
    if (library == NULL) {
        FXFT_Init_FreeType(&library);
    }
    if (library == NULL) {
        return FALSE;
    }
    FXFT_Face pFace = NULL;
    CFX_ByteString bsPath;
    while ((bsPath = GetNextFile()) != "") {
        if (0 != FXFT_New_Face(library, bsPath, 0, &pFace)) {
            continue;
        }
        FX_INT32 nFaceCount = pFace->num_faces;
        ReportFace(pFace, Fonts, bsPath);
        FXFT_Done_Face(pFace);
        for (FX_INT32 i = 1; i < nFaceCount; i++) {
            if (0 != FXFT_New_Face(library, bsPath, i, &pFace)) {
                continue;
            }
            ReportFace(pFace, Fonts, bsPath);
            FXFT_Done_Face(pFace);
        }
    }
    return TRUE;
}
CFX_ByteString CFX_FileFontEnumerator::GetNextFile()
{
Restart:
    FX_LPVOID pCurHandle = m_FolderQueue.GetSize() == 0 ? NULL : m_FolderQueue.GetDataPtr(m_FolderQueue.GetSize() - 1)->pFileHandle;
    if (NULL == pCurHandle) {
        if (m_FolderPaths.GetSize() < 1) {
            return "";
        }
        pCurHandle = FX_OpenFolder(m_FolderPaths[m_FolderPaths.GetSize() - 1]);
        FX_HandleParentPath hpp;
        hpp.pFileHandle = pCurHandle;
        hpp.bsParentPath = m_FolderPaths[m_FolderPaths.GetSize() - 1];
        m_FolderQueue.Add(hpp);
    }
    CFX_ByteString bsName;
    FX_BOOL	bFolder;
    CFX_ByteString bsFolderSpearator = CFX_ByteString::FromUnicode(CFX_WideString(FX_GetFolderSeparator()));
    while (TRUE) {
        if (!FX_GetNextFile(pCurHandle, bsName, bFolder)) {
            FX_CloseFolder(pCurHandle);
            m_FolderQueue.RemoveAt(m_FolderQueue.GetSize() - 1);
            if (m_FolderQueue.GetSize() == 0) {
                m_FolderPaths.RemoveAt(m_FolderPaths.GetSize() - 1);
                if (m_FolderPaths.GetSize() == 0) {
                    return "";
                } else {
                    goto Restart;
                }
            }
            pCurHandle = m_FolderQueue.GetDataPtr(m_FolderQueue.GetSize() - 1)->pFileHandle;
            continue;
        }
        if (bsName == "." || bsName == "..") {
            continue;
        }
        if (bFolder) {
            FX_HandleParentPath hpp;
            hpp.bsParentPath = m_FolderQueue.GetDataPtr(m_FolderQueue.GetSize() - 1)->bsParentPath + bsFolderSpearator + bsName;
            hpp.pFileHandle = FX_OpenFolder(hpp.bsParentPath);
            if (hpp.pFileHandle == NULL) {
                continue;
            }
            m_FolderQueue.Add(hpp);
            pCurHandle = hpp.pFileHandle;
            continue;
        }
        bsName = m_FolderQueue.GetDataPtr(m_FolderQueue.GetSize() - 1)->bsParentPath + bsFolderSpearator + bsName;
        break;
    }
    return bsName;
}
void CFX_FileFontEnumerator::ReportFace( FXFT_Face pFace, CFX_FontDescriptors& Fonts, const CFX_ByteString& bsPath )
{
    FX_LPFONTDESCRIPTOR pFont = FX_Alloc(FX_FONTDESCRIPTOR, 1);
    if (NULL == pFont) {
        return;
    }
    FXSYS_memset(pFont, 0, sizeof(FX_FONTDESCRIPTOR));
    pFont->dwFontStyles |= FXFT_Is_Face_Bold(pFace) ? FX_FONTSTYLE_Bold : 0;
    pFont->dwFontStyles |= FXFT_Is_Face_Italic(pFace) ? FX_FONTSTYLE_Italic : 0;
    pFont->dwFontStyles |= GetFlags(pFace);
    CFX_WordArray Charsets;
    GetCharsets(pFace, Charsets);
    GetUSBCSB(pFace, pFont->FontSignature.fsUsb, pFont->FontSignature.fsCsb);
    pFont->wsFontFace[31] = 0;
    CFX_WideStringArray FamilyNames;
    unsigned long nLength = 0;
    FT_ULong dwTag;
    FX_LPBYTE pTable = NULL;
    FT_ENC_TAG(dwTag, 'n', 'a', 'm', 'e');
    unsigned int error = FXFT_Load_Sfnt_Table(pFace, dwTag, 0, NULL, &nLength);
    if (0 != error || 0 == nLength) {
        return ;
    } else {
        pTable = FX_Alloc(FX_BYTE, nLength);
        if (NULL != pTable) {
            error = FXFT_Load_Sfnt_Table(pFace, dwTag, 0, pTable, NULL);
            if (0 != error) {
                FX_Free(pTable);
                pTable = NULL;
            }
        }
    }
    GetNames(pTable, FamilyNames);
    FX_Free(pTable);
    FX_BOOL bDuplicate;
    FX_INT32 nCharsetCount = Charsets.GetSize();
    FX_INT32 nFamilyNameCount = FamilyNames.GetSize();
    FX_INT32 nUniqueFontCount = 0;
    for (FX_INT32 i = 0; i < nCharsetCount; i ++) {
        for (FX_INT32 j = 0; j < nFamilyNameCount; j ++) {
            bDuplicate = FALSE;
            pFont->uCharSet = (FX_BYTE)Charsets[i];
            CFX_WideString wsCurName = FamilyNames[j];
            FXSYS_memcpy(pFont->wsFontFace, (FX_LPCWSTR)wsCurName, (FX_MIN(31, wsCurName.GetLength())) * sizeof(FX_WCHAR));
            FXSYS_assert(nUniqueFontCount <= Fonts.GetSize());
            FX_INT32 nEnd = FX_MIN(Fonts.GetSize(), Fonts.GetSize() - nUniqueFontCount);
            for (FX_INT32 k = Fonts.GetSize() - 1; k >= nEnd; k--) {
                if (*Fonts.GetPtrAt(k) == *pFont) {
                    bDuplicate = TRUE;
                    break;
                }
            }
            if (!bDuplicate) {
                nUniqueFontCount ++;
                pFont->pFontHandler = CFX_FontHandler::Create(bsPath, pFace->face_index);
                Fonts.Add(*pFont);
            }
        }
    }
    FX_Free(pFont);
}
FX_DWORD CFX_FileFontEnumerator::GetFlags(FXFT_Face pFace)
{
    FX_DWORD flag = 0;
    if (FT_IS_FIXED_WIDTH(pFace)) {
        flag |= FX_FONTSTYLE_FixedPitch;
    }
    TT_OS2* pOS2 = (TT_OS2*)FT_Get_Sfnt_Table(pFace, ft_sfnt_os2);
    if (!pOS2) {
        return flag;
    }
    if (pOS2->ulCodePageRange1 & (1 << 31)) {
        flag |= FX_FONTSTYLE_Symbolic;
    }
    if (pOS2->panose[0] == 2) {
        FX_BYTE uSerif = pOS2->panose[1];
        if ((uSerif > 1 && uSerif < 10) || uSerif > 13) {
            flag |= FX_FONTSTYLE_Serif;
        }
    }
    return flag;
}
#define GetUInt8(p) ((FX_UINT8)((p)[0]))
#define GetUInt16(p) ((FX_UINT16)((p)[0] << 8 | (p)[1]))
#define GetUInt32(p) ((FX_UINT32)((p)[0] << 24 | (p)[1] << 16 | (p)[2] << 8 | (p)[3]))
void CFX_FileFontEnumerator::GetNames(FX_LPCBYTE name_table, CFX_WideStringArray& Names)
{
    if (NULL == name_table) {
        return;
    }
    FX_LPBYTE lpTable = (FX_LPBYTE)name_table;
    FX_UINT32 nTableOffset = GetUInt32(lpTable + 8);
    FX_UINT32 nTableLength = GetUInt32(lpTable + 12);
    CFX_WideString wsFamily;
    FX_LPBYTE sp = lpTable + 2;
    FX_LPBYTE lpNameRecord = lpTable + 6;
    FX_UINT16 nNameCount = GetUInt16(sp);
    FX_LPBYTE lpStr = lpTable + GetUInt16(sp + 2);
    CFX_WordArray lid;
    for (FX_UINT16 j = 0; j < nNameCount; j++) {
        FX_UINT16 nNameID = GetUInt16(lpNameRecord + j * 12 + 6);
        if (nNameID != 1) {
            continue;
        }
        FX_UINT16 nPlatformID = GetUInt16(lpNameRecord + j * 12 + 0);
        FX_UINT16 nLanguageID = GetUInt16(lpNameRecord + j * 12 + 4);
        FX_UINT16 nNameLength = GetUInt16(lpNameRecord + j * 12 + 8);
        FX_UINT16 nNameOffset = GetUInt16(lpNameRecord + j * 12 + 10);
        wsFamily.Empty();
        if (nPlatformID != 1) {
            for (FX_UINT16 k = 0; k < nNameLength / 2; k++) {
                FX_WCHAR wcTemp = GetUInt16(lpStr + nNameOffset + k * 2);
                wsFamily += wcTemp;
            }
            Names.Add(wsFamily);
        } else {
            for (FX_UINT16 k = 0; k < nNameLength; k++) {
                FX_WCHAR wcTemp = GetUInt8(lpStr + nNameOffset + k);
                wsFamily += wcTemp;
            }
            Names.Add(wsFamily);
        }
    }
}
#undef GetUInt8
#undef GetUInt16
#undef GetUInt32
struct FX_BIT2CHARSET {
    FX_WORD wBit;
    FX_WORD wCharset;
};
FX_BIT2CHARSET g_FX_Bit2Charset1[16] = {
    {1 << 0 , FX_CHARSET_ANSI},
    {1 << 1 , FX_CHARSET_MSWin_EasterEuropean},
    {1 << 2 , FX_CHARSET_MSWin_Cyrillic},
    {1 << 3 , FX_CHARSET_MSWin_Greek},
    {1 << 4 , FX_CHARSET_MSWin_Turkish},
    {1 << 5 , FX_CHARSET_MSWin_Hebrew},
    {1 << 6 , FX_CHARSET_MSWin_Arabic},
    {1 << 7 , FX_CHARSET_MSWin_Baltic},
    {1 << 8 , FX_CHARSET_MSWin_Vietnamese},
    {1 << 9 , FX_CHARSET_Default},
    {1 << 10 , FX_CHARSET_Default},
    {1 << 11 , FX_CHARSET_Default},
    {1 << 12 , FX_CHARSET_Default},
    {1 << 13 , FX_CHARSET_Default},
    {1 << 14 , FX_CHARSET_Default},
    {1 << 15 , FX_CHARSET_Default},
};
FX_BIT2CHARSET g_FX_Bit2Charset2[16] = {
    {1 << 0 , FX_CHARSET_Thai},
    {1 << 1 , FX_CHARSET_ShiftJIS},
    {1 << 2 , FX_CHARSET_ChineseSimplified},
    {1 << 3 , FX_CHARSET_Korean},
    {1 << 4 , FX_CHARSET_ChineseTriditional},
    {1 << 5 , FX_CHARSET_Johab},
    {1 << 6 , FX_CHARSET_Default},
    {1 << 7 , FX_CHARSET_Default},
    {1 << 8 , FX_CHARSET_Default},
    {1 << 9 , FX_CHARSET_Default},
    {1 << 10 , FX_CHARSET_Default},
    {1 << 11 , FX_CHARSET_Default},
    {1 << 12 , FX_CHARSET_Default},
    {1 << 13 , FX_CHARSET_Default},
    {1 << 14 , FX_CHARSET_OEM},
    {1 << 15 , FX_CHARSET_Symbol},
};
FX_BIT2CHARSET g_FX_Bit2Charset3[16] = {
    {1 << 0 , FX_CHARSET_Default},
    {1 << 1 , FX_CHARSET_Default},
    {1 << 2 , FX_CHARSET_Default},
    {1 << 3 , FX_CHARSET_Default},
    {1 << 4 , FX_CHARSET_Default},
    {1 << 5 , FX_CHARSET_Default},
    {1 << 6 , FX_CHARSET_Default},
    {1 << 7 , FX_CHARSET_Default},
    {1 << 8 , FX_CHARSET_Default},
    {1 << 9 , FX_CHARSET_Default},
    {1 << 10 , FX_CHARSET_Default},
    {1 << 11 , FX_CHARSET_Default},
    {1 << 12 , FX_CHARSET_Default},
    {1 << 13 , FX_CHARSET_Default},
    {1 << 14 , FX_CHARSET_Default},
    {1 << 15 , FX_CHARSET_Default},
};
FX_BIT2CHARSET g_FX_Bit2Charset4[16] = {
    {1 << 0 , FX_CHARSET_Default },
    {1 << 1 , FX_CHARSET_Default },
    {1 << 2 , FX_CHARSET_Default },
    {1 << 3 , FX_CHARSET_Default },
    {1 << 4 , FX_CHARSET_Default },
    {1 << 5 , FX_CHARSET_Default },
    {1 << 6 , FX_CHARSET_Default },
    {1 << 7 , FX_CHARSET_Default },
    {1 << 8 , FX_CHARSET_Default },
    {1 << 9 , FX_CHARSET_Default },
    {1 << 10 , FX_CHARSET_Default },
    {1 << 11 , FX_CHARSET_Default },
    {1 << 12 , FX_CHARSET_Default },
    {1 << 13 , FX_CHARSET_Default },
    {1 << 14 , FX_CHARSET_Default },
    {1 << 15 , FX_CHARSET_US },
};
#define CODEPAGERANGE_IMPLEMENT(n) \
    for (FX_INT32 i = 0; i < 16; i++)\
    {\
        if ((a##n & g_FX_Bit2Charset##n[i].wBit) != 0)\
        {\
            Charsets.Add(g_FX_Bit2Charset##n[i].wCharset);\
        }\
    }
void CFX_FileFontEnumerator::GetCharsets( FXFT_Face pFace, CFX_WordArray& Charsets )
{
    Charsets.RemoveAll();
    TT_OS2* pOS2 = (TT_OS2*)FT_Get_Sfnt_Table(pFace, ft_sfnt_os2);
    if (NULL != pOS2) {
        FX_WORD a1, a2, a3, a4;
        a1 = pOS2->ulCodePageRange1 & 0x0000ffff;
        CODEPAGERANGE_IMPLEMENT(1);
        a2 = (pOS2->ulCodePageRange1 >> 16) & 0x0000ffff;
        CODEPAGERANGE_IMPLEMENT(2);
        a3 = pOS2->ulCodePageRange2 & 0x0000ffff;
        CODEPAGERANGE_IMPLEMENT(3);
        a4 = (pOS2->ulCodePageRange2 >> 16) & 0x0000ffff;
        CODEPAGERANGE_IMPLEMENT(4);
    } else {
        Charsets.Add(FX_CHARSET_Default);
    }
}
#undef CODEPAGERANGE_IMPLEMENT
void CFX_FileFontEnumerator::GetUSBCSB( FXFT_Face pFace, FX_DWORD* USB, FX_DWORD* CSB )
{
    TT_OS2* pOS2 = (TT_OS2*)FT_Get_Sfnt_Table(pFace, ft_sfnt_os2);
    if (NULL != pOS2) {
        USB[0] = pOS2->ulUnicodeRange1;
        USB[1] = pOS2->ulUnicodeRange2;
        USB[2] = pOS2->ulUnicodeRange3;
        USB[3] = pOS2->ulUnicodeRange4;
        CSB[0] = pOS2->ulCodePageRange1;
        CSB[1] = pOS2->ulCodePageRange2;
    } else {
        USB[0] = 0;
        USB[1] = 0;
        USB[2] = 0;
        USB[3] = 0;
        CSB[0] = 0;
        CSB[1] = 0;
    }
}
FXFT_Face CFX_FontHandler::CreateFace()
{
    FXFT_Library& library = CFX_GEModule::Get()->GetFontMgr()->m_FTLibrary;
    if (library == NULL) {
        FXFT_Init_FreeType(&library);
    }
    FXFT_Face pFace = NULL;
    FXFT_New_Face(library, m_bsPath, m_nFaceIndex, &pFace);
    return pFace;
}
#endif
