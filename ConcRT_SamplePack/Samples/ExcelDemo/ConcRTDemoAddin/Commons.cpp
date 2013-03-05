/***
* ==++==
*
* Copyright (c) Microsoft Corporation.  All rights reserved.
*
* ==--==
* =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
*
* Commons.cpp
*
* Common implementations required to register the Excel(c) add-in
* as descriped in MICROSOFT OFFICE EXCEL 2007 XLL SOFTWARE DEVELOPMENT KIT
*
* =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
****/

#include "stdafx.h"
#include "MonteCarlo.h"
#include "Commons.h"

/*
** rgFuncs
**
** This is a table of all the functions exported by this module.
** These functions are all registered (in xlAutoOpen) when you
** open the XLL. Before every string, leave a space for the
** byte count. The format of this table is the same as
** the last seven arguments to the REGISTER function.
** rgFuncsRows define the number of rows in the table. The
** dimension [3] represents the number of columns in the table.
*/
extern const int rgFuncsRows = 10;
extern LPWSTR rgFuncs[rgFuncsRows][4] = {
	{L"PayOff",				L"BJBBBBBB#!", L"PayOff", L"trials,asset1,annualised_vol1,asset2,annualised_vol2,correlation,expiration"},
	{L"PayOff_Parallel",	L"BJBBBBBB$", L"PayOff_Parallel", L"trials,asset1,annualised_vol1,asset2,annualised_vol2,correlation,expiration"},
	{L"StockPortfolio",	L"JCBBJJJJJJJBBB#!", L"StockPortfolio", L"dataFolder,start,end,results row,results col,data row,data col,maxContributors,numIterations,numPortfolios,mutationFactor,crossoverRate,swapProbability"},
	{L"StockPortfolio_Parallel",	L"JCBBJJJJJJJBBBJ$", L"StockPortfolio_Parallel", L"dataFolder,start,end,results row,results col,data row,data col,maxContributors,numIterations,numPortfolios,mutationFactor,crossoverRate,swapProbability,maxConcurrency"},
	{L"StockPortfolio_Update",	L"AJ$", L"StockPortfolio_Update", L"instance handle"},
	{L"StockPortfolio_IsDone",	L"AJ$", L"StockPortfolio_IsDone", L"instance handle"},
	{L"StockPortfolio_Cancel",	L"AJ$", L"StockPortfolio_Cancel", L"instance handle"},
	{L"StockPortfolio_Delete",	L"AJ$", L"StockPortfolio_Delete", L"instance handle"},
	{L"StockPortfolio_GetCalcTime",	L"JJ$", L"StockPortfolio_GetCalcTime", L"instance handle"},
	{L"StockPortfolio_GetFileReadTime",	L"JJ$", L"StockPortfolio_GetFileReadTime", L"instance handle"}
};

/*
** xlAutoOpen
**
** xlAutoOpen is how Microsoft Excel loads XLL files.
** When you open an XLL, Microsoft Excel calls the xlAutoOpen
** function, and nothing more.
**
** More specifically, xlAutoOpen is called by Microsoft Excel:
**
**  - when you open this XLL file from the File menu,
**  - when this XLL is in the XLSTART directory, and is
**		automatically opened when Microsoft Excel starts,
**  - when Microsoft Excel opens this XLL for any other reason, or
**  - when a macro calls REGISTER(), with only one argument, which is the
**		name of this XLL.
**
** xlAutoOpen is also called by the Add-in Manager when you add this XLL
** as an add-in. The Add-in Manager first calls xlAutoAdd, then calls
** REGISTER("EXAMPLE.XLL"), which in turn calls xlAutoOpen.
**
** xlAutoOpen should:
**
**  - register all the functions you want to make available while this
**		XLL is open,
**
**  - add any menus or menu items that this XLL supports,
**
**  - perform any other initialization you need, and
**
**  - return 1 if successful, or return 0 if your XLL cannot be opened.
*/
CONCRTDEMOADDIN_API int WINAPI xlAutoOpen(void)
{

	static XLOPER12 xDLL;	/* name of this DLL */
	int i;					/* Loop index */

	/*
	** In the following block of code the name of the XLL is obtained by
	** calling xlGetName. This name is used as the first argument to the
	** REGISTER function to specify the name of the XLL. Next, the XLL loops
	** through the rgFuncs[] table, registering each function in the table using
	** xlfRegister. Functions must be registered before you can add a menu
	** item.
	*/

	Excel12f(xlGetName, &xDLL, 0);

        for (i=0;i<rgFuncsRows;i++) 
		{
			Excel12f(xlfRegister, 0, 5,
				(LPXLOPER12)&xDLL,
				(LPXLOPER12)TempStr12(rgFuncs[i][0]),
				(LPXLOPER12)TempStr12(rgFuncs[i][1]),
				(LPXLOPER12)TempStr12(rgFuncs[i][2]),
				(LPXLOPER12)TempStr12(rgFuncs[i][3]));
		}

	/* Free the XLL filename */
	Excel12f(xlFree, 0, 1, (LPXLOPER12)&xDLL);

	return 1;
}

