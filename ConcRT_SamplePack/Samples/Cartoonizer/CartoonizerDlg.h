//--------------------------------------------------------------------------
// 
//  Copyright (c) Microsoft Corporation.  All rights reserved. 
// 
//  File: CartoonizerDlg.h
//
//  Cartoonizer Dialog
//
//--------------------------------------------------------------------------

#pragma once
#include "afxwin.h"
#include "ppl.h"
#include <agents.h>
using namespace Concurrency;
#include "ImageProcessingAgent.h"
#include "VideoAgent.h"
#include "CartoonAgentBase.h"

// CCartoonizerDlg dialog
class CCartoonizerDlg : public CDialog, ICartoonDlg
{
// Construction
public:
    CCartoonizerDlg(CWnd* pParent = NULL);    // standard constructor
    ~CCartoonizerDlg();

// Dialog Data
    enum { IDD = IDD_CARTOONIZER_DIALOG };

    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support


// Implementation
protected:
    HICON m_hIcon;

    // Generated message map functions
    virtual BOOL OnInitDialog();
    afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
    afx_msg void OnPaint();
    afx_msg HCURSOR OnQueryDragIcon();
    DECLARE_MESSAGE_MAP()
public:
    afx_msg void OnBnClickedButton2();
    CString m_strFilePath;
    afx_msg void OnBnClickedButton1();
    afx_msg void OnBnClickedOk();
    void ReportPerf(double time);
    void updateUI()
    {
        Invalidate(false);
    }

    void ResetPerf()
    {
        if(m_fImageType)
        {
            m_strPerf = L"Serial Time = N/A";
            m_strParallelPerf = L"Paralle Time = N/A";
            m_strSpeedup = L"Speedup = N/A";
        }
    }

private:
    void ProcessImagePFor();
    void ProcessImageOpenMP();
    void ProcessImageSinglePipeline();
    void ProcessImageMultiplePipeline();
    void ProcessVideoPFor();
    void ProcessVideoOpenMP();
    void ProcessVideoSingleFramePipeline();
    void ProcessVideoMultiFramePipeline();
    void OpenImage(CString filePath);

private:
    enum ProcessingType
    {
        kSerial = 0,
        kPallel_for = 1,
        kSinglePipeline = 2,
        kMultiplePipeline = 3,
        kSimpleParallel_for = 4,
        kOpenMP = 5
    };

    CString m_strFileName;
    BYTE* m_pInitialImage;
    FrameData m_InitialImageData;
    int m_PhasesCount;
    unbounded_buffer<FrameData> m_bufferFrames;
    CartoonAgentBase* m_pProcessingAgent;
public:
    ProcessingType m_enProcessingType;
    BOOL m_fImageType;
    CString m_strPerf;
    int m_StartTime;
    int m_EndTime;
    double m_SerialTime;
    double m_ParallelTime;
    CStatic m_PerfStatic;
    CFont m_BoldFont;
    CStatic m_ParallelPerf;
    CStatic m_Speedup;
    CString m_strParallelPerf;
    CString m_strSpeedup;
    int m_NeighbourWindow;
    int m_CoreCount;
};
