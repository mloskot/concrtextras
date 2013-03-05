//--------------------------------------------------------------------------
// 
//  Copyright (c) Microsoft Corporation.  All rights reserved. 
// 
//  File: Cartoonizer.h
//
//  Cartoonizer App
//
//--------------------------------------------------------------------------

#pragma once

#ifndef __AFXWIN_H__
    #error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"        // main symbols


// CCartoonizerApp:
// See Cartoonizer.cpp for the implementation of this class
//

class CCartoonizerApp : public CWinApp
{
public:
    CCartoonizerApp();

// Overrides
    public:
    virtual BOOL InitInstance();

// Implementation

    DECLARE_MESSAGE_MAP()
};

extern CCartoonizerApp theApp;