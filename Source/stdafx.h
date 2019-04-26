/*
PowerProps Library Source File

Copyright © 2009-2019, Keelan Stuart. All rights reserved.

PowerProps is a generic property library which one can use to maintain
easily discoverable data in a number of types, as well as convert that
data to other formats and de/serialize in multiple modes

PowerProps is free software; you can redistribute it and/or modify it under
the terms of the MIT License:

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/


// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers

#include <stdint.h>
#include <windows.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <iostream>
#include <xstring>
#include <deque>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <assert.h>

typedef std::basic_string<TCHAR, std::char_traits<TCHAR>, std::allocator<TCHAR> > tstring;

#if defined(_UNICODE) || defined(UNICODE)

#define LOCAL_TCS2MBCS(wcs, mbcs) {               \
  size_t origsize = _tcslen(wcs) + 1;             \
  size_t newsize = (origsize * 2) * sizeof(char); \
  mbcs = (char *)_malloca(newsize);               \
  wcstombs(mbcs, wcs, newsize); }

#define LOCAL_TCS2WCS(mbcs, wcs) wcs = mbcs

#else

#define LOCAL_TCS2MBCS(wcs, mbcs) mbcs = wcs

#define LOCAL_TCS2WCS(mbcs, wcs) {                \
  size_t origsize = strlen(mbcs) + 1;             \
  size_t newsize = origsize * sizeof(TCHAR);      \
  wcs = (TCHAR *)_malloca(newsize);               \
  mbstowcs(wcs, mbcs, newsize); }

#endif
