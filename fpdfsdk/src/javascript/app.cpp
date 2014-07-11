// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
 
// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "../../include/javascript/JavaScript.h"
#include "../../include/javascript/IJavaScript.h"
#include "../../include/javascript/JS_Define.h"
#include "../../include/javascript/JS_Object.h"
#include "../../include/javascript/JS_Value.h"
#include "../../include/javascript/app.h"
#include "../../include/javascript/JS_EventHandler.h"
#include "../../include/javascript/resource.h"
#include "../../include/javascript/JS_Context.h"
#include "../../include/javascript/JS_Runtime.h"
#include "../../include/javascript/Document.h"


static v8::Isolate* GetIsolate(IFXJS_Context* cc)
{
    CJS_Context* pContext = (CJS_Context *)cc;
    ASSERT(pContext != NULL);

    CJS_Runtime* pRuntime = pContext->GetJSRuntime();
    ASSERT(pRuntime != NULL);

    return pRuntime->GetIsolate();
}

/* ---------------------------- TimerObj ---------------------------- */

BEGIN_JS_STATIC_CONST(CJS_TimerObj)
END_JS_STATIC_CONST()

BEGIN_JS_STATIC_PROP(CJS_TimerObj)
END_JS_STATIC_PROP()

BEGIN_JS_STATIC_METHOD(CJS_TimerObj)
END_JS_STATIC_METHOD()

IMPLEMENT_JS_CLASS(CJS_TimerObj, TimerObj)

TimerObj::TimerObj(CJS_Object* pJSObject)
: CJS_EmbedObj(pJSObject),
m_pTimer(NULL)
{
    
}

TimerObj::~TimerObj()
{
}

void TimerObj::SetTimer(CJS_Timer* pTimer)
{
    m_pTimer = pTimer;
}

CJS_Timer* TimerObj::GetTimer() const
{
    return m_pTimer;
}

#define JS_STR_VIEWERTYPE_READER		L"Reader"
#define JS_STR_VIEWERTYPE_STANDARD		L"Exchange"
#define JS_STR_VIEWERVARIATION			L"Full"
#define JS_STR_PLATFORM					L"WIN"
#define JS_STR_LANGUANGE				L"ENU"
#define JS_STR_VIEWERVERSION			8
#define JS_NUM_FORMSVERSION				7

#define JS_FILEPATH_MAXLEN				2000

/* ---------------------------- app ---------------------------- */

BEGIN_JS_STATIC_CONST(CJS_App)
END_JS_STATIC_CONST()

BEGIN_JS_STATIC_PROP(CJS_App)
    JS_STATIC_PROP_ENTRY(activeDocs)
    JS_STATIC_PROP_ENTRY(calculate)
    JS_STATIC_PROP_ENTRY(formsVersion)
    JS_STATIC_PROP_ENTRY(fs)
    JS_STATIC_PROP_ENTRY(fullscreen)
    JS_STATIC_PROP_ENTRY(language)
    JS_STATIC_PROP_ENTRY(media)
    JS_STATIC_PROP_ENTRY(platform)
    JS_STATIC_PROP_ENTRY(runtimeHighlight)
    JS_STATIC_PROP_ENTRY(viewerType)
    JS_STATIC_PROP_ENTRY(viewerVariation)
    JS_STATIC_PROP_ENTRY(viewerVersion)	
END_JS_STATIC_PROP()

BEGIN_JS_STATIC_METHOD(CJS_App)
    JS_STATIC_METHOD_ENTRY(alert, 6)
    JS_STATIC_METHOD_ENTRY(beep, 1)
    JS_STATIC_METHOD_ENTRY(browseForDoc, 0)
    JS_STATIC_METHOD_ENTRY(clearInterval, 1)
    JS_STATIC_METHOD_ENTRY(clearTimeOut, 1)
    JS_STATIC_METHOD_ENTRY(execDialog, 3)
    JS_STATIC_METHOD_ENTRY(execMenuItem,  1)
    JS_STATIC_METHOD_ENTRY(findComponent, 1)
    JS_STATIC_METHOD_ENTRY(goBack, 0)
    JS_STATIC_METHOD_ENTRY(goForward, 0)
    JS_STATIC_METHOD_ENTRY(launchURL, 0)
    JS_STATIC_METHOD_ENTRY(mailMsg, 0)	
    JS_STATIC_METHOD_ENTRY(newFDF, 0)
    JS_STATIC_METHOD_ENTRY(newDoc, 0)
    JS_STATIC_METHOD_ENTRY(openDoc, 0)
    JS_STATIC_METHOD_ENTRY(openFDF, 5)
    JS_STATIC_METHOD_ENTRY(popUpMenuEx, 0)
    JS_STATIC_METHOD_ENTRY(popUpMenu, 0)
    JS_STATIC_METHOD_ENTRY(response, 0)
    JS_STATIC_METHOD_ENTRY(setInterval, 2)
    JS_STATIC_METHOD_ENTRY(setTimeOut, 2)
