// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "testing/gtest/include/gtest/gtest.h"
#include "../../../testing/fx_string_testhelpers.h"
#include "../../include/fxcrt/fx_basic.h"

TEST(fxcrt, WideStringUTF16LE_Encode) {
  CFX_WideString wide_strings[] = {
	  L"",
	  L"abc",
	  L"abcdef",
	  L"abc\0def",
	  L"23\0456",
	  L"\x3132\x6162"  // This is wrong? Endianness matters here?
  };
  CFX_ByteString byte_strings[] = {
	  CFX_ByteString(FX_BSTRC("\0\0")),
	  CFX_ByteString(FX_BSTRC("a\0b\0c\0\0\0")),
	  CFX_ByteString(FX_BSTRC("a\0b\0c\0d\0e\0f\0\0\0")),
	  CFX_ByteString(FX_BSTRC("a\0b\0c\0\0\0")),
	  CFX_ByteString(FX_BSTRC("\x32\x00\x33\x00\045\x00\x36\x00\x00\x00")),
	  CFX_ByteString(FX_BSTRC("12ab\0\0"))
  };
  for (size_t i = 0; i < FX_ArraySize(wide_strings); ++i) {
	  EXPECT_EQ(byte_strings[i], wide_strings[i].UTF16LE_Encode())
		  << " for case number " << i;
  }
}
