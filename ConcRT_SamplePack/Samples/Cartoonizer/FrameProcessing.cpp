//--------------------------------------------------------------------------
// 
//  Copyright (c) Microsoft Corporation.  All rights reserved. 
// 
//  File: FrameProcessing.cpp
//
//  Implementation of Cartoonizer Algorithms
//
//--------------------------------------------------------------------------

#include "StdAfx.h"
#include "FrameProcessing.h"
#include "SimpleParallel_for.h"
#include "ppl.h"
using namespace Concurrency;
#include <omp.h>


const double Util::Wr = 0.299;
const double Util::Wb = 0.114;
const double Util::wg = 1 - Util::Wr - Util::Wb;


FrameProcessing::FrameProcessing(CartoonAgentBase* pAgent)
{
    m_Size = 0;
    m_BPP = 0;
    m_Pitch = 0;
    m_NeighborWindow = 3;
    m_pCartoonAgent = pAgent;
    m_fParallel = true;
    m_pCurrentImage = NULL;
    m_pOutputImage = NULL;
    m_pBufferImage = NULL;
    m_fVideo = false;
    m_fUpdateUI = true;
    m_enFrameType = kStaticImage;
    m_fSimpleParalleFor = false;
}


FrameProcessing::~FrameProcessing(void)
{
    if(NULL != m_pCurrentImage)
    {
        Concurrency::Free(m_pCurrentImage);
    }

    if(NULL != m_pBufferImage)
    {
        Concurrency::Free(m_pBufferImage);
    }

    if(NULL != m_pOutputImage)
    {
        Concurrency::Free(m_pOutputImage);
    }
}

void FrameProcessing::ApplyFilters(int nPhases, bool fParallel)
{
    m_fParallel = fParallel;
    ApplyColorSimplifier(nPhases);
    ApplyEdgeDetection();
    UpdateUI();
}

void FrameProcessing::SetCurrentFrame(BYTE* pFrame, int size, int width, int height, int pitch, int bpp, int clrPlanes)
{
    m_Width = width;
    m_Height = height;
    m_Pitch = pitch;
    m_BPP = bpp;
    m_ColorPlanes = clrPlanes;
    if(size != m_Size)
    {
        if(NULL != m_pOutputImage)
        {
            Concurrency::Free(m_pOutputImage);
            m_pOutputImage = NULL;
        }
        if(NULL != m_pCurrentImage)
        {
            Concurrency::Free(m_pCurrentImage);
            m_pCurrentImage = NULL;
        }

        if(NULL != m_pBufferImage)
        {
            Concurrency::Free(m_pBufferImage);
            m_pBufferImage = NULL;
        }
    }

    if(NULL == m_pCurrentImage)
    {
        m_pCurrentImage = (BYTE*)Concurrency::Alloc(size);
    }
    if(NULL == m_pOutputImage)
    {
        m_pOutputImage = (BYTE*)Concurrency::Alloc(size);
    }
    if(NULL == m_pBufferImage)
    {
        m_pBufferImage = (BYTE*)Concurrency::Alloc(size);
    }

    memcpy_s(m_pCurrentImage, size, pFrame, size);
    memcpy_s(m_pBufferImage, size, pFrame, size);
    m_Size = size;
}

BYTE* FrameProcessing::GetSourceFrame()
{
    return m_pCurrentImage;
}
BYTE* FrameProcessing::GetOutputFrame()
{
    BYTE* pRet = m_pOutputImage;
    if(kStaticImage == m_enFrameType)
    {
        pRet = m_pBufferImage;
    }
    return pRet;
}

BYTE* FrameProcessing::GetOutputFrame(int& size, unsigned int& height, unsigned int& width, int& BPP, int& pitch, int& clrPlanes)
{
    BYTE* pRet = m_pOutputImage;
    if(kStaticImage == m_enFrameType)
    {
        pRet = m_pBufferImage;
    }

    size = m_Size;
    height = m_Height;
    width = m_Width;
    BPP = m_BPP;
    pitch = m_Pitch;
    clrPlanes = m_ColorPlanes;
    return pRet;
}

