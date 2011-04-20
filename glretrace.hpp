/**************************************************************************
 *
 * Copyright 2011 Jose Fonseca
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
 *
 **************************************************************************/

#ifndef _GLRETRACE_HPP_
#define _GLRETRACE_HPP_

#include <ostream>

#include "trace_parser.hpp"
#include "glws.hpp"


namespace glretrace {


extern bool double_buffer;
extern bool insideGlBeginEnd;
extern Trace::Parser parser;
extern glws::WindowSystem *ws;
extern glws::Visual *visual;
extern glws::Drawable *drawable;
extern glws::Context *context;

extern int window_width;
extern int window_height;

extern GLint fb;

extern unsigned frame;
extern unsigned unsaved_draws;
extern long long startTime;
extern bool wait;

extern bool benchmark;
extern const char *compare_prefix;
extern const char *snapshot_prefix;

extern unsigned dump_state;

void
checkGlError(void);

void snapshot(unsigned call_no);

void state_dump(std::ostream &os);


} /* namespace glretrace */


#endif /* _GLRETRACE_HPP_ */
