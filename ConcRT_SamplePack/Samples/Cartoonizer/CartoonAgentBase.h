//--------------------------------------------------------------------------
// 
//  Copyright (c) Microsoft Corporation.  All rights reserved. 
// 
//  File: CartoonAgentBase.h
//
//  base classes for Cartoon Agents
//
//--------------------------------------------------------------------------

#pragma once
#include "agents.h"
using namespace Concurrency;

class CartoonAgentBase;
class IFrameProcesser;
class VideoReader;

struct FrameData
{
    int m_Size;
    unsigned int m_StartWidth;
    unsigned int m_StartHeight;
    unsigned int m_EndWidth;
    unsigned int m_EndHeight;
    int m_Pitch;
    int m_BBP;
    int m_ColorPlanes;
    BYTE* m_pFrame;
    int m_PhaseCount;
    bool m_fParallel;
    bool m_fSimpleParallelFor;
    bool m_final;
    int m_neighbourArea;
    IFrameProcesser* m_pFrameProcesser;
    CartoonAgentBase* m_pCartoonAgent;
    VideoReader * m_pVideoReader;

    FrameData()
    {
        m_fSimpleParallelFor = false;
        m_neighbourArea = 3;
        m_PhaseCount = 0;
        m_Size = 0;
        m_StartWidth = 0;
        m_EndWidth = 0;
        m_StartHeight = 0;
        m_EndHeight = 0;
        m_Pitch = 0;
        m_BBP = 0;
        m_pFrame = NULL;
        m_fParallel = false;
        m_pFrameProcesser = NULL;
        m_ColorPlanes = 0;
        m_final = false;
        m_pCartoonAgent = NULL;
        m_pVideoReader = NULL;
    }
};
//////////////////////////////////////////////////////

class IFrameProcesser
{
public:
    enum FrameType
    {
        kStaticImage,
        kVideoFrame
    };

    virtual ~IFrameProcesser()
    {
    }

    virtual void SetCurrentFrame(BYTE* pFrame,int size, int width, int height, int pitch, int bpp, int clrPlanes) = 0;
    virtual BYTE* GetOutputFrame() = 0;
    virtual BYTE* GetSourceFrame() = 0;
    virtual BYTE* GetOutputFrame(int& size, unsigned int& height, unsigned int& width, int& BPP, int& pitch, int& clrPlanes) = 0;
    virtual BYTE* GetSourceFrame(int& size, unsigned int& height, unsigned int& width, int& BPP, int& pitch, int& clrPlanes) = 0;
    virtual void ApplyFilters(int nPhases, bool fParallel) = 0;
    virtual void FrameDone() = 0;
    virtual void ApplyColorSimplifier(unsigned int startHeight, unsigned int endHeight, unsigned int startWidth, unsigned int endWidth) = 0;
    virtual void ApplyColorSimplifierOpenMP(unsigned int startHeight, unsigned int endHeight, unsigned int startWidth, unsigned int endWidth) = 0;
    virtual void ApplyEdgeDetection() = 0;
    virtual void ApplyEdgeDetectionParallel(unsigned int startHeight, unsigned int endHeight, unsigned int startWidth, unsigned int endWidth) = 0;
    virtual void ApplyEdgeDetectionOpenMP(unsigned int startHeight, unsigned int endHeight, unsigned int startWidth, unsigned int endWidth) = 0;
    virtual void ApplyEdgeDetection(BYTE* pFrame, unsigned int startHeight, unsigned int endHeight, unsigned int startWidth, unsigned int endWidth) = 0;
    virtual void SetParallelOption(bool fParallel) = 0;
    virtual void SetUpdateFlag(bool fUpdate) = 0;
    virtual void UseSimpleParallelFor(bool fSimple) = 0;
    virtual void SetFrameType(FrameType enFrameType) = 0; 
    virtual void SetNeighbourArea(unsigned int area) = 0;
    virtual void ApplyEdgeDetectionArea(unsigned int startHeight, unsigned int endHeight, unsigned int startWidth, unsigned int endWidth) = 0;
    virtual unsigned int GetHeight()
    {
        return m_Height;
    }
    virtual unsigned int GetWidth()
    {
        return m_Width;
    }
protected:   
    CartoonAgentBase* m_pCartoonAgent;
    unsigned int  m_Width;
    unsigned int  m_Height;
    unsigned int m_Size;
    int m_Pitch;
    unsigned int m_BPP;
    unsigned int m_ColorPlanes;
};


/////////////////////////////////////
class ICartoonDlg
{
public:
    virtual void updateUI() = 0;
    virtual void ReportPerf(double totaltime) = 0;
};

////////////////////////////////
class CartoonAgentBase: public agent
{
public:
    CartoonAgentBase(ICartoonDlg* pDlg);
    virtual ~CartoonAgentBase();
    virtual void UpdateUI();
    virtual void GetFrame(BYTE*& src, BYTE*& dest, int& size, unsigned int& height, unsigned int& width, int& BPP, int& pitch, int& clrPlanes);
    virtual void GetFrame(BYTE*& src, BYTE*& dest);
    virtual void SetCurrentFrame(BYTE* pFrame,int size, int width, int height, int pitch, int bpp, int clrPlanes);
    virtual void start()
    {
    }
    virtual void FrameFinished(FrameData data)
    {
        m_pUIDlg->updateUI();
    }
protected:
    IFrameProcesser* m_pFrameProcessor;
    ICartoonDlg* m_pUIDlg;
};