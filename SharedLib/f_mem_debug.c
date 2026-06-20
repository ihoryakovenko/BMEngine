// Disable specific Visual Studio warnings for this file
#pragma warning(push)
#pragma warning(disable: 6308) // 'realloc' might return null pointer
#pragma warning(disable: 6011) // Dereferencing NULL pointer
#pragma warning(disable: 28182) // Dereferencing NULL pointer (same as 6011)
#pragma warning(disable: 6271) // Extra argument passed to 'printf'

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#define F_MEMORY_INTERNAL
#define F_NO_MEMORY_DEBUG
#include "forge_memory_debugger.h"

extern void f_debug_mem_print(unsigned int min_allocs);

#define FALSE 0
#define TRUE !FALSE

#define F_MEMORY_MAGIC_NUMBER 0xCF
#define F_MEMORY_INITIALIZATION 0xCD
#define F_MEMORY_FREED 0xCE

typedef struct{
	size_t size;
	void *buf;
	char *comment;
	boolean active;
}ForgeMemAllocBuf;

typedef struct{
	unsigned int line;
	char file[256];
	ForgeMemAllocBuf *allocs;
	unsigned int alloc_count;
	size_t alloc_alocated;
	size_t size;
	size_t alocated;
	size_t freed;
}ForgeMemAllocLine;

#define FORGE_FREE_POINTER_BUFFER_SIZE 1024

typedef struct{
	unsigned int alloc_line;
	char alloc_file[256];
	unsigned int free_line;
	char free_file[256];
	size_t size;
	void *pointer; 
	boolean realloc;
	boolean active;
}ForgeMemFreeBuf;

typedef struct{
	FILE *forge_memory_log_file;
	boolean forge_memory_active;
	unsigned char *forge_memory_stack_pointer;
	size_t forge_memory_stack_size;
	ForgeMemAllocLine *f_alloc_lines;
	unsigned int f_alloc_line_count;
	ForgeMemFreeBuf *f_freed_memory;
	unsigned int f_freed_memory_count;
	unsigned int f_freed_memory_store;
	void *f_alloc_mutex;
	int (*f_alloc_mutex_lock)(void *mutex);
	int (*f_alloc_mutex_unlock)(void *mutex);
	size_t total_heap_memory_allocated;
	size_t total_allocation_count;
}forge_mem_debug;

forge_mem_debug g_forge_mem_debug = {
	.forge_memory_log_file = NULL,
	.forge_memory_active = TRUE,
	.forge_memory_stack_pointer = NULL,
	.forge_memory_stack_size = 0,
	.f_alloc_lines = NULL,
	.f_alloc_line_count = 0,
	.f_freed_memory = NULL,
	.f_freed_memory_count = 0,
	.f_freed_memory_store = 1024,
	.f_alloc_mutex = NULL,
	.f_alloc_mutex_lock = NULL,
	.f_alloc_mutex_unlock = NULL,
	.total_heap_memory_allocated = 0,
	.total_allocation_count = 0
};

void f_debug_mem_thread_safe_init(int (*lock)(void *mutex), int (*unlock)(void *mutex), void *mutex)
{
	g_forge_mem_debug.f_alloc_mutex = mutex;
	g_forge_mem_debug.f_alloc_mutex_lock = lock;
	g_forge_mem_debug.f_alloc_mutex_unlock = unlock;
}


void f_debug_mem_stack_pointer_set(void *lowest_stack_pinter, size_t stack_size_in_bytes)
{
	g_forge_mem_debug.forge_memory_stack_pointer = lowest_stack_pinter;
	g_forge_mem_debug.forge_memory_stack_size = stack_size_in_bytes;
}

void f_debug_mem_active(boolean active)
{
	if(g_forge_mem_debug.f_alloc_mutex != NULL)
		g_forge_mem_debug.f_alloc_mutex_lock(g_forge_mem_debug.f_alloc_mutex);
	g_forge_mem_debug.forge_memory_active = active;
	if(g_forge_mem_debug.f_alloc_mutex != NULL)
		g_forge_mem_debug.f_alloc_mutex_unlock(g_forge_mem_debug.f_alloc_mutex);
}

void f_debug_mem_log(void *file_pointer)
{
	if(g_forge_mem_debug.f_alloc_mutex != NULL)
		g_forge_mem_debug.f_alloc_mutex_lock(g_forge_mem_debug.f_alloc_mutex);
	g_forge_mem_debug.forge_memory_log_file = file_pointer;
	if(g_forge_mem_debug.f_alloc_mutex != NULL)
		g_forge_mem_debug.f_alloc_mutex_unlock(g_forge_mem_debug.f_alloc_mutex);
}