END_JS_STATIC_METHOD()

IMPLEMENT_JS_CLASS(CJS_App,app)

app::app(CJS_Object * pJSObject) : CJS_EmbedObj(pJSObject) ,
    m_bCalculate(true),
    m_pRuntime(NULL),
    m_bRuntimeHighLight(false)
//	m_pMenuHead(NULL)
{
}

app::~app(void)
{
    for (int i=0,sz=m_aTimer.GetSize(); i<sz; i++)
        delete m_aTimer[i];

    m_aTimer.RemoveAll();
}

FX_BOOL app::activeDocs(OBJ_PROP_PARAMS)
{
    if (vp.IsGetting())
    {

        CJS_Context* pContext = (CJS_Context *)cc;
        ASSERT(pContext != NULL);
        
        CPDFDoc_Environment* pApp = pContext->GetReaderApp();
        ASSERT(pApp != NULL);

        CJS_Runtime* pRuntime = pContext->GetJSRuntime();
        ASSERT(pRuntime != NULL);
        
        CPDFSDK_Document* pCurDoc = pContext->GetReaderDocument();
        
        CJS_Array aDocs(pRuntime->GetIsolate());
//		int iNumDocs = pApp->CountDocuments();
        
// 		for(int iIndex = 0; iIndex<iNumDocs; iIndex++)
// 		{
            CPDFSDK_Document* pDoc = pApp->GetCurrentDoc();
            if (pDoc)
            {
                CJS_Document * pJSDocument = NULL;
                
                if (pDoc == pCurDoc)
                {
                    JSFXObject pObj = JS_GetThisObj(*pRuntime);
                    
                    if (JS_GetObjDefnID(pObj) == JS_GetObjDefnID(*pRuntime, L"Document"))
                    {
                        pJSDocument = (CJS_Document*)JS_GetPrivate(pRuntime->GetIsolate(),pObj);
                    }
                }
                else
                {
                    JSFXObject pObj = JS_NewFxDynamicObj(*pRuntime, pContext, JS_GetObjDefnID(*pRuntime,L"Document"));
                    pJSDocument = (CJS_Document*)JS_GetPrivate(pRuntime->GetIsolate(),pObj);
                    ASSERT(pJSDocument != NULL);
                    
                    
                    //			pDocument->AttachDoc(pDoc);
                }
                
                aDocs.SetElement(0,CJS_Value(pRuntime->GetIsolate(),pJSDocument));
            }
    //		}
        
        if (aDocs.GetLength() > 0)
            vp << aDocs;
        else
            vp.SetNull();
        return TRUE;
    }
    return FALSE;
}

FX_BOOL app::calculate(OBJ_PROP_PARAMS)
{
    if (vp.IsSetting())
    {
        bool bVP;
        vp >> bVP;
        m_bCalculate = (FX_BOOL)bVP;

        CJS_Context* pContext = (CJS_Context*)cc;
        ASSERT(pContext != NULL);
        
        CPDFDoc_Environment* pApp = pContext->GetReaderApp();
        ASSERT(pApp != NULL);
        
        CJS_Runtime* pRuntime = pContext->GetJSRuntime();
        ASSERT(pRuntime != NULL);

        CJS_Array aDocs(pRuntime->GetIsolate());
// 		int iNumDocs = pApp->CountDocuments();
// 		
// 		for (int iIndex = 0;iIndex < iNumDocs; iIndex++)
// 		{
            if (CPDFSDK_Document* pDoc = pApp->GetCurrentDoc())
            {
                CPDFSDK_InterForm* pInterForm = (CPDFSDK_InterForm*)pDoc->GetInterForm();
                ASSERT(pInterForm != NULL);
                pInterForm->EnableCalculate((FX_BOOL)m_bCalculate);
            }			
//		}
    }
    else
    {
        vp << (bool)m_bCalculate;
    }
    
    return TRUE;
}

FX_BOOL app::formsVersion(OBJ_PROP_PARAMS)
{
    if (vp.IsGetting())
    {
        vp << JS_NUM_FORMSVERSION;
        return TRUE;
    }
    
    return FALSE;
}

FX_BOOL app::viewerType(OBJ_PROP_PARAMS)
{


    
    
    

    if (vp.IsGetting())
    {
// 		if (pApp->GetAppName() == PHANTOM)
// 			vp << JS_STR_VIEWERTYPE_STANDARD;
// 		else
// 			vp << JS_STR_VIEWERTYPE_READER;
        vp << L"unknown";

        //vp << pApp->GetAppTitle();
        return TRUE;
    }
    
    return FALSE;
}

