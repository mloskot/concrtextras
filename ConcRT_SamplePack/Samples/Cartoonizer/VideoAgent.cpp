//--------------------------------------------------------------------------
// 
//  Copyright (c) Microsoft Corporation.  All rights reserved. 
// 
//  File: VideoAgent.cpp
//
//  Implementation of all video agent classes
//
//--------------------------------------------------------------------------

#include "StdAfx.h"
#include "Cartoonizer.h"
#include "CartoonizerDlg.h"
#include "VideoAgent.h"


VideoHelper::VideoHelper(IVideoAgent* pAgent)
{
    m_pAviStream = NULL;
    m_pVideoAgent = pAgent;
}

VideoHelper::~VideoHelper()
{
}

void VideoHelper::OpenVideo(CString strFilePath, FrameData& data)
{    
    AVIFileInit();

    LONG hr;  
    hr = AVIStreamOpenFromFile(&m_pAviStream, strFilePath, streamtypeVIDEO, 0, OF_READ, NULL);
    if (hr != 0){ 
        // Handle failure.
        AfxMessageBox(L"Failed to open file."); 
    }
    else
    {
        PAVIFILE         pf; 
        PAVISTREAM       psSmall; 
        HRESULT          hr; 
        AVISTREAMINFO    strhdr; 
        BITMAPINFOHEADER bi; 
        BITMAPINFOHEADER biNew; 
        LONG             lStreamSize; 
        LPVOID           lpOld; 
        LPVOID           lpNew; 

        // Determine the size of the format data using 
        // AVIStreamFormatSize. 
        AVIStreamFormatSize(m_pAviStream, 0, &lStreamSize); 
        if (lStreamSize > sizeof(bi)) // Format too large? 
            return; 

        lStreamSize = sizeof(bi); 
        hr = AVIStreamReadFormat(m_pAviStream, 0, &bi, &lStreamSize); // Read format 
        if (bi.biCompression != BI_RGB) // Wrong compression format? 
            return; 

        hr = AVIStreamInfo(m_pAviStream, &strhdr, sizeof(strhdr)); 

        // Create new AVI file using AVIFileOpen. 
        hr = AVIFileOpen(&pf, strFilePath + L".Processed.avi", OF_WRITE | OF_CREATE, NULL); 
        if (hr != 0) 
            return; 

        // Set parameters for the new stream. 
        biNew = bi; 

        SetRect(&strhdr.rcFrame, 0, 0, (int) biNew.biWidth, 
            (int) biNew.biHeight); 

        // Create a stream using AVIFileCreateStream. 
        hr = AVIFileCreateStream(pf, &psSmall, &strhdr); 
        if (hr != 0) {            //Stream created OK? If not, close file. 
            AVIFileRelease(pf); 
            return; 
        } 

        // Set format of new stream using AVIStreamSetFormat. 
        hr = AVIStreamSetFormat(psSmall, 0, &biNew, sizeof(biNew)); 
        if (hr != 0) { 
            AVIStreamRelease(psSmall); 
            AVIFileRelease(pf); 
            return; 
        } 

        // Allocate memory for the bitmaps. 
        lpOld = malloc(bi.biSizeImage); 

        // Read the stream data using AVIStreamRead. 
        for (lStreamSize = AVIStreamStart(m_pAviStream); lStreamSize <
            AVIStreamEnd(m_pAviStream)/*1500*/; lStreamSize++) { 
                //Context::Oversubscribe(true);
                hr = AVIStreamRead(m_pAviStream, lStreamSize, 1, lpOld, bi.biSizeImage,
                    NULL, NULL); 
                //Context::Oversubscribe(false);
                //memcpy_s(lpNew, bi.biSizeImage, lpOld, bi.biSizeImage);
                data.m_BBP = bi.biBitCount;
                data.m_ColorPlanes = bi.biPlanes;
                data.m_EndHeight = bi.biHeight;
                data.m_EndWidth = bi.biWidth;
                data.m_pFrame = (BYTE*)lpOld;
                data.m_Pitch = bi.biWidth * (bi.biBitCount / 8);
                data.m_Size = bi.biSizeImage;
                data.m_StartHeight = 0;
                data.m_StartWidth = 0;
                lpNew = m_pVideoAgent->ProcessFrame(data);
               

                if(NULL != lpNew)
                {
                    // Save the compressed data using AVIStreamWrite.
                    hr = AVIStreamWrite(psSmall, lStreamSize, 1, lpNew,
                    biNew.biSizeImage, AVIIF_KEYFRAME, NULL, NULL);
                }
        } 
        free(lpOld);
        // Close the stream and file. 
        AVIStreamRelease(psSmall); 
        AVIFileRelease(pf); 
    }
    AVIFileExit();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void VideoReader::Open(CString strFilePath)
{    
    AVIFileInit();

    LONG hr;  
    hr = AVIStreamOpenFromFile(&m_pAviStream, strFilePath, streamtypeVIDEO, 0, OF_READ, NULL);
    if (hr != 0){ 
        // Handle failure.
        AfxMessageBox(L"Failed to open file, file must be an uncompressed video."); 
    }
    else
    {
        HRESULT          hr; 
        AVISTREAMINFO    strhdr; 
        LONG             lStreamSize; 
 

        // Determine the size of the format data using 
        // AVIStreamFormatSize. 
        AVIStreamFormatSize(m_pAviStream, 0, &lStreamSize); 
        if (lStreamSize > sizeof(m_bi)) // Format too large? 
            return; 

        lStreamSize = sizeof(m_bi); 
        hr = AVIStreamReadFormat(m_pAviStream, 0, &m_bi, &lStreamSize); // Read format 
        if (m_bi.biCompression != BI_RGB) // Wrong compression format? 
            return; 

        hr = AVIStreamInfo(m_pAviStream, &strhdr, sizeof(strhdr)); 

        // Create new AVI file using AVIFileOpen. 
        hr = AVIFileOpen(&m_pf, strFilePath + L".Processed.avi", OF_WRITE | OF_CREATE, NULL); 
        if (hr != 0) 
            return; 

        m_currentSize = AVIStreamStart(m_pAviStream);

        // Allocate memory for the bitmaps. 
        m_lpBuffer = (BYTE *)malloc(m_bi.biSizeImage); 
    }

}

void VideoReader::Close()
{
    // Close the stream and file. 
    AVIFileRelease(m_pf); 
    AVIFileExit();

    if (m_lpBuffer != NULL)
    {
        free(m_lpBuffer);
    }
}

void VideoReader::ReadNextFrame(FrameData& data)
{
    
    // Read the stream data using AVIStreamRead. 
    if (m_currentSize < AVIStreamEnd(m_pAviStream)) 
    { 
            //Context::Oversubscribe(true);
            HRESULT hr = AVIStreamRead(m_pAviStream, m_currentSize, 1, (LPVOID)m_lpBuffer, m_bi.biSizeImage,
                NULL, NULL); 

            data.m_BBP = m_bi.biBitCount;
            data.m_ColorPlanes = m_bi.biPlanes;
            data.m_EndHeight = m_bi.biHeight;
            data.m_EndWidth = m_bi.biWidth;
            data.m_pFrame = m_lpBuffer;
            data.m_Pitch = m_bi.biWidth * (m_bi.biBitCount / 8);
            data.m_Size = m_bi.biSizeImage;
            data.m_StartHeight = 0;
            data.m_StartWidth = 0;
            m_pVideoAgent->ProcessFrame(data);
            ++m_currentSize;

    } 
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////

VideoAgent::VideoAgent(CString filePath, int nPhases, bool fParallel, bool fSimplePfor, ICartoonDlg* pDlg)
    :IVideoAgent(pDlg), m_Helper(this)
{
    m_strFilePath = filePath;
    m_fParallel = fParallel;
    m_fSimplePFor = fSimplePfor;
    m_nPhases = nPhases;
}


VideoAgent::~VideoAgent(void)
{
    
}

void VideoAgent::run()
{
    QueryPerformanceFrequency(&m_Frequency);
    QueryPerformanceCounter(&m_StartTime);
    QueryPerformanceCounter(&m_EndTime);
    m_Overhead = m_EndTime.QuadPart - m_StartTime.QuadPart;

    QueryPerformanceCounter(&m_StartTime);

    FrameData data;
    data.m_pCartoonAgent = this;
    data.m_fParallel = m_fParallel;
    data.m_fSimpleParallelFor = m_fSimplePFor;
    data.m_pFrameProcesser = m_pFrameProcessor;
    data.m_PhaseCount = m_nPhases;
    data.m_neighbourArea = ((CCartoonizerDlg*)m_pUIDlg)->m_NeighbourWindow;   
    m_pFrameProcessor->SetNeighbourArea(data.m_neighbourArea);
    data.m_pFrameProcesser->UseSimpleParallelFor(data.m_fSimpleParallelFor);
    m_Helper.OpenVideo(m_strFilePath, data);
    done();
}

BYTE* VideoAgent::ProcessFrame(FrameData& data)
{
    m_pFrameProcessor->SetCurrentFrame(data.m_pFrame, data.m_Size, data.m_EndWidth, data.m_EndHeight, data.m_Pitch, data.m_BBP, data.m_ColorPlanes);
    m_pFrameProcessor->ApplyFilters(m_nPhases, m_fParallel);
    BYTE* pRet = m_pFrameProcessor->GetOutputFrame();
    m_pFrameProcessor->FrameDone();

    QueryPerformanceCounter(&m_EndTime);
    double totalTime =  (m_EndTime.QuadPart - m_StartTime.QuadPart - m_Overhead) / (double)m_Frequency.QuadPart;
    m_TotalTime = totalTime;
    ++m_FrameCount;

    double FPS = m_TotalTime / m_FrameCount;
    m_pUIDlg->ReportPerf(FPS);
    m_pUIDlg->updateUI();
    return pRet;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VideoAgentOpenMP::VideoAgentOpenMP(CString filePath, int nPhases, ICartoonDlg* pDlg)
    :IVideoAgent(pDlg), m_Helper(this)
{
    m_strFilePath = filePath;
    m_nPhases = nPhases;
}


VideoAgentOpenMP::~VideoAgentOpenMP(void)
{
    
}

void VideoAgentOpenMP::run()
{
    QueryPerformanceFrequency(&m_Frequency);
    QueryPerformanceCounter(&m_StartTime);
    QueryPerformanceCounter(&m_EndTime);
    m_Overhead = m_EndTime.QuadPart - m_StartTime.QuadPart;

    QueryPerformanceCounter(&m_StartTime);

    FrameData data;
    data.m_pCartoonAgent = this;
    data.m_fParallel = false;
    data.m_pFrameProcesser = m_pFrameProcessor;
    data.m_PhaseCount = m_nPhases;
    data.m_neighbourArea = ((CCartoonizerDlg*)m_pUIDlg)->m_NeighbourWindow;
    m_pFrameProcessor->SetNeighbourArea(data.m_neighbourArea);
    m_Helper.OpenVideo(m_strFilePath, data);
    done();
}

BYTE* VideoAgentOpenMP::ProcessFrame(FrameData& data)
{
    m_pFrameProcessor->SetCurrentFrame(data.m_pFrame, data.m_Size, data.m_EndWidth, data.m_EndHeight, data.m_Pitch, data.m_BBP, data.m_ColorPlanes);
    data.m_pFrameProcesser = m_pFrameProcessor;
    data.m_pFrameProcesser->SetParallelOption(false);
    int shift = data.m_neighbourArea / 2;
    for(int i = 0 ; i < data.m_PhaseCount; ++i)
    {
        data.m_pFrameProcesser->ApplyColorSimplifierOpenMP(shift, data.m_EndHeight  - shift, shift, data.m_EndWidth - shift);
    }
    data.m_pFrameProcesser->ApplyEdgeDetectionOpenMP(shift, data.m_EndHeight  - shift, shift, data.m_EndWidth - shift);
    BYTE* pRet = m_pFrameProcessor->GetOutputFrame();
    m_pFrameProcessor->FrameDone();

     QueryPerformanceCounter(&m_EndTime);
    double totalTime =  (m_EndTime.QuadPart - m_StartTime.QuadPart - m_Overhead) / (double)m_Frequency.QuadPart;
    m_TotalTime = totalTime;
    ++m_FrameCount;

    double FPS = m_TotalTime / m_FrameCount;
    m_pUIDlg->ReportPerf(FPS);
    m_pUIDlg->updateUI();
    return pRet;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

VideoSingleFramePipelineAgent::VideoSingleFramePipelineAgent(CString filePath, int nPhases, ICartoonDlg* pDlg)
    :IVideoAgent(pDlg), m_Helper(this)
{
    m_strFilePath = filePath;
    m_fParallel = false;
    m_pFrameProcessor->SetParallelOption(m_fParallel);
    m_nPhases = nPhases;
    m_procCount = Concurrency::GetProcessorCount();

    m_pSrc = NULL;
    m_pDest = NULL;
    m_pNetwork = NULL;
    m_frameIndex = 0;
    m_Size = 0;
}


VideoSingleFramePipelineAgent::~VideoSingleFramePipelineAgent(void)
{
    if(NULL != m_pNetwork)
    {
        delete m_pNetwork;
    }

    if(NULL != m_pSrc)
    {
        Concurrency::Free(m_pSrc);
    }

    if(NULL != m_pDest)
    {
        Concurrency::Free(m_pDest);
    }
}

void VideoSingleFramePipelineAgent::run()
{
    QueryPerformanceFrequency(&m_Frequency);
    QueryPerformanceCounter(&m_StartTime);
    QueryPerformanceCounter(&m_EndTime);
    m_Overhead = m_EndTime.QuadPart - m_StartTime.QuadPart;

    QueryPerformanceCounter(&m_StartTime);

    FrameData data;
    data.m_pCartoonAgent = this;
    data.m_fParallel = m_fParallel;  
    data.m_PhaseCount = m_nPhases;
    data.m_neighbourArea = ((CCartoonizerDlg*)m_pUIDlg)->m_NeighbourWindow;
    m_pFrameProcessor->SetNeighbourArea(data.m_neighbourArea);
    m_Helper.OpenVideo(m_strFilePath, data);
    done();
}

BYTE* VideoSingleFramePipelineAgent::ProcessFrame(FrameData& data)
{
    BYTE* pRet = NULL;
    m_Size = data.m_Size;
    if(NULL == m_pNetwork)
    {
        m_pNetwork = new MultiplePipelineNetworkAgent(data.m_PhaseCount, m_procCount, data.m_EndHeight, data.m_neighbourArea / 2);
        m_pFrameProcessor->SetCurrentFrame(data.m_pFrame, data.m_Size, data.m_EndWidth, data.m_EndHeight, data.m_Pitch, data.m_BBP, data.m_ColorPlanes);
        m_pSrc = (BYTE*)Concurrency::Alloc(m_Size);
        m_pDest = (BYTE*)Concurrency::Alloc(m_Size);
    }
        
    data.m_pFrameProcesser = new FrameProcessing(this);
    data.m_pFrameProcesser->SetCurrentFrame(data.m_pFrame, data.m_Size, data.m_EndWidth, data.m_EndHeight, data.m_Pitch, data.m_BBP, data.m_ColorPlanes);
    data.m_pFrameProcesser->SetNeighbourArea(data.m_neighbourArea);
    data.m_pFrameProcesser->SetParallelOption(false);

    m_pNetwork->DoWork(data);
    m_pNetwork->WaitForNetWorkToFinish();

    QueryPerformanceCounter(&m_EndTime);
    double totalTime =  (m_EndTime.QuadPart - m_StartTime.QuadPart - m_Overhead) / (double)m_Frequency.QuadPart;
    m_TotalTime = totalTime;
    ++m_FrameCount;

    double FPS = m_TotalTime / m_FrameCount;
    m_pUIDlg->ReportPerf(FPS);
    return pRet;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


VideoMultiFramePipelineAgent::VideoMultiFramePipelineAgent(CString filePath, int nPhases, ICartoonDlg* pDlg)
    :IVideoAgent(pDlg), m_VideoReader(this)
{
    m_strFilePath = filePath;
    m_fParallel = false;
    m_pFrameProcessor->SetParallelOption(m_fParallel);
    m_nPhases = nPhases;
    m_procCount = Concurrency::GetProcessorCount();

    m_pSrc = NULL;
    m_pDest = NULL;
    m_pNetwork = NULL;
    m_frameIndex = 0;
    m_Size = 0;
    m_VideoReader.Open(m_strFilePath);
}


VideoMultiFramePipelineAgent::~VideoMultiFramePipelineAgent(void)
{
    if(NULL != m_pNetwork)
    {
        delete m_pNetwork;
    }

    if(NULL != m_pSrc)
    {
        Concurrency::Free(m_pSrc);
    }

    if(NULL != m_pDest)
    {
        Concurrency::Free(m_pDest);
    }
    
    m_VideoReader.Close();
}

void VideoMultiFramePipelineAgent::run()
{
    QueryPerformanceFrequency(&m_Frequency);
    QueryPerformanceCounter(&m_StartTime);
    QueryPerformanceCounter(&m_EndTime);
    m_Overhead = m_EndTime.QuadPart - m_StartTime.QuadPart;

    QueryPerformanceCounter(&m_StartTime);

    FrameData data;
    data.m_pCartoonAgent = this;
    data.m_fParallel = m_fParallel;
    data.m_PhaseCount = m_nPhases;
    data.m_pVideoReader = &m_VideoReader;
    data.m_neighbourArea = ((CCartoonizerDlg*)m_pUIDlg)->m_NeighbourWindow;
    m_pFrameProcessor->SetNeighbourArea(data.m_neighbourArea);

    for (int i = 0; i < 4 * m_nPhases; ++i)
    {
        m_VideoReader.ReadNextFrame(data);
    }

    done();
}

BYTE* VideoMultiFramePipelineAgent::ProcessFrame(FrameData& data)
{
    BYTE* pRet = NULL;
    m_Size = data.m_Size;
    if(NULL == m_pNetwork)
    {
        m_pNetwork = new MultiplePipelineNetworkAgent(data.m_PhaseCount, m_procCount, data.m_EndHeight, data.m_neighbourArea / 2);
        m_pFrameProcessor->SetCurrentFrame(data.m_pFrame, data.m_Size, data.m_EndWidth, data.m_EndHeight, data.m_Pitch, data.m_BBP, data.m_ColorPlanes);
        m_pSrc = (BYTE*)Concurrency::Alloc(m_Size);
        m_pDest = (BYTE*)Concurrency::Alloc(m_Size);
    }
       
    data.m_pFrameProcesser = new FrameProcessing(this);
    data.m_pFrameProcesser->SetCurrentFrame(data.m_pFrame, data.m_Size, data.m_EndWidth, data.m_EndHeight, data.m_Pitch, data.m_BBP, data.m_ColorPlanes);
    data.m_pFrameProcesser->SetNeighbourArea(data.m_neighbourArea);
    data.m_pFrameProcesser->SetParallelOption(false);
    m_pNetwork->DoWork(data);
    
    return pRet;
}

void VideoMultiFramePipelineAgent::FrameFinished(FrameData data)
{
    memcpy_s(m_pSrc, m_Size, data.m_pFrameProcesser->GetSourceFrame(), m_Size);
    memcpy_s(m_pDest, m_Size, data.m_pFrameProcesser->GetOutputFrame(), m_Size);
    m_pUIDlg->updateUI();
    QueryPerformanceCounter(&m_EndTime);
    double totalTime =  (m_EndTime.QuadPart - m_StartTime.QuadPart - m_Overhead) / (double)m_Frequency.QuadPart;
    m_TotalTime = totalTime;
    ++m_FrameCount;

    double FPS = m_TotalTime / m_FrameCount;
    m_pUIDlg->ReportPerf(FPS);
    delete data.m_pFrameProcesser;
}