boolean f_debug_mem_check_bounds()
{
	boolean output = FALSE;
	size_t size;
	unsigned char *buf;
	size_t i, j, k;
	if(g_forge_mem_debug.f_alloc_mutex != NULL)
		g_forge_mem_debug.f_alloc_mutex_lock(g_forge_mem_debug.f_alloc_mutex);
	for(i = 0; i < g_forge_mem_debug.f_alloc_line_count; i++)
	{
		for(j = 0; j < g_forge_mem_debug.f_alloc_lines[i].alloc_count; j++)
		{
			if(g_forge_mem_debug.f_alloc_lines[i].allocs[j].active)
			{
				buf = g_forge_mem_debug.f_alloc_lines[i].allocs[j].buf;
				size = g_forge_mem_debug.f_alloc_lines[i].allocs[j].size;
				for(k = 0; k < FORGE_MEMORY_OVER_ALLOC - FORGE_MEMORY_PRE_PADDIG; k++)
					if(buf[size + k] != F_MEMORY_MAGIC_NUMBER)
						break;
				if(k < FORGE_MEMORY_OVER_ALLOC - FORGE_MEMORY_PRE_PADDIG)
				{
					if(g_forge_mem_debug.f_alloc_lines[i].allocs[j].comment == NULL)
						printf("FORGE Mem debugger error: Memory Overrun of allocation made on line %u in file %s\n", g_forge_mem_debug.f_alloc_lines[i].line, g_forge_mem_debug.f_alloc_lines[i].file);
					else
						printf("FORGE Mem debugger error: Memory Overrun of allocation made on  line %u in file %s /* %s */\n", g_forge_mem_debug.f_alloc_lines[i].line, g_forge_mem_debug.f_alloc_lines[i].file, g_forge_mem_debug.f_alloc_lines[i].allocs[j].comment);
					FORGE_CALL_ON_ERROR
					output = TRUE;
				}
				buf = ((unsigned char*)g_forge_mem_debug.f_alloc_lines[i].allocs[j].buf) - FORGE_MEMORY_PRE_PADDIG;
				size = g_forge_mem_debug.f_alloc_lines[i].allocs[j].size;
				for(k = 0; k < FORGE_MEMORY_PRE_PADDIG; k++)
					if(buf[k] != F_MEMORY_MAGIC_NUMBER)
						break;
				if(k < FORGE_MEMORY_PRE_PADDIG)
				{
					if(g_forge_mem_debug.f_alloc_lines[i].allocs[j].comment == NULL)
						printf("FORGE Mem debugger error: Memory underrun of allocation made on line %u in file %s\n", g_forge_mem_debug.f_alloc_lines[i].line, g_forge_mem_debug.f_alloc_lines[i].file);
					else
						printf("FORGE Mem debugger error: Memory underrun of allocation made on line %u in file %s /* %s */\n", g_forge_mem_debug.f_alloc_lines[i].line, g_forge_mem_debug.f_alloc_lines[i].file, g_forge_mem_debug.f_alloc_lines[i].allocs[j].comment);
					FORGE_CALL_ON_ERROR
					output = TRUE;
				}
			}
		}
	}
#ifdef FORGE_USE_AFTER_FREE_CHECK
	for(i = 0; i < g_forge_mem_debug.f_freed_memory_count && i < g_forge_mem_debug.f_freed_memory_store; i++)
	{
		buf = g_forge_mem_debug.f_freed_memory[i].pointer;
		size = g_forge_mem_debug.f_freed_memory[i].size;
		for(k = 0; k < size && buf[k] == F_MEMORY_FREED; k++);
		if(k < size)
		{
			if(g_forge_mem_debug.f_freed_memory[i].realloc)
				printf("FORGE Mem debugger error: Pointer that was reallocated on line %u in file %s, and freed on line %u in file %s was written to %u bytes in to the allocation after being freed.\n", 
					g_forge_mem_debug.f_freed_memory[i].alloc_line, g_forge_mem_debug.f_freed_memory[i].alloc_file,
					g_forge_mem_debug.f_freed_memory[i].free_line, g_forge_mem_debug.f_freed_memory[i].free_file, (unsigned int)k);
			else
				printf("FORGE Mem debugger error: Pointer that was allocated on line %u in file %s, and freed on line %u in file %s was written to %u bytes in to the allocation after being freed.\n", 
					g_forge_mem_debug.f_freed_memory[i].alloc_line, g_forge_mem_debug.f_freed_memory[i].alloc_file,
					g_forge_mem_debug.f_freed_memory[i].free_line, g_forge_mem_debug.f_freed_memory[i].free_file, (unsigned int)k);
			FORGE_CALL_ON_ERROR
		}
	}
#endif
	if(g_forge_mem_debug.f_alloc_mutex != NULL)
		g_forge_mem_debug.f_alloc_mutex_unlock(g_forge_mem_debug.f_alloc_mutex);
	return output;
}

void f_debug_mem_add(void *pointer, size_t size, const char *file, unsigned int line)
{
	unsigned int i, j;
	unsigned char *pre;
	pre = ((unsigned char *)pointer) - FORGE_MEMORY_PRE_PADDIG;
	for(i = 0; i < FORGE_MEMORY_PRE_PADDIG; i++)
		((unsigned char *)pre)[i] = F_MEMORY_MAGIC_NUMBER;

	for(i = 0; i < FORGE_MEMORY_OVER_ALLOC - FORGE_MEMORY_PRE_PADDIG; i++)
		((unsigned char *)pointer)[size + i] = F_MEMORY_MAGIC_NUMBER;

	for(i = 0; i < g_forge_mem_debug.f_alloc_line_count; i++)
	{
		if(line == g_forge_mem_debug.f_alloc_lines[i].line)
		{
			for(j = 0; file[j] != 0 && file[j] == g_forge_mem_debug.f_alloc_lines[i].file[j] ; j++);
			if(file[j] == g_forge_mem_debug.f_alloc_lines[i].file[j])
				break;
		}
	}
	if(i < g_forge_mem_debug.f_alloc_line_count)
	{
		if(g_forge_mem_debug.f_alloc_lines[i].alloc_alocated == g_forge_mem_debug.f_alloc_lines[i].alloc_count)
		{
			g_forge_mem_debug.f_alloc_lines[i].alloc_alocated += 1024;
			g_forge_mem_debug.f_alloc_lines[i].allocs = realloc(g_forge_mem_debug.f_alloc_lines[i].allocs, (sizeof *g_forge_mem_debug.f_alloc_lines[i].allocs) * g_forge_mem_debug.f_alloc_lines[i].alloc_alocated);
			if(g_forge_mem_debug.f_alloc_lines[i].allocs == NULL)
			{			
				printf("FORGE Mem debugger error: Realloc returns NULL when trying to allocate %u bytes at line %u in file %s\n", (unsigned int)size, line, file);
				FORGE_CALL_ON_ERROR
				return;
			}
		}
		g_forge_mem_debug.f_alloc_lines[i].allocs[g_forge_mem_debug.f_alloc_lines[i].alloc_count].size = size;
		g_forge_mem_debug.f_alloc_lines[i].allocs[g_forge_mem_debug.f_alloc_lines[i].alloc_count].comment = NULL;
		g_forge_mem_debug.f_alloc_lines[i].allocs[g_forge_mem_debug.f_alloc_lines[i].alloc_count].active = g_forge_mem_debug.forge_memory_active;
		g_forge_mem_debug.f_alloc_lines[i].allocs[g_forge_mem_debug.f_alloc_lines[i].alloc_count++].buf = pointer;
		if(g_forge_mem_debug.forge_memory_active)
		{
			g_forge_mem_debug.f_alloc_lines[i].size += size;
			g_forge_mem_debug.f_alloc_lines[i].alocated++;
		}
	}else
	{
		if(i % 1024 == 0)
			g_forge_mem_debug.f_alloc_lines = realloc(g_forge_mem_debug.f_alloc_lines, (sizeof *g_forge_mem_debug.f_alloc_lines) * (i + 1024));
		g_forge_mem_debug.f_alloc_lines[i].line = line;
		for(j = 0; j < 255 && file[j] != 0; j++)
			g_forge_mem_debug.f_alloc_lines[i].file[j] = file[j];
		g_forge_mem_debug.f_alloc_lines[i].file[j] = 0;
		g_forge_mem_debug.f_alloc_lines[i].alloc_alocated = 256;
		g_forge_mem_debug.f_alloc_lines[i].allocs = malloc((sizeof *g_forge_mem_debug.f_alloc_lines[i].allocs) * g_forge_mem_debug.f_alloc_lines[i].alloc_alocated);
		g_forge_mem_debug.f_alloc_lines[i].allocs[0].size = size;
		g_forge_mem_debug.f_alloc_lines[i].allocs[0].buf = pointer;
		g_forge_mem_debug.f_alloc_lines[i].allocs[0].comment = NULL;
		g_forge_mem_debug.f_alloc_lines[i].alloc_count = 1;
		g_forge_mem_debug.f_alloc_lines[i].freed = 0;
		if(g_forge_mem_debug.forge_memory_active)
		{
			g_forge_mem_debug.f_alloc_lines[i].alocated = 1;
			g_forge_mem_debug.f_alloc_lines[i].size = size;
		}else
		{
			g_forge_mem_debug.f_alloc_lines[i].alocated = 0;
			g_forge_mem_debug.f_alloc_lines[i].size = 0;
		}
		g_forge_mem_debug.f_alloc_line_count++;
	}
}