FX_BOOL app::viewerVariation(OBJ_PROP_PARAMS)
{
    if (vp.IsGetting())
    {
        vp << JS_STR_VIEWERVARIATION;
        return TRUE;
    }
    
    return FALSE;
}

FX_BOOL app::viewerVersion(OBJ_PROP_PARAMS)
{
    if (vp.IsGetting())
    {
        vp << JS_STR_VIEWERVERSION;
        return TRUE;
    }
    
    return FALSE;
}

FX_BOOL app::platform(OBJ_PROP_PARAMS)
{
    if (vp.IsGetting())
    {
        vp << JS_STR_PLATFORM;
        return TRUE;
    }
    
    return FALSE;
}

FX_BOOL app::language(OBJ_PROP_PARAMS)
{
    if (vp.IsGetting())
    {
        vp << JS_STR_LANGUANGE;
        return TRUE;
    }
    
    return FALSE;
}

//creates a new fdf object that contains no data
//comment: need reader support
//note:
//CFDF_Document * CPDFDoc_Environment::NewFDF();
FX_BOOL app::newFDF(OBJ_METHOD_PARAMS)
{
    return TRUE;
}
//opens a specified pdf document and returns its document object
//comment:need reader support
//note: as defined in js reference, the proto of this function's fourth parmeters, how old an fdf document while do not show it.
//CFDF_Document * CPDFDoc_Environment::OpenFDF(string strPath,bool bUserConv);

FX_BOOL app::openFDF(OBJ_METHOD_PARAMS)
{
    return TRUE;
}

FX_BOOL app::alert(OBJ_METHOD_PARAMS)
{
    int iSize = params.size();
    if (iSize < 1)
        return FALSE;

    CFX_WideString swMsg = L"";
    CFX_WideString swTitle = L"";
    int iIcon = 0;
    int iType = 0;

    v8::Isolate* isolate = GetIsolate(cc);

    if (iSize == 1)
    {
        if (params[0].GetType() == VT_object)
        {
            JSObject pObj = params[0];
            {
                v8::Handle<v8::Value> pValue = JS_GetObjectElement(isolate, pObj, L"cMsg");
                swMsg = CJS_Value(isolate,pValue,VT_unknown).operator CFX_WideString();

                pValue = JS_GetObjectElement(isolate,pObj,L"cTitle");
                swTitle = CJS_Value(isolate, pValue,VT_unknown).operator CFX_WideString();

                pValue = JS_GetObjectElement(isolate,pObj,L"nIcon");
                iIcon = (int)CJS_Value(isolate,pValue,VT_unknown);

                pValue = JS_GetObjectElement(isolate,pObj,L"nType");
                iType = (int)CJS_Value(isolate,pValue,VT_unknown);
            }

            if (swMsg == L"")
            {
                CJS_Array carray(isolate);
                if (params[0].ConvertToArray(carray))
                {
                    int iLenth = carray.GetLength();
                    CJS_Value* pValue = new CJS_Value(isolate);
//					if (iLenth == 1)
//						pValue = new CJS_Value(isolate);
// 					else if (iLenth > 1)
// 						pValue = new CJS_Value[iLenth];

                    for(int i = 0; i < iLenth; i++)
                    {
                        carray.GetElement(i, *pValue);
                        swMsg += (*pValue).operator CFX_WideString();
                        if (i < iLenth - 1)
                            swMsg += L",  ";
                    }

                    if(pValue) delete pValue;
// 					if ((iLenth > 1) && pValue)
// 					{
// 						delete[]pValue;
// 						pValue = NULL;
// 					}
// 					else if ((iLenth == 1) && pValue)
// 					{
// 						delete pValue;
// 						pValue = NULL;
// 					}
                }
            }

            if (swTitle == L"")
                swTitle = JSGetStringFromID((CJS_Context*)cc, IDS_STRING_JSALERT);
        }
        else if (params[0].GetType() == VT_boolean)
        {
            FX_BOOL bGet = (FX_BOOL)params[0];
            if (bGet)
                swMsg = L"true";
            else
                swMsg = L"false";

            swTitle = JSGetStringFromID((CJS_Context*)cc, IDS_STRING_JSALERT);
        }
        else
        {
            swMsg = params[0];
            swTitle = JSGetStringFromID((CJS_Context*)cc, IDS_STRING_JSALERT);
        }
    }
    else
    {
        if (params[0].GetType() == VT_boolean)
        {
            FX_BOOL bGet = (FX_BOOL)params[0];
            if (bGet)
                swMsg = L"true";
            else
                swMsg = L"false";
        }
        else
        {
            swMsg = params[0];
        }
        swTitle = JSGetStringFromID((CJS_Context*)cc, IDS_STRING_JSALERT);
        
        for(int i = 1;i<iSize;i++)
        {
            if (i == 1)
                iIcon = int(params[i]);
            if (i == 2)
                iType = int(params[i]);
            if (i == 3)
                swTitle = params[i];			
        }
    }


    CJS_Context* pContext = (CJS_Context*)cc;
    ASSERT(pContext != NULL);
    CJS_Runtime* pRuntime = pContext->GetJSRuntime();
    ASSERT(pRuntime != NULL);
    pRuntime->BeginBlock();
    vRet = MsgBox(pRuntime->GetReaderApp(), JSGetPageView(cc),swMsg,swTitle,iType,iIcon);
    pRuntime->EndBlock();
    
    return TRUE;
}


