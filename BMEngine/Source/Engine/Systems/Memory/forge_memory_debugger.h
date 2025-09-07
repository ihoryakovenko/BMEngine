/*
Copyright (C) 2025 by Eskil Steenberg Hald eskil at quelsolaar dot com

Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby granted.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

typedef unsigned char boolean;

#if !defined(F_NO_MEMORY_DEBUG)
#if defined(DEBUG) || defined(_DEBUG)
#define FORGE_MEMORY_DEBUG  /* By default, only run the memory debugger in debug mode. */
#endif
#endif

/* ----- Meomory Debugging -----
If FORGE_MEMORY_DEBUG  is enabled, the memory debugging system will create macros that replace malloc, free and realloc and allows the system to keppt track of and report where memory is beeing allocated, how much and if the memory is beeing freed. This is very useful for finding memory leaks in large applications. The system can also over allocate memory and fill it with a magic number and can therfor detect if the application writes outside of the allocated memory. if F_EXIT_CRASH is defined, then exit(); will be replaced with a funtion that writes to NULL. This will make it trivial ti find out where an application exits using any debugger., */


#define FORGE_MEMORY_NULL_ALLOCATION_ERROR /* Triggers and error if an allocation function returns NULL (out of memory)*/
#define FORGE_MEMORY_OVER_ALLOC 4096 /* Each allocation will be made larger, and the over allocation filled with a magic number, that can be used to detect buffer overruns. */
#define FORGE_MEMORY_PRE_PADDIG 256 /* How much of the over allocation is made infront of the allocations. Must be smaller than FORGE_MEMORY_OVER_ALLOC and even to the largest alignment of any type */
#define FORGE_DOUBLE_FREE_CHECK /* Stores reed pointers, to check if they are ever freed again. */
//#define FORGE_USE_AFTER_FREE_CHECK /* Stores freed memory, to check if it is ever written to again. */
/*#define FORGE_MEMORY_CHECK_ALWAYS /* calls f_debug_mem_check_bounds(); any time malloc, free or realloc is called. Very slow but good for narrowing down bugs. */
#define FORGE_CALL_ON_ERROR  abort(); // Set to anything you want to happen when the memory debugger finds an error. (can be abort(), exit(1) or a function with a break point in it for example.) */

#define FORGE_STACK_SIZE (1024 * 1024) /* a guestimate size of that stack that Forge will use to guestimate if a pointer is a stack pointer */

#ifdef __cplusplus
extern "C" {
#endif

extern void		f_debug_mem_thread_safe_init(int (*lock)(void *mutex), int (*unlock)(void *mutex), void *mutex);  /* Required for memory debugger to be thread safe. If you dont use threads you do not need to initialize. */

/*
	Sample code for initiailizing using posix therads;

	pthread_mutex_t *mutex;
	pthread_mutex_init(mutex, NULL);
	f_debug_mem_thread_safe_init(pthread_mutex_lock, pthread_mutex_unlock, mutex);
*/

extern void		f_debug_mem_active(boolean active); /* Enable/disable debugging. Basic error tracking will still execute if disabled, but printouts, loging and f_debug_mem_check_ functions will ignore memory allocated / freed when disabled.  Active by default. */

/* replacement functions */

extern void		*f_debug_mem_malloc(size_t size, const char *file, unsigned int line); /* Replaces malloc and records the c file and line where it was called */
extern void		*f_debug_mem_calloc(size_t num, size_t size, const char *file, unsigned int line); /* Replaces calloc and records the c file and line where it was called */
extern void		*f_debug_mem_realloc(void *pointer, size_t size, const char *file, unsigned int line); /* Replaces realloc and records the c file and line where it was called*/
extern void		f_debug_mem_free(void *buf, const char *file, unsigned int line); /* Replaces free and records the c file and line where it was called*/

/* memory debugging utilities */

extern boolean	f_debug_mem_comment(void *buf, char *comment); /* add a comment to an allocation that can help identyfy its use. */
extern void		f_debug_mem_print(unsigned int min_allocs); /* Prints out a list of Iall allocations made, their location, how much memory each has allocated, freed, and how many allocations have been made. The min_allocs parameter can be set to avoid printing any allocations that have been made fewer times then min_allocs */
extern size_t	f_debug_mem_consumption(void); /* add up all memory consumed by mallocs and reallocs coverd by the memory debugger .*/

/* query pointers */

extern void		*f_debug_mem_query_allocation(void *pointer, unsigned int *line, char **file, size_t *size); /* query the size and place of allocation of a pointer, and returns the base pointer of the allocation. */
extern boolean	f_debug_mem_query_is_allocated(void *pointer, size_t size, boolean ignore_not_found); /* query if a bit of memory has been allocated on the heap. */

/* functions to verify memory */

extern boolean	f_debug_mem_check_bounds(); /* f_debug_mem_check_bounds checks if any of the bounds of any allocation has been over written and reports where to standard out. The function returns TRUE if any error was found*/
extern boolean	f_debug_mem_check_stack_reference(); /* check if any allocated memory contains any pointers that points to suspected stack pointers. */
extern void		f_debug_mem_check_heap_reference(unsigned int minimum_allocations); /* Find any allocations that cant be found any reference to in heap memory. ignores any allocation that has been made less than minimum_allocations times */

/* logging */

extern void		f_debug_mem_log(void *file_pointer); /* Give a FILE pointer to this function to print out a log line every time memory is allocated, freed or realloced. Set to NULL to turn off. */

#ifdef __cplusplus
}
#endif

#ifdef FORGE_MEMORY_DEBUG

#define malloc(n) f_debug_mem_malloc(n, __FILE__, __LINE__) /* Replaces malloc. */
#define calloc(n, m) f_debug_mem_calloc(n, m, __FILE__, __LINE__) /* Replaces calloc. */
#define realloc(n, m) f_debug_mem_realloc(n, m, __FILE__, __LINE__) /* Replaces realloc. */
#define free(n) f_debug_mem_free(n, __FILE__, __LINE__) /* Replaces free. */

#else
#ifndef F_MEMORY_INTERNAL

/* if the memory debugging system is turned off, all calls to debug it are removed. */

#define f_debug_mem_thread_safe_init(n, m, k)
#define f_debug_mem_comment(n, m)
#define f_debug_mem_print(n)
#define f_debug_mem_reset()
#define f_debug_mem_consumption() 0
#define f_debug_mem_query_allocation(n, m, k, l)
#define f_debug_mem_query_is_allocated(n, m, k)
#define f_debug_mem_check_bounds()
#define f_debug_mem_check_stack_reference()
#define f_debug_mem_check_heap_reference(n)
#define f_debug_mem_log(n)

#endif
#endif