void *f_debug_mem_malloc(size_t size, const char *file, unsigned int line)
{
	unsigned char *pointer;
#ifdef FORGE_MEMORY_CHECK_ALWAYS
	f_debug_mem_check_bounds();
#endif
	if(g_forge_mem_debug.f_alloc_mutex != NULL)
		g_forge_mem_debug.f_alloc_mutex_lock(g_forge_mem_debug.f_alloc_mutex);
	if(size == 0)
	{
		printf("FORGE Mem debugger warning: malloc size ZERO in file %s line %u\n", file, line);
		FORGE_CALL_ON_ERROR
		if(g_forge_mem_debug.f_alloc_mutex != NULL)
			g_forge_mem_debug.f_alloc_mutex_unlock(g_forge_mem_debug.f_alloc_mutex);
		return NULL;
	}
	pointer = malloc(size + FORGE_MEMORY_OVER_ALLOC);

	if(g_forge_mem_debug.forge_memory_log_file != NULL && g_forge_mem_debug.forge_memory_active)
		fprintf(g_forge_mem_debug.forge_memory_log_file, "malloc %u bytes at pointer %p at %s line %u\n", (unsigned int)size, pointer, file, line);

	if(pointer == NULL)
	{
#ifdef FORGE_MEMORY_NULL_ALLOCATION_ERROR
		if(size > (size_t)(~((unsigned int)0)))
			printf("FORGE Mem debugger warning: malloc returns NULL at line %u in file %s\n", line, file);
		else
			printf("FORGE Mem debugger warning: malloc returns NULL when trying to allocate %u bytes at line %u in file %s\n", (unsigned int)size, line, file);
		if(g_forge_mem_debug.f_alloc_mutex != NULL)
			g_forge_mem_debug.f_alloc_mutex_unlock(g_forge_mem_debug.f_alloc_mutex);
		FORGE_CALL_ON_ERROR
#endif
		if(g_forge_mem_debug.f_alloc_mutex != NULL)
			g_forge_mem_debug.f_alloc_mutex_unlock(g_forge_mem_debug.f_alloc_mutex);
		return NULL;
	}
	pointer += FORGE_MEMORY_PRE_PADDIG;
	memset(pointer, F_MEMORY_INITIALIZATION, size);
	f_debug_mem_add(pointer, size, file, line);
	
	// Track total memory allocation
	g_forge_mem_debug.total_heap_memory_allocated += size;
	g_forge_mem_debug.total_allocation_count++;
	
	if(g_forge_mem_debug.f_alloc_mutex != NULL)
		g_forge_mem_debug.f_alloc_mutex_unlock(g_forge_mem_debug.f_alloc_mutex);
	return pointer;
}

void *f_debug_mem_calloc(size_t num, size_t size, const char *file, unsigned int line)
{
	unsigned char *pointer;
#ifdef FORGE_MEMORY_CHECK_ALWAYS
	f_debug_mem_check_bounds();
#endif
	if(g_forge_mem_debug.f_alloc_mutex != NULL)
		g_forge_mem_debug.f_alloc_mutex_lock(g_forge_mem_debug.f_alloc_mutex);
	size *= num;
	if(size == 0)
	{
		printf("FORGE Mem debugger warning: calloc size ZERO in file %s line %u\n", file, line);
		FORGE_CALL_ON_ERROR

		if(g_forge_mem_debug.f_alloc_mutex != NULL)
			g_forge_mem_debug.f_alloc_mutex_unlock(g_forge_mem_debug.f_alloc_mutex);
		return NULL;
	}


	pointer = malloc(size + FORGE_MEMORY_OVER_ALLOC);

	if(g_forge_mem_debug.forge_memory_log_file != NULL && g_forge_mem_debug.forge_memory_active)
		fprintf(g_forge_mem_debug.forge_memory_log_file, "Calloc %u bytes at pointer %p at %s line %u\n", (unsigned int)size, pointer, file, line);

	if(pointer == NULL)
	{
#ifdef FORGE_MEMORY_NULL_ALLOCATION_ERROR
		if(size > (size_t)(~((unsigned int)0)))
			printf("FORGE Mem debugger Warning: Calloc returns NULL at line %u in file %s\n", line, file);
		else
			printf("FORGE Mem debugger Warning: Calloc returns NULL when trying to allocate %u bytes at line %u in file %s\n", (unsigned int)size, line, file);
		if(g_forge_mem_debug.f_alloc_mutex != NULL)
			g_forge_mem_debug.f_alloc_mutex_unlock(g_forge_mem_debug.f_alloc_mutex);
		FORGE_CALL_ON_ERROR
#endif

		if(g_forge_mem_debug.f_alloc_mutex != NULL)
			g_forge_mem_debug.f_alloc_mutex_unlock(g_forge_mem_debug.f_alloc_mutex);
		return NULL;
	}
	pointer += FORGE_MEMORY_PRE_PADDIG;
	memset(pointer, 0, size);
	f_debug_mem_add(pointer, size, file, line);
	
	// Track total memory allocation
	g_forge_mem_debug.total_heap_memory_allocated += size;
	g_forge_mem_debug.total_allocation_count++;
	
	if(g_forge_mem_debug.f_alloc_mutex != NULL)
		g_forge_mem_debug.f_alloc_mutex_unlock(g_forge_mem_debug.f_alloc_mutex);
	return pointer;
}