FX_BOOL app::beep(OBJ_METHOD_PARAMS)
{
    if (params.size() == 1)
    {
        CJS_Context* pContext = (CJS_Context*)cc;
        CJS_Runtime* pRuntime = pContext->GetJSRuntime();
        CPDFDoc_Environment * pEnv = pRuntime->GetReaderApp();
        pEnv->JS_appBeep((int)params[0]);

        return TRUE;
    }
    else
    {
        sError = JSGetStringFromID((CJS_Context*)cc, IDS_STRING_JSPARAMERROR);
        return FALSE;
    }
}

FX_BOOL app::findComponent(OBJ_METHOD_PARAMS)
{
    return TRUE;
}

FX_BOOL app::popUpMenuEx(OBJ_METHOD_PARAMS)
{	
    return FALSE;
}

FX_BOOL app::fs(OBJ_PROP_PARAMS)
{
#ifdef FOXIT_CHROME_BUILD
    return FALSE;
#else
    CJS_Context* pContext = (CJS_Context*)cc;
    ASSERT(pContext != NULL);
    CJS_Runtime* pRuntime = pContext->GetJSRuntime();
    ASSERT(pRuntime != NULL);

    if (vp.IsGetting())
    {
        return TRUE;
    }
    else
    {
        return TRUE;
    }
#endif
}

FX_BOOL app::setInterval(OBJ_METHOD_PARAMS)
{
    if (params.size() > 2 || params.size() == 0) 
    {
        sError = JSGetStringFromID((CJS_Context*)cc, IDS_STRING_JSPARAMERROR);	
        return FALSE;
    }
    
    CJS_Context* pContext = (CJS_Context*)cc;
    ASSERT(pContext != NULL);
    CJS_Runtime* pRuntime = pContext->GetJSRuntime();
    ASSERT(pRuntime != NULL);

    CFX_WideString script = params.size() > 0 ?  (FX_LPCWSTR)(params[0].operator CFX_WideString()) : (FX_LPCWSTR)L"";
    if (script.IsEmpty()) 
    {
        sError = JSGetStringFromID((CJS_Context*)cc, IDS_STRING_JSAFNUMBER_KEYSTROKE);
        return TRUE;
    }

    FX_DWORD dwInterval = params.size() > 1 ? (int)params[1] : 1000;
    
    CPDFDoc_Environment* pApp = pRuntime->GetReaderApp();
    ASSERT(pApp);
    CJS_Timer* pTimer = new CJS_Timer(this, pApp);
    m_aTimer.Add(pTimer);

    pTimer->SetType(0);
    pTimer->SetRuntime(pRuntime);
    pTimer->SetJScript(script);
    pTimer->SetTimeOut(0);
//	pTimer->SetStartTime(GetTickCount());
    pTimer->SetJSTimer(dwInterval);
    
    JSFXObject pRetObj = JS_NewFxDynamicObj(*pRuntime, pContext, JS_GetObjDefnID(*pRuntime, L"TimerObj"));
    
    CJS_TimerObj* pJS_TimerObj = (CJS_TimerObj*)JS_GetPrivate(pRuntime->GetIsolate(),pRetObj);
    ASSERT(pJS_TimerObj != NULL);
    
    TimerObj* pTimerObj = (TimerObj*)pJS_TimerObj->GetEmbedObject();
    ASSERT(pTimerObj != NULL);
    
    pTimerObj->SetTimer(pTimer); 
    
    vRet = pRetObj;
    
    return TRUE;
}