/*
** xlAutoClose
**
** xlAutoClose is called by Microsoft Excel:
**
**  - when you quit Microsoft Excel, or
**  - when a macro sheet calls UNREGISTER(), giving a string argument
**		which is the name of this XLL.
**
** xlAutoClose is called by the Add-in Manager when you remove this XLL from
** the list of loaded add-ins. The Add-in Manager first calls xlAutoRemove,
** then calls UNREGISTER("EXAMPLE.XLL"), which in turn calls xlAutoClose.
**
**
** xlAutoClose should:
**
**  - Remove any menus or menu items that were added in xlAutoOpen,
**
**  - do any necessary global cleanup, and
**
**  - delete any names that were added (names of exported functions, and
**		so on). Remember that registering functions may cause names to be created.
**
** xlAutoClose does NOT have to unregister the functions that were registered
** in xlAutoOpen. This is done automatically by Microsoft Excel after
** xlAutoClose returns.
**
** xlAutoClose should return 1.
*/
CONCRTDEMOADDIN_API int WINAPI xlAutoClose(void)
{
	int i;

	/*
	** This block first deletes all names added by xlAutoOpen or by
	** xlAutoRegister.
	*/

	for (i = 0; i < rgFuncsRows; i++)
		Excel12f(xlfSetName, 0, 1, TempStr12(rgFuncs[i][2]));

	return 1;
}

/*
** xlAutoRegister
**
** This function is called by Microsoft Excel if a macro sheet tries to
** register a function without specifying the type_text argument. If that
** happens, Microsoft Excel calls xlAutoRegister, passing the name of the
** function that the user tried to register. xlAutoRegister should use the
** normal REGISTER function to register the function, but this time it must
** specify the type_text argument. If xlAutoRegister does not recognize the
** function name, it should return a #VALUE! error. Otherwise, it should
** return whatever REGISTER returned.
**
** Arguments:
**
**	    LPXLOPER12 pxName   xltypeStr containing the
**                          name of the function
**                          to be registered. This is not
**                          case sensitive, because
**                          Microsoft Excel uses Pascal calling
**                          convention.
**
** Returns:
**
**      LPXLOPER12          xltypeNum containing the result
**                          of registering the function,
**                          or xltypeErr containing #VALUE!
**                          if the function could not be
**                          registered.
*/
CONCRTDEMOADDIN_API LPXLOPER12 WINAPI xlAutoRegister12(LPXLOPER12 pxName)
{
	static XLOPER12 xDLL, xRegId;
	int i;

	/*
	** This block initializes xRegId to a #VALUE! error first. This is done in
	** case a function is not found to register. Next, the code loops through the
	** functions in rgFuncs[] and uses lpstricmp to determine if the current
	** row in rgFuncs[] represents the function that needs to be registered.
	** When it finds the proper row, the function is registered and the
	** register ID is returned to Microsoft Excel. If no matching function is
	** found, an xRegId is returned containing a #VALUE! error.
	*/

	xRegId.xltype = xltypeErr;
	xRegId.val.err = xlerrValue;

	for (i = 0; i < rgFuncsRows; i++) 
	{
		if (!lpwstricmp(rgFuncs[i][0], pxName->val.str)) 
		{
			Excel12f(xlGetName, &xDLL, 0);

			Excel12f(xlfRegister, 0, 5,
		  		(LPXLOPER12)&xDLL,
		  		(LPXLOPER12)TempStr12(rgFuncs[i][0]),
		  		(LPXLOPER12)TempStr12(rgFuncs[i][1]),
		  		(LPXLOPER12)TempStr12(rgFuncs[i][2]),
		  		(LPXLOPER12)TempStr12(rgFuncs[i][3]));

			/* Free the XLL filename */
			Excel12f(xlFree, 0, 1, (LPXLOPER12)&xDLL);

			return (LPXLOPER12)&xRegId;
		}
	}

	//Word of caution - returning static XLOPERs/XLOPER12s is not thread safe
	//for UDFs declared as thread safe, use alternate memory allocation mechanisms

	return (LPXLOPER12)&xRegId;
}