boolean f_debug_mem_remove(unsigned char *buf, const char *file, unsigned int line, boolean realloced, size_t *size)
{
	ForgeMemFreeBuf  *f;
	unsigned int i, j, k;
	size_t distance;

#if defined(FORGE_DOUBLE_FREE_CHECK) || defined(FORGE_USE_AFTER_FREE_CHECK) 
	if(g_forge_mem_debug.f_freed_memory_count % 1024 == 0 && g_forge_mem_debug.f_freed_memory_count < g_forge_mem_debug.f_freed_memory_store)
	{
		if(g_forge_mem_debug.f_freed_memory_count == 0)
			g_forge_mem_debug.f_freed_memory = malloc((sizeof *g_forge_mem_debug.f_freed_memory) * 1024);
		else
			g_forge_mem_debug.f_freed_memory = realloc(g_forge_mem_debug.f_freed_memory, (sizeof *g_forge_mem_debug.f_freed_memory) * (g_forge_mem_debug.f_freed_memory_count + 1024));
	}
#ifdef FORGE_USE_AFTER_FREE_CHECK
	if(g_forge_mem_debug.f_freed_memory_count >= g_forge_mem_debug.f_freed_memory_store)
	{
		if(g_forge_mem_debug.f_freed_memory[g_forge_mem_debug.f_freed_memory_count % g_forge_mem_debug.f_freed_memory_store].pointer != NULL)
		{
			free(((unsigned char *)g_forge_mem_debug.f_freed_memory[g_forge_mem_debug.f_freed_memory_count % g_forge_mem_debug.f_freed_memory_store].pointer) - FORGE_MEMORY_PRE_PADDIG);	
			g_forge_mem_debug.f_freed_memory[g_forge_mem_debug.f_freed_memory_count % g_forge_mem_debug.f_freed_memory_store].pointer = NULL;
		}
	}
#endif
	f = &g_forge_mem_debug.f_freed_memory[g_forge_mem_debug.f_freed_memory_count++ % g_forge_mem_debug.f_freed_memory_store];
	for(i = 0; i < 255 && file[i] != 0; i++)
		f->free_file[i] = file[i];
	f->free_file[i] = 0;
	f->free_line = line;
	f->realloc = realloced;
	f->size = 0;
	f->pointer = buf;
	f->active = g_forge_mem_debug.forge_memory_active;

#endif
	for(i = 0; i < g_forge_mem_debug.f_alloc_line_count; i++)
	{
		for(j = 0; j < g_forge_mem_debug.f_alloc_lines[i].alloc_count; j++)
		{
			if(g_forge_mem_debug.f_alloc_lines[i].allocs[j].buf == buf)
			{
				buf -= FORGE_MEMORY_PRE_PADDIG;
				for(k = 0; k < FORGE_MEMORY_PRE_PADDIG; k++)
					if(((unsigned char *)buf)[k] != F_MEMORY_MAGIC_NUMBER)
						break;
				if(k < FORGE_MEMORY_PRE_PADDIG)
				{
					unsigned int *a = NULL;
					printf("FORGE Mem debugger error: Buffer underrun of allocation made on line %u in file %s\n", g_forge_mem_debug.f_alloc_lines[i].line, g_forge_mem_debug.f_alloc_lines[i].file);
					FORGE_CALL_ON_ERROR;
				}
				for(k = 0; k < FORGE_MEMORY_OVER_ALLOC - FORGE_MEMORY_PRE_PADDIG; k++)
					if(((unsigned char *)buf)[g_forge_mem_debug.f_alloc_lines[i].allocs[j].size + FORGE_MEMORY_PRE_PADDIG + k] != F_MEMORY_MAGIC_NUMBER)
						break;
				if(k < FORGE_MEMORY_OVER_ALLOC - FORGE_MEMORY_PRE_PADDIG)
				{
					unsigned int *a = NULL;
					printf("FORGE Mem debugger error: Buffer overrun of allocation made on line  %u in file %s\n", g_forge_mem_debug.f_alloc_lines[i].line, g_forge_mem_debug.f_alloc_lines[i].file);
					FORGE_CALL_ON_ERROR;
				}
				memset(buf, F_MEMORY_FREED, g_forge_mem_debug.f_alloc_lines[i].allocs[j].size + FORGE_MEMORY_OVER_ALLOC);
				f->alloc_line = g_forge_mem_debug.f_alloc_lines[i].line;
				for(k = 0; k < 255 && g_forge_mem_debug.f_alloc_lines[i].file[k] != 0; k++)
					f->alloc_file[k] = g_forge_mem_debug.f_alloc_lines[i].file[k];
				f->alloc_file[k] = 0;
				f->size = g_forge_mem_debug.f_alloc_lines[i].allocs[j].size;
				*size = g_forge_mem_debug.f_alloc_lines[i].allocs[j].size;
				g_forge_mem_debug.f_alloc_lines[i].size -= g_forge_mem_debug.f_alloc_lines[i].allocs[j].size;
				g_forge_mem_debug.f_alloc_lines[i].allocs[j] = g_forge_mem_debug.f_alloc_lines[i].allocs[--g_forge_mem_debug.f_alloc_lines[i].alloc_count];
				if(g_forge_mem_debug.forge_memory_active)
					g_forge_mem_debug.f_alloc_lines[i].freed++;

#ifndef FORGE_USE_AFTER_FREE_CHECK
				free(buf);
#endif
				return TRUE;
			}
			if((unsigned char *)g_forge_mem_debug.f_alloc_lines[i].allocs[j].buf < (unsigned char *)buf && (unsigned char *)g_forge_mem_debug.f_alloc_lines[i].allocs[j].buf + g_forge_mem_debug.f_alloc_lines[i].allocs[j].size > (unsigned char *)buf)
			{
				printf("FORGE Mem debugger error: Trying to free pointer %p that is not at the start (%i bytes in) of allocation made on line %u in file %s\n", g_forge_mem_debug.f_freed_memory[i].pointer, (int)((unsigned char *)buf - (unsigned char *)g_forge_mem_debug.f_alloc_lines[i].allocs[j].buf), g_forge_mem_debug.f_freed_memory[i].free_line, g_forge_mem_debug.f_freed_memory[i].free_file);
				FORGE_CALL_ON_ERROR;
				return FALSE;
			}
		}	
	}
	for(i = 0; i < g_forge_mem_debug.f_freed_memory_count && i < g_forge_mem_debug.f_freed_memory_store; i++)
	{
		if(f != &g_forge_mem_debug.f_freed_memory[i] && buf == g_forge_mem_debug.f_freed_memory[i].pointer)
		{
			unsigned int *a = NULL;
			if(f->realloc)
				printf("FORGE Mem debugger error: Pointer %p s freed twice! it was freed on line %u in %s, was reallocated (%u bytes) on line %u in file %s\n", g_forge_mem_debug.f_freed_memory[i].pointer, g_forge_mem_debug.f_freed_memory[i].free_line, g_forge_mem_debug.f_freed_memory[i].free_file, (unsigned int)g_forge_mem_debug.f_freed_memory[i].size, g_forge_mem_debug.f_freed_memory[i].alloc_line, g_forge_mem_debug.f_freed_memory[i].alloc_file);
			else
				printf("FORGE Mem debugger error: Pointer %p is freed twice! it was freed on line %u in %s, was allocated (%u bytes) on line %u in file %s\n", g_forge_mem_debug.f_freed_memory[i].pointer, g_forge_mem_debug.f_freed_memory[i].free_line, g_forge_mem_debug.f_freed_memory[i].free_file, (unsigned int)g_forge_mem_debug.f_freed_memory[i].size, g_forge_mem_debug.f_freed_memory[i].alloc_line, g_forge_mem_debug.f_freed_memory[i].alloc_file);
			FORGE_CALL_ON_ERROR;
			return FALSE;
		}
	}
	if(g_forge_mem_debug.forge_memory_stack_size != 0)
	{
		if(g_forge_mem_debug.forge_memory_stack_pointer <= buf && &g_forge_mem_debug.forge_memory_stack_pointer[g_forge_mem_debug.forge_memory_stack_size] > buf)
		{
			printf("FORGE Mem debugger error: Trying to free stack pointer on line %u in file %s.\n", line, file);
			FORGE_CALL_ON_ERROR;
			return TRUE;
		}
	}else
	{
		if(buf > (unsigned char *)&i)
			distance = buf - (unsigned char *)&i;
		else
			distance = (unsigned char *)&i - buf;
		if(distance < FORGE_STACK_SIZE)
		{
			printf("FORGE Mem debugger error: Trying to free pointer, not allocated as far as FORGE knows on line %u in file %s. Very likly to be stack pointer (%u bytes from known stack pointer)\n", line, file, (unsigned int)distance);
			FORGE_CALL_ON_ERROR;
			return TRUE;
		}
	}

	printf("FORGE Mem debugger Warning: Trying to free pointer, not allocated as far as FORGE knows on line %u in file %s. \n", line, file);
	free(buf);

	return TRUE;
}



