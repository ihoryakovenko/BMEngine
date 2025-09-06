/*
 * Forge Memory Debugger - part of Quel Solaar
 * Copyright (c) 2007-2025 Eskil Steenberg
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */


/*Forge is a basic utility library, consisting of pure c functions that are useful in any development. Forge is meant to be included in to all c files and all other Quel Solaar librariues and applications. Because it is included in all files it can provide some very useful functiuonality such as debugging memory and help find memory leaks. Forge is designed to be lightweight and compleatly platform and dependency indipendent.
*/


#if !defined(TYPES_H)
#define	TYPES_H

#ifndef NULL
#ifdef __cplusplus
#define NULL    0 /* Defines NULL in C++*/
#else
#define NULL    ((void *)0) /* Defines NULL in C*/
#endif
#endif

#if !defined(TRUE)
#define TRUE 1 /* Defines TRUE */
#endif
#if !defined(FALSE)
#define FALSE 0 /* Defines FALSE*/
#endif
#if defined _WIN32
typedef unsigned int uint;
#else
#include <sys/types.h>
#endif
#if !defined(VERSE_TYPES)
typedef signed char int8;
typedef unsigned char uint8;
typedef signed short int16;
typedef unsigned short uint16;
typedef signed int int32;
typedef unsigned int uint32;
typedef float real32;
typedef double real64;
typedef unsigned char boolean;
#endif
typedef signed __int64 int64;
typedef unsigned __int64 uint64;

#define F_DOUBLE_PRECISION /* if F_DOUBLE_PRECISION is defined the type "freal" is defined as double otherwhise it will be defined as float. This is very usefull if you want to write an application that can be compiled to use either 32 or 64 bit floating point math. Forges entire match librarie are defined in both 32 and 64 bit precission and all functionality can be accessed using the freal type, a macro will replace all freal calls with the aropriate funtion using either 32 or 64 bit depending on if F_DOUBLE_PRECISION is defined*/

#ifdef F_DOUBLE_PRECISION 
typedef double freal;
#else
typedef float freal;
#endif

#define PI  3.141592653 /* Defines PI */



#if !defined(F_NO_MEMORY_DEBUG)
//#define F_MEMORY_DEBUG /* turns on the memory debugging system */
#endif
#if !defined(F_EXIT_CRASH)
//#define F_EXIT_CRASH /* turns on the crash on exit */
#endif

#ifdef F_MEMORY_DEBUG

/* ----- Debugging -----
If F_MEMORY_DEBUG  is enabled, the memory debugging system will create macros that replace malloc, free and realloc and allows the system to keppt track of and report where memory is beeing allocated, how much and if the memory is beeing freed. This is very useful for finding memory leaks in large applications. The system can also over allocate memory and fill it with a magic number and can therfor detect if the application writes outside of the allocated memory. if F_EXIT_CRASH is defined, then exit(); will be replaced with a funtion that writes to NULL. This will make it trivial ti find out where an application exits using any debugger., */

#ifdef __cplusplus
extern "C" {
#endif

extern void f_debug_memory_init(void (*lock)(void* mutex), void (*unlock)(void* mutex), void* mutex); /* Required for memory debugger to be thread safe */
extern void* f_debug_mem_malloc(uint size, const char* file, uint line); /* Replaces malloc and records the c file and line where it was called*/
extern void* f_debug_mem_calloc(uint count, uint size, const char* file, uint line);
extern void* f_debug_mem_realloc(void* pointer, uint size, const char* file, uint line); /* Replaces realloc and records the c file and line where it was called*/
extern void f_debug_mem_free(void* buf); /* Replaces free and records the c file and line where it was called*/
extern void f_debug_mem_print(uint min_allocs); /* Prints out a list of all allocations made, their location, how much memorey each has allocated, freed, and how many allocations have been made. The min_allocs parameter can be set to avoid printing any allocations that have been made fewer times then min_allocs */
extern void f_debug_mem_reset(); /* f_debug_mem_reset allows you to clear all memory stored in the debugging system if you only want to record allocations after a specific point in your code*/
extern boolean f_debug_memory(); /*f_debug_memory checks if any of the bounds of any allocation has been over written and reports where to standard out. The function returns TRUE if any error was found*/

#ifdef __cplusplus
}
#endif

#define malloc(n) f_debug_mem_malloc(n, __FILE__, __LINE__) /* Replaces malloc. */
#define calloc(n, m) f_debug_mem_calloc(n, m, __FILE__, __LINE__)
#define realloc(n, m) f_debug_mem_realloc(n, m, __FILE__, __LINE__) /* Replaces realloc. */
#define free(n) f_debug_mem_free(n) /* Replaces free. */

#endif

#ifdef F_EXIT_CRASH

#ifdef __cplusplus
extern "C" {
#endif

extern void exit_crash(uint i); /* finction guaranteed to crash (Writes to NULL).*/
#define exit(n) exit_crash(n) /* over writhing exit(0)  with afunction guaraneed tyo crash. */

#ifdef __cplusplus
}
#endif

#endif

#endif