BYTE* FrameProcessing::GetSourceFrame(int& size, unsigned int& height, unsigned int& width, int& BPP, int& pitch, int& clrPlanes)
{
    size = m_Size;
    height = m_Height;
    width = m_Width;
    BPP = m_BPP;
    pitch = m_Pitch;
    clrPlanes = m_ColorPlanes;
    return m_pCurrentImage;
}

void FrameProcessing::ApplyColorSimplifier(int nPhases)
{
    for(int i = 0; i < nPhases; ++i)
    {
        ApplyColorSimplifier();
    }
}

void FrameProcessing::ApplyColorSimplifier()
{
    unsigned int shift = m_NeighborWindow / 2;
    ApplyColorSimplifier(shift, (m_Height - shift), shift, ( m_Width - shift));
}

void FrameProcessing::ApplyColorSimplifier(unsigned int startHeight, unsigned int endHeight, unsigned int startWidth, unsigned int endWidth)
{
    if(NULL != m_pBufferImage)
    {
        if(m_fSimpleParalleFor)
        {
            SimpleParallelFor((int)startHeight, (int)endHeight, [&](int j)
            {
                for(unsigned int i = startWidth; i < endWidth; ++i)
                {
                    SimplifyIndexOptimized(m_pBufferImage, i, j);
                }
                UpdateUI();
            });
        }
        else if(m_fParallel)
        {
            parallel_for(startHeight, endHeight, [&](unsigned int j)
            {
                for(unsigned int i = startWidth; i < endWidth; ++i)
                {
                    SimplifyIndexOptimized(m_pBufferImage, i, j);
                }
                UpdateUI();
            });
        }
        else
        {
            for (unsigned int j = startHeight; j < endHeight; ++j)
            {
                for(unsigned int i = startWidth; i < endWidth; ++i)
                {
                    SimplifyIndexOptimized(m_pBufferImage, i, j);
                }
                UpdateUI();
            }
        }
    }
    UpdateUI();
}

void FrameProcessing::ApplyColorSimplifierOpenMP(unsigned int startHeight, unsigned int endHeight, unsigned int startWidth, unsigned int endWidth)
{
    if(NULL != m_pBufferImage)
    {
        int j = startHeight;
        unsigned int i = startWidth;

        #pragma omp parallel \
        private (j, i)
        #pragma omp for 
        for (j = startHeight; j < (int)endHeight; ++j)
        {
            for(i = startWidth; i < endWidth; ++i)
            {
                SimplifyIndexOptimized(m_pBufferImage, i, j);
            }
            UpdateUI();
        }
    }
    UpdateUI();
}

void FrameProcessing::SimplifyIndexOptimized(BYTE* pFrame, int x, int y)
{
    COLORREF orgClr =  GetPixel(pFrame, x, y, m_Pitch, m_BPP);

    int shift = m_NeighborWindow / 2;
    double sSum = 0;
    double partialSumR = 0, partialSumG = 0, partialSumB = 0;
    double standardDeviation = 0.025;

    for(int j = y - shift; j <= (y + shift); ++j)
    {
        for(int i = x - shift; i <= (x + shift); ++i)
        {
            if(i != x ||  j != y) // don't apply filter to the requested index, only to the neighbors
            {
                COLORREF clr = GetPixel(pFrame, i, j, m_Pitch, m_BPP);
                int index = (j - (y - shift)) * m_NeighborWindow + i - (x - shift);
                double distance = Util::GetDistance(orgClr, clr);
                double sValue = pow(M_E, -0.5 * pow(distance / standardDeviation, 2));
                sSum += sValue;
                partialSumR += GetRValue(clr) * sValue;
                partialSumG += GetGValue(clr) * sValue;
                partialSumB += GetBValue(clr) * sValue;
            }
        }
    }

    COLORREF simplifiedClr;
    int simpleRed, simpleGreen, simpleBlue;

    simpleRed     = (int)min(max(partialSumR / sSum, 0), 255);
    simpleGreen = (int)min(max(partialSumG / sSum, 0), 255);
    simpleBlue     = (int)min(max(partialSumB / sSum, 0), 255);
    simplifiedClr = RGB(simpleRed, simpleGreen, simpleBlue);
    SetPixel(m_pBufferImage, x, y, m_Pitch, m_BPP, simplifiedClr);
}