void f_debug_mem_free(void *buf, const char *file, unsigned int line)
{
	size_t size = 0;
#ifdef FORGE_MEMORY_CHECK_ALWAYS
	f_debug_mem_check_bounds();
#endif
	if(g_forge_mem_debug.f_alloc_mutex != NULL)
		g_forge_mem_debug.f_alloc_mutex_lock(g_forge_mem_debug.f_alloc_mutex);
	if(f_debug_mem_remove(buf, file, line, FALSE, &size))
		if(g_forge_mem_debug.forge_memory_log_file != NULL && g_forge_mem_debug.forge_memory_active)
			fprintf(g_forge_mem_debug.forge_memory_log_file, "Free %u bytes at pointer %p at %s line %u\n", (unsigned int)size, buf, file, line);

	if(g_forge_mem_debug.f_alloc_mutex != NULL)
		g_forge_mem_debug.f_alloc_mutex_unlock(g_forge_mem_debug.f_alloc_mutex);
}

boolean f_debug_mem_comment(void *buf, char *comment)
{
	unsigned int i, j;
	if(g_forge_mem_debug.f_alloc_mutex != NULL)
		g_forge_mem_debug.f_alloc_mutex_lock(g_forge_mem_debug.f_alloc_mutex);
	for(i = 0; i < g_forge_mem_debug.f_alloc_line_count; i++)
	{
		for(j = 0; j < g_forge_mem_debug.f_alloc_lines[i].alloc_count; j++)
		{
			if(g_forge_mem_debug.f_alloc_lines[i].allocs[j].buf == buf)
			{
				g_forge_mem_debug.f_alloc_lines[i].allocs[j].comment = comment;
				if(g_forge_mem_debug.f_alloc_mutex != NULL)
					g_forge_mem_debug.f_alloc_mutex_unlock(g_forge_mem_debug.f_alloc_mutex);
				return TRUE;
			}
		}
	}
	if(g_forge_mem_debug.f_alloc_mutex != NULL)
		g_forge_mem_debug.f_alloc_mutex_unlock(g_forge_mem_debug.f_alloc_mutex);
	return FALSE;
}


