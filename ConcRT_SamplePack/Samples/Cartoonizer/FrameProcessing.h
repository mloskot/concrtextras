//--------------------------------------------------------------------------
// 
//  Copyright (c) Microsoft Corporation.  All rights reserved. 
// 
//  File: FrameProcessing.h
//
//  Implementation of Cartoonizer Algorithms
//
//--------------------------------------------------------------------------

#pragma once
#include <math.h>
#include "CartoonAgentBase.h"


class FrameProcessing: public IFrameProcesser
{
public:
    FrameProcessing(CartoonAgentBase* pAgent);
    virtual ~FrameProcessing(void);

    
    void ApplyFilters(int nPhases, bool fParallel);
    void StopFilters();
    void SetCurrentFrame(BYTE* pFrame,int size, int width, int height, int pitch, int bpp, int clrPlanes);
    BYTE* GetSourceFrame();
    BYTE* GetOutputFrame();
    BYTE* GetOutputFrame(int& size, unsigned int& height, unsigned int& width, int& BPP, int& pitch, int& clrPlanes);
    BYTE* GetSourceFrame(int& size, unsigned int& height, unsigned int& width, int& BPP, int& pitch, int& clrPlanes);

    void SetNeighbourArea(unsigned int area)
    {
        m_NeighborWindow = area;
    }
    unsigned int GetNeighbourArea()
    {
        return m_NeighborWindow;
    }

    void SetFrameType(FrameType enFrameType)
    {
        m_enFrameType = enFrameType;
    }
    unsigned int GetFrameType()
    {
        return m_enFrameType;
    }

    void SetParallelOption(bool fParallel)
    {
        m_fParallel = fParallel;
    }

    void UseSimpleParallelFor(bool fSimple)
    {
        m_fSimpleParalleFor = fSimple;
    }

    void SetUpdateFlag(bool fUpdate)
    {
        m_fUpdateUI = fUpdate;
    }

    void UpdateUI()
    {
        if(NULL != m_pCartoonAgent && true == m_fUpdateUI)
        {
            m_pCartoonAgent->UpdateUI();
        }
    }

    void FrameDone()
    {
        memcpy_s(m_pOutputImage, m_Size, m_pBufferImage, m_Size);
        UpdateUI();
    }

public: //methods
    void ApplyColorSimplifier();
    void ApplyColorSimplifier(int nPhases);
    void ApplyColorSimplifier(unsigned int startHeight, unsigned int endHeight, unsigned int startWidth, unsigned int endWidth);
    void ApplyColorSimplifierOpenMP(unsigned int startHeight, unsigned int endHeight, unsigned int startWidth, unsigned int endWidth);
    void SimplifyIndexOptimized(BYTE* pFrame, int x, int y);
    void ApplyEdgeDetection();
    void ApplyEdgeDetectionParallel(unsigned int startHeight, unsigned int endHeight, unsigned int startWidth, unsigned int endWidth);
    void ApplyEdgeDetectionSimpleParallelFor(unsigned int startHeight, unsigned int endHeight, unsigned int startWidth, unsigned int endWidth);
    void ApplyEdgeDetectionOpenMP(unsigned int startHeight, unsigned int endHeight, unsigned int startWidth, unsigned int endWidth);
    void ApplyEdgeDetection(BYTE* pFrame, unsigned int startHeight, unsigned int endHeight, unsigned int startWidth, unsigned int endWidth);
    void ApplyEdgeDetectionArea(unsigned int startHeight, unsigned int endHeight, unsigned int startWidth, unsigned int endWidth);
    void CalculateSobel(BYTE* pSource, int row, int column, float& dy, float& du, float& dv);

public:
    static inline COLORREF GetPixel(BYTE* pFrame, int x, int y, int pitch, int bpp)
    {
        int width = abs((int)pitch);
        int bytesPerPixel = bpp / 8;
        int byteIndex = y * width + x * bytesPerPixel;
        return RGB(pFrame[byteIndex + 2], 
            pFrame[byteIndex + 1],
            pFrame[byteIndex]);
    }

    static inline void SetPixel(BYTE* pFrame, int x, int y, int pitch, int bpp, COLORREF color)
    {
        int width = abs((int)pitch);
        int bytesPerPixel = bpp / 8;
        int byteIndex = y * width + x * bytesPerPixel;
        pFrame[byteIndex + 2] = GetRValue(color);
        pFrame[byteIndex + 1] = GetGValue(color);
        pFrame[byteIndex] = GetBValue(color);
    }

private: //members

    BYTE* m_pCurrentImage;  // src for the current frame
    BYTE* m_pBufferImage;  // current image being processed
    BYTE* m_pOutputImage;  // frame after Processing
    bool m_fParallel;
    bool m_fSimpleParalleFor;
    bool m_fVideo;
    unsigned int m_NeighborWindow;
    bool m_fUpdateUI;
    FrameType m_enFrameType;
};

class Util
{
public:
    static const double Wr;
    static const double Wb;
    static const double wg;

    static inline void RGBToYUV(COLORREF color, double& y, double& u, double& v)
    {
        double r = GetRValue(color) / 255.0;
        double g = GetGValue(color) / 255.0;
        double b = GetBValue(color) / 255.0;

        y = Wr * r + Wb * b + wg * g;
        u = 0.436 * (b - y) / (1 - Wb);
        v = 0.615 * (r - y) / (1 - Wr);
    }

    static inline double GetDistance(COLORREF color1, COLORREF color2)
    {
        double y1, u1, v1, y2, u2, v2;
        RGBToYUV(color1, y1, u1, v1);
        RGBToYUV(color2, y2, u2, v2);

        return sqrt(pow(u1 - u2, 2) + pow(v1 - v2, 2));
    }

    static inline float SmoothStep(float a, float b, float x)
    {
        if (x < a) return 0.0f;
        else if (x >= b) return 1.0f;

        x = (x - a) / (b - a);
        return (x * x * (3.0f - 2.0f * x));
    }

    static inline double CombinePlus(double i, double j)
    {
        return i + j;
    }
};

