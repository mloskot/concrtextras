//--------------------------------------------------------------------------
// 
//  Copyright (c) Microsoft Corporation.  All rights reserved. 
// 
//  File: CartoonizerDlg.cpp
//
//  Cartoonizer Dialog Implementation
//
//--------------------------------------------------------------------------

#include "stdafx.h"
#include "Cartoonizer.h"
#include "CartoonizerDlg.h"
#include "ImageProcessingAgent.h"
#include <math.h>
#include <omp.h>
#include "Windows.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
    CAboutDlg();

// Dialog Data
    enum { IDD = IDD_ABOUTBOX };

    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
    DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()


// CCartoonizerDlg dialog




CCartoonizerDlg::CCartoonizerDlg(CWnd* pParent /*=NULL*/)
    : CDialog(CCartoonizerDlg::IDD, pParent)
    , m_strFilePath(_T(""))
    , m_strPerf(_T(""))
    , m_fImageType(TRUE)
{
    m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
    m_StartTime = 0;
    m_EndTime = 0;
    m_SerialTime = -1;
    m_ParallelTime = -1;
    m_pProcessingAgent = NULL;
    m_strParallelPerf = _T("");
    m_strSpeedup = _T("");
    m_enProcessingType = kSerial;
    m_NeighbourWindow = 3;
    m_PhasesCount = 3;
    m_CoreCount = Concurrency::GetProcessorCount();
    m_pInitialImage = NULL;
}

CCartoonizerDlg::~CCartoonizerDlg()
{
}

void CCartoonizerDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Text(pDX, IDC_EDIT1, m_strFilePath);
    DDX_Radio(pDX, IDC_RADIO1, (int&)m_enProcessingType);
    DDX_Radio(pDX, IDC_RADIO3, (int)m_fImageType);
    DDX_Text(pDX, IDC_STATIC_PERF, m_strPerf);
    DDX_Control(pDX, IDC_STATIC_PERF, m_PerfStatic);
    DDX_Control(pDX, IDC_STATIC_PERF2, m_ParallelPerf);
    DDX_Control(pDX, IDC_STATIC_PERF3, m_Speedup);
    DDX_Text(pDX, IDC_STATIC_PERF2, m_strParallelPerf);
    DDX_Text(pDX, IDC_STATIC_PERF3, m_strSpeedup);
    DDX_Text(pDX, IDC_EDIT3, m_NeighbourWindow);
    DDX_Text(pDX, IDC_EDIT2, m_PhasesCount);
    DDX_Text(pDX, IDC_EDIT4, m_CoreCount);
}

BEGIN_MESSAGE_MAP(CCartoonizerDlg, CDialog)
    ON_WM_SYSCOMMAND()
    ON_WM_PAINT()
    ON_WM_QUERYDRAGICON()
    ON_BN_CLICKED(IDC_BUTTON2, &CCartoonizerDlg::OnBnClickedButton2)
    ON_BN_CLICKED(IDC_BUTTON1, &CCartoonizerDlg::OnBnClickedButton1)
    ON_BN_CLICKED(IDOK, &CCartoonizerDlg::OnBnClickedOk)
END_MESSAGE_MAP()


// CCartoonizerDlg message handlers