FX_BOOL app::setTimeOut(OBJ_METHOD_PARAMS)
{
    if (params.size() > 2 || params.size() == 0)
    {
        sError = JSGetStringFromID((CJS_Context*)cc, IDS_STRING_JSPARAMERROR);	
        return FALSE;
    }
    
    CJS_Context* pContext = (CJS_Context*)cc;
    ASSERT(pContext != NULL);
    CJS_Runtime* pRuntime = pContext->GetJSRuntime();
    ASSERT(pRuntime != NULL);
    
    CFX_WideString script = params.size() > 0 ?  (FX_LPCWSTR)(params[0].operator CFX_WideString()) : (FX_LPCWSTR)L"";
    if (script.IsEmpty()) 
    {
        sError = JSGetStringFromID((CJS_Context*)cc, IDS_STRING_JSAFNUMBER_KEYSTROKE);
        return TRUE;
    }
    
    FX_DWORD dwTimeOut = params.size() > 1 ? (int)params[1] : 1000;
    
    CPDFDoc_Environment* pApp = pRuntime->GetReaderApp();
    ASSERT(pApp);
    CJS_Timer* pTimer = new CJS_Timer(this, pApp);
    m_aTimer.Add(pTimer);
    
    pTimer->SetType(1);
    pTimer->SetRuntime(pRuntime);
    pTimer->SetJScript(script);
    pTimer->SetTimeOut(dwTimeOut);
//	pTimer->SetStartTime(GetTickCount());
//	pTimer->SetJSTimer(1000);
    pTimer->SetJSTimer(dwTimeOut);
    
    JSFXObject pRetObj = JS_NewFxDynamicObj(*pRuntime, pContext, JS_GetObjDefnID(*pRuntime, L"TimerObj"));
//	ASSERT(pRetObj != NULL);
    
    CJS_TimerObj* pJS_TimerObj = (CJS_TimerObj*)JS_GetPrivate(pRuntime->GetIsolate(),pRetObj);
    ASSERT(pJS_TimerObj != NULL);
    
    TimerObj* pTimerObj = (TimerObj*)pJS_TimerObj->GetEmbedObject();
    ASSERT(pTimerObj != NULL);
    
    pTimerObj->SetTimer(pTimer); 
    
    vRet = pRetObj;
    
    return TRUE;
}

FX_BOOL app::clearTimeOut(OBJ_METHOD_PARAMS)
{
    CJS_Context* pContext = (CJS_Context*)cc;
    ASSERT(pContext != NULL);
    CJS_Runtime* pRuntime = pContext->GetJSRuntime();
    ASSERT(pRuntime != NULL);
    
    if (params.size() != 1)
    {
        sError = JSGetStringFromID((CJS_Context*)cc, IDS_STRING_JSPARAMERROR);	
        return FALSE;
    }
    
    if (params[0].GetType() == VT_fxobject)
    {
        JSFXObject pObj = (JSFXObject)params[0];
        {
            if (JS_GetObjDefnID(pObj) == JS_GetObjDefnID(*pRuntime, L"TimerObj"))
            {
                if (CJS_Object* pJSObj = (CJS_Object*)params[0])
                {
                    if (TimerObj* pTimerObj = (TimerObj*)pJSObj->GetEmbedObject())
                    {
                        if (CJS_Timer* pTimer = pTimerObj->GetTimer())
                        {
                            pTimer->KillJSTimer();
                            
                            for (int i=0,sz=m_aTimer.GetSize(); i<sz; i++)
                            {
                                if (m_aTimer[i] == pTimer)
                                {
                                    m_aTimer.RemoveAt(i);
                                    break;
                                }
                            }
                            
                            delete pTimer;
                            pTimerObj->SetTimer(NULL);
                        }
                    }
                }
            }
        }
    }
    
    return TRUE;
}

FX_BOOL app::clearInterval(OBJ_METHOD_PARAMS)
{
    CJS_Context* pContext = (CJS_Context*)cc;
    ASSERT(pContext != NULL);
    CJS_Runtime* pRuntime = pContext->GetJSRuntime();
    ASSERT(pRuntime != NULL);

    if (params.size() != 1)
    {
        sError = JSGetStringFromID((CJS_Context*)cc, IDS_STRING_JSPARAMERROR);	
        return FALSE;
    }
    
    if (params[0].GetType() == VT_fxobject)
    {
        JSFXObject pObj = (JSFXObject)params[0];
        {
            if (JS_GetObjDefnID(pObj) == JS_GetObjDefnID(*pRuntime, L"TimerObj"))
            {
                if (CJS_Object* pJSObj = (CJS_Object*)params[0])
                {
                    if (TimerObj* pTimerObj = (TimerObj*)pJSObj->GetEmbedObject())
                    {
                        if (CJS_Timer* pTimer = pTimerObj->GetTimer())
                        {
                            pTimer->KillJSTimer();
                            
                            for (int i=0,sz=m_aTimer.GetSize(); i<sz; i++)
                            {
                                if (m_aTimer[i] == pTimer)
                                {
                                    m_aTimer.RemoveAt(i);
                                    break;
                                }
                            }
                            
                            delete pTimer;
                            pTimerObj->SetTimer(NULL);
                        }
                    }
                }
            }
        }
    }
    
    return TRUE;
}

FX_BOOL app::execMenuItem(OBJ_METHOD_PARAMS)
{	
    return FALSE;
}

