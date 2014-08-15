// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef _FX_FONTMGR_IMP
#define _FX_FONTMGR_IMP
FX_INT32 FX_GetSimilarValue(FX_LPCFONTDESCRIPTOR pFont, FX_DWORD dwFontStyles);
FX_LPCFONTDESCRIPTOR FX_DefFontMatcher(FX_LPFONTMATCHPARAMS pParams, const CFX_FontDescriptors &fonts, FX_LPVOID pUserData);
class CFX_StdFontMgrImp : public IFX_FontMgr, public CFX_Object
{
public:
    CFX_StdFontMgrImp(FX_LPEnumAllFonts pEnumerator, FX_LPMatchFont pMatcher, FX_LPVOID pUserData);
    ~CFX_StdFontMgrImp();
    virtual void			Release()
    {
        delete this;
    }
    virtual IFX_Font*		GetDefFontByCodePage(FX_WORD wCodePage, FX_DWORD dwFontStyles, FX_LPCWSTR pszFontFamily = NULL);
    virtual IFX_Font*		GetDefFontByCharset(FX_BYTE nCharset, FX_DWORD dwFontStyles, FX_LPCWSTR pszFontFamily = NULL);
    virtual IFX_Font*		GetDefFontByUnicode(FX_WCHAR wUnicode, FX_DWORD dwFontStyles, FX_LPCWSTR pszFontFamily = NULL);
    virtual IFX_Font*		GetDefFontByLanguage(FX_WORD wLanguage, FX_DWORD dwFontStyles, FX_LPCWSTR pszFontFamily = NULL);
    virtual IFX_Font*		LoadFont(FX_LPCWSTR pszFontFamily, FX_DWORD dwFontStyles, FX_WORD wCodePage = 0xFFFF);
    virtual IFX_Font*		LoadFont(FX_LPCBYTE pBuffer, FX_INT32 iLength);
    virtual IFX_Font*		LoadFont(FX_LPCWSTR pszFileName);
    virtual IFX_Font*		LoadFont(IFX_Stream *pFontStream, FX_LPCWSTR pszFontAlias = NULL, FX_DWORD dwFontStyles = 0, FX_WORD wCodePage = 0, FX_BOOL bSaveStream = FALSE);
    virtual IFX_Font*		LoadFont(IFX_Font *pSrcFont, FX_DWORD dwFontStyles, FX_WORD wCodePage = 0xFFFF);
    virtual void			ClearFontCache();
    virtual void			RemoveFont(IFX_Font *pFont);
protected:
    FX_LPMatchFont			m_pMatcher;
    FX_LPEnumAllFonts		m_pEnumerator;
    CFX_FontDescriptors		m_FontFaces;
    CFX_PtrArray			m_Fonts;
    CFX_MapPtrToPtr			m_CPFonts;
    CFX_MapPtrToPtr			m_FamilyFonts;
    CFX_MapPtrToPtr			m_UnicodeFonts;
    CFX_MapPtrToPtr			m_BufferFonts;
    CFX_MapPtrToPtr			m_FileFonts;
    CFX_MapPtrToPtr			m_StreamFonts;
    CFX_MapPtrToPtr			m_DeriveFonts;
    CFX_MapPtrToPtr			m_CachedFace;
    FX_LPVOID				m_pUserData;
    void					RemoveFont(CFX_MapPtrToPtr &fontMap, IFX_Font *pFont);
    FX_LPCFONTDESCRIPTOR	FindFont(FX_LPCWSTR pszFontFamily, FX_DWORD dwFontStyles, FX_DWORD dwMatchFlags, FX_WORD wCodePage, FX_DWORD dwUSB = 999, FX_WCHAR wUnicode = 0);
    IFX_Font*				GetFont(FX_LPCFONTDESCRIPTOR pFD, FX_DWORD dwFontStyles);
};
#if _FXM_PLATFORM_ != _FXM_PLATFORM_WINDOWS_
struct FX_HandleParentPath : public CFX_Object {
    FX_HandleParentPath()
    {
    }
    FX_HandleParentPath(const FX_HandleParentPath& x)
    {
        pFileHandle = x.pFileHandle;
        bsParentPath = x.bsParentPath;
    }
    void* pFileHandle;
    CFX_ByteString bsParentPath;
};
class CFX_FontHandler : public IFX_FontHandler, public CFX_Object
{
public:
    static IFX_FontHandler* Create(CFX_ByteString bsPath, FX_INT32 nFaceIndex)
    {
        CFX_FontHandler* pFontHandler = FX_NEW CFX_FontHandler;
        if (NULL == pFontHandler) {
            return NULL;
        }
        pFontHandler->m_bsPath = bsPath;
        pFontHandler->m_nFaceIndex = nFaceIndex;
        return (IFX_FontHandler*)pFontHandler;
    }
    virtual void		Release()
    {
        delete this;
    };
    virtual FXFT_Face	CreateFace();
private:
    CFX_ByteString m_bsPath;
    FX_INT32 m_nFaceIndex;
};
class CFX_FileFontEnumerator : public CFX_Object
{
public:
    CFX_FileFontEnumerator()
    {
    }
    ~CFX_FileFontEnumerator()
    {
        m_FolderQueue.RemoveAll();
        m_FolderPaths.RemoveAll();
    }
    virtual void			AddFolder(FX_LPCSTR path);
    virtual FX_BOOL			EnumFonts(CFX_FontDescriptors& Fonts);
private:
    CFX_ByteString			GetNextFile();
    void					ReportFace(FXFT_Face pFace, CFX_FontDescriptors& Fonts, const CFX_ByteString& bsPath);
    void					GetNames(FX_LPCBYTE name_table, CFX_WideStringArray& Names);
    void					GetCharsets(FXFT_Face pFace, CFX_WordArray& Charsets);
    void					GetUSBCSB(FXFT_Face pFace, FX_DWORD* USB, FX_DWORD* CSB);
    FX_DWORD				GetFlags(FXFT_Face pFace);
    CFX_ObjectArray<FX_HandleParentPath>	m_FolderQueue;
    CFX_ByteStringArray		m_FolderPaths;
};
#endif
#endif
