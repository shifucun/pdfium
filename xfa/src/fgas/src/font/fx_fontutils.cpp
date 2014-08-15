// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "../fgas_base.h"
#include "fx_fontutils.h"
FX_DWORD FGAS_GetFontHashCode(FX_WORD wCodePage, FX_DWORD dwFontStyles)
{
    FX_DWORD dwHash = wCodePage;
    if (dwFontStyles & FX_FONTSTYLE_FixedPitch) {
        dwHash |= 0x00010000;
    }
    if (dwFontStyles & FX_FONTSTYLE_Serif) {
        dwHash |= 0x00020000;
    }
    if (dwFontStyles & FX_FONTSTYLE_Symbolic) {
        dwHash |= 0x00040000;
    }
    if (dwFontStyles & FX_FONTSTYLE_Script) {
        dwHash |= 0x00080000;
    }
    if (dwFontStyles & FX_FONTSTYLE_Italic) {
        dwHash |= 0x00100000;
    }
    if (dwFontStyles & FX_FONTSTYLE_Bold) {
        dwHash |= 0x00200000;
    }
    return dwHash;
}
FX_DWORD FGAS_GetFontFamilyHash(FX_LPCWSTR pszFontFamily, FX_DWORD dwFontStyles, FX_WORD wCodePage)
{
    CFX_WideString wsFont(pszFontFamily);
    if (dwFontStyles & FX_FONTSTYLE_Bold) {
        wsFont += L"Bold";
    }
    if (dwFontStyles & FX_FONTSTYLE_Italic) {
        wsFont += L"Italic";
    }
    wsFont += wCodePage;
    return FX_HashCode_String_GetW((FX_LPCWSTR)wsFont, wsFont.GetLength());
}
static const FGAS_FONTUSB g_FXGdiFontUSBTable[] = {
    {0x0000  ,	0x007F  ,	0  ,	1252  },
    {0x0080  ,	0x00FF  ,	1  ,	1252  },
    {0x0100  ,	0x017F  ,	2  ,	1250  },
    {0x0180  ,	0x024F  ,	3  ,	1250  },
    {0x0250  ,	0x02AF  ,	4  ,	0xFFFF},
    {0x02B0  ,	0x02FF  ,	5  ,	0xFFFF},
    {0x0300  ,	0x036F  ,	6  ,	0xFFFF},
    {0x0370  ,	0x03FF  ,	7  ,	1253  },
    {0x0400  ,	0x04FF  ,	9  ,	1251  },
    {0x0500  ,	0x052F  ,	9  ,	0xFFFF},
    {0x0530  ,	0x058F  ,	10 ,	0xFFFF},
    {0x0590  ,	0x05FF  ,	11 ,	1255  },
    {0x0600  ,	0x06FF  ,	13 ,	1256  },
    {0x0700  ,	0x074F  ,	71 ,	0xFFFF},
    {0x0750  ,	0x077F  ,	13 ,	0xFFFF},
    {0x0780  ,	0x07BF  ,	72 ,	0xFFFF},
    {0x07C0  ,	0x07FF  ,	14 ,	0xFFFF},
    {0x0800  ,	0x08FF  ,	999,	0xFFFF},
    {0x0900  ,	0x097F  ,	15 ,	0xFFFF},
    {0x0980  ,	0x09FF  ,	16 ,	0xFFFF},
    {0x0A00  ,	0x0A7F  ,	17 ,	0xFFFF},
    {0x0A80  ,	0x0AFF  ,	18 ,	0xFFFF},
    {0x0B00  ,	0x0B7F  ,	19 ,	0xFFFF},
    {0x0B80  ,	0x0BFF  ,	20 ,	0xFFFF},
    {0x0C00  ,	0x0C7F  ,	21 ,	0xFFFF},
    {0x0C80  ,	0x0CFF  ,	22 ,	0xFFFF},
    {0x0D00  ,	0x0D7F  ,	23 ,	0xFFFF},
    {0x0D80  ,	0x0DFF  ,	73 ,	0xFFFF},
    {0x0E00  ,	0x0E7F  ,	24 ,	874   },
    {0x0E80  ,	0x0EFF  ,	25 ,	0xFFFF},
    {0x0F00  ,	0x0FFF  ,	70 ,	0xFFFF},
    {0x1000  ,	0x109F  ,	74 ,	0xFFFF},
    {0x10A0  ,	0x10FF  ,	26 ,	0xFFFF},
    {0x1100  ,	0x11FF  ,	28 ,	0xFFFF},
    {0x1200  ,	0x137F  ,	75 ,	0xFFFF},
    {0x1380  ,	0x139F  ,	75 ,	0xFFFF},
    {0x13A0  ,	0x13FF  ,	76 ,	0xFFFF},
    {0x1400  ,	0x167F  ,	77 ,	0xFFFF},
    {0x1680  ,	0x169F  ,	78 ,	0xFFFF},
    {0x16A0  ,	0x16FF  ,	79 ,	0xFFFF},
    {0x1700  ,	0x171F  ,	84 ,	0xFFFF},
    {0x1720  ,	0x173F  ,	84 ,	0xFFFF},
    {0x1740  ,	0x175F  ,	84 ,	0xFFFF},
    {0x1760  ,	0x177F  ,	84 ,	0xFFFF},
    {0x1780  ,	0x17FF  ,	80 ,	0xFFFF},
    {0x1800  ,	0x18AF  ,	81 ,	0xFFFF},
    {0x18B0  ,	0x18FF  ,	999,	0xFFFF},
    {0x1900  ,	0x194F  ,	93 ,	0xFFFF},
    {0x1950  ,	0x197F  ,	94 ,	0xFFFF},
    {0x1980  ,	0x19DF  ,	95 ,	0xFFFF},
    {0x19E0  ,	0x19FF  ,	80 ,	0xFFFF},
    {0x1A00  ,	0x1A1F  ,	96 ,	0xFFFF},
    {0x1A20  ,	0x1AFF  ,	999,	0xFFFF},
    {0x1B00  ,	0x1B7F  ,	27 ,	0xFFFF},
    {0x1B80  ,	0x1BBF  ,	112,	0xFFFF},
    {0x1BC0  ,	0x1BFF  ,	999,	0xFFFF},
    {0x1C00  ,	0x1C4F  ,	113,	0xFFFF},
    {0x1C50  ,	0x1C7F  ,	114,	0xFFFF},
    {0x1C80  ,	0x1CFF  ,	999,	0xFFFF},
    {0x1D00  ,	0x1D7F  ,	4  ,	0xFFFF},
    {0x1D80  ,	0x1DBF  ,	4  ,	0xFFFF},
    {0x1DC0  ,	0x1DFF  ,	6  ,	0xFFFF},
    {0x1E00  ,	0x1EFF  ,	29 ,	0xFFFF},
    {0x1F00  ,	0x1FFF  ,	30 ,	0xFFFF},
    {0x2000  ,	0x206F  ,	31 ,	0xFFFF},
    {0x2070  ,	0x209F  ,	32 ,	0xFFFF},
    {0x20A0  ,	0x20CF  ,	33 ,	0xFFFF},
    {0x20D0  ,	0x20FF  ,	34 ,	0xFFFF},
    {0x2100  ,	0x214F  ,	35 ,	0xFFFF},
    {0x2150  ,	0x215F  ,	36 ,	0xFFFF},
    {0x2160  ,	0x216B  ,	36 ,	936   },
    {0x216C  ,	0x216F  ,	36 ,	0xFFFF},
    {0x2170  ,	0x2179  ,	36 ,	936   },
    {0x217A  ,	0x218F  ,	36 ,	0xFFFF},
    {0x2190  ,	0x2199  ,	37 ,	949   },
    {0x219A  ,	0x21FF  ,	37 ,	0xFFFF},
    {0x2200  ,	0x22FF  ,	38 ,	0xFFFF},
    {0x2300  ,	0x23FF  ,	39 ,	0xFFFF},
    {0x2400  ,	0x243F  ,	40 ,	0xFFFF},
    {0x2440  ,	0x245F  ,	41 ,	0xFFFF},
    {0x2460  ,	0x2473  ,	42 ,	932   },
    {0x2474  ,	0x249B  ,	42 ,	936   },
    {0x249C  ,	0x24E9  ,	42 ,	949   },
    {0x24EA  ,	0x24FF  ,	42 ,	0xFFFF},
    {0x2500  ,	0x2573  ,	43 ,	936   },
    {0x2574  ,	0x257F  ,	43 ,	0xFFFF},
    {0x2580  ,	0x2580  ,	44 ,	0xFFFF},
    {0x2581  ,	0x258F  ,	44 ,	936   },
    {0x2590  ,	0x259F  ,	44 ,	0xFFFF},
    {0x25A0  ,	0x25FF  ,	45 ,	0xFFFF},
    {0x2600  ,	0x26FF  ,	46 ,	0xFFFF},
    {0x2700  ,	0x27BF  ,	47 ,	0xFFFF},
    {0x27C0  ,	0x27EF  ,	38 ,	0xFFFF},
    {0x27F0  ,	0x27FF  ,	37 ,	0xFFFF},
    {0x2800  ,	0x28FF  ,	82 ,	0xFFFF},
    {0x2900  ,	0x297F  ,	37 ,	0xFFFF},
    {0x2980  ,	0x29FF  ,	38 ,	0xFFFF},
    {0x2A00  ,	0x2AFF  ,	38 ,	0xFFFF},
    {0x2B00  ,	0x2BFF  ,	37 ,	0xFFFF},
    {0x2C00  ,	0x2C5F  ,	97 ,	0xFFFF},
    {0x2C60  ,	0x2C7F  ,	29 ,	0xFFFF},
    {0x2C80  ,	0x2CFF  ,	8  ,	0xFFFF},
    {0x2D00  ,	0x2D2F  ,	26 ,	0xFFFF},
    {0x2D30  ,	0x2D7F  ,	98 ,	0xFFFF},
    {0x2D80  ,	0x2DDF  ,	75 ,	0xFFFF},
    {0x2DE0  ,	0x2DFF  ,	9  ,	0xFFFF},
    {0x2E00  ,	0x2E7F  ,	31 ,	0xFFFF},
    {0x2E80  ,	0x2EFF  ,	59 ,	0xFFFF},
    {0x2F00  ,	0x2FDF  ,	59 ,	0xFFFF},
    {0x2FE0  ,	0x2FEF  ,	999,	0xFFFF},
    {0x2FF0  ,	0x2FFF  ,	59 ,	0xFFFF},
    {0x3000  ,	0x303F  ,	48 ,	0xFFFF},
    {0x3040  ,	0x309F  ,	49 ,	932   },
    {0x30A0  ,	0x30FF  ,	50 ,	932   },
    {0x3100  ,	0x3129  ,	51 ,	936   },
    {0x312A  ,	0x312F  ,	51 ,	0xFFFF},
    {0x3130  ,	0x318F  ,	52 ,	949   },
    {0x3190  ,	0x319F  ,	59 ,	0xFFFF},
    {0x31A0  ,	0x31BF  ,	51 ,	0xFFFF},
    {0x31C0  ,	0x31EF  ,	61 ,	0xFFFF},
    {0x31F0  ,	0x31FF  ,	50 ,	0xFFFF},
    {0x3200  ,	0x321C  ,	54 ,	949   },
    {0x321D  ,	0x325F  ,	54 ,	0xFFFF},
    {0x3260  ,	0x327F  ,	54 ,	949   },
    {0x3280  ,	0x32FF  ,	54 ,	0xFFFF},
    {0x3300  ,	0x3387  ,	55 ,	0xFFFF},
    {0x3388  ,	0x33D0  ,	55 ,	949   },
    {0x33D1  ,	0x33FF  ,	55 ,	0xFFFF},
    {0x3400  ,	0x4DBF  ,	59 ,	0xFFFF},
    {0x4DC0  ,	0x4DFF  ,	99 ,	0xFFFF},
    {0x4E00  ,	0x9FA5  ,	59 ,	936   },
    {0x9FA6  ,	0x9FFF  ,	59 ,	0xFFFF},
    {0xA000  ,	0xA48F  ,	83 ,	0xFFFF},
    {0xA490  ,	0xA4CF  ,	83 ,	0xFFFF},
    {0xA4D0  ,	0xA4FF  ,	999,	0xFFFF},
    {0xA500  ,	0xA63F  ,	12 ,	0xFFFF},
    {0xA640  ,	0xA69F  ,	9  ,	0xFFFF},
    {0xA6A0  ,	0xA6FF  ,	999,	0xFFFF},
    {0xA700  ,	0xA71F  ,	5  ,	0xFFFF},
    {0xA720  ,	0xA7FF  ,	29 ,	0xFFFF},
    {0xA800  ,	0xA82F  ,	100,	0xFFFF},
    {0xA830  ,	0xA8FF  ,	999,	0xFFFF},
    {0xA840  ,	0xA87F  ,	53 ,	0xFFFF},
    {0xA880  ,	0xA8DF  ,	115,	0xFFFF},
    {0xA8E0  ,	0xA8FF  ,	999,	0xFFFF},
    {0xA900  ,	0xA92F  ,	116,	0xFFFF},
    {0xA930  ,	0xA95F  ,	117,	0xFFFF},
    {0xA960  ,	0xA9FF  ,	999,	0xFFFF},
    {0xAA00  ,	0xAA5F  ,	118,	0xFFFF},
    {0xAA60  ,	0xABFF  ,	999,	0xFFFF},
    {0xAC00  ,	0xD7AF  ,	56 ,	949   },
    {0xD7B0  ,	0xD7FF  ,	999,	0xFFFF},
    {0xD800  ,	0xDB7F  ,	57 ,	0xFFFF},
    {0xDB80  ,	0xDBFF  ,	57 ,	0xFFFF},
    {0xDC00  ,	0xDFFF  ,	57 ,	0xFFFF},
    {0xE000  ,	0xE814  ,	60 ,	0xFFFF},
    {0xE815  ,	0xE864  ,	60 ,	936   },
    {0xE865  ,	0xF8FF  ,	60 ,	0xFFFF},
    {0xF900  ,	0xFA0B  ,	61 ,	949   },
    {0xFA0C  ,	0xFA0D  ,	61 ,	936   },
    {0xFA0E  ,	0xFA2D  ,	61 ,	932   },
    {0xFA2E  ,	0xFAFF  ,	61 ,	0xFFFF},
    {0xFB00  ,	0xFB4F  ,	62 ,	0xFFFF},
    {0xFB50  ,	0xFDFF  ,	63 ,	1256  },
    {0xFE00  ,	0xFE0F  ,	91 ,	0xFFFF},
    {0xFE10  ,	0xFE1F  ,	65 ,	0xFFFF},
    {0xFE20  ,	0xFE2F  ,	64 ,	0xFFFF},
    {0xFE30  ,	0xFE4F  ,	65 ,	0xFFFF},
    {0xFE50  ,	0xFE6F  ,	66 ,	0xFFFF},
    {0xFE70  ,	0xFEFF  ,	67 ,	1256  },
    {0xFF00  ,	0xFF5F  ,	68 ,	936   },
    {0xFF60  ,	0xFF9F  ,	68 ,	932   },
    {0xFFA0  ,	0xFFEF  ,	68 ,	0xFFFF},
};
FGAS_LPCFONTUSB FGAS_GetUnicodeBitField(FX_WCHAR wUnicode)
{
    FX_INT32 iEnd = sizeof(g_FXGdiFontUSBTable) / sizeof(FGAS_FONTUSB) - 1;
    FXSYS_assert(iEnd >= 0);
    FX_INT32 iStart = 0, iMid;
    do {
        iMid = (iStart + iEnd) / 2;
        const FGAS_FONTUSB &usb = g_FXGdiFontUSBTable[iMid];
        if (wUnicode < usb.wStartUnicode) {
            iEnd = iMid - 1;
        } else if (wUnicode > usb.wEndUnicode) {
            iStart = iMid + 1;
        } else {
            return &usb;
        }
    } while (iStart <= iEnd);
    return NULL;
}