void *f_debug_mem_realloc(void *pointer, size_t size, const char *file, unsigned int line)
{
	size_t i, j = 0, k, move;
	unsigned char *pointer2;	
#ifdef FORGE_MEMORY_CHECK_ALWAYS
	f_debug_mem_check_bounds();
#endif

	if(pointer == NULL)
	{
#ifdef FORGE_WARN_ON_REALLOC_NULL
		printf("FORGE Mem debugger warning: Realocating NULL in %s line %u. UB since C23.\n", file, line);
#endif
		return f_debug_mem_malloc(size, file, line);
	}
	
	if(size == 0)
	{
		printf("FORGE Mem debugger warning: realloc size ZERO in file %s line %u\n", file, line);
		FORGE_CALL_ON_ERROR
		return NULL;
	}

	if(g_forge_mem_debug.f_alloc_mutex != NULL)
		g_forge_mem_debug.f_alloc_mutex_lock(g_forge_mem_debug.f_alloc_mutex);
	for(i = 0; i < g_forge_mem_debug.f_alloc_line_count; i++)
	{
		for(j = 0; j < g_forge_mem_debug.f_alloc_lines[i].alloc_count; j++)
			if(g_forge_mem_debug.f_alloc_lines[i].allocs[j].buf == pointer)
				break;
		if(j < g_forge_mem_debug.f_alloc_lines[i].alloc_count)
			break;
	}
	if(i == g_forge_mem_debug.f_alloc_line_count)
	{
		printf("FORGE Mem debugger error: Trying to reallocate pointer %p in %s line %u. Pointer is not allocated.", pointer, file, line);
		for(i = 0; i < g_forge_mem_debug.f_alloc_line_count; i++)
		{
			for(j = 0; j < g_forge_mem_debug.f_alloc_lines[i].alloc_count; j++)
			{
				unsigned int *buf;
				buf = g_forge_mem_debug.f_alloc_lines[i].allocs[j].buf;
				for(k = 0; k < g_forge_mem_debug.f_alloc_lines[i].allocs[j].size; k++)
				{
					if(&buf[k] == pointer)
					{
						printf("Trying to reallocate pointer %u bytes (out of %u) in to allocation made in %s on line %u.\n", (unsigned int)k, (unsigned int)g_forge_mem_debug.f_alloc_lines[i].allocs[j].size, g_forge_mem_debug.f_alloc_lines[i].file, g_forge_mem_debug.f_alloc_lines[i].line);
						FORGE_CALL_ON_ERROR;
						if(g_forge_mem_debug.f_alloc_mutex != NULL)
							g_forge_mem_debug.f_alloc_mutex_unlock(g_forge_mem_debug.f_alloc_mutex);
						return NULL;
					}
				}
			}
		}

		printf("\n");
		FORGE_CALL_ON_ERROR;

#ifdef FORGE_WARN_ON_REALLOC_NULL
		if(size == 0)
			printf("FORGE Mem debugger warning. Realocating pointer to zero size on line %u in file %s. UB since C23.\n", line, file);
#endif;
		return realloc(pointer, size);
	}
	if(size == 0)
	{
#ifdef FORGE_WARN_ON_REALLOC_NULL
		printf("FORGE Mem debugger warning. Realocating pointer to zero size on line %u in file %s. UB since C23.\n", line, file);
#endif
		f_debug_mem_free(pointer, file, line);
		return NULL;
	}


	pointer2 = malloc(size + FORGE_MEMORY_OVER_ALLOC);
	if(pointer2 == NULL)
	{
		printf("FORGE Mem debugger warning: Realloc returns NULL when trying to allocate %u bytes at line %u in file %s\n", (unsigned int)size, line, file);
		FORGE_CALL_ON_ERROR;
		if(g_forge_mem_debug.f_alloc_mutex != NULL)
			g_forge_mem_debug.f_alloc_mutex_unlock(g_forge_mem_debug.f_alloc_mutex);
		return NULL;
	}
	memset(pointer2, F_MEMORY_MAGIC_NUMBER, FORGE_MEMORY_PRE_PADDIG);
	move = g_forge_mem_debug.f_alloc_lines[i].allocs[j].size;
	if(move > size)
	{
		move = size;
		memcpy(&pointer2[FORGE_MEMORY_PRE_PADDIG], pointer, move);
		memset(&pointer2[FORGE_MEMORY_PRE_PADDIG + move], F_MEMORY_MAGIC_NUMBER, FORGE_MEMORY_OVER_ALLOC - FORGE_MEMORY_PRE_PADDIG);
	}else
	{
		memcpy(&pointer2[FORGE_MEMORY_PRE_PADDIG], pointer, move);
		memset(&pointer2[FORGE_MEMORY_PRE_PADDIG + move], F_MEMORY_INITIALIZATION, size - move);
		memset(&pointer2[FORGE_MEMORY_PRE_PADDIG + size], F_MEMORY_MAGIC_NUMBER, FORGE_MEMORY_OVER_ALLOC - FORGE_MEMORY_PRE_PADDIG);
	}
	pointer2 += FORGE_MEMORY_PRE_PADDIG;
	f_debug_mem_add(pointer2, size, file, line);
	move = 0;
	f_debug_mem_remove(pointer, file, line, TRUE, &move);
	
	// Track reallocation: subtract old size, add new size
	// move contains the old size from f_debug_mem_remove
	g_forge_mem_debug.total_heap_memory_allocated = g_forge_mem_debug.total_heap_memory_allocated - move + size;
	// Note: g_forge_mem_debug.total_allocation_count doesn't change for realloc
	
	if(g_forge_mem_debug.forge_memory_log_file != NULL && g_forge_mem_debug.forge_memory_active)
		fprintf(g_forge_mem_debug.forge_memory_log_file, "Relloc %u bytes at pointer %p to %u bytes at pointer %p at %s line %u\n", (unsigned int)size, pointer, (unsigned int)move, pointer2, file, line);
	if(g_forge_mem_debug.f_alloc_mutex != NULL)
		g_forge_mem_debug.f_alloc_mutex_unlock(g_forge_mem_debug.f_alloc_mutex);
	return pointer2;
}

void f_debug_mem_print(unsigned int min_allocs)
{
	unsigned int i, j, alloc_count;
	if(g_forge_mem_debug.f_alloc_mutex != NULL)
		g_forge_mem_debug.f_alloc_mutex_lock(g_forge_mem_debug.f_alloc_mutex);
	printf("Memory report:\n----------------------------------------------\n");
	
	// Print total lifetime allocations
	printf("Total lifetime allocations: %.2f MB (%zu bytes)\n", 
		g_forge_mem_debug.total_heap_memory_allocated / (1024.0 * 1024.0), g_forge_mem_debug.total_heap_memory_allocated);
	printf("Total allocation count: %zu\n\n", g_forge_mem_debug.total_allocation_count);
	
	for(i = 0; i < g_forge_mem_debug.f_alloc_line_count; i++)
	{
		if(min_allocs < g_forge_mem_debug.f_alloc_lines[i].alocated - g_forge_mem_debug.f_alloc_lines[i].freed)
		{
			for(j = alloc_count = 0; j < g_forge_mem_debug.f_alloc_lines[i].alloc_count; j++)
				if(g_forge_mem_debug.f_alloc_lines[i].allocs[j].active)
					alloc_count++;
			if(alloc_count > 0)
			{
				printf("%s line: %u\n", g_forge_mem_debug.f_alloc_lines[i].file, g_forge_mem_debug.f_alloc_lines[i].line);
				printf(" - Bytes allocated: %u\n - Allocations: %u\n - Frees: %u\n\n", (unsigned int)g_forge_mem_debug.f_alloc_lines[i].size, (unsigned int)alloc_count, (unsigned int)g_forge_mem_debug.f_alloc_lines[i].freed);
				for(j = 0; j < g_forge_mem_debug.f_alloc_lines[i].alloc_count; j++)
					if(g_forge_mem_debug.f_alloc_lines[i].allocs[j].comment != NULL)
						printf("\t\t comment %p : %s\n", g_forge_mem_debug.f_alloc_lines[i].allocs[j].buf, g_forge_mem_debug.f_alloc_lines[i].allocs[j].comment);
			}
		}	
	}
	printf("----------------------------------------------\n");
	if(g_forge_mem_debug.f_alloc_mutex != NULL)
		g_forge_mem_debug.f_alloc_mutex_unlock(g_forge_mem_debug.f_alloc_mutex);
}


size_t f_debug_mem_footprint(unsigned int min_allocs)
{
	unsigned int i;
	size_t size = 0;
	if(g_forge_mem_debug.f_alloc_mutex != NULL)
		g_forge_mem_debug.f_alloc_mutex_lock(g_forge_mem_debug.f_alloc_mutex);
	for(i = 0; i < g_forge_mem_debug.f_alloc_line_count; i++)
		size += g_forge_mem_debug.f_alloc_lines[i].size;
	if(g_forge_mem_debug.f_alloc_mutex != NULL)
		g_forge_mem_debug.f_alloc_mutex_unlock(g_forge_mem_debug.f_alloc_mutex);
	return size;
}

