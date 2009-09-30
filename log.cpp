/**************************************************************************
 *
 * Copyright 2008-2009 VMware, Inc.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * Copyright 2007-2009 VMware, Inc.
 * All rights reserved.
 *
 **************************************************************************/


#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <windows.h>

#include <zlib.h>

#include "log.hpp"


#ifdef WIN32
#ifndef PATH_MAX
#define PATH_MAX _MAX_PATH
#endif
#ifndef snprintf
#define snprintf _snprintf
#endif
#ifndef vsnprintf
#define vsnprintf _vsnprintf
#endif
#endif


#if defined(_MSC_VER)

unsigned __int64 __rdtsc();
#pragma intrinsic(__rdtsc)
#define rdtsc() __rdtsc()

#elif defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))

#include <stdint.h>

static inline unsigned long long
rdtsc(void)
{
   unsigned long hi, lo;
   __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
   return ((unsigned long long)lo) | (((unsigned long long)hi) << 32);
}

#else

#define rdtsc() 0

#endif


namespace Log {

static gzFile g_gzFile = NULL;
static char g_szFileName[PATH_MAX];
static CRITICAL_SECTION CriticalSection;

static void _Close(void) {
    if(g_gzFile != NULL) {
        gzclose(g_gzFile);
        g_gzFile = NULL;
        DeleteCriticalSection(&CriticalSection);
    }
}

static void _Open(const char *szName, const char *szExtension) {
    _Close();
    
    static unsigned dwCounter = 0;

    char szProcessPath[PATH_MAX];
    char *lpProcessName;
    char *lpProcessExt;

    GetModuleFileNameA(NULL, szProcessPath, sizeof(szProcessPath)/sizeof(szProcessPath[0]));

    lpProcessName = strrchr(szProcessPath, '\\');
    lpProcessName = lpProcessName ? lpProcessName + 1 : szProcessPath;
    lpProcessExt = strrchr(lpProcessName, '.');
    if(lpProcessExt)
	*lpProcessExt = '\0';

    for(;;) {
        FILE *file;
        
        if(dwCounter)
            snprintf(g_szFileName, PATH_MAX, "%s.%s.%u.%s.gz", lpProcessName, szName, dwCounter, szExtension);
        else
            snprintf(g_szFileName, PATH_MAX, "%s.%s.%s.gz", lpProcessName, szName, szExtension);
        
        file = fopen(g_szFileName, "rb");
        if(file == NULL)
            break;
        
        fclose(file);
        
        ++dwCounter;
    }

    g_gzFile = gzopen(g_szFileName, "wb");
    InitializeCriticalSection(&CriticalSection);
}

static inline void _ReOpen(void) {
    /* XXX */
}

static inline void Write(const char *sBuffer, size_t dwBytesToWrite) {
    if(g_gzFile == NULL)
        return;
    
    gzwrite(g_gzFile, sBuffer, dwBytesToWrite);
}

static inline void Write(const char *szText) {
    Write(szText, strlen(szText));
}

static inline void 
Escape(const char *s) {
    /* FIXME */
    Write(s);
}

static inline void 
Indent(unsigned level) {
    for(unsigned i = 0; i < level; ++i)
        Write("\t");
}

static inline void 
NewLine(void) {
    Write("\r\n");
}

static inline void 
Tag(const char *name) {
    Write("<");
    Write(name);
    Write("/>");
}

static inline void 
BeginTag(const char *name) {
    Write("<");
    Write(name);
    Write(">");
}

static inline void 
BeginTag(const char *name, 
         const char *attr1, const char *value1) {
    Write("<");
    Write(name);
    Write(" ");
    Write(attr1);
    Write("=\"");
    Escape(value1);
    Write("\">");
}

static inline void 
BeginTag(const char *name, 
         const char *attr1, const char *value1,
         const char *attr2, const char *value2) {
    Write("<");
    Write(name);
    Write(" ");
    Write(attr1);
    Write("=\"");
    Escape(value1);
    Write("\" ");
    Write(attr2);
    Write("=\"");
    Escape(value2);
    Write("\">");
}

static inline void 
BeginTag(const char *name, 
              const char *attr1, const char *value1,
              const char *attr2, const char *value2,
              const char *attr3, const char *value3) {
    Write("<");
    Write(name);
    Write(" ");
    Write(attr1);
    Write("=\"");
    Escape(value1);
    Write("\" ");
    Write(attr2);
    Write("=\"");
    Escape(value2);
    Write("\" ");
    Write(attr3);
    Write("=\"");
    Escape(value3);
    Write("\">");
}

static inline void
EndTag(const char *name) {
    Write("</");
    Write(name);
    Write(">");
}

void Open(const char *name) {
    _Open(name, "xml");
    Write("<?xml version='1.0' encoding='UTF-8'?>");
    NewLine();
    Write("<?xml-stylesheet type='text/xsl' href='apitrace.xsl'?>");
    NewLine();
    BeginTag("trace");
    NewLine();
}

void ReOpen(void) {
    _ReOpen();
}

void Close(void) {
    EndTag("trace");
    NewLine();
    _Close();
}

void Text(const char *text) {
    Escape(text);
}

static void TextChar(char c) {
    char szText[2];
    szText[0] = c;
    szText[1] = 0;
    Text(szText);
}

void TextF(const char *format, ...) {
    char szBuffer[4196];
    va_list ap;
    va_start(ap, format);
    vsnprintf(szBuffer, sizeof(szBuffer), format, ap);
    va_end(ap);
    Text(szBuffer);
}

template<class T>
void TextHex(T value) {
    const char *hex = "0123456789abcdef";
    char szBuffer[2*sizeof(T) + 1];
    for(unsigned i = 0; i < 2*sizeof(T); ++i)
        szBuffer[i] = hex[(value >> (4*(2*sizeof(T) - (i + 1)))) & 0xf];
    szBuffer[2*sizeof(T)] = 0;
    Text(szBuffer);
}

void BeginCall(const char *function) {
    //EnterCriticalSection(&CriticalSection); 
    Indent(1);
    BeginTag("call", "name", function);
    NewLine();

    Indent(2);
    BeginTag("starttsc");
    TextHex(rdtsc());
    EndTag("starttsc");
    NewLine();
}

void EndCall(void) {
    Indent(2);
    BeginTag("endtsc");
    TextHex(rdtsc());
    EndTag("endtsc");
    NewLine();

    Indent(1);
    EndTag("call");
    NewLine();
#if 0
    gzflush(g_gzFile, Z_SYNC_FLUSH);
#endif
    //LeaveCriticalSection(&CriticalSection); 
}

void BeginArg(const char *type, const char *name) {
    Indent(2);
    BeginTag("arg", "type", type, "name", name);
}

void EndArg(void) {
    EndTag("arg");
    NewLine();
}

void BeginReturn(const char *type) {
    Indent(2);
    BeginTag("ret", "type", type);
}

void EndReturn(void) {
    EndTag("ret");
    NewLine();
}

void BeginElement(const char *type, const char *name) {
    BeginTag("elem", "type", type, "name", name);
}

void BeginElement(const char *type) {
    BeginTag("elem", "type", type);
}

void EndElement(void) {
    EndTag("elem");
}

void BeginReference(const char *type, const void *addr) {
    char saddr[256];
    snprintf(saddr, sizeof(saddr), "%p", addr);
    BeginTag("ref", "type", type, "addr", saddr);
}

void EndReference(void) {
    EndTag("ref");
}

void DumpString(const char *str) {
    const unsigned char *p = (const unsigned char *)str;
    Log::Text("\"");
    unsigned char c;
    while((c = *p++) != 0) {
        if(c == '\"')
            Text("\\\"");
        else if(c == '\\')
            Text("\\\\");
        else if(c >= 0x20 && c <= 0x7e)
            TextChar(c);
        else if(c == '\t')
            Text("\\t");
        else if(c == '\r')
            Text("\\r");
        else if(c == '\n')
            Text("&#10;");
        else {
            unsigned char octal0 = c & 0x7;
            unsigned char octal1 = (c >> 3) & 0x7;
            unsigned char octal2 = (c >> 3) & 0x7;
            if(octal2)
                TextF("\\%u%u%u", octal2, octal1, octal0);
            else if(octal1)
                TextF("\\%u%u", octal1, octal0);
            else
                TextF("\\%u", octal0);
        }
    }
    Log::Text("\"");
}

void DumpWString(const wchar_t *str) {
    const wchar_t *p = str;
    Log::Text("L\"");
    wchar_t c;
    while((c = *p++) != 0) {
        if(c == '\"')
            Text("\\\"");
        else if(c == '\\')
            Text("\\\\");
        else if(c >= 0x20 && c <= 0x7e)
            TextChar((char)c);
        else if(c == '\t')
            Text("\\t");
        else if(c == '\r')
            Text("\\r");
        else if(c == '\n')
            Text("&#10;");
        else {
            unsigned octal0 = c & 0x7;
            unsigned octal1 = (c >> 3) & 0x7;
            unsigned octal2 = (c >> 3) & 0x7;
            if(octal2)
                TextF("\\%u%u%u", octal2, octal1, octal0);
            else if(octal1)
                TextF("\\%u%u", octal1, octal0);
            else
                TextF("\\%u", octal0);
        }
    }
    Log::Text("\"");
}

} /* namespace Log */
