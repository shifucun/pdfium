// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "../../../foxitlib.h"
#include "../common/xfa_utils.h"
#include "../common/xfa_object.h"
#include "../common/xfa_document.h"
#include "../common/xfa_parser.h"
#include "../common/xfa_script.h"
#include "../common/xfa_docdata.h"
#include "../common/xfa_doclayout.h"
#include "../common/xfa_debug.h"
#include "../common/xfa_localemgr.h"
#include "../common/xfa_fm2jsapi.h"
#include "xfa_debug_parser.h"
#include "xfa_basic_imp.h"
extern const XFA_PACKETINFO g_XFAPacketData[];
extern const FX_INT32 g_iXFAPacketCount;
extern const XFA_ATTRIBUTEENUMINFO g_XFAEnumData[];
extern const FX_INT32 g_iXFAEnumCount;
extern const XFA_ATTRIBUTEINFO g_XFAAttributeData[];
extern const FX_INT32 g_iXFAAttributeCount;
extern const XFA_ELEMENTINFO g_XFAElementData[];
extern const FX_INT32 g_iXFAElementCount;
extern const XFA_ELEMENTHIERARCHY g_XFAElementChildrenIndex[];
extern const FX_WORD g_XFAElementChildrenData[];
extern const XFA_ELEMENTHIERARCHY g_XFAElementAttributeIndex[];
extern const FX_BYTE g_XFAElementAttributeData[];
extern const XFA_NOTSUREATTRIBUTE g_XFANotsureAttributes[];
extern const FX_INT32 g_iXFANotsureCount;
extern const XFA_ELEMENTHIERARCHY g_XFAElementPropertyIndex[];
extern const XFA_PROPERTY g_XFAElementPropertyData[];
extern const XFA_SCRIPTHIERARCHY g_XFAScriptIndex[];
extern const XFA_METHODINFO g_SomMethodData[];
extern const FX_INT32 g_iSomMethodCount;
extern const XFA_SCRIPTATTRIBUTEINFO g_SomAttributeData[];
extern const FX_INT32 g_iSomAttributeCount;
XFA_LPCPACKETINFO XFA_GetPacketByName(FX_WSTR wsName)
{
    FX_INT32 iLength = wsName.GetLength();
    if (iLength == 0) {
        return NULL;
    }
    FX_UINT32 uHash = FX_HashCode_String_GetW(wsName.GetPtr(), iLength);
    FX_INT32 iStart = 0, iEnd = g_iXFAPacketCount - 1;
    do {
        FX_INT32 iMid = (iStart + iEnd) / 2;
        XFA_LPCPACKETINFO pInfo = g_XFAPacketData + iMid;
        if (uHash == pInfo->uHash) {
            return pInfo;
        } else if (uHash < pInfo->uHash) {
            iEnd = iMid - 1;
        } else {
            iStart = iMid + 1;
        }
    } while (iStart <= iEnd);
    return NULL;
}
XFA_LPCPACKETINFO XFA_GetPacketByID(FX_DWORD dwPacket)
{
    FX_INT32 iStart = 0, iEnd = g_iXFAPacketCount - 1;
    do {
        FX_INT32 iMid = (iStart + iEnd) / 2;
        FX_DWORD dwFind = (g_XFAPacketData + iMid)->eName;
        if (dwPacket == dwFind) {
            return g_XFAPacketData + iMid;
        } else if (dwPacket < dwFind) {
            iEnd = iMid - 1;
        } else {
            iStart = iMid + 1;
        }
    } while (iStart <= iEnd);
    return NULL;
}
XFA_LPCATTRIBUTEENUMINFO XFA_GetAttributeEnumByName(FX_WSTR wsName)
{
    FX_INT32 iLength = wsName.GetLength();
    if (iLength == 0) {
        return NULL;
    }
    FX_UINT32 uHash = FX_HashCode_String_GetW(wsName.GetPtr(), iLength);
    FX_INT32 iStart = 0, iEnd = g_iXFAEnumCount - 1;
    do {
        FX_INT32 iMid = (iStart + iEnd) / 2;
        XFA_LPCATTRIBUTEENUMINFO pInfo = g_XFAEnumData + iMid;
        if (uHash == pInfo->uHash) {
            return pInfo;
        } else if (uHash < pInfo->uHash) {
            iEnd = iMid - 1;
        } else {
            iStart = iMid + 1;
        }
    } while (iStart <= iEnd);
    return NULL;
}
XFA_LPCATTRIBUTEENUMINFO XFA_GetAttributeEnumByID(XFA_ATTRIBUTEENUM eName)
{
    return g_XFAEnumData + eName;
}
FX_INT32 XFA_GetAttributeCount()
{
    return g_iXFAAttributeCount;
}
XFA_LPCATTRIBUTEINFO XFA_GetAttributeByName(FX_WSTR wsName)
{
    FX_INT32 iLength = wsName.GetLength();
    if (iLength == 0) {
        return NULL;
    }
    FX_UINT32 uHash = FX_HashCode_String_GetW(wsName.GetPtr(), iLength);
    FX_INT32 iStart = 0, iEnd = g_iXFAAttributeCount - 1;
    do {
        FX_INT32 iMid = (iStart + iEnd) / 2;
        XFA_LPCATTRIBUTEINFO pInfo = g_XFAAttributeData + iMid;
        if (uHash == pInfo->uHash) {
            return pInfo;
        } else if (uHash < pInfo->uHash) {
            iEnd = iMid - 1;
        } else {
            iStart = iMid + 1;
        }
    } while (iStart <= iEnd);
    return NULL;
}
XFA_LPCATTRIBUTEINFO XFA_GetAttributeByID(XFA_ATTRIBUTE eName)
{
    return (eName < g_iXFAAttributeCount) ? (g_XFAAttributeData + eName) : NULL;
}
FX_BOOL XFA_GetAttributeDefaultValue(FX_LPVOID &pValue, XFA_ELEMENT eElement, XFA_ATTRIBUTE eAttribute, XFA_ATTRIBUTETYPE eType, FX_DWORD dwPacket)
{
    XFA_LPCATTRIBUTEINFO pInfo = XFA_GetAttributeByID(eAttribute);
    if (pInfo == NULL) {
        return FALSE;
    }
    if (dwPacket && (dwPacket & pInfo->dwPackets) == 0) {
        return FALSE;
    }
    if (pInfo->eType == eType) {
        pValue = pInfo->pDefValue;
        return TRUE;
    } else if (pInfo->eType == XFA_ATTRIBUTETYPE_NOTSURE) {
        XFA_LPCNOTSUREATTRIBUTE pAttr = XFA_GetNotsureAttribute(eElement, eAttribute, eType);
        if (pAttr) {
            pValue = pAttr->pValue;
            return TRUE;
        }
    }
    return FALSE;
}
XFA_ATTRIBUTEENUM XFA_GetAttributeDefaultValue_Enum(XFA_ELEMENT eElement, XFA_ATTRIBUTE eAttribute, FX_DWORD dwPacket)
{
    FX_LPVOID pValue;
    if (XFA_GetAttributeDefaultValue(pValue, eElement, eAttribute, XFA_ATTRIBUTETYPE_Enum, dwPacket)) {
        return (XFA_ATTRIBUTEENUM)(FX_UINTPTR)pValue;
    }
    return XFA_ATTRIBUTEENUM_Unknown;
}
CFX_WideStringC XFA_GetAttributeDefaultValue_Cdata(XFA_ELEMENT eElement, XFA_ATTRIBUTE eAttribute, FX_DWORD dwPacket)
{
    FX_LPVOID pValue;
    if (XFA_GetAttributeDefaultValue(pValue, eElement, eAttribute, XFA_ATTRIBUTETYPE_Cdata, dwPacket)) {
        return (FX_LPCWSTR)pValue;
    }
    return NULL;
}
FX_BOOL XFA_GetAttributeDefaultValue_Boolean(XFA_ELEMENT eElement, XFA_ATTRIBUTE eAttribute, FX_DWORD dwPacket)
{
    FX_LPVOID pValue;
    if (XFA_GetAttributeDefaultValue(pValue, eElement, eAttribute, XFA_ATTRIBUTETYPE_Boolean, dwPacket)) {
        return (FX_BOOL)(FX_UINTPTR)pValue;
    }
    return FALSE;
}
FX_INT32 XFA_GetAttributeDefaultValue_Integer(XFA_ELEMENT eElement, XFA_ATTRIBUTE eAttribute, FX_DWORD dwPacket)
{
    FX_LPVOID pValue;
    if (XFA_GetAttributeDefaultValue(pValue, eElement, eAttribute, XFA_ATTRIBUTETYPE_Integer, dwPacket)) {
        return (FX_INT32)(FX_UINTPTR)pValue;
    }
    return 0;
}
CXFA_Measurement XFA_GetAttributeDefaultValue_Measure(XFA_ELEMENT eElement, XFA_ATTRIBUTE eAttribute, FX_DWORD dwPacket)
{
    FX_LPVOID pValue;
    if (XFA_GetAttributeDefaultValue(pValue, eElement, eAttribute, XFA_ATTRIBUTETYPE_Measure, dwPacket)) {
        return *(CXFA_Measurement*)pValue;
    }
    return CXFA_Measurement();
}
FX_INT32 XFA_GetElementCount()
{
    return g_iXFAElementCount;
}
XFA_LPCELEMENTINFO XFA_GetElementByName(FX_WSTR wsName)
{
    FX_INT32 iLength = wsName.GetLength();
    if (iLength == 0) {
        return NULL;
    }
    FX_UINT32 uHash = FX_HashCode_String_GetW(wsName.GetPtr(), iLength);
    FX_INT32 iStart = 0, iEnd = g_iXFAElementCount - 1;
    do {
        FX_INT32 iMid = (iStart + iEnd) / 2;
        XFA_LPCELEMENTINFO pInfo = g_XFAElementData + iMid;
        if (uHash == pInfo->uHash) {
            return pInfo;
        } else if (uHash < pInfo->uHash) {
            iEnd = iMid - 1;
        } else {
            iStart = iMid + 1;
        }
    } while (iStart <= iEnd);
    return NULL;
}
XFA_LPCELEMENTINFO XFA_GetElementByID(XFA_ELEMENT eName)
{
    return (eName < g_iXFAElementCount) ? (g_XFAElementData + eName) : NULL;
}
FX_LPCWORD XFA_GetElementChildren(XFA_ELEMENT eElement, FX_INT32 &iCount)
{
    if (eElement >= g_iXFAElementCount) {
        return NULL;
    }
    XFA_LPCELEMENTHIERARCHY pElement = g_XFAElementChildrenIndex + eElement;
    iCount = pElement->wCount;
    return g_XFAElementChildrenData + pElement->wStart;
}
FX_LPCBYTE XFA_GetElementAttributes(XFA_ELEMENT eElement, FX_INT32 &iCount)
{
    if (eElement >= g_iXFAElementCount) {
        return NULL;
    }
    XFA_LPCELEMENTHIERARCHY pElement = g_XFAElementAttributeIndex + eElement;
    iCount = pElement->wCount;
    return g_XFAElementAttributeData + pElement->wStart;
}
XFA_LPCATTRIBUTEINFO XFA_GetAttributeOfElement(XFA_ELEMENT eElement, XFA_ATTRIBUTE eAttribute, FX_DWORD dwPacket)
{
    FX_INT32 iCount = 0;
    FX_LPCBYTE pAttr = XFA_GetElementAttributes(eElement, iCount);
    if (pAttr == NULL || iCount < 1) {
        return NULL;
    }
    CFX_DSPATemplate<FX_BYTE> search;
    FX_INT32 index = search.Lookup(eAttribute, pAttr, iCount);
    if (index < 0) {
        return NULL;
    }
    XFA_LPCATTRIBUTEINFO pInfo = XFA_GetAttributeByID(eAttribute);
    ASSERT(pInfo != NULL);
    if (dwPacket == XFA_XDPPACKET_UNKNOWN) {
        return pInfo;
    }
    return (dwPacket & pInfo->dwPackets) ? pInfo : NULL;
}
XFA_LPCELEMENTINFO XFA_GetChildOfElement(XFA_ELEMENT eElement, XFA_ELEMENT eChild, FX_DWORD dwPacket)
{
    FX_INT32 iCount = 0;
    FX_LPCWORD pChild = XFA_GetElementChildren(eElement, iCount);
    if (pChild == NULL || iCount < 1) {
        return NULL;
    }
    CFX_DSPATemplate<FX_WORD> search;
    FX_INT32 index = search.Lookup(eChild, pChild, iCount);
    if (index < 0) {
        return NULL;
    }
    XFA_LPCELEMENTINFO pInfo = XFA_GetElementByID(eChild);
    ASSERT(pInfo != NULL);
    if (dwPacket == XFA_XDPPACKET_UNKNOWN) {
        return pInfo;
    }
    return (dwPacket & pInfo->dwPackets) ? pInfo : NULL;
}
XFA_LPCPROPERTY XFA_GetElementProperties(XFA_ELEMENT eElement, FX_INT32 &iCount)
{
    if (eElement >= g_iXFAElementCount) {
        return NULL;
    }
    XFA_LPCELEMENTHIERARCHY pElement = g_XFAElementPropertyIndex + eElement;
    iCount = pElement->wCount;
    return g_XFAElementPropertyData + pElement->wStart;
}
XFA_LPCPROPERTY XFA_GetPropertyOfElement(XFA_ELEMENT eElement, XFA_ELEMENT eProperty, FX_DWORD dwPacket)
{
    FX_INT32 iCount = 0;
    XFA_LPCPROPERTY pProperty = XFA_GetElementProperties(eElement, iCount);
    if (pProperty == NULL || iCount < 1) {
        return NULL;
    }
    FX_INT32 iStart = 0, iEnd = iCount - 1, iMid;
    do {
        iMid = (iStart + iEnd) / 2;
        XFA_ELEMENT eName = (XFA_ELEMENT)pProperty[iMid].eName;
        if (eProperty == eName) {
            break;
        } else if (eProperty < eName) {
            iEnd = iMid - 1;
        } else {
            iStart = iMid + 1;
        }
    } while (iStart <= iEnd);
    if (iStart > iEnd) {
        return NULL;
    }
    XFA_LPCELEMENTINFO pInfo = XFA_GetElementByID(eProperty);
    ASSERT(pInfo != NULL);
    if (dwPacket == XFA_XDPPACKET_UNKNOWN) {
        return pProperty + iMid;
    }
    return (dwPacket & pInfo->dwPackets) ? (pProperty + iMid) : NULL;
}
XFA_LPCNOTSUREATTRIBUTE XFA_GetNotsureAttribute(XFA_ELEMENT eElement, XFA_ATTRIBUTE eAttribute, XFA_ATTRIBUTETYPE eType)
{
    FX_INT32 iStart = 0, iEnd = g_iXFANotsureCount - 1;
    do {
        FX_INT32 iMid = (iStart + iEnd) / 2;
        XFA_LPCNOTSUREATTRIBUTE pAttr = g_XFANotsureAttributes + iMid;
        if (eElement == pAttr->eElement) {
            if (pAttr->eAttribute == eAttribute) {
                if (eType == XFA_ATTRIBUTETYPE_NOTSURE || eType == pAttr->eType) {
                    return pAttr;
                }
                return NULL;
            } else {
                FX_INT32 iBefore = iMid - 1;
                if(iBefore >= 0) {
                    pAttr = g_XFANotsureAttributes + iBefore;
                    while(eElement == pAttr->eElement) {
                        if(pAttr->eAttribute == eAttribute) {
                            if (eType == XFA_ATTRIBUTETYPE_NOTSURE || eType == pAttr->eType) {
                                return pAttr;
                            }
                            return NULL;
                        }
                        iBefore --;
                        if(iBefore < 0) {
                            break;
                        }
                        pAttr = g_XFANotsureAttributes + iBefore;
                    }
                }
                FX_INT32 iAfter = iMid + 1;
                if(iAfter <= g_iXFANotsureCount - 1) {
                    pAttr = g_XFANotsureAttributes + iAfter;
                    while(eElement == pAttr->eElement) {
                        if(pAttr->eAttribute == eAttribute) {
                            if (eType == XFA_ATTRIBUTETYPE_NOTSURE || eType == pAttr->eType) {
                                return pAttr;
                            }
                            return NULL;
                        }
                        iAfter ++;
                        if(iAfter > g_iXFANotsureCount - 1) {
                            break;
                        }
                        pAttr = g_XFANotsureAttributes + iAfter;
                    }
                }
                return NULL;
            }
        } else if (eElement < pAttr->eElement) {
            iEnd = iMid - 1;
        } else {
            iStart = iMid + 1;
        }
    } while (iStart <= iEnd);
    return NULL;
}
FX_INT32 XFA_GetMethodCount()
{
    return g_iSomMethodCount;
}
XFA_LPCMETHODINFO XFA_GetMethodByName(XFA_ELEMENT eElement, FX_WSTR wsMethodName)
{
    FX_INT32 iLength = wsMethodName.GetLength();
    if (iLength == 0) {
        return NULL;
    }
    FX_INT32 iElementIndex = eElement;
    while(iElementIndex != -1) {
        XFA_LPCSCRIPTHIERARCHY scriptIndex = g_XFAScriptIndex + iElementIndex;
        FX_INT32 icount = scriptIndex->wMethodCount;
        if(icount == 0) {
            iElementIndex = scriptIndex->wParentIndex;
            continue;
        }
        FX_UINT32 uHash = FX_HashCode_String_GetW(wsMethodName.GetPtr(), iLength);
        FX_INT32 iStart = scriptIndex->wMethodStart, iEnd = iStart + icount - 1;
        do {
            FX_INT32 iMid = (iStart + iEnd) / 2;
            XFA_LPCMETHODINFO pInfo = g_SomMethodData + iMid;
            if (uHash == pInfo->uHash) {
                return pInfo;
            } else if (uHash < pInfo->uHash) {
                iEnd = iMid - 1;
            } else {
                iStart = iMid + 1;
            }
        } while (iStart <= iEnd);
        iElementIndex = scriptIndex->wParentIndex;
    }
    return NULL;
}
XFA_LPCSCRIPTATTRIBUTEINFO XFA_GetScriptAttributeByName(XFA_ELEMENT eElement,	FX_WSTR wsAttributeName)
{
    FX_INT32 iLength = wsAttributeName.GetLength();
    if (iLength == 0) {
        return NULL;
    }
    FX_INT32 iElementIndex = eElement;
    while(iElementIndex != -1) {
        XFA_LPCSCRIPTHIERARCHY scriptIndex = g_XFAScriptIndex + iElementIndex;
        FX_INT32 icount = scriptIndex->wAttributeCount;
        if(icount == 0) {
            iElementIndex = scriptIndex->wParentIndex;
            continue;
        }
        FX_UINT32 uHash = FX_HashCode_String_GetW(wsAttributeName.GetPtr(), iLength);
        FX_INT32 iStart = scriptIndex->wAttributeStart, iEnd = iStart + icount - 1;
        do {
            FX_INT32 iMid = (iStart + iEnd) / 2;
            XFA_LPCSCRIPTATTRIBUTEINFO pInfo = g_SomAttributeData + iMid;
            if (uHash == pInfo->uHash) {
                return pInfo;
            } else if (uHash < pInfo->uHash) {
                iEnd = iMid - 1;
            } else {
                iStart = iMid + 1;
            }
        } while (iStart <= iEnd);
        iElementIndex = scriptIndex->wParentIndex;
    }
    return NULL;
}
void CXFA_Measurement::Set(FX_WSTR wsMeasure)
{
    if (wsMeasure.IsEmpty()) {
        m_fValue = 0;
        m_eUnit = XFA_UNIT_Unknown;
        return;
    }
    FX_INT32 iUsedLen = 0;
    FX_INT32 iOffset = (wsMeasure.GetAt(0) == L'=') ? 1 : 0;
    FX_FLOAT fValue = FX_wcstof(wsMeasure.GetPtr() + iOffset, wsMeasure.GetLength() - iOffset, &iUsedLen);
    XFA_UNIT eUnit = GetUnit(wsMeasure.Mid(iOffset + iUsedLen));
    Set(fValue, eUnit);
}
FX_BOOL CXFA_Measurement::ToString(CFX_WideString &wsMeasure) const
{
    switch (GetUnit()) {
        case XFA_UNIT_Mm:
            wsMeasure.Format((FX_LPCWSTR)L"%.8gmm", GetValue());
            return TRUE;
        case XFA_UNIT_Pt:
            wsMeasure.Format((FX_LPCWSTR)L"%.8gpt", GetValue());
            return TRUE;
        case XFA_UNIT_In:
            wsMeasure.Format((FX_LPCWSTR)L"%.8gin", GetValue());
            return TRUE;
        case XFA_UNIT_Cm:
            wsMeasure.Format((FX_LPCWSTR)L"%.8gcm", GetValue());
            return TRUE;
        case XFA_UNIT_Mp:
            wsMeasure.Format((FX_LPCWSTR)L"%.8gmp", GetValue());
            return TRUE;
        case XFA_UNIT_Pc:
            wsMeasure.Format((FX_LPCWSTR)L"%.8gpc", GetValue());
            return TRUE;
        case XFA_UNIT_Em:
            wsMeasure.Format((FX_LPCWSTR)L"%.8gem", GetValue());
            return TRUE;
        case XFA_UNIT_Percent:
            wsMeasure.Format((FX_LPCWSTR)L"%.8g%%", GetValue());
            return TRUE;
        default:
            wsMeasure.Format((FX_LPCWSTR)L"%.8g", GetValue());
            return FALSE;
    }
}
FX_BOOL CXFA_Measurement::ToUnit(XFA_UNIT eUnit, FX_FLOAT &fValue) const
{
    fValue = GetValue();
    XFA_UNIT eFrom = GetUnit();
    if (eFrom == eUnit) {
        return TRUE;
    }
    switch (eFrom) {
        case XFA_UNIT_Pt:
            break;
        case XFA_UNIT_Mm:
            fValue *= 72 / 2.54f / 10;
            break;
        case XFA_UNIT_In:
            fValue *= 72;
            break;
        case XFA_UNIT_Cm:
            fValue *= 72 / 2.54f;
            break;
        case XFA_UNIT_Mp:
            fValue *= 0.001f;
            break;
        case XFA_UNIT_Pc:
            fValue *= 12.0f;
            break;
        default:
            fValue = 0;
            return FALSE;
    }
    switch (eUnit) {
        case XFA_UNIT_Pt:
            return TRUE;
        case XFA_UNIT_Mm:
            fValue /= 72 / 2.54f / 10;
            return TRUE;
        case XFA_UNIT_In:
            fValue /= 72;
            return TRUE;
        case XFA_UNIT_Cm:
            fValue /= 72 / 2.54f;
            return TRUE;
        case XFA_UNIT_Mp:
            fValue /= 0.001f;
            return TRUE;
        case XFA_UNIT_Pc:
            fValue /= 12.0f;
            return TRUE;
        default:
            fValue = 0;
            return FALSE;
    }
    return FALSE;
}
XFA_UNIT CXFA_Measurement::GetUnit(FX_WSTR wsUnit)
{
    if (wsUnit == FX_WSTRC(L"mm")) {
        return XFA_UNIT_Mm;
    } else if (wsUnit == FX_WSTRC(L"pt")) {
        return XFA_UNIT_Pt;
    } else if (wsUnit == FX_WSTRC(L"in")) {
        return XFA_UNIT_In;
    } else if (wsUnit == FX_WSTRC(L"cm")) {
        return XFA_UNIT_Cm;
    } else if (wsUnit == FX_WSTRC(L"pc")) {
        return XFA_UNIT_Pc;
    } else if (wsUnit == FX_WSTRC(L"mp")) {
        return XFA_UNIT_Mp;
    } else if (wsUnit == FX_WSTRC(L"em")) {
        return XFA_UNIT_Em;
    } else if (wsUnit == FX_WSTRC(L"%")) {
        return XFA_UNIT_Percent;
    } else {
        return XFA_UNIT_Unknown;
    }
}
IFX_Stream* XFA_CreateWideTextRead(const CFX_WideString &wsBuffer)
{
    return FX_NEW CXFA_WideTextRead(wsBuffer);
}
CXFA_WideTextRead::CXFA_WideTextRead(const CFX_WideString &wsBuffer)
    : m_wsBuffer(wsBuffer)
    , m_iPosition(0)
    , m_iRefCount(1)
{
}
void CXFA_WideTextRead::Release()
{
    if (--m_iRefCount < 1) {
        delete this;
    }
}
IFX_Stream* CXFA_WideTextRead::Retain()
{
    m_iRefCount++;
    return this;
}
FX_DWORD CXFA_WideTextRead::GetAccessModes() const
{
    return FX_STREAMACCESS_Read | FX_STREAMACCESS_Text;
}
FX_INT32 CXFA_WideTextRead::GetLength() const
{
    return m_wsBuffer.GetLength() * sizeof(FX_WCHAR);
}
FX_INT32 CXFA_WideTextRead::Seek(FX_STREAMSEEK eSeek, FX_INT32 iOffset)
{
    switch (eSeek) {
        case FX_STREAMSEEK_Begin:
            m_iPosition = iOffset;
            break;
        case FX_STREAMSEEK_Current:
            m_iPosition += iOffset;
            break;
        case FX_STREAMSEEK_End:
            m_iPosition = m_wsBuffer.GetLength() + iOffset;
            break;
    }
    if (m_iPosition < 0) {
        m_iPosition = 0;
    }
    if (m_iPosition > m_wsBuffer.GetLength()) {
        m_iPosition = m_wsBuffer.GetLength();
    }
    return GetPosition();
}
FX_INT32 CXFA_WideTextRead::GetPosition()
{
    return m_iPosition * sizeof(FX_WCHAR);
}
FX_BOOL CXFA_WideTextRead::IsEOF() const
{
    return m_iPosition >= m_wsBuffer.GetLength();
}
FX_INT32 CXFA_WideTextRead::ReadString(FX_LPWSTR pStr, FX_INT32 iMaxLength, FX_BOOL &bEOS, FX_INT32 const *pByteSize )
{
    if (iMaxLength > m_wsBuffer.GetLength() - m_iPosition) {
        iMaxLength = m_wsBuffer.GetLength() - m_iPosition;
    }
    FXSYS_wcsncpy(pStr, (FX_LPCWSTR)m_wsBuffer + m_iPosition, iMaxLength);
    m_iPosition += iMaxLength;
    bEOS = IsEOF();
    return iMaxLength;
}
FX_WORD CXFA_WideTextRead::GetCodePage() const
{
    return (sizeof(FX_WCHAR) == 2) ? FX_CODEPAGE_UTF16LE : FX_CODEPAGE_UTF32LE;
}
FX_WORD CXFA_WideTextRead::SetCodePage(FX_WORD wCodePage)
{
    return GetCodePage();
}