void FrameProcessing::ApplyEdgeDetection()
{
    if(m_fParallel)
    {
        ApplyEdgeDetectionParallel(1, (m_Height - 1), 1, (m_Width - 1));
    }
    else if(m_fSimpleParalleFor)
    {
        ApplyEdgeDetectionSimpleParallelFor(1, (m_Height - 1), 1, (m_Width - 1));
    }
    else
    {
        ApplyEdgeDetection(m_pBufferImage, 1, (m_Height - 1), 1, (m_Width - 1));
    }
}


void FrameProcessing::ApplyEdgeDetectionArea(unsigned int startHeight, unsigned int endHeight, unsigned int startWidth, unsigned int endWidth)
{
    const float alpha = 0.3f;
    const float beta = 0.8f;
    const float s0 = 0.054f;
    const float s1 = 0.064f;
    const float a0 = 0.3f;
    const float a1 = 0.7f;

    int shift = m_NeighborWindow / 2;

    int startRow = startHeight - shift;
    startRow = (startRow < 0)? 0: startRow;

    unsigned int height = endHeight - startHeight;
    int size = (endHeight - startRow) * abs(m_Pitch);
    int startIndex = (startRow * abs(m_Pitch));
    if(size > 0 )
    {
        BYTE* pFrame = new BYTE[size];
        memcpy_s(pFrame, size, m_pBufferImage + startIndex, size);

        unsigned int row = 0;
        if(0 >= startRow)
        {
            startRow = 1;
            row = 1;
            --height;
        }
        if(endHeight == m_Height - shift)
        {
            endHeight = m_Height - 1;
            --height;
        }

        for( unsigned int y = startRow; y < endHeight, row < height; ++y, ++row)
        {
            for(unsigned int x = 1; x < m_Width - 1; ++x)
            {
                float Sy, Su, Sv;
                float Ay, Au, Av;

                CalculateSobel(m_pBufferImage, x, y, Sy, Su, Sv);
                CalculateSobel(m_pCurrentImage, x, y, Ay, Au, Av);

                float edgeS = (1 - alpha) * Sy +
                    alpha * (Su + Sv) / 2;

                float edgeA = (1 - alpha) * Ay +
                    alpha * (Au + Av) / 2;

                float i = (1 - beta) * Util::SmoothStep(s0, s1, edgeS)
                    + beta * Util::SmoothStep(a0, a1, edgeA);

                float oneMinusi = 1 - i;
                COLORREF clr = GetPixel(m_pBufferImage, x, y, m_Pitch, m_BPP);
                COLORREF newClr = RGB(GetRValue(clr) * oneMinusi, GetGValue(clr) * oneMinusi, GetBValue(clr) * oneMinusi);
                this->SetPixel(pFrame, x, row, m_Pitch, m_BPP, newClr);
            }
            UpdateUI();
        }
        memcpy_s(m_pBufferImage + startIndex, size, pFrame, size);
        delete[] pFrame;
    }
}