void app::TimerProc(CJS_Timer* pTimer)
{
    ASSERT(pTimer != NULL);

    switch (pTimer->GetType())
    {
    case 0: //interval
        RunJsScript(pTimer->GetRuntime(), pTimer->GetJScript());
        break;
    case 1:
        if (pTimer->GetTimeOut() > 0)
        {
            RunJsScript(pTimer->GetRuntime(), pTimer->GetJScript());
            pTimer->KillJSTimer();
        }
        break;
    }
    
}

void app::RunJsScript(CJS_Runtime* pRuntime,const CFX_WideString& wsScript)
{
    ASSERT(pRuntime != NULL);

    if (!pRuntime->IsBlocking())
    {
        IFXJS_Context* pContext = pRuntime->NewContext();
        ASSERT(pContext != NULL);
        pContext->OnExternal_Exec();
        CFX_WideString wtInfo;
        pContext->RunScript(wsScript,wtInfo);
        pRuntime->ReleaseContext(pContext);
    }
}

FX_BOOL app::goBack(OBJ_METHOD_PARAMS)
{


    
    
    
    
    return TRUE;
}

FX_BOOL app::goForward(OBJ_METHOD_PARAMS)
{	






    return TRUE;
}

FX_BOOL app::mailMsg(OBJ_METHOD_PARAMS)
{
    CJS_Context* pContext = (CJS_Context*)cc;
    ASSERT(pContext != NULL);

    v8::Isolate* isolate = GetIsolate(cc);

    FX_BOOL bUI = TRUE;
    CFX_WideString cTo = L"";
    CFX_WideString cCc = L"";
    CFX_WideString cBcc = L"";
    CFX_WideString cSubject = L"";
    CFX_WideString cMsg = L"";
    if(params.size() < 2)
        return FALSE;

    bUI = params.size()>=1?(int)params[0]:TRUE;
    cTo = params.size()>=2?(const wchar_t*)(FX_LPCWSTR)params[1].operator CFX_WideString():L"";
    cCc = params.size()>=3?(const wchar_t*)(FX_LPCWSTR)params[2].operator CFX_WideString():L"";
    cBcc = params.size()>=4?(const wchar_t*)(FX_LPCWSTR)params[3].operator CFX_WideString():L"";
    cSubject = params.size()>=5?(const wchar_t*)(FX_LPCWSTR)params[4].operator CFX_WideString():L"";
    cMsg = params.size()>=6?(const wchar_t*)(FX_LPCWSTR)params[5].operator CFX_WideString():L"";		


    if (params[0].GetType() == VT_object)
    {
        JSObject pObj = (JSObject)params[0];

        v8::Handle<v8::Value> pValue = JS_GetObjectElement(isolate,pObj, L"bUI");
            bUI = (int)CJS_Value(isolate,pValue,GET_VALUE_TYPE(pValue));

        pValue = JS_GetObjectElement(isolate, pObj, L"cTo");
            cTo = CJS_Value(isolate,pValue,GET_VALUE_TYPE(pValue)).operator CFX_WideString();

        pValue = JS_GetObjectElement(isolate,pObj, L"cCc");
            cCc = CJS_Value(isolate,pValue,GET_VALUE_TYPE(pValue)).operator CFX_WideString();

        pValue = JS_GetObjectElement(isolate,pObj, L"cBcc");
            cBcc = CJS_Value(isolate,pValue,GET_VALUE_TYPE(pValue)).operator CFX_WideString();

        pValue = JS_GetObjectElement(isolate,pObj, L"cSubject");
            cSubject = CJS_Value(isolate,pValue,GET_VALUE_TYPE(pValue)).operator CFX_WideString();

        pValue = JS_GetObjectElement(isolate,pObj, L"cMsg");
            cMsg = CJS_Value(isolate,pValue,GET_VALUE_TYPE(pValue)).operator CFX_WideString();
    }
    
    

    CJS_Runtime* pRuntime = pContext->GetJSRuntime();
    ASSERT(pRuntime != NULL);

    CPDFDoc_Environment* pApp = pContext->GetReaderApp();
    ASSERT(pApp != NULL);

    pRuntime->BeginBlock();
    pApp->JS_docmailForm(NULL, 0, bUI, (FX_LPCWSTR)cTo, (FX_LPCWSTR)cSubject, (FX_LPCWSTR)cCc, (FX_LPCWSTR)cBcc, (FX_LPCWSTR)cMsg);
    ///////////////////////////////////////////////////////////////////////////////////////////////
    pRuntime->EndBlock();

    //return bRet;
    return FALSE;
}