BOOL CCartoonizerDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    // Add "About..." menu item to system menu.

    // IDM_ABOUTBOX must be in the system command range.
    ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
    ASSERT(IDM_ABOUTBOX < 0xF000);

    CMenu* pSysMenu = GetSystemMenu(FALSE);
    if (pSysMenu != NULL)
    {
        BOOL bNameValid;
        CString strAboutMenu;
        bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
        ASSERT(bNameValid);
        if (!strAboutMenu.IsEmpty())
        {
            pSysMenu->AppendMenu(MF_SEPARATOR);
            pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
        }
    } 

    // Set the icon for this dialog.  The framework does this automatically
    //  when the application's main window is not a dialog
    SetIcon(m_hIcon, TRUE);            // Set big icon
    SetIcon(m_hIcon, FALSE);        // Set small icon

    // TODO: Add extra initialization here

    CFont* pFont = m_PerfStatic.GetFont();
    LOGFONT lf;
    pFont->GetLogFont(&lf);
    lf.lfWeight = FW_BOLD;
    m_BoldFont.CreateFontIndirect(&lf);
    m_PerfStatic.SetFont(&m_BoldFont);
    m_ParallelPerf.SetFont(&m_BoldFont);
    m_Speedup.SetFont(&m_BoldFont);

    ResetPerf();

    UpdateData(false);
    SchedulerType enType = ThreadScheduler;
    bool fSeMinMax = false;
    //Concurrency::EnableTracing();
    TCHAR strNumCores[256];
    if(0 != GetEnvironmentVariable(L"NUM_CORES", strNumCores, 256))
    {
        m_CoreCount = _tstoi(strNumCores);
    }

    TCHAR strSchedulerType[256];
    if(0 != GetEnvironmentVariable(L"SchedulerKind", strSchedulerType, 256))
    {
       enType = (SchedulerType)_tstoi(strSchedulerType);
    }

    if(ThreadScheduler == enType)
    {
        m_strPerf.Format(L"%d Scheduler is ThreadScheduler", enType);
    }
    else
    {
        m_strPerf.Format(L"%d Scheduler is UMS", enType);
    }

    TCHAR strSetMinMax[256];
    if(0 != GetEnvironmentVariable(L"SetMinMax", strSetMinMax, 256))
    {
       fSeMinMax = !!_tstoi(strSetMinMax);
    }

    m_fImageType = false;
    m_enProcessingType = kMultiplePipeline;

    UpdateData(false);

    if(fSeMinMax)
    {
        Concurrency::Scheduler::SetDefaultSchedulerPolicy(SchedulerPolicy(3, SchedulerKind, enType, MinConcurrency, m_CoreCount, MaxConcurrency, m_CoreCount));
        m_strSpeedup = L"Min/Max is Set";
    }
    else
    {
        Concurrency::Scheduler::SetDefaultSchedulerPolicy(SchedulerPolicy(1, SchedulerKind, enType));
    }

    return TRUE;  // return TRUE  unless you set the focus to a control
}

void CCartoonizerDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
    if ((nID & 0xFFF0) == IDM_ABOUTBOX)
    {
        CAboutDlg dlgAbout;
        dlgAbout.DoModal();
    }
    else
    {
        CDialog::OnSysCommand(nID, lParam);
    }
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CCartoonizerDlg::OnPaint()
{
    if (IsIconic())
    {
        CPaintDC dc(this); // device context for painting

        SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

        // Center icon in client rectangle
        int cxIcon = GetSystemMetrics(SM_CXICON);
        int cyIcon = GetSystemMetrics(SM_CYICON);
        CRect rect;
        GetClientRect(&rect);
        int x = (rect.Width() - cxIcon + 1) / 2;
        int y = (rect.Height() - cyIcon + 1) / 2;

        // Draw the icon
        dc.DrawIcon(x, y, m_hIcon);
    }
    else
    {
        LARGE_INTEGER start;
        LARGE_INTEGER end;
        LARGE_INTEGER frequency;
        QueryPerformanceFrequency(&frequency);
        QueryPerformanceCounter(&start);
        QueryPerformanceCounter(&end);
        LONGLONG overhead = end.QuadPart - start.QuadPart;

        QueryPerformanceCounter(&start);

        CPaintDC dc(this); 
        RECT windowRect;
        this->GetWindowRect(&windowRect);
        int rectWidth = windowRect.right - windowRect.left;

        int newWidth = 600;
        int newHeight = 450;
        SetStretchBltMode(dc, HALFTONE);

        int size = 0;
        unsigned int width = 0;
        unsigned int height = 0;
        int bpp = 0;
        int pitch = 0;
        int clrPlanes = 0;
        BYTE* pSrcFrame = NULL;
        BYTE* pOutFrame = NULL;
        if(NULL!= m_pProcessingAgent)
        {
            m_pProcessingAgent->GetFrame(pSrcFrame, pOutFrame, size, height, width, bpp, pitch, clrPlanes);
        }
        else if(m_pInitialImage != NULL)
        {
            pSrcFrame = m_pInitialImage;
            pOutFrame = m_pInitialImage;
            size = m_InitialImageData.m_Size;
            width = m_InitialImageData.m_EndWidth;
            height = m_InitialImageData.m_EndHeight;
            bpp = m_InitialImageData.m_BBP;
            pitch = m_InitialImageData.m_Pitch;
            clrPlanes = m_InitialImageData.m_ColorPlanes;
        }

        if(NULL != pSrcFrame)
        {
            CImage src;
            src.Create(width, height, bpp);
            memcpy_s((BYTE*)src.GetBits() - ((height - 1) * abs((int)pitch)), size, pSrcFrame, size);
            src.StretchBlt(dc, 10, 100, newWidth, newHeight, 0, 0,  width, height);
        }

        if(NULL != pOutFrame)
        {
            CImage dest;
            dest.Create(width, height, bpp);
            memcpy_s((BYTE*)dest.GetBits() - ((height - 1) * abs((int)pitch)), size, pOutFrame, size);
            dest.StretchBlt(dc, rectWidth / 2, 100, newWidth, newHeight, 0, 0,  width, height);
        }
        QueryPerformanceCounter(&end);
        double totalTime =  (end.QuadPart - start.QuadPart - overhead) / (double)frequency.QuadPart;
        UpdateData(false);
        CDialog::OnPaint();
    }
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CCartoonizerDlg::OnQueryDragIcon()
{
    return static_cast<HCURSOR>(m_hIcon);
}

void CCartoonizerDlg::OpenImage(CString filePath)
{
    CImage src;

    if(S_OK == src.Load(m_strFilePath))
    {
        BYTE* pFrame = (BYTE*)src.GetBits();
        if(src.GetPitch() < 0)
        {
            pFrame -= ((src.GetHeight() - 1) * abs((int)src.GetPitch()));
        }
        int size = src.GetHeight() * src.GetWidth() * (src.GetBPP() / 8);
        if(NULL!= m_pInitialImage)
        {
            free(m_pInitialImage);
        }
        m_pInitialImage = (BYTE*)malloc(size);
        memcpy_s(m_pInitialImage, size, pFrame, size);
        m_InitialImageData.m_BBP = src.GetBPP();
        m_InitialImageData.m_ColorPlanes = 1;
        m_InitialImageData.m_EndHeight = src.GetHeight();
        m_InitialImageData.m_EndWidth = src.GetWidth();
        m_InitialImageData.m_final = true;
        m_InitialImageData.m_fParallel = false;
        m_InitialImageData.m_neighbourArea = m_NeighbourWindow;
        m_InitialImageData.m_pFrame = m_pInitialImage;
        m_InitialImageData.m_pFrameProcesser = NULL;
        m_InitialImageData.m_PhaseCount = m_PhasesCount;
        m_InitialImageData.m_Pitch = src.GetPitch();
        m_InitialImageData.m_Size = size;
        m_InitialImageData.m_StartHeight = 0;
        m_InitialImageData.m_StartWidth = 0;

        Invalidate(false);
    }
    else
    {
        MessageBox(L"Failed to load the image");
    }
}

void CCartoonizerDlg::OnBnClickedButton2()
{
    if(NULL != m_pProcessingAgent)
    {
        agent::wait((agent*)m_pProcessingAgent);
        delete m_pProcessingAgent;
        m_pProcessingAgent = NULL;
    }

    UpdateData();
    CFileDialog fileDlg(true);
    if(IDOK == fileDlg.DoModal())
    {
        m_strFileName = fileDlg.GetFileName();
        m_strFilePath = fileDlg.GetFolderPath() + CString("\\") + m_strFileName;
        UpdateData(false);
        if(m_fImageType)
        {
            OpenImage(m_strFilePath);
        }
        else
        {
            if(NULL!= m_pInitialImage)
            {
                free(m_pInitialImage);
            }
        }
    }
    m_SerialTime = -1;
    m_ParallelTime = -1;
    ResetPerf();
    UpdateData(false);
}



void CCartoonizerDlg::ProcessImagePFor()
{
    FrameData data = m_InitialImageData;

    data.m_neighbourArea = m_NeighbourWindow;
    data.m_PhaseCount = m_PhasesCount;
    data.m_fParallel = kPallel_for == m_enProcessingType;
    data.m_fSimpleParallelFor = kSimpleParallel_for == m_enProcessingType;
    if(NULL != data.m_pFrame)
    {
        m_pProcessingAgent = new ImageAgentPFor(m_bufferFrames, this);
        ((agent*)m_pProcessingAgent)->start();
        send(&m_bufferFrames, data);    
    }
}

void CCartoonizerDlg::ProcessImageOpenMP()
{
    FrameData data = m_InitialImageData;

    data.m_neighbourArea = m_NeighbourWindow;
    data.m_PhaseCount = m_PhasesCount;
    data.m_fParallel = kPallel_for == m_enProcessingType;
    if(NULL != data.m_pFrame)
    {
        m_pProcessingAgent = new ImageAgentOpenMP(m_bufferFrames, this);
        ((agent*)m_pProcessingAgent)->start();
        send(&m_bufferFrames, data);    
    }
}

void CCartoonizerDlg::ProcessImageSinglePipeline()
{
    FrameData data = m_InitialImageData;
    data.m_neighbourArea = m_NeighbourWindow;

    data.m_PhaseCount = m_PhasesCount;
    data.m_fParallel = false;
    if(NULL != data.m_pFrame)
    {
        m_pProcessingAgent = new ImageAgentPipelineSingle(m_bufferFrames, m_PhasesCount, this);
        ((agent*)m_pProcessingAgent)->start();
        send(&m_bufferFrames, data);    
    }
}

void CCartoonizerDlg::ProcessImageMultiplePipeline()
{
    FrameData data = m_InitialImageData;
    data.m_neighbourArea = m_NeighbourWindow;

    data.m_PhaseCount = m_PhasesCount;
    data.m_fParallel = false;
    if(NULL != data.m_pFrame)
    {
        m_pProcessingAgent = new ImageAgentPipelineMultiple(m_bufferFrames, m_PhasesCount, m_CoreCount, this, data.m_EndHeight, data.m_neighbourArea / 2);
        ((agent*)m_pProcessingAgent)->start();
        send(&m_bufferFrames, data);    
    }
}

void CCartoonizerDlg::ProcessVideoPFor()
{    
    if(!m_strFilePath.IsEmpty())
    {
        m_pProcessingAgent = new VideoAgent(m_strFilePath, m_PhasesCount, kPallel_for == m_enProcessingType, kSimpleParallel_for == m_enProcessingType, this);
        ((agent*)m_pProcessingAgent)->start();
    }
}

void CCartoonizerDlg::ProcessVideoOpenMP()
{    
    if(!m_strFilePath.IsEmpty())
    {
        m_pProcessingAgent = new VideoAgentOpenMP(m_strFilePath, m_PhasesCount, this);
        ((agent*)m_pProcessingAgent)->start();
    }
}

void CCartoonizerDlg::ProcessVideoSingleFramePipeline()
{    
    if(!m_strFilePath.IsEmpty())
    {        
        m_pProcessingAgent = new VideoSingleFramePipelineAgent(m_strFilePath, m_PhasesCount, this);
        ((agent*)m_pProcessingAgent)->start();
    }
}

void CCartoonizerDlg::ProcessVideoMultiFramePipeline()
{    
    if(!m_strFilePath.IsEmpty())
    {        
        m_pProcessingAgent = new VideoMultiFramePipelineAgent(m_strFilePath, m_PhasesCount, this);
        ((agent*)m_pProcessingAgent)->start();
    }
}

void CCartoonizerDlg::OnBnClickedButton1()
{
    UpdateData(true);
    
    if(NULL != m_pProcessingAgent)
    {
        agent::wait((agent*)m_pProcessingAgent);
        delete m_pProcessingAgent;
        m_pProcessingAgent = NULL;
    }

    if(m_fImageType)
    {
        OpenImage(m_strFilePath);
        switch(m_enProcessingType)
        {
        case kSerial:
        case kPallel_for:
        case kSimpleParallel_for:
            ProcessImagePFor();
            break;
        case kSinglePipeline:
            ProcessImageSinglePipeline();
            break;
        case kMultiplePipeline:
            ProcessImageMultiplePipeline();
            break;
        case kOpenMP:
            ProcessImageOpenMP();
            break;
        }
    }
    else
    {
        switch(m_enProcessingType)
        {
        case kSerial:
        case kPallel_for:
        case kSimpleParallel_for:
            ProcessVideoPFor();
            break;
        case kSinglePipeline:
            ProcessVideoSingleFramePipeline();
            break;
        case kMultiplePipeline:
            ProcessVideoMultiFramePipeline();
            break;
        case kOpenMP:
            ProcessVideoOpenMP();
            break;
        }
    }
}


void CCartoonizerDlg::OnBnClickedOk()
{
    CDialog::OnOK();
}


void CCartoonizerDlg::ReportPerf(double time)
{
    if(m_fImageType)
    {
        if(kSerial != m_enProcessingType)
        {
            m_ParallelTime = time;
            m_strParallelPerf.Format(L"Parallel Time = %f sec", m_ParallelTime);
            if(-1 != m_SerialTime)
            {
                double speedup = m_SerialTime / m_ParallelTime;
                m_strSpeedup.Format(L"Speedup = %fX", speedup);
            }
        }
        else
        {
            m_SerialTime = time;
            m_strPerf.Format( L"Serial Time = %f sec",m_SerialTime);
            if(-1 != m_ParallelTime)
            {
                double speedup = m_SerialTime / m_ParallelTime;
                m_strSpeedup.Format(L"Speedup = %fX", speedup);
            }
        }
    }
    else
    {
        m_strParallelPerf.Format(L"fps = %f", 1 / time);
    }
    Invalidate(false);
}