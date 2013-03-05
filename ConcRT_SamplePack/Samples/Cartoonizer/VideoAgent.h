//--------------------------------------------------------------------------
// 
//  Copyright (c) Microsoft Corporation.  All rights reserved. 
// 
//  File: VideoAgent.h
//
//  Implementation of all video agent classes
//
//--------------------------------------------------------------------------

#pragma once
#include "Vfw.h"
#include "agents.h"
#include "FrameProcessing.h"
#include "ImageProcessingAgent.h"
using namespace Concurrency;




class IVideoAgent: public CartoonAgentBase
{
public:
    IVideoAgent(ICartoonDlg* pDlg)
        :CartoonAgentBase(pDlg)
    {
        m_pFrameProcessor->SetUpdateFlag(false);
        m_pFrameProcessor->SetFrameType(IFrameProcesser::kVideoFrame);
        m_FrameCount = 0;
        m_TotalTime = 0.0;
    }
    virtual BYTE* ProcessFrame(FrameData& data) = 0;
public:
     //Perf Measurement
    LARGE_INTEGER m_StartTime;
    LARGE_INTEGER m_EndTime;
    LONGLONG m_Overhead;
    LARGE_INTEGER m_Frequency;
    double m_TotalTime;
    int m_FrameCount;
};

//////////////////////////////////////////////////////////////////////
class VideoHelper
{
public:
    VideoHelper(IVideoAgent* pAgent);
    ~VideoHelper();
    void OpenVideo(CString filePath, FrameData& data);
private:
    IVideoAgent* m_pVideoAgent;
    // opening video members
    PAVISTREAM      m_pAviStream;        // Pointer to the AVI stream
    AVISTREAMINFO m_aviStreamInfo;    // AVI stream informations
    LPVOID m_lpInputFrames;
};

class VideoReader
{
public:
    VideoReader(IVideoAgent* pAgent) : m_pVideoAgent(pAgent)
    {
    }

    void Open(CString filePath);
    void Close();
    void ReadNextFrame(FrameData& data);

private:
    IVideoAgent* m_pVideoAgent;
    // opening video members
    PAVISTREAM      m_pAviStream;        // Pointer to the AVI stream
    AVISTREAMINFO m_aviStreamInfo;    // AVI stream informations
    LPVOID m_lpInputFrames;

    PAVIFILE         m_pf; 
    BITMAPINFOHEADER m_bi; 
    LONG             m_currentSize; 
    BYTE *           m_lpBuffer;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////
class VideoAgent: public IVideoAgent
{
public:
    VideoAgent(CString filePath, int nPhases, bool fParallel, bool fSimplePfor, ICartoonDlg* pDlg);
    ~VideoAgent(void);

    void run();    
    virtual BYTE* ProcessFrame(FrameData& data);
private:
    VideoHelper    m_Helper;

    bool m_fParallel;
    bool m_fSimplePFor;
    LONG m_lStreamSize; 
    int m_nPhases;
    CString m_strFilePath;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////
class VideoAgentOpenMP: public IVideoAgent
{
public:
    VideoAgentOpenMP(CString filePath, int nPhases, ICartoonDlg* pDlg);
    ~VideoAgentOpenMP(void);

    void run();    
    virtual BYTE* ProcessFrame(FrameData& data);
private:
    VideoHelper    m_Helper;
    LONG m_lStreamSize; 
    int m_nPhases;
    CString m_strFilePath;
};

/////////////////////////////////////////////////////////////////////////////////////////////////

class VideoSingleFramePipelineAgent: public IVideoAgent
{
public:
    VideoSingleFramePipelineAgent(CString filePath, int nPhases, ICartoonDlg* pDlg);
    ~VideoSingleFramePipelineAgent(void);

    void run();    
    virtual BYTE* ProcessFrame(FrameData& data);

    void FrameFinished(FrameData data)
    {
       memcpy_s(m_pSrc, m_Size, data.m_pFrameProcesser->GetSourceFrame(), m_Size);
       memcpy_s(m_pDest, m_Size, data.m_pFrameProcesser->GetOutputFrame(), m_Size);
        
        m_pUIDlg->updateUI();        
        delete data.m_pFrameProcesser;
    }

    void UpdateUI()
    {

    }

    void GetFrame(BYTE*& src, BYTE*& dest, int& size, unsigned int& height, unsigned int& width, int& bpp, int& pitch, int& clrPlanes)
    {
        m_pFrameProcessor->GetSourceFrame(size, height, width, bpp, pitch, clrPlanes);
        src = m_pSrc;
        dest = m_pDest;
    }

private:
    VideoHelper    m_Helper;
    unbounded_buffer<FrameData> m_bufferFrames;

    bool m_fParallel;
    LONG m_lStreamSize; 
    int m_nPhases;
    CString m_strFilePath;
    int m_procCount;
    BYTE* m_pSrc;
    BYTE* m_pDest;
    MultiplePipelineNetworkAgent* m_pNetwork;
    int m_frameIndex;
    int m_Size;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class VideoMultiFramePipelineAgent: public IVideoAgent
{
public:
    VideoMultiFramePipelineAgent(CString filePath, int nPhases, ICartoonDlg* pDlg);
    ~VideoMultiFramePipelineAgent(void);

    void run();    
    virtual BYTE* ProcessFrame(FrameData& data);

    void FrameFinished(FrameData data);

    void UpdateUI()
    {

    }

    void GetFrame(BYTE*& src, BYTE*& dest, int& size, unsigned int& height, unsigned int& width, int& bpp, int& pitch, int& clrPlanes)
    {
        m_pFrameProcessor->GetSourceFrame(size, height, width, bpp, pitch, clrPlanes);
        src = m_pSrc;
        dest = m_pDest;
    }

private:
    VideoReader m_VideoReader;
    unbounded_buffer<FrameData> m_bufferFrames;

    bool m_fParallel;
    LONG m_lStreamSize; 
    int m_nPhases;
    CString m_strFilePath;
    int m_procCount;
    BYTE* m_pSrc;
    BYTE* m_pDest;
    MultiplePipelineNetworkAgent* m_pNetwork;
    int m_frameIndex;
    int m_Size;
};