void FrameProcessing::ApplyEdgeDetection(BYTE* pImageFrame, unsigned int startHeight, unsigned int endHeight, unsigned int startWidth, unsigned int endWidth)
{
    if(pImageFrame == NULL)
    {
        pImageFrame = m_pBufferImage;
    }
    const float alpha = 0.3f;
    const float beta = 0.8f;
    const float s0 = 0.054f;
    const float s1 = 0.064f;
    const float a0 = 0.3f;
    const float a1 = 0.7f;

    BYTE* pFrame = new BYTE[m_Size];
    memcpy_s(pFrame, m_Size, pImageFrame, m_Size);

    for(unsigned int y = startHeight; y < endHeight; ++y)
    {
        for(unsigned int x = startWidth; x < endWidth; ++x)
        {
            float Sy, Su, Sv;
            float Ay, Au, Av;

            CalculateSobel(m_pBufferImage, x, y, Sy, Su, Sv);
            CalculateSobel(m_pCurrentImage, x, y, Ay, Au, Av);

            float edgeS = (1 - alpha) * Sy +
                alpha * (Su + Sv) / 2;

            float edgeA = (1 - alpha) * Ay +
                alpha * (Au + Av) / 2;

            float i = (1 - beta) * Util::SmoothStep(s0, s1, edgeS)
                + beta * Util::SmoothStep(a0, a1, edgeA);

            float oneMinusi = 1 - i;
            COLORREF clr = GetPixel(m_pBufferImage, x, y, m_Pitch, m_BPP);
            COLORREF newClr = RGB(GetRValue(clr) * oneMinusi, GetGValue(clr) * oneMinusi, GetBValue(clr) * oneMinusi);
            this->SetPixel(pFrame, x, y, m_Pitch, m_BPP, newClr);
        }
        UpdateUI();
    }
    memcpy_s(pImageFrame, m_Size, pFrame, m_Size);
    delete[] pFrame;
}

void FrameProcessing::ApplyEdgeDetectionParallel(unsigned int startHeight, unsigned int endHeight, unsigned int startWidth, unsigned int endWidth)
{
    const float alpha = 0.3f;
    const float beta = 0.8f;
    const float s0 = 0.054f;
    const float s1 = 0.064f;
    const float a0 = 0.3f;
    const float a1 = 0.7f;

    BYTE* pFrame = new BYTE[m_Size];
    memcpy_s(pFrame, m_Size, m_pBufferImage, m_Size);

    parallel_for(startHeight , endHeight,  [&](int y)
    {
        for(unsigned int x = startWidth; x < endWidth; ++x)
        {
            float Sy, Su, Sv;
            float Ay, Au, Av;

            CalculateSobel(m_pBufferImage, x, y, Sy, Su, Sv);
            CalculateSobel(m_pCurrentImage, x, y, Ay, Au, Av);

            float edgeS = (1 - alpha) * Sy +
                alpha * (Su + Sv) / 2;

            float edgeA = (1 - alpha) * Ay +
                alpha * (Au + Av) / 2;

            float i = (1 - beta) * Util::SmoothStep(s0, s1, edgeS)
                + beta * Util::SmoothStep(a0, a1, edgeA);

            float oneMinusi = 1 - i;
            COLORREF clr = this->GetPixel(m_pBufferImage, x, y, m_Pitch, m_BPP);
            COLORREF newClr = RGB(GetRValue(clr) * oneMinusi, GetGValue(clr) * oneMinusi, GetBValue(clr) * oneMinusi);
            this->SetPixel(pFrame, x, y, m_Pitch, m_BPP, newClr);            
        }
        UpdateUI();
    });
    memcpy_s(m_pBufferImage, m_Size, pFrame, m_Size);
    delete[] pFrame;
}

void FrameProcessing::ApplyEdgeDetectionSimpleParallelFor(unsigned int startHeight, unsigned int endHeight, unsigned int startWidth, unsigned int endWidth)
{
    const float alpha = 0.3f;
    const float beta = 0.8f;
    const float s0 = 0.054f;
    const float s1 = 0.064f;
    const float a0 = 0.3f;
    const float a1 = 0.7f;

    BYTE* pFrame = new BYTE[m_Size];
    memcpy_s(pFrame, m_Size, m_pBufferImage, m_Size);

    SimpleParallelFor((int)startHeight , (int)endHeight,  [&](int y)
    {
        for(unsigned int x = startWidth; x < endWidth; ++x)
        {
            float Sy, Su, Sv;
            float Ay, Au, Av;

            CalculateSobel(m_pBufferImage, x, y, Sy, Su, Sv);
            CalculateSobel(m_pCurrentImage, x, y, Ay, Au, Av);

            float edgeS = (1 - alpha) * Sy +
                alpha * (Su + Sv) / 2;

            float edgeA = (1 - alpha) * Ay +
                alpha * (Au + Av) / 2;

            float i = (1 - beta) * Util::SmoothStep(s0, s1, edgeS)
                + beta * Util::SmoothStep(a0, a1, edgeA);

            float oneMinusi = 1 - i;
            COLORREF clr = this->GetPixel(m_pBufferImage, x, y, m_Pitch, m_BPP);
            COLORREF newClr = RGB(GetRValue(clr) * oneMinusi, GetGValue(clr) * oneMinusi, GetBValue(clr) * oneMinusi);
            this->SetPixel(pFrame, x, y, m_Pitch, m_BPP, newClr);            
        }
        UpdateUI();
    });
    memcpy_s(m_pBufferImage, m_Size, pFrame, m_Size);
    delete[] pFrame;
    UpdateUI();
}

