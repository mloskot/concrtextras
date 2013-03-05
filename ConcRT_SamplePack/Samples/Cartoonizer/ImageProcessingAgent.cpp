//--------------------------------------------------------------------------
// 
//  Copyright (c) Microsoft Corporation.  All rights reserved. 
// 
//  File: ImageProcessingAgent.cpp
//
//  Implementation of all Image agent classes
//
//--------------------------------------------------------------------------

#include "StdAfx.h"
#include "Cartoonizer.h"
#include "CartoonizerDlg.h"
#include "FrameProcessing.h"
#include "ImageProcessingAgent.h"



ImageAgentPFor::ImageAgentPFor(ISource<FrameData>& src, ICartoonDlg* pDlg)
    :CartoonAgentBase(pDlg), m_Source(src)
{
}


ImageAgentPFor::~ImageAgentPFor(void)
{
}

void ImageAgentPFor::run()
{
    FrameData data = receive(m_Source);
    data.m_pCartoonAgent = this;
    data.m_pFrameProcesser = m_pFrameProcessor;
    data.m_pFrameProcesser->UseSimpleParallelFor(data.m_fSimpleParallelFor);
    data.m_pFrameProcesser->SetFrameType(FrameProcessing::kStaticImage);
    m_pFrameProcessor->SetUpdateFlag(true);
    m_pFrameProcessor->SetNeighbourArea(data.m_neighbourArea);
    m_pFrameProcessor->SetCurrentFrame(data.m_pFrame, data.m_Size, data.m_EndWidth, data.m_EndHeight, data.m_Pitch, data.m_BBP, data.m_ColorPlanes);
    DoWork(data);
    done();
}

void ImageAgentPFor::DoWork(FrameData& data)
{
    // calculate perf measurment
    LARGE_INTEGER start;
    LARGE_INTEGER end;
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&start);
    QueryPerformanceCounter(&end);
    LONGLONG overhead = end.QuadPart - start.QuadPart;

    QueryPerformanceCounter(&start);

    data.m_pFrameProcesser->ApplyFilters(data.m_PhaseCount, data.m_fParallel);
    data.m_pFrameProcesser->FrameDone();

    QueryPerformanceCounter(&end);
    double totalTime =  (end.QuadPart - start.QuadPart - overhead) / (double)frequency.QuadPart;
    
    m_pUIDlg->ReportPerf(totalTime);
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ImageAgentOpenMP::ImageAgentOpenMP(ISource<FrameData>& src, ICartoonDlg* pDlg)
    :CartoonAgentBase(pDlg), m_Source(src)
{
}


ImageAgentOpenMP::~ImageAgentOpenMP(void)
{
}

void ImageAgentOpenMP::run()
{
    FrameData data = receive(m_Source);
    SetCurrentFrame(data.m_pFrame, data.m_Size, data.m_EndWidth, data.m_EndHeight, data.m_Pitch, data.m_BBP, data.m_ColorPlanes);
    data.m_pCartoonAgent = this;
    data.m_pFrameProcesser = m_pFrameProcessor;
    data.m_pFrameProcesser->SetFrameType(FrameProcessing::kStaticImage);
    data.m_pFrameProcesser->SetUpdateFlag(true);
    data.m_pFrameProcesser->SetNeighbourArea(data.m_neighbourArea);
    DoWork(data);
    done();
}

void ImageAgentOpenMP::DoWork(FrameData& data)
{
    // calculate perf measurment
    LARGE_INTEGER start;
    LARGE_INTEGER end;
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&start);
    QueryPerformanceCounter(&end);
    LONGLONG overhead = end.QuadPart - start.QuadPart;

    QueryPerformanceCounter(&start);

    data.m_pFrameProcesser->SetParallelOption(false);
    int shift = data.m_neighbourArea / 2;
    for(int i = 0 ; i < data.m_PhaseCount; ++i)
    {
        data.m_pFrameProcesser->ApplyColorSimplifierOpenMP(shift, data.m_EndHeight  - shift, shift, data.m_EndWidth - shift);
    }
    data.m_pFrameProcesser->ApplyEdgeDetectionOpenMP(shift, data.m_EndHeight  - shift, shift, data.m_EndWidth - shift);
    data.m_pFrameProcesser->FrameDone();

    QueryPerformanceCounter(&end);
    double totalTime =  (end.QuadPart - start.QuadPart - overhead) / (double)frequency.QuadPart;
    
    m_pUIDlg->ReportPerf(totalTime);
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

