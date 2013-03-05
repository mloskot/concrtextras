// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once
#define _ITERATOR_DEBUG_LEVEL 0 // this becomes unnecessary when when_all supports iterators properly

#include "targetver.h"

#include <stdio.h>
#include <tchar.h>
#include <windows.h>
#include <iostream>
#include <ctime>
#include <fstream>
#include <string>
#include <map>
#include <list>
#include <atlbase.h>
#include <xmllite.h>
#include <strsafe.h>
#include <vector>
#include <ppl.h>

#pragma once

// TODO: reference additional headers your program requires here

// Prellocate 10K
char buffer[1024*10];

typedef std::vector<std::string> string_list;
typedef std::map<std::string,string_list> string_string_list_map;