void *f_debug_mem_query_allocation(void *pointer, unsigned int *line, char **file, size_t *size)
{
	unsigned int i, j;  
	if(g_forge_mem_debug.f_alloc_mutex != NULL)
		g_forge_mem_debug.f_alloc_mutex_lock(g_forge_mem_debug.f_alloc_mutex);
	for(i = 0; i < g_forge_mem_debug.f_alloc_line_count; i++)
	{
		for(j = 0; j < g_forge_mem_debug.f_alloc_lines[i].alloc_count; j++)
		{
			if((unsigned char*)g_forge_mem_debug.f_alloc_lines[i].allocs[j].buf <= (unsigned char*)pointer &&
			   (unsigned char *)g_forge_mem_debug.f_alloc_lines[i].allocs[j].buf + g_forge_mem_debug.f_alloc_lines[i].allocs[j].size > (unsigned char *)pointer) /* technically UB */
			{
				if(line != NULL)
					*line = g_forge_mem_debug.f_alloc_lines[i].line;
				if(file != NULL)
					*file = g_forge_mem_debug.f_alloc_lines[i].file;
				if(size != NULL)
					*size = g_forge_mem_debug.f_alloc_lines[i].allocs[j].size;
				if(g_forge_mem_debug.f_alloc_mutex != NULL)
					g_forge_mem_debug.f_alloc_mutex_unlock(g_forge_mem_debug.f_alloc_mutex);
				return g_forge_mem_debug.f_alloc_lines[i].allocs[j].buf;
			}
		}
	}
	if(g_forge_mem_debug.f_alloc_mutex != NULL)
		g_forge_mem_debug.f_alloc_mutex_unlock(g_forge_mem_debug.f_alloc_mutex);
	return NULL;
}

boolean f_debug_mem_query_is_allocated(void *pointer, size_t size, boolean ignore_not_found)
{
	unsigned int i, j;  
	if(g_forge_mem_debug.f_alloc_mutex != NULL)
		g_forge_mem_debug.f_alloc_mutex_lock(g_forge_mem_debug.f_alloc_mutex);
	for(i = 0; i < g_forge_mem_debug.f_alloc_line_count; i++)
	{
		for(j = 0; j < g_forge_mem_debug.f_alloc_lines[i].alloc_count; j++)
		{
			if(g_forge_mem_debug.f_alloc_lines[i].allocs[j].buf >= pointer && ((unsigned char *)g_forge_mem_debug.f_alloc_lines[i].allocs[j].buf) + g_forge_mem_debug.f_alloc_lines[i].allocs[j].size <= (unsigned char *)pointer) /* technically UB */
			{
				if(((unsigned char *)g_forge_mem_debug.f_alloc_lines[i].allocs[j].buf) + g_forge_mem_debug.f_alloc_lines[i].allocs[j].size < ((unsigned char *)pointer) + size)
				{
					printf("FORGE Mem debugger error: Not enough memory to access pointer %p, %u bytes missing\n", pointer, (unsigned int)(((unsigned char *)g_forge_mem_debug.f_alloc_lines[i].allocs[j].buf) + g_forge_mem_debug.f_alloc_lines[i].allocs[j].size) - (unsigned int)(((unsigned char *)pointer) + size));
					if(g_forge_mem_debug.f_alloc_mutex != NULL)
						g_forge_mem_debug.f_alloc_mutex_unlock(g_forge_mem_debug.f_alloc_mutex);
					return FALSE;
				}else
				{
					if(g_forge_mem_debug.f_alloc_mutex != NULL)
						g_forge_mem_debug.f_alloc_mutex_unlock(g_forge_mem_debug.f_alloc_mutex);
					return TRUE;
				}
			}
		}
	}


	for(i = 0; i < g_forge_mem_debug.f_freed_memory_count; i++)
	{
		if(pointer >= g_forge_mem_debug.f_freed_memory[i].pointer && ((unsigned char *)g_forge_mem_debug.f_freed_memory[i].pointer) + g_forge_mem_debug.f_freed_memory[i].size >= ((unsigned char *)pointer) + size) /* technically UB */
		{
			printf("FORGE Mem debugger error: Pointer %p was freed on line %u in file %s\n", pointer, g_forge_mem_debug.f_freed_memory[i].free_line, g_forge_mem_debug.f_freed_memory[i].free_file);
		}
	}
	if(g_forge_mem_debug.forge_memory_stack_size != 0 && pointer >= g_forge_mem_debug.forge_memory_stack_pointer && &g_forge_mem_debug.forge_memory_stack_pointer[g_forge_mem_debug.forge_memory_stack_size] > pointer)
	{
		if(&g_forge_mem_debug.forge_memory_stack_pointer[g_forge_mem_debug.forge_memory_stack_size] <  &((unsigned char *)pointer)[size])
			printf("FORGE Mem debugger Eror: memmory is in stack, but does not fit.\n", pointer, g_forge_mem_debug.f_freed_memory[i].free_line, g_forge_mem_debug.f_freed_memory[i].free_file);
		else
			printf("FORGE Mem debugger warning: memmory is in stack.\n", pointer, g_forge_mem_debug.f_freed_memory[i].free_line, g_forge_mem_debug.f_freed_memory[i].free_file);
		return FALSE;
	}
	if(g_forge_mem_debug.f_alloc_mutex != NULL)
		g_forge_mem_debug.f_alloc_mutex_unlock(g_forge_mem_debug.f_alloc_mutex);
	if(ignore_not_found)
		return FALSE;
	printf("FORGE Mem debugger warning: No matching memory for pointer %p found!\n", pointer);
	return FALSE;
}


size_t f_debug_mem_consumption(void)
{
	unsigned int i;
	size_t sum = 0;
	if(g_forge_mem_debug.f_alloc_mutex != NULL)
		g_forge_mem_debug.f_alloc_mutex_lock(g_forge_mem_debug.f_alloc_mutex);
	for(i = 0; i < g_forge_mem_debug.f_alloc_line_count; i++)
		sum += g_forge_mem_debug.f_alloc_lines[i].size;
	if(g_forge_mem_debug.f_alloc_mutex != NULL)
		g_forge_mem_debug.f_alloc_mutex_unlock(g_forge_mem_debug.f_alloc_mutex);
	return sum;
}

void exit_crash(unsigned int i)
{
	unsigned int *a = NULL;
	a[0] = 0;
} 

