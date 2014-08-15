// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
 
// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "../../include/fsdk_define.h"
#include "../../include/fpdfformfill.h"
#include "../../include/fsdk_mgr.h"
#include "../../include/fpdfxfa/fpdfxfa_doc.h"
#include "../../include/fpdfxfa/fpdfxfa_app.h"
#include "../../include/fpdfxfa/fpdfxfa_util.h"

//#include "../../include/jsapi/fxjs_v8.h"
//#include "../../include/javascript/IJavaScript.h"

CPDFXFA_App* CPDFXFA_App::m_pApp = NULL;

CPDFXFA_App* FPDFXFA_GetApp()
{
	if (!CPDFXFA_App::m_pApp)
		CPDFXFA_App::m_pApp = FX_NEW CPDFXFA_App();

	return CPDFXFA_App::m_pApp;
}

void FPDFXFA_ReleaseApp()
{
	if (CPDFXFA_App::m_pApp)
		delete CPDFXFA_App::m_pApp;
	CPDFXFA_App::m_pApp = NULL;
}

CJS_RuntimeFactory* g_GetJSRuntimeFactory()
{
	static CJS_RuntimeFactory g_JSRuntimeFactory;
	return &g_JSRuntimeFactory;
}

CPDFXFA_App::CPDFXFA_App() : 
	m_pXFAApp(NULL), 
	m_pFontMgr(NULL),
	m_hJSERuntime(NULL),
	m_pJSRuntime(NULL),
	m_pEnv(NULL),
	m_csAppType(JS_STR_VIEWERTYPE_STANDARD)
{
	m_pJSRuntimeFactory = NULL;
	m_pJSRuntimeFactory = g_GetJSRuntimeFactory();
	m_pJSRuntimeFactory->AddRef();
	
}
IFXJS_Runtime* CPDFXFA_App::GetJSRuntime()
{
	FXSYS_assert(m_pJSRuntimeFactory);
	if(!m_pJSRuntime)
		m_pJSRuntime = m_pJSRuntimeFactory->NewJSRuntime(this);
	return m_pJSRuntime;
}

CPDFXFA_App::~CPDFXFA_App()
{
	if (m_pFontMgr)
	{
		m_pFontMgr->Release();
		m_pFontMgr = NULL;
	}

	if (m_pXFAApp)
	{
		m_pXFAApp->Release();
		m_pXFAApp = NULL;
	}

	if (m_pJSRuntime && m_pJSRuntimeFactory)
		m_pJSRuntimeFactory->DeleteJSRuntime(m_pJSRuntime);
	m_pJSRuntimeFactory->Release();

	if (m_hJSERuntime)
	{
		FXJSE_Runtime_Release(m_hJSERuntime);
		m_hJSERuntime = NULL;
	}

	FXJSE_Finalize();

	BC_Library_Destory();
}

FX_BOOL CPDFXFA_App::Initialize()
{
	BC_Library_Init();

	FXJSE_Initialize();
	m_hJSERuntime = FXJSE_Runtime_Create();

	if (!m_hJSERuntime) 
		return FALSE;

	m_pJSRuntime = m_pJSRuntimeFactory->NewJSRuntime(this);
	
	m_pXFAApp = IXFA_App::Create(this);
	if (!m_pXFAApp)
		return FALSE;

	m_pFontMgr = XFA_GetDefaultFontMgr();
	if (!m_pFontMgr)
		return FALSE;

	m_pXFAApp->SetDefaultFontMgr(m_pFontMgr);

	return TRUE;
}

FX_BOOL CPDFXFA_App::SetFormFillEnv(CPDFDoc_Environment* pEnv)
{
	m_pEnv = pEnv;
	return TRUE;
}

void CPDFXFA_App::GetAppType(CFX_WideString &wsAppType)
{
	wsAppType = m_csAppType;
}

void CPDFXFA_App::GetAppName(CFX_WideString& wsName)
{
	if (m_pEnv)
	{
		wsName = m_pEnv->FFI_GetAppName();
	}
}

void CPDFXFA_App::SetAppType(FX_WSTR wsAppType)
{
	m_csAppType = wsAppType;
}

void CPDFXFA_App::GetLanguage(CFX_WideString &wsLanguage)
{
	if (m_pEnv)
	{
		wsLanguage = m_pEnv->FFI_GetLanguage();
	}
}