void FrameProcessing::ApplyEdgeDetectionOpenMP(unsigned int startHeight, unsigned int endHeight, unsigned int startWidth, unsigned int endWidth)
{
    const float alpha = 0.3f;
    const float beta = 0.8f;
    const float s0 = 0.054f;
    const float s1 = 0.064f;
    const float a0 = 0.3f;
    const float a1 = 0.7f;

    BYTE* pFrame = new BYTE[m_Size];
    memcpy_s(pFrame, m_Size, m_pBufferImage, m_Size);

    int y = startHeight;
    unsigned int x = startWidth;

    #pragma omp parallel \
    private (y, x)
    #pragma omp for 
    for(y = startHeight; y < (int)endHeight; ++y)
    {
        for(x = startWidth; x < endWidth; ++x)
        {
            float Sy, Su, Sv;
            float Ay, Au, Av;

            CalculateSobel(m_pBufferImage, x, y, Sy, Su, Sv);
            CalculateSobel(m_pCurrentImage, x, y, Ay, Au, Av);

            float edgeS = (1 - alpha) * Sy +
                alpha * (Su + Sv) / 2;

            float edgeA = (1 - alpha) * Ay +
                alpha * (Au + Av) / 2;

            float i = (1 - beta) * Util::SmoothStep(s0, s1, edgeS)
                + beta * Util::SmoothStep(a0, a1, edgeA);

            float oneMinusi = 1 - i;
            COLORREF clr = this->GetPixel(m_pBufferImage, x, y, m_Pitch, m_BPP);
            COLORREF newClr = RGB(GetRValue(clr) * oneMinusi, GetGValue(clr) * oneMinusi, GetBValue(clr) * oneMinusi);
            this->SetPixel(pFrame, x, y, m_Pitch, m_BPP, newClr);
        }
        UpdateUI();
    }
    memcpy_s(m_pBufferImage, m_Size, pFrame, m_Size);
    delete[] pFrame;
}

void FrameProcessing::CalculateSobel(BYTE* pSource, int row, int column, float& dy, float& du, float& dv)
{
    int gx[3][3] = { { -1, 0, 1 }, { -2, 0, 2 }, { -1, 0, 1 } };   //  The matrix Gx
    int gy[3][3] = { { 1, 2, 1 }, { 0, 0, 0 }, { -1, -2, -1 } };  //  The matrix Gy

    double new_yX = 0, new_yY = 0;
    double new_uX = 0, new_uY = 0;
    double new_vX = 0, new_vY = 0;
    for (int i = -1; i < 2; i++)
    {
        for (int j = -1; j < 2; j++)
        {
            double y, u, v;
            Util::RGBToYUV(GetPixel(pSource, row + i, column + j, m_Pitch, m_BPP), y, u, v);

            new_yX += gx[i + 1][j + 1] * y;
            new_yY += gy[i + 1][j + 1] * y;

            new_uX += gx[i + 1][j + 1] * u;
            new_uY += gy[i + 1][j + 1] * u;

            new_vX += gx[i + 1][j + 1] * v;
            new_vY += gy[i + 1][j + 1] * v;
        }
    }

    dy = (float)sqrt(pow(new_yX, 2) + pow(new_yY, 2));
    du = (float)sqrt(pow(new_uX, 2) + pow(new_uY, 2));
    dv = (float)sqrt(pow(new_vX, 2) + pow(new_vY, 2));
}