boolean f_debug_mem_check_stack_reference()
{
	boolean output = FALSE;
	size_t size;
	size_t i, j, k, distance, best = 1024 * 1024, found = ~0, **buf;
	if(g_forge_mem_debug.f_alloc_mutex != NULL)
		g_forge_mem_debug.f_alloc_mutex_lock(g_forge_mem_debug.f_alloc_mutex);

	for(i = 0; i < g_forge_mem_debug.f_alloc_line_count; i++)
	{
		for(j = 0; j < g_forge_mem_debug.f_alloc_lines[i].alloc_count; j++)
		{
			if(g_forge_mem_debug.f_alloc_lines[i].allocs[j].active)
			{
				buf = g_forge_mem_debug.f_alloc_lines[i].allocs[j].buf;
				size = g_forge_mem_debug.f_alloc_lines[i].allocs[j].size / sizeof(void *);
				if(g_forge_mem_debug.forge_memory_stack_size != 0)
				{
					for(k = 0; k < size; k++)
					{
						if(buf[k] >= g_forge_mem_debug.forge_memory_stack_pointer && buf[k] < &g_forge_mem_debug.forge_memory_stack_pointer[g_forge_mem_debug.forge_memory_stack_size])
						{
							if(g_forge_mem_debug.f_alloc_lines[i].allocs[j].comment == NULL)
								printf("FORGE Mem debugger Warning: Suspected reference to stack variable %u bytes in to in allocation made on line %u in file %s (%u bytes off known stack pointer)\n", (unsigned int)(k * sizeof(void*)), g_forge_mem_debug.f_alloc_lines[i].line, g_forge_mem_debug.f_alloc_lines[i].file, (unsigned int)best);
							else
								printf("FORGE Mem debugger Warning: Suspected reference to stack variable %u bytes in to in allocation made on line %u in file %s /* %s */ (%u bytes off known stack pointer)\n", (unsigned int)(k * sizeof(void*)), g_forge_mem_debug.f_alloc_lines[i].line, g_forge_mem_debug.f_alloc_lines[i].file, g_forge_mem_debug.f_alloc_lines[i].allocs[j].comment, (unsigned int)best);

						}
					}
				}else
				{
					for(k = 0; k < size; k++)
					{
						if(buf[k] > &i)
							distance = buf[k] - &i;
						else
							distance = &i - buf[k];
						if(distance < best)
						{
							best = distance;
							found = k * sizeof(void *);
						}
					}
					if(found != ~0)
					{
						if(g_forge_mem_debug.f_alloc_lines[i].allocs[j].comment == NULL)
							printf("FORGE Mem debugger Warning: Suspected reference to stack variable %u bytes in to in allocation made on line %u in file %s (%u bytes off known stack pointer)\n", (unsigned int)found, g_forge_mem_debug.f_alloc_lines[i].line, g_forge_mem_debug.f_alloc_lines[i].file, (unsigned int)best);
						else
							printf("FORGE Mem debugger Warning: Suspected reference to stack variable %u bytes in to in allocation made on line %u in file %s /* %s */ (%u bytes off known stack pointer)\n", (unsigned int)found, g_forge_mem_debug.f_alloc_lines[i].line, g_forge_mem_debug.f_alloc_lines[i].file, g_forge_mem_debug.f_alloc_lines[i].allocs[j].comment, (unsigned int)best);
						output = TRUE;
					}
				}
			}
		}
	}
	if(g_forge_mem_debug.f_alloc_mutex != NULL)
		g_forge_mem_debug.f_alloc_mutex_unlock(g_forge_mem_debug.f_alloc_mutex);
	return output;
}





boolean f_debug_find_pointer_in_memory(void *pointer)
{
	boolean output = FALSE;
	size_t size;
	size_t i, j, k;
	void **buf, *read;
	if(g_forge_mem_debug.f_alloc_mutex != NULL)
		g_forge_mem_debug.f_alloc_mutex_lock(g_forge_mem_debug.f_alloc_mutex);
	for(i = 0; i < g_forge_mem_debug.f_alloc_line_count; i++)
	{
		for(j = 0; j < g_forge_mem_debug.f_alloc_lines[i].alloc_count; j++)
		{
			buf = g_forge_mem_debug.f_alloc_lines[i].allocs[j].buf;
			size = g_forge_mem_debug.f_alloc_lines[i].allocs[j].size / sizeof(void *);
			for(k = 0; k < size; k++)
			{
				memcpy(&read, &buf[k], sizeof(void *));
				if(pointer == read)
				{
					if(g_forge_mem_debug.f_alloc_mutex != NULL)
						g_forge_mem_debug.f_alloc_mutex_unlock(g_forge_mem_debug.f_alloc_mutex);
					return TRUE;
				}
			}
		}
	}
	buf = (void **)g_forge_mem_debug.forge_memory_stack_pointer;
	for(j = 0; j < g_forge_mem_debug.forge_memory_stack_size / sizeof(void *) && buf[j] != pointer; j++);
	if(g_forge_mem_debug.f_alloc_mutex != NULL)
		g_forge_mem_debug.f_alloc_mutex_unlock(g_forge_mem_debug.f_alloc_mutex);
	return j < g_forge_mem_debug.forge_memory_stack_size / sizeof(void *);
}

void f_debug_mem_check_heap_reference(unsigned int minimum_allocations)
{
	boolean output = FALSE;
	size_t i, j, found, alloc_count;

	if(g_forge_mem_debug.f_alloc_mutex != NULL)
		g_forge_mem_debug.f_alloc_mutex_lock(g_forge_mem_debug.f_alloc_mutex);

	for(i = 0; i < g_forge_mem_debug.f_alloc_line_count; i++)
	{
		if(g_forge_mem_debug.f_alloc_lines[i].alloc_count >= minimum_allocations)
		{
			for(found = alloc_count = j = 0; j < g_forge_mem_debug.f_alloc_lines[i].alloc_count; j++)
			{
				if(g_forge_mem_debug.f_alloc_lines[i].allocs[j].active)
				{
					alloc_count++;
					if(f_debug_find_pointer_in_memory(g_forge_mem_debug.f_alloc_lines[i].allocs[j].buf))
						found++;
				}
			}
			if(found != g_forge_mem_debug.f_alloc_lines[i].alloc_count)
			{
				if(g_forge_mem_debug.forge_memory_stack_size)
					printf("FORGE Mem debugger Error: Cant find any reference in heap memory or stack of %u out of %u allocations made on line %u in file %s\n", g_forge_mem_debug.f_alloc_lines[i].alloc_count - (unsigned int)found, g_forge_mem_debug.f_alloc_lines[i].alloc_count, g_forge_mem_debug.f_alloc_lines[i].line, g_forge_mem_debug.f_alloc_lines[i].file);
				else
					printf("FORGE Mem debugger Warning: Cant find any reference in heap memory of %u out of %u allocations made on line %u in file %s\n", g_forge_mem_debug.f_alloc_lines[i].alloc_count - (unsigned int)found, g_forge_mem_debug.f_alloc_lines[i].alloc_count, g_forge_mem_debug.f_alloc_lines[i].line, g_forge_mem_debug.f_alloc_lines[i].file);
			}
		}
		}
		if(g_forge_mem_debug.f_alloc_mutex != NULL)
				g_forge_mem_debug.f_alloc_mutex_unlock(g_forge_mem_debug.f_alloc_mutex);
}

// Utility functions to access total memory tracking
size_t f_debug_mem_get_total_heap_memory_allocated(void)
{
	return g_forge_mem_debug.total_heap_memory_allocated;
}

size_t f_debug_mem_get_total_allocation_count(void)
{
	return g_forge_mem_debug.total_allocation_count;
}

// Restore warning settings for other files
#pragma warning(pop)