FX_BOOL app::launchURL(OBJ_METHOD_PARAMS)
{
    if (IsSafeMode(cc)) return TRUE;

    CJS_Context* pContext = (CJS_Context*)cc;
    ASSERT(pContext != NULL);

    


    CFX_WideString swURL = params[0].operator CFX_WideString();

    CJS_Runtime* pRuntime = pContext->GetJSRuntime();
    ASSERT(pRuntime != NULL);

    pRuntime->BeginBlock();
//	FX_BOOL bRet = pApp->OpenURL(swURL);
    pRuntime->EndBlock();

//	return bRet;
    return FALSE;
}

FX_BOOL app::runtimeHighlight(OBJ_PROP_PARAMS)
{
    if (vp.IsSetting())
    {
        vp>>m_bRuntimeHighLight;
    }
    else
    {
        vp<<m_bRuntimeHighLight;
    }

    return TRUE;
}

FX_BOOL app::fullscreen(OBJ_PROP_PARAMS)
{
    return FALSE;
}

FX_BOOL app::popUpMenu(OBJ_METHOD_PARAMS)
{
    return FALSE;
}


FX_BOOL app::browseForDoc(OBJ_METHOD_PARAMS)
{
    //This method may trigger a "file save" dialog,while enable user to save contents of the document.
    //Such action is considered to be unsafe.
    if (IsSafeMode(cc)) return TRUE;

    v8::Isolate* isolate = GetIsolate(cc);

    bool bSave = false;
    CFX_ByteString cFilenameInit = CFX_ByteString();
    CFX_ByteString cFSInit = CFX_ByteString();

    if(params.size()>0 && (params[0].GetType() == VT_object))
    {
        JSObject pObj = (JSObject )params[0];

        v8::Handle<v8::Value> pValue = JS_GetObjectElement(isolate,pObj,L"bSave");
            bSave = (bool)CJS_Value(isolate,pValue,GET_VALUE_TYPE(pValue));
        
        pValue = JS_GetObjectElement(isolate, pObj,L"cFilenameInit");
        {
            CJS_Value t = CJS_Value(isolate, pValue, GET_VALUE_TYPE(pValue));
            cFilenameInit = t.operator CFX_ByteString();
        }
        
        pValue = JS_GetObjectElement(isolate,pObj,L"cFSInit");
        {
            CJS_Value t = CJS_Value(isolate, pValue, GET_VALUE_TYPE(pValue));
            cFSInit = t.operator CFX_ByteString();
        }
    }
    else
    {
        if(params.size() >= 1)
        {
            bSave = (bool)params[0];
        }
        if(params.size() >= 2)
        {
            CJS_Value t = params[1];
            cFilenameInit = t.operator CFX_ByteString();
        }
        if(params.size() >= 3)
        {
            CJS_Value t = params[2];
            cFSInit = t.operator CFX_ByteString();
        }
    }
    CJS_Context* pContext = (CJS_Context *)cc;
    ASSERT(pContext != NULL);
    
    CPDFDoc_Environment* pApp = pContext->GetReaderApp();
    ASSERT(pApp != NULL);

    CJS_Runtime* pRuntime = pContext->GetJSRuntime();
    ASSERT(pRuntime != NULL);

    CFX_WideString wsFileNameInit = CFX_WideString::FromLocal(cFilenameInit);
    CFX_WideString wsFSInit = CFX_WideString::FromLocal(cFSInit);
    CFX_WideString wsFilePath = pApp->JS_appbrowseForDoc(bSave, wsFileNameInit);
    if(wsFilePath.IsEmpty())
        return FALSE;

    JSFXObject pRetObj = JS_NewFxDynamicObj(*pRuntime, pContext, -1);

    JS_PutObjectString(isolate,pRetObj, L"cPath", SysPathToPDFPath(wsFilePath));	
    JS_PutObjectString(isolate,pRetObj, L"cURL", SysPathToPDFPath(wsFilePath));

    if (!cFSInit.IsEmpty())
    {
        JS_PutObjectString(isolate,pRetObj, L"cFS", CFX_WideString::FromLocal(cFSInit.GetBuffer(cFSInit.GetLength())));
    }
    else
    {
        JS_PutObjectString(isolate,pRetObj, L"cFS", CFX_WideString::FromLocal("DOS"));
    }
    
    vRet =  pRetObj;

    return TRUE;
}

CFX_WideString app::SysPathToPDFPath(const CFX_WideString& sOldPath)
{
    CFX_WideString sRet = L"/";
    
    for (int i=0,sz=sOldPath.GetLength(); i<sz; i++)
    {
        wchar_t c = sOldPath.GetAt(i);
        if (c == L':')
        {
        }
        else
        {
            if (c == L'\\')
            {
                sRet += L"/";
            }
            else
            {
                sRet += c;
            }
        }
    }
    
    return sRet;
}