/*
** xlAutoAdd
**
** This function is called by the Add-in Manager only. When you add a
** DLL to the list of active add-ins, the Add-in Manager calls xlAutoAdd()
** and then opens the XLL, which in turn calls xlAutoOpen.
**
*/
CONCRTDEMOADDIN_API int WINAPI xlAutoAdd(void)
{
	//XCHAR szBuf[255];

	//wsprintfW((LPWSTR)szBuf, L"Thank you for adding ConcRTDemoAddin.XLL\n build date %hs, time %hs",__DATE__, __TIME__);

	/* Display a dialog box indicating that the XLL was successfully added */
	//Excel12f(xlcAlert, 0, 2, TempStr12(szBuf), TempInt12(2));
	return 1;
}

/*
** xlAutoRemove
**
** This function is called by the Add-in Manager only. When you remove
** an XLL from the list of active add-ins, the Add-in Manager calls
** xlAutoRemove() and then UNREGISTER("EXAMPLE.XLL").
**
** You can use this function to perform any special tasks that need to be
** performed when you remove the XLL from the Add-in Manager's list
** of active add-ins. For example, you may want to delete an
** initialization file when the XLL is removed from the list.
*/
CONCRTDEMOADDIN_API int WINAPI xlAutoRemove(void)
{
	/* Display a dialog box indicating that the XLL was successfully removed */
	//Excel12f(xlcAlert, 0, 2, TempStr12(L"Thank you for removing ConcRTDemoAddin.XLL!"), TempInt12(2));
	return 1;
}

/* xlAddInManagerInfo12
**
**
** This function is called by the Add-in Manager to find the long name
** of the add-in. If xAction = 1, this function should return a string
** containing the long name of this XLL, which the Add-in Manager will use
** to describe this XLL. If xAction = 2 or 3, this function should return
** #VALUE!.
**
** Arguments
**
**      LPXLOPER12 xAction    The information you want; either
**                          1 = the long name of the
**                              add in, or
**                          2 = reserved
**                          3 = reserved
**
** Return value
**
**      LPXLOPER12            The long name or #VALUE!.
**
*/
CONCRTDEMOADDIN_API LPXLOPER12 WINAPI xlAddInManagerInfo12(LPXLOPER12 xAction)
{
	static XLOPER12 xInfo, xIntAction;

	/*
	** This code coerces the passed-in value to an integer. This is how the
	** code determines what is being requested. If it receives a 1, it returns a
	** string representing the long name. If it receives anything else, it
	** returns a #VALUE! error.
	*/

	Excel12f(xlCoerce, &xIntAction, 2, xAction, TempInt12(xltypeInt));

	if(xIntAction.val.w == 1) 
	{
		xInfo.xltype = xltypeStr;
		xInfo.val.str = L"\026ConcRT Demo DLL";
	}
	else 
	{
		xInfo.xltype = xltypeErr;
		xInfo.val.err = xlerrValue;
	}

	//Word of caution - returning static XLOPERs/XLOPER12s is not thread safe
	//for UDFs declared as thread safe, use alternate memory allocation mechanisms

	return (LPXLOPER12)&xInfo;
}