PipelineNetwork::PipelineNetwork()
{
}

PipelineNetwork::PipelineNetwork(int nPhases)
    :m_phaseCount(nPhases)
{
    InitializeNetwork(nPhases);
}

PipelineNetwork::~PipelineNetwork()
{ 
    if(true == m_fNetworkInitialized)
    {
        delete m_edgeDetection;
        for (int count = 0; count < m_phaseCount; ++count)
        {
            delete m_colorSimplifier[count];
        }
        delete[] m_colorSimplifier;
    }
}

void PipelineNetwork::InitializeNetwork(int nphases)
{
    m_phaseCount = nphases;
    m_fNetworkInitialized = true;
    m_RecievedMsgs = 0;
    m_ProcessedMsgs = 0;
    m_fAllMsgsRecieved = false;

    m_colorSimplifier = new transformer<FrameData, FrameData>* [m_phaseCount];
    for (int count = 0; count < m_phaseCount; ++count)
    {
        m_colorSimplifier[count] = new transformer<FrameData, FrameData>([](FrameData const& data) -> FrameData
        {            
            data.m_pFrameProcesser->ApplyColorSimplifier(data.m_StartHeight, data.m_EndHeight, data.m_StartWidth, data.m_EndWidth);
            return data;
        });

        if (count > 0)
        {
            m_colorSimplifier[count-1]->link_target(m_colorSimplifier[count]);           
        }
    }

    m_edgeDetection = new transformer<FrameData, FrameData>([this](FrameData const& oldData) -> FrameData
    {
        FrameData data = oldData;
        if (data.m_final)
        {
            data.m_StartHeight -= 1;
        }
        else
        {
            data.m_StartHeight -= 1;
            data.m_EndHeight -= 1;
        }

        if (data.m_StartHeight <= 0) 
        {
            data.m_StartHeight = 1;
        }

        if(data.m_fParallel == true)
        {
            data.m_pFrameProcesser->ApplyEdgeDetectionParallel(data.m_StartHeight, data.m_EndHeight, data.m_StartWidth, data.m_EndWidth);
        }
        else
        {
            data.m_pFrameProcesser->ApplyEdgeDetectionArea(data.m_StartHeight, data.m_EndHeight, data.m_StartWidth, data.m_EndWidth);
        }

        int processedMsgs = InterlockedIncrement(&m_ProcessedMsgs);
        data.m_pCartoonAgent->UpdateUI();
        return data;
    });
    m_colorSimplifier[m_phaseCount-1]->link_target(m_edgeDetection);
}

void PipelineNetwork::DoWork(FrameData& data)
{
    InterlockedIncrement(&m_RecievedMsgs);
     asend(m_colorSimplifier[0], data);
}

void PipelineNetwork::DoneSendingMsgs()
{
    InterlockedExchange(&m_fAllMsgsRecieved, true);
}

