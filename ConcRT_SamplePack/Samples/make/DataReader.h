// DataReader.h : Defines the XML parser that parses the DGML file.
//
#include "stdafx.h"

#pragma once

using namespace std;

class XMLReader
{
public:
    //Adapted from http://msdn.microsoft.com/en-us/library/ms753116(v=VS.85).aspx
    XMLReader(LPCWSTR fileName)
    {
        HRESULT hr;
        CComPtr<IStream> pFileStream;
        CComPtr<IXmlReader> pReader;

        if (FAILED(hr = SHCreateStreamOnFile((LPCWSTR)fileName, STGM_READ, &pFileStream)))
        {
            wprintf(L"Error creating xml file reader, error is %08.8lx", hr);
            throw;
        }            
        if (FAILED(hr = CreateXmlReader(__uuidof(IXmlReader), (void**) &pReader, NULL))) 
        {
            wprintf(L"Error creating xml reader, error is %08.8lx", hr);
            throw;
        }

        if (FAILED(hr = pReader->SetProperty(XmlReaderProperty_DtdProcessing, DtdProcessing_Prohibit))) 
        {
            wprintf(L"Error setting XmlReaderProperty_DtdProcessing, error is %08.8lx", hr);
            throw;
        }

        if (FAILED(hr = pReader->SetInput(pFileStream))) 
        {
            wprintf(L"Error setting input for reader, error is %08.8lx", hr);
            throw;
        }    

        // read until there are no more nodes
        XmlNodeType nodeType;
        const WCHAR* pwszPrefix;
        const WCHAR* pwszLocalName;
        UINT cwchPrefix;

        while (S_OK == (hr = pReader->Read(&nodeType))) 
        {
            switch (nodeType) 
            {
            case XmlNodeType_Element:
                {
                    if (FAILED(hr = pReader->GetPrefix(&pwszPrefix, &cwchPrefix))) 
                    {
                        wprintf(L"Error getting prefix, error is %08.8lx", hr);
                        throw;
                    }
                    if (FAILED(hr = pReader->GetLocalName(&pwszLocalName, NULL))) 
                    {
                        wprintf(L"Error getting local name, error is %08.8lx", hr);
                        throw;
                    }

                    UINT cnt;
                    if (FAILED(hr = pReader->GetAttributeCount(&cnt))) 
                    {
                        wprintf(L"Error getting attribute count, error is %08.8lx", hr);
                        throw;
                    }

                    string elem = ConvertToString(pwszLocalName);
                    if(elem == "Node")
                    {
                        if (cnt != 2)
                        {
                            wprintf(L"Node should contain Id and Background attributes");
                            throw;
                        }
                        if (FAILED(hr = WriteAttributes(pReader))) 
                        {
                            wprintf(L"Error writing attributes, error is %08.8lx", hr);
                            throw;
                        }

                    } 
                    else if (elem == "Link")
                    {
                        if (cnt != 2)
                        {
                            wprintf(L"Link should contain Source and Target attributes");
                            throw;
                        }
                        if (FAILED(hr = WriteAttributes(pReader))) 
                        {
                            wprintf(L"Error writing attributes, error is %08.8lx", hr);
                            throw;
                        }
                    }
                    
                    if (FAILED(hr = pReader->MoveToElement())) 
                    {
                        wprintf(L"Error moving to the element that owns the current attribute node, error is %08.8lx", hr);
                        throw;
                    }
                }
                break;
            }
        }
    }

    string_string_list_map GetParams()
    {
        return m_solution;
    }

private:


    HRESULT WriteAttributes(IXmlReader* pReader) 
    {
        const WCHAR* pwszPrefix;
        const WCHAR* pwszLocalName;
        const WCHAR* pwszValue;
        string elem, attrib;
        string source, target;
        HRESULT hr = pReader->MoveToFirstAttribute();

        if (S_FALSE == hr)
            return hr;
        if (S_OK != hr) 
        {
            wprintf(L"Error moving to first attribute, error is %08.8lx", hr);
            return -1;
        }
        else 
        {
            while (TRUE) 
            {
                if (!pReader->IsDefault()) 
                {
                    UINT cwchPrefix;
                    if (FAILED(hr = pReader->GetPrefix(&pwszPrefix, &cwchPrefix))) 
                    {
                        wprintf(L"Error getting prefix, error is %08.8lx", hr);
                        return -1;
                    }

                    if (FAILED(hr = pReader->GetLocalName(&pwszLocalName, NULL))) 
                    {
                        wprintf(L"Error getting local name, error is %08.8lx", hr);
                        return -1;
                    }

                    if (FAILED(hr = pReader->GetValue(&pwszValue, NULL))) 
                    {
                        wprintf(L"Error getting value, error is %08.8lx", hr);
                        return -1;
                    }

                   
                    elem = ConvertToString(pwszLocalName);
                    attrib = ConvertToString(pwszValue);

                    if (elem == "Id")
                    {
                        if (attrib.empty())
                        {
                            wprintf(L"The value for Id tag cannot be empty\n");
                            throw;
                        }
                        string_list strlist;
                        m_solution[attrib] = strlist;
                    }
                    else if (elem == "Source")
                    {
                        auto src_proj = m_solution.find(attrib);
                        if( src_proj == m_solution.end() ) 
                        { 
                            wprintf(L"Source project '%s' not found\n",attrib);
                            throw;
                        }
                        source = attrib;
                    }
                    else if (elem == "Target")
                    {
                        auto tgt_proj = m_solution.find(attrib);
                        if( tgt_proj == m_solution.end() ) 
                        { 
                            wprintf(L"Target project '%s' not found\n",attrib);
                            throw;
                        }
                        target = attrib;
                    }

                    if ((!source.empty()) && (!target.empty()))
                    {
                        auto src_proj = m_solution.find(source);
                        auto tgt_proj = m_solution.find(target);
                        (*tgt_proj).second.push_back(source);
                    }
                }

                if (S_OK != pReader->MoveToNextAttribute())
                    break;
            }
        }
        return hr;
    }

    string ConvertToString(const WCHAR* orig)
    {
        // Convert to a char*
        size_t convertedChars = 0;
        size_t  sizeInBytes = wcslen(orig) + 1;
        errno_t err = 0;
        char    *ch = (char *)malloc(sizeInBytes);
        err = wcstombs_s(&convertedChars, 
            ch, sizeInBytes,
            orig, sizeInBytes);
        if (err != 0)
            throw;
        string retStr(ch);
        free(ch);
        return retStr;
    }

private:
    string_string_list_map m_solution;
};