///***************************************************************************
// lpwstricmp()
//
// Purpose: Compares a pascal string and a null-terminated C-string to see
// if they are equal.  Method is case insensitive
//
// Parameters:
//
//      LPWSTR s     First string (null-terminated)
//      LPWSTR t     Second string (byte counted)
//
// Returns: 
//
//      int         0 if they are equal
//                  Nonzero otherwise
//
// Comments:
//
//      Unlike the usual string functions, lpwstricmp
//      doesn't care about collating sequence.
//
// History:  Date       Author        Reason
///***************************************************************************

int lpwstricmp(LPWSTR s, LPWSTR t)
{
	int i;

	if (wcslen(s) != *t)
		return 1;

	for (i = 1; i <= s[0]; i++)
	{
		if (towlower(s[i-1]) != towlower(t[i]))
			return 1;
	}
	return 0;
}


/* xlAddInManagerInfo12
**
**
** This function is called by the Add-in to output an integer ( DWORD )
** to a specific cell
**
** Arguments
**
**      short rw  : Excel cell row
**      short col : Excel cell col
**      DWORD iVal: value to be written
**
** Return value ( void )
**
*/
__declspec(dllexport) void WINAPI xlSetInt(short rw, short col, DWORD iVal)
{
	XLOPER12 xRef, xValue;

	xRef.xltype = xltypeSRef;
	xRef.val.sref.count = 1;
	xRef.val.sref.ref.rwFirst = rw;
	xRef.val.sref.ref.rwLast = rw;
	xRef.val.sref.ref.colFirst = col;
	xRef.val.sref.ref.colLast = col;
	xValue.xltype = xltypeInt;
	xValue.val.w = iVal;
	Excel12(xlSet, 0, 2, (LPXLOPER12)&xRef, (LPXLOPER12)&xValue);
}

/* xlAddInManagerInfo12
**
**
** This function is called by the Add-in to output a float ( double )
** to a specific cell
**
** Arguments
**
**      short  rw  : Excel cell row
**      short  col : Excel cell col
**      double iVal: value to be written
**
** Return value ( void )
**
*/
__declspec(dllexport) void WINAPI xlSetDouble(short rw, short col, double iVal)
{
	XLOPER12 xRef, xValue;

	xRef.xltype = xltypeSRef;
	xRef.val.sref.count = 1;
	xRef.val.sref.ref.rwFirst = rw;
	xRef.val.sref.ref.rwLast = rw;
	xRef.val.sref.ref.colFirst = col;
	xRef.val.sref.ref.colLast = col;
	xValue.xltype = xltypeNum;
	xValue.val.num = iVal;
	Excel12(xlSet, 0, 2, (LPXLOPER12)&xRef, (LPXLOPER12)&xValue);
}

/* xlAddInManagerInfo12
**
**
** This function is called by the Add-in to output a date ( DATE )
** to a specific cell
**
** Arguments
**
**      short rw : Excel cell row
**      short col: Excel cell col
**      DATE iVal: value to be written
**
** Return value ( void )
**
*/
__declspec(dllexport) void WINAPI xlSetDate(short rw, short col, DATE iVal)
{
	XLOPER12 xRef, xValue;

	xRef.xltype = xltypeSRef;
	xRef.val.sref.count = 1;
	xRef.val.sref.ref.rwFirst = rw;
	xRef.val.sref.ref.rwLast = rw;
	xRef.val.sref.ref.colFirst = col;
	xRef.val.sref.ref.colLast = col;
	xValue.xltype = xltypeNum;
	xValue.val.num = iVal;
	Excel12(xlSet, 0, 2, (LPXLOPER12)&xRef, (LPXLOPER12)&xValue);
}
