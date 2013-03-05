/***
* ==++==
*
* Copyright (c) Microsoft Corporation.  All rights reserved.
*
* ==--==
* =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
*
* Commons.h
*
* Common declarations required to register the Excel(c) add-in
* as descriped in MICROSOFT OFFICE EXCEL 2007 XLL SOFTWARE DEVELOPMENT KIT
*
* =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
****/

#include "stdafx.h"
#include "MonteCarlo.h"

#pragma once

extern LPWSTR rgFuncs[][4];
extern const int rgFuncsRows;

extern "C"
{
	CONCRTDEMOADDIN_API int WINAPI xlAutoOpen(void);
	CONCRTDEMOADDIN_API int WINAPI xlAutoClose(void);
	CONCRTDEMOADDIN_API LPXLOPER12 WINAPI xlAutoRegister12(LPXLOPER12 pxName);
	CONCRTDEMOADDIN_API int WINAPI xlAutoAdd(void);
	CONCRTDEMOADDIN_API int WINAPI xlAutoRemove(void);
	CONCRTDEMOADDIN_API LPXLOPER12 WINAPI xlAddInManagerInfo12(LPXLOPER12 xAction);
	CONCRTDEMOADDIN_API void WINAPI xlSetInt(short rw, short col, DWORD iVal);
	CONCRTDEMOADDIN_API void WINAPI xlSetDouble(short rw, short col, double iVal);
}

int lpwstricmp(LPWSTR s, LPWSTR t);