void CPDFXFA_App::GetPlatform(CFX_WideString &wsPlatform)
{
	if (m_pEnv)
	{
		wsPlatform = m_pEnv->FFI_GetPlatform();
	}
}

void CPDFXFA_App::GetVariation(CFX_WideString &wsVariation)
{
	wsVariation = JS_STR_VIEWERVARIATION;
}

void CPDFXFA_App::GetVersion(CFX_WideString &wsVersion)
{
	wsVersion = JS_STR_VIEWERVERSION;
}

void CPDFXFA_App::Beep(FX_DWORD dwType)
{
	if (m_pEnv)
	{
		m_pEnv->JS_appBeep(dwType);
	}
}

FX_INT32 CPDFXFA_App::MsgBox(FX_WSTR wsMessage, FX_WSTR wsTitle, FX_DWORD dwIconType, FX_DWORD dwButtonType)
{
	if (m_pEnv)
	{
		return m_pEnv->JS_appAlert(wsMessage.GetPtr(), wsTitle.GetPtr(), dwIconType, dwButtonType);
	}
	return -1;
}

void CPDFXFA_App::Response(CFX_WideString &wsAnswer, FX_WSTR wsQuestion, FX_WSTR wsTitle, FX_WSTR wsDefaultAnswer, FX_BOOL bMark)
{
	if (m_pEnv)
	{
		int nLength = 2048;
		char* pBuff = new char[nLength];
		nLength = m_pEnv->JS_appResponse(wsQuestion.GetPtr(), wsTitle.GetPtr(), wsDefaultAnswer.GetPtr(), NULL, bMark, pBuff, nLength);
		if(nLength > 0)
		{
			nLength = nLength>2046?2046:nLength;
			pBuff[nLength] = 0;
			pBuff[nLength+1] = 0;
			wsAnswer = CFX_WideString::FromUTF16LE((unsigned short*)pBuff, nLength);
		}
		delete[] pBuff;
	}
}

FX_INT32 CPDFXFA_App::GetCurDocumentInBatch()
{
	if (m_pEnv)
	{
		return m_pEnv->FFI_GetCurDocument();
	}
	return 0;
}

FX_INT32 CPDFXFA_App::GetDocumentCountInBatch()
{
	if (m_pEnv)
	{
		return m_pEnv->FFI_GetDocumentCount();
	}

	return 0;
}

IFX_FileRead* CPDFXFA_App::DownloadURL(FX_WSTR wsURL)
{
	if (m_pEnv)
	{
		return m_pEnv->FFI_DownloadFromURL(wsURL.GetPtr());
	}
	return NULL;
}

FX_BOOL CPDFXFA_App::PostRequestURL(FX_WSTR wsURL, FX_WSTR wsData, FX_WSTR wsContentType, 
	FX_WSTR wsEncode, FX_WSTR wsHeader, CFX_WideString &wsResponse)
{
	if (m_pEnv)
	{
		wsResponse = m_pEnv->FFI_PostRequestURL(wsURL.GetPtr(), wsData.GetPtr(), wsContentType.GetPtr(), wsEncode.GetPtr(), wsHeader.GetPtr());
		return TRUE;
	}
	return FALSE;
}

FX_BOOL CPDFXFA_App::PutRequestURL(FX_WSTR wsURL, FX_WSTR wsData, FX_WSTR wsEncode)
{
	if (m_pEnv)
	{
		return m_pEnv->FFI_PutRequestURL(wsURL.GetPtr(), wsData.GetPtr(), wsEncode.GetPtr());
	}
	return FALSE;
}

void CPDFXFA_App::LoadString(FX_INT32 iStringID, CFX_WideString &wsString)
{

}

FX_BOOL CPDFXFA_App::ShowFileDialog(FX_WSTR wsTitle, FX_WSTR wsFilter, CFX_WideStringArray &wsPathArr, FX_BOOL bOpen)
{
	//if (m_pEnv)
	//{
	//	return m_pEnv->FFI_ShowFileDialog(wsTitle.GetPtr(), wsFilter.GetPtr(), wsPathArr, bOpen);
	//}
	return FALSE;
}

IFWL_AdapterTimerMgr* CPDFXFA_App::GetTimerMgr()
{
	CXFA_FWLAdapterTimerMgr* pAdapter = NULL;
	if (m_pEnv)
		pAdapter = FX_NEW CXFA_FWLAdapterTimerMgr(m_pEnv);
	return pAdapter;
}
