//--------------------------------------------------------------------------
// 
//  Copyright (c) Microsoft Corporation.  All rights reserved. 
// 
//  File: CartoonAgentBase.cpp
//
//  base classes for Cartoon Agents
//
//--------------------------------------------------------------------------

#include "StdAfx.h"
#include "CartoonAgentBase.h"
#include "FrameProcessing.h"

CartoonAgentBase::CartoonAgentBase(ICartoonDlg* pDlg)
{
    m_pUIDlg = pDlg;
    m_pFrameProcessor = new FrameProcessing(this);
    m_pFrameProcessor->SetUpdateFlag(true);
}

CartoonAgentBase::~CartoonAgentBase()
{
    delete m_pFrameProcessor;
}

void CartoonAgentBase::UpdateUI()
{
    if(NULL!= m_pUIDlg)
    {
        m_pUIDlg->updateUI();
    }
}

void CartoonAgentBase::GetFrame(BYTE*& src, BYTE*& dest, int& size, unsigned int& height, unsigned int& width, int& bpp, int& pitch, int& clrPlanes)
{
    src = m_pFrameProcessor->GetSourceFrame(size, height, width, bpp, pitch, clrPlanes);
    dest = m_pFrameProcessor->GetOutputFrame();
}

void CartoonAgentBase::GetFrame(BYTE*& src, BYTE*& dest)
{
    src = m_pFrameProcessor->GetSourceFrame();
    dest = m_pFrameProcessor->GetOutputFrame();
}

void CartoonAgentBase::SetCurrentFrame(BYTE* pFrame,int size, int width, int height, int pitch, int bpp, int clrPlanes)
{
    m_pFrameProcessor->SetCurrentFrame(pFrame, size, width, height, pitch, bpp, clrPlanes);
}