CFX_WideString app::PDFPathToSysPath(const CFX_WideString& sOldPath)
{
    //strLPath = "D:\temporay.fdf";
    CFX_WideString strOPath = sOldPath;
    strOPath.TrimLeft();
    strOPath.TrimRight();
    
    if (strOPath.GetAt(0) == L'/' && strOPath.GetAt(2) == L'/')
    {
        wchar_t c_Drive = strOPath.GetAt(1);
        if ((c_Drive >= L'a' && c_Drive <= L'z' )||( c_Drive >= L'A' && c_Drive <= L'Z'))
        {
            strOPath.Replace((FX_LPCWSTR)L"/",(FX_LPCWSTR)L"\\");
            //strOPath.SetAt(0,'');
            strOPath.Insert(2,':');
            strOPath.Delete(0);
        }
    }
    
    return strOPath;
}

CFX_WideString app::RelativePathToSysPath(const CFX_WideString& sOldPath, const CFX_WideString& sFilePath)
{
//	if (!PathIsRelative(sOldPath)) return sOldPath;
    
    int nSplit = 0;
    for (int i=sFilePath.GetLength()-1; i>=0; i--)
    {
        if (sFilePath[i] == '\\' || sFilePath[i] == '/')
        {
            nSplit = i;
            break;
        }
    }
    
    return sFilePath.Left(nSplit+1) + sOldPath;
}

FX_BOOL app::newDoc(OBJ_METHOD_PARAMS)
{
    return FALSE;
}

FX_BOOL app::openDoc(OBJ_METHOD_PARAMS)
{
    return FALSE;
}

FX_BOOL app::response(OBJ_METHOD_PARAMS)
{
    CFX_WideString swQuestion = L"";
    CFX_WideString swLabel = L"";
#ifndef FOXIT_CHROME_BUILD
    CFX_WideString swTitle = L"Foxit";
#else
    CFX_WideString swTitle = L"PDF";
#endif
    CFX_WideString swDefault = L"";
    CFX_WideString swResponse = L"";
    bool bPassWord = false;
    
    v8::Isolate* isolate = GetIsolate(cc);
    
    int iLength = params.size();	
    if (iLength > 0 && params[0].GetType() == VT_object)
    {
        
        JSObject pObj = (JSObject )params[0];
        v8::Handle<v8::Value> pValue = JS_GetObjectElement(isolate,pObj,L"cQuestion");
            swQuestion = CJS_Value(isolate,pValue,GET_VALUE_TYPE(pValue)).operator CFX_WideString();

        pValue = JS_GetObjectElement(isolate,pObj,L"cTitle");
            swTitle = CJS_Value(isolate,pValue,GET_VALUE_TYPE(pValue)).operator CFX_WideString();

        pValue = JS_GetObjectElement(isolate,pObj,L"cDefault");
            swDefault = CJS_Value(isolate,pValue,GET_VALUE_TYPE(pValue)).operator CFX_WideString();

        pValue = JS_GetObjectElement(isolate,pObj,L"cLabel");
            swLabel = CJS_Value(isolate,pValue,GET_VALUE_TYPE(pValue)).operator CFX_WideString();

        pValue = JS_GetObjectElement(isolate,pObj,L"bPassword");
            bPassWord = (bool)CJS_Value(isolate,pValue,GET_VALUE_TYPE(pValue));
    }
    else
    {
        switch(iLength)
        {
        case 1:
            swQuestion = params[0];
            break;
        case 2:
            swQuestion = params[0];
            swTitle = params[1];
            break;
        case 3:
            swQuestion = params[0];
            swTitle = params[1];
            swDefault = params[2];
            break;
        case 4:
            swQuestion = params[0];
            swTitle = params[1];
            swDefault = params[2];
            bPassWord = params[3];
            break;
        case 5:
            swQuestion = params[0];
            swTitle = params[1];
            swDefault = params[2];
            bPassWord = params[3];
            swLabel = params[4];
            break;
        default:
            break;
        }
    }

    CJS_Context* pContext = (CJS_Context *)cc;
    ASSERT(pContext != NULL);

    CPDFDoc_Environment* pApp = pContext->GetReaderApp();
    ASSERT(pApp != NULL);
    int nLength = 2048;
    char* pBuff = new char[nLength];
    nLength = pApp->JS_appResponse(swQuestion, swTitle, swDefault, swLabel, bPassWord, pBuff, nLength);
    if(nLength<=0)
    {
        delete[] pBuff;
        vRet.SetNull();
        return FALSE;
    }
    else
    {
        nLength = nLength > sizeof(pBuff) ? sizeof(pBuff) : nLength;
        swResponse = CFX_WideString::FromUTF16LE((unsigned short*)pBuff, nLength/2);
        vRet = swResponse;
    }
    delete[] pBuff;

    return TRUE;
}

FX_BOOL app::media(OBJ_PROP_PARAMS)
{
    return FALSE;
}

FX_BOOL app::execDialog(OBJ_METHOD_PARAMS)
{
    return TRUE;
}