void PipelineNetwork::WaitForNetWorkToFinish()
{
    while(true)
    {
        int processedMsgs = InterlockedCompareExchange(&m_ProcessedMsgs, m_ProcessedMsgs, 0);
        int recievedMsgs = InterlockedCompareExchange(&m_RecievedMsgs, m_RecievedMsgs, 0);
        bool fCheckForFinal = !!InterlockedCompareExchange(&m_fAllMsgsRecieved, m_fAllMsgsRecieved, false);
        if (fCheckForFinal)
        {
            if(processedMsgs == recievedMsgs)
            {
                return;
            }
            Context::Yield();
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

MultiplePipelineNetworkAgent::MultiplePipelineNetworkAgent()
{
}

MultiplePipelineNetworkAgent::MultiplePipelineNetworkAgent(int nPhases, int pipelines, unsigned int frameHeight, unsigned int shift)
    :m_phaseCount(nPhases),
    m_SentMsgs(pipelines),
    m_pipeLines(pipelines),
    m_step((frameHeight) / (pipelines) + 1)
{
    InitializeNetwork();
}

MultiplePipelineNetworkAgent::~MultiplePipelineNetworkAgent()
{ 
    if(true == m_fNetworkInitialized)
    {
        for(int i = 0; i < m_pipeLines; ++i)
        {
            for (int count = 0; count < m_phaseCount /*- 1*/; ++count)
            {
                delete m_colorSimplifier[i][count];
            }
            delete[] m_colorSimplifier[i];
        }

        delete[] m_colorSimplifier;    
        delete m_edgeDetectionJoin;
        delete m_edgeDetection;
        delete m_reader;
    }
}

void MultiplePipelineNetworkAgent::InitializeNetwork()
{
    m_fNetworkInitialized = true;
    m_ProcessedMsgs = 0;
    m_fAllMsgsRecieved = false;

    m_colorSimplifier = new transformer<FrameData, FrameData>** [m_pipeLines];
    
    m_edgeDetectionJoin = new join<FrameData>(m_pipeLines);
    m_edgeDetection = new transformer<vector<FrameData>, FrameData>([&](vector<FrameData> const& arrData) -> FrameData
    {
        if(arrData[1].m_EndHeight != 0)
        {
            arrData[1].m_pFrameProcesser->SetParallelOption(true);
            arrData[1].m_pFrameProcesser->ApplyEdgeDetection();
            arrData[1].m_pFrameProcesser->FrameDone();
            arrData[1].m_pCartoonAgent->FrameFinished(arrData[0]);
            m_Finished = true;
        }
        return arrData[1];
    });


    for(int i = 0; i < m_pipeLines; ++i)
    {
        m_colorSimplifier[i] = new transformer<FrameData, FrameData>* [m_phaseCount];

        for (int count = 0; count < m_phaseCount; ++count)
        {
            m_colorSimplifier[i][count] = new transformer<FrameData, FrameData>([](FrameData const& data) -> FrameData
            {
                data.m_pFrameProcesser->ApplyColorSimplifier(data.m_StartHeight, data.m_EndHeight, data.m_StartWidth, data.m_EndWidth);
                return data;
            });

            if (count > 0)
            {
                m_colorSimplifier[i][count-1]->link_target(m_colorSimplifier[i][count]);           
            }
        }
        m_colorSimplifier[i][m_phaseCount-1]->link_target(m_edgeDetectionJoin);
    }
    m_edgeDetectionJoin->link_target(m_edgeDetection);

    m_reader = new call<FrameData>([](const FrameData & data)
    {
        FrameData newData = data;
        if(NULL != data.m_pVideoReader)
        {
            data.m_pVideoReader->ReadNextFrame(newData);
        }
    });

    m_edgeDetection->link_target(m_reader);
}

void MultiplePipelineNetworkAgent::DoWork(FrameData& data)
{
    m_Finished = false;
    unsigned int shift = data.m_neighbourArea / 2;
    int count = 0;
    m_ProcessedMsgs = 0;
    int index = 0;
    for (unsigned int h = shift; h < (data.m_EndHeight - shift); h += m_step, ++count)
    {
        FrameData localData     = data;
        localData.m_StartHeight = h;
        localData.m_EndHeight   = min(h + m_step, (data.m_EndHeight - shift)) ;
        localData.m_StartWidth  = shift;
        localData.m_EndWidth    = data.m_EndWidth - shift;
        localData.m_final       = (localData.m_EndHeight == (data.m_EndHeight - shift));

        index = count % m_pipeLines;
        asend(m_colorSimplifier[index][0], localData);
    }

    int diff = m_pipeLines - (index + 1);
    for(int i = 0; i < diff; ++i)
    {
        FrameData dummy = data;
        dummy.m_StartHeight = 0;
        dummy.m_EndHeight = 0;
        dummy.m_StartWidth = 0;
        dummy.m_EndWidth = 0;
        dummy.m_Size = 0;
        asend(m_colorSimplifier[++index][0], dummy);
    }
}

void MultiplePipelineNetworkAgent::DoneSendingMsgs()
{
    InterlockedExchange(&m_fAllMsgsRecieved, true);
}

void MultiplePipelineNetworkAgent::WaitForNetWorkToFinish()
{
    while(!m_Finished)
    {
        Context::Yield();
    }

}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ImageAgentPipelineSingle::ImageAgentPipelineSingle(ISource<FrameData>& src, int nPhases, ICartoonDlg* pDlg)
    :CartoonAgentBase(pDlg), m_Source(src), m_Network(nPhases)
{
}


ImageAgentPipelineSingle::~ImageAgentPipelineSingle(void)
{
}

void ImageAgentPipelineSingle::run()
{
    FrameData data = receive(m_Source); 
    data.m_pCartoonAgent = this;
    m_pFrameProcessor->SetNeighbourArea(data.m_neighbourArea);
    data.m_pFrameProcesser = m_pFrameProcessor;
    m_pFrameProcessor->SetCurrentFrame(data.m_pFrame, data.m_Size, data.m_EndWidth, data.m_EndHeight, data.m_Pitch, data.m_BBP, data.m_ColorPlanes);
    data.m_pFrameProcesser->SetUpdateFlag(false);
    data.m_pFrameProcesser->SetParallelOption(false);
    DoWork(data);
    data.m_pFrameProcesser->SetUpdateFlag(true);
    done();
}


void ImageAgentPipelineSingle::DoWork(FrameData& data)
{
    // calculate perf measurment
    LARGE_INTEGER start;
    LARGE_INTEGER end;
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&start);
    QueryPerformanceCounter(&end);
    LONGLONG overhead = end.QuadPart - start.QuadPart;

    QueryPerformanceCounter(&start);

    unsigned int shift = data.m_neighbourArea / 2;
    unsigned int step = 16;
    for (unsigned int h = shift; h < (data.m_EndHeight - shift); h += step)
    {
        FrameData localData     = data;
        localData.m_StartHeight = h;
        localData.m_EndHeight   = min(h+step, (data.m_EndHeight - shift)) ;
        localData.m_StartWidth  = shift;
        localData.m_EndWidth    = data.m_EndWidth - shift;
        localData.m_final       = (localData.m_EndHeight == (data.m_EndHeight - shift));
        m_Network.DoWork(localData);
       
    }
    m_Network.DoneSendingMsgs();
    m_Network.WaitForNetWorkToFinish();
    data.m_pFrameProcesser->FrameDone();
    

    QueryPerformanceCounter(&end);
    double totalTime =  (end.QuadPart - start.QuadPart - overhead) / (double)frequency.QuadPart;
    
    m_pUIDlg->ReportPerf(totalTime);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ImageAgentPipelineMultiple::ImageAgentPipelineMultiple(ISource<FrameData>& src, int nPhases, int pipelines, ICartoonDlg* pDlg, unsigned int imageHeight, unsigned int shift)
    :CartoonAgentBase(pDlg), m_Source(src)
{
     m_pNetwork = new MultiplePipelineNetworkAgent(nPhases, pipelines, imageHeight, shift);
     m_fNetwrokCreated = true;
}

ImageAgentPipelineMultiple::ImageAgentPipelineMultiple(ISource<FrameData>& src, MultiplePipelineNetworkAgent* pNetwork, ICartoonDlg* pDlg)
    :CartoonAgentBase(pDlg), m_Source(src)
{
    m_pNetwork = pNetwork;
    m_fNetwrokCreated = false;
}

ImageAgentPipelineMultiple::~ImageAgentPipelineMultiple()
{
    if(m_fNetwrokCreated)
    {
        delete m_pNetwork;
    }
}

void ImageAgentPipelineMultiple::run()
{
    FrameData data = receive(m_Source); 
  
    if(NULL == data.m_pFrameProcesser)
    {
        data.m_pCartoonAgent = this;
        data.m_pFrameProcesser = m_pFrameProcessor;
        m_pFrameProcessor->SetNeighbourArea(data.m_neighbourArea);
        data.m_pFrameProcesser->SetUpdateFlag(false);
        m_pFrameProcessor->SetCurrentFrame(data.m_pFrame, data.m_Size, data.m_EndWidth, data.m_EndHeight, data.m_Pitch, data.m_BBP, data.m_ColorPlanes);
    }
        
    DoWork(data);
    done();
}

void ImageAgentPipelineMultiple::DoWork(FrameData& data)
{
    // calculate perf measurment
    LARGE_INTEGER start;
    LARGE_INTEGER end;
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&start);
    QueryPerformanceCounter(&end);
    LONGLONG overhead = end.QuadPart - start.QuadPart;

    QueryPerformanceCounter(&start);

    m_pNetwork->DoWork(data);
    m_pNetwork->DoneSendingMsgs();
    m_pNetwork->WaitForNetWorkToFinish();
    
    data.m_pFrameProcesser->FrameDone();

    QueryPerformanceCounter(&end);
    double totalTime =  (end.QuadPart - start.QuadPart - overhead) / (double)frequency.QuadPart;

    m_pUIDlg->updateUI();
    m_pUIDlg->ReportPerf(totalTime);
}