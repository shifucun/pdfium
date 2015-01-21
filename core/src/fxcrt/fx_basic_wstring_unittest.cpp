// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "testing/gtest/include/gtest/gtest.h"
#include "../../../testing/fx_string_testhelpers.h"
#include "../../include/fxcrt/fx_basic.h"

TEST(fxcrt, WideStringUTF16LE_Encode) {
  CFX_WideString wide_strings[] = {L"", L"abc", L"abcdef", L"abc\0def",
	  L"\x4F60\x597d"};
  CFX_ByteString byte_strings[] = {
	  CFX_ByteString(FX_BSTRC("\0\0")),
	  CFX_ByteString(FX_BSTRC("a\0b\0c\0")),
	  CFX_ByteString(FX_BSTRC("a\0b\0c\0d\0e\0f\0")),
	  CFX_ByteString(FX_BSTRC("a\0b\0c\0\0\0d\0e\0f\0")),
	  CFX_ByteString(FX_BSTRC("\x4F60\x597d")),
  }
}
