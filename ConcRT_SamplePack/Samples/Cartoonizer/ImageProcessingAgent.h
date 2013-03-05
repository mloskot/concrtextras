//--------------------------------------------------------------------------
// 
//  Copyright (c) Microsoft Corporation.  All rights reserved. 
// 
//  File: ImageProcessingAgent.h
//
//  Implementation of all image agent classe
//
//--------------------------------------------------------------------------
#pragma once
#include "FrameProcessing.h"
#include <agents.h>
using namespace Concurrency;
#include "vector"
using namespace std;


class ImageAgentPFor : public CartoonAgentBase
{
public:
    ImageAgentPFor(ISource<FrameData>& src, ICartoonDlg* pDlg);
    ~ImageAgentPFor(void);
    void run();
    void DoWork(FrameData& data);
private:
    ISource<FrameData>& m_Source;
};

///////////////////////////////////////////////////////////////////////
class ImageAgentOpenMP : public CartoonAgentBase
{
public:
    ImageAgentOpenMP(ISource<FrameData>& src, ICartoonDlg* pDlg);
    ~ImageAgentOpenMP(void);
    void run();
    void DoWork(FrameData& data);
private:
    ISource<FrameData>& m_Source;
};



///////////////////////////////////////////////////////////////
class PipelineNetwork
{
public:
    PipelineNetwork();
    PipelineNetwork(int nPhases);
    ~PipelineNetwork();

    void InitializeNetwork(int nPhases);
    void DoWork(FrameData& data);
    void DoneSendingMsgs();
    void WaitForNetWorkToFinish();
private:
    int m_phaseCount;
    volatile LONG m_RecievedMsgs;
    volatile LONG m_ProcessedMsgs;
    volatile LONG m_fAllMsgsRecieved;
    bool m_fNetworkInitialized;
    transformer<FrameData, FrameData> ** m_colorSimplifier;
    transformer<FrameData, FrameData> * m_edgeDetection;
};

///////////////////////////////////////////////////////////////////////////
class MultiplePipelineNetworkAgent
{
public:
    MultiplePipelineNetworkAgent();
    MultiplePipelineNetworkAgent(int nPhases, int pipelines, unsigned int frameHeight, unsigned int shift);
    ~MultiplePipelineNetworkAgent();

    void InitializeNetwork();
    void DoWork(FrameData& data);
    void DoneSendingMsgs();
    void WaitForNetWorkToFinish();
private:
    int m_pipeLines;
    int m_phaseCount;
    int m_SentMsgs;
    volatile LONG m_ProcessedMsgs;
    volatile LONG m_fAllMsgsRecieved;
    bool m_fNetworkInitialized;
    transformer<FrameData, FrameData> *** m_colorSimplifier;
    join<FrameData>* m_edgeDetectionJoin;
    transformer<vector<FrameData>, FrameData> *m_edgeDetection;
    call<FrameData> * m_reader;
    bool m_Finished;
    int m_step;
};

///////////////////////////////////////////////////////////////////////
class ImageAgentPipelineSingle : public CartoonAgentBase
{
public:
    ImageAgentPipelineSingle(ISource<FrameData>& src, int nPhases, ICartoonDlg* pDlg);
    ~ImageAgentPipelineSingle(void);
    void run();
    void DoWork(FrameData& data);
private:
    ISource<FrameData>& m_Source;
    PipelineNetwork m_Network;
};

/////////////////////////////////////////////////////////////////////////////////////
class ImageAgentPipelineMultiple : public CartoonAgentBase
{
public:
    ImageAgentPipelineMultiple(ISource<FrameData>& src, int nPhases, int pipelines, ICartoonDlg* pDlg, unsigned int imageHeight, unsigned int shift);
    ImageAgentPipelineMultiple(ISource<FrameData>& src, MultiplePipelineNetworkAgent* pNetwork, ICartoonDlg* pDlg);
    ~ImageAgentPipelineMultiple(void);
    void run();
    void DoWork(FrameData& data);

private:
    ISource<FrameData>& m_Source;
    MultiplePipelineNetworkAgent* m_pNetwork;
    bool m_fNetwrokCreated;
};
