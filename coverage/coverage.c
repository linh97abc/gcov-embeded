/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef CONFIG_COVERAGE

#include <stdint.h>
#include <stddef.h>
#include "coverage.h"
#include "gcov-port.h"
#include "gcov.h"

#ifndef GCOV_BUFF_STATIC_ALLOCATE
#include <stdlib.h>
#endif

static struct gcov_info *gcov_info_head = NULL;

/**
 * Is called by gcc-generated constructor code for each object file compiled
 * with -fprofile-arcs.
 */
void __gcov_init(struct gcov_info *info)
{
	info->next = gcov_info_head;
	gcov_info_head = info;
}

void __gcov_merge_add(gcov_type *counters, unsigned int n_counters)
{
	/* Unused. */
}

void __gcov_exit(void)
{
	/* Unused. */
}

/**
 * buff_write_u64 - Store 64 bit data on a buffer and return the size
 */

#define MASK_32BIT (0xffffffffUL)
static inline void buff_write_u64(void *buffer, size_t *off, uint64_t v)
{
	*((uint32_t *)((uint8_t *)buffer + *off) + 0) = (uint32_t)(v & MASK_32BIT);
	*((uint32_t *)((uint8_t *)buffer + *off) + 1) = (uint32_t)((v >> 32) &
															   MASK_32BIT);
	*off = *off + sizeof(uint64_t);
}

/**
 * buff_write_u32 - Store 32 bit data on a buffer and return the size
 */
static inline void buff_write_u32(void *buffer, size_t *off, uint32_t v)
{
	*((uint32_t *)((uint8_t *)buffer + *off)) = v;
	*off = *off + sizeof(uint32_t);
}

static size_t calculate_buff_size(struct gcov_info *info)
{
	uint32_t iter;
	uint32_t iter_1;
	uint32_t iter_2;
	/* few Fixed values at the start version, stamp and magic number. */
	uint32_t size = sizeof(uint32_t) * 3;

	for (iter = 0U; iter < info->n_functions; iter++)
	{
		/* space for TAG_FUNCTION and FUNCTION_LENGTH
		 * ident
		 * lineno_checksum
		 * cfg_checksum
		 */
		size += (sizeof(uint32_t) * 5);

		for (iter_1 = 0U; iter_1 < GCOV_COUNTERS; iter_1++)
		{
			if (!info->merge[iter_1])
			{
				continue;
			}

			/*  for function counter and number of values  */
			size += (sizeof(uint32_t) * 2);

			for (iter_2 = 0U;
				 iter_2 < info->functions[iter]->ctrs->num;
				 iter_2++)
			{

				/* Iter for values which is uint64_t */
				size += (sizeof(uint64_t));
			}
		}
	}

	return size;
}

/**
 * populate_buffer - convert from gcov data set (info) to
 * .gcda file format.
 * This buffer will now have info similar to a regular gcda
 * format.
 */
static size_t populate_buffer(uint8_t *buffer, struct gcov_info *info)
{
	struct gcov_fn_info *functions;
	struct gcov_ctr_info *counters_per_func;
	uint32_t iter_functions;
	uint32_t iter_counts;
	uint32_t iter_counter_values;
	size_t buffer_write_position = 0;

	/* File header. */
	buff_write_u32(buffer,
				   &buffer_write_position,
				   GCOV_DATA_MAGIC);

	buff_write_u32(buffer,
				   &buffer_write_position,
				   info->version);

	buff_write_u32(buffer,
				   &buffer_write_position,
				   info->stamp);

	for (iter_functions = 0U;
		 iter_functions < info->n_functions;
		 iter_functions++)
	{

		functions = info->functions[iter_functions];

		buff_write_u32(buffer,
					   &buffer_write_position,
					   GCOV_TAG_FUNCTION);

		buff_write_u32(buffer,
					   &buffer_write_position,
					   GCOV_TAG_FUNCTION_LENGTH);

		buff_write_u32(buffer,
					   &buffer_write_position,
					   functions->ident);

		buff_write_u32(buffer,
					   &buffer_write_position,
					   functions->lineno_checksum);

		buff_write_u32(buffer,
					   &buffer_write_position,
					   functions->cfg_checksum);

		counters_per_func = functions->ctrs;

		for (iter_counts = 0U;
			 iter_counts < GCOV_COUNTERS;
			 iter_counts++)
		{

			if (!info->merge[iter_counts])
			{
				continue;
			}

			buff_write_u32(buffer,
						   &buffer_write_position,
						   GCOV_TAG_FOR_COUNTER(iter_counts));

			buff_write_u32(buffer,
						   &buffer_write_position,
						   counters_per_func->num * 2U);

			for (iter_counter_values = 0U;
				 iter_counter_values < counters_per_func->num;
				 iter_counter_values++)
			{

				buff_write_u64(buffer,
							   &buffer_write_position,
							   counters_per_func->values[iter_counter_values]);
			}

			counters_per_func++;
		}
	}
	return buffer_write_position;
}

#if defined(GCOV_BUFF_STATIC_ALLOCATE)
static uint8_t s_buffer[GCOV_BUFF_SIZE];
#endif
/**
 * Retrieves gcov coverage data and sends it over the given interface.
 */
void gcov_coverage_dump(void)
{
#if defined(GCOV_BUFF_STATIC_ALLOCATE)
	uint8_t *buffer = &s_buffer[0];
#else
	uint8_t *buffer;
#endif

	size_t size;
	size_t written_size;
	struct gcov_info *gcov_list_first = gcov_info_head;
	struct gcov_info *gcov_list = gcov_info_head;

	OS_ENTER_CRITICAL();

	GCOV_PRINT("\nGCOV_COVERAGE_DUMP_START");
	while (gcov_list)
	{

		size = calculate_buff_size(gcov_list);

#ifndef GCOV_BUFF_STATIC_ALLOCATE
		buffer = (uint8_t *)malloc(size);

		if (!buffer)
		{
			GCOV_PRINT("No Mem available to continue dump\n");
			goto coverage_dump_end;
		}
#endif
		written_size = populate_buffer(buffer, gcov_list);
		if (written_size != size)
		{
			GCOV_PRINT("Write Error on buff\n");
			goto coverage_dump_end;
		}

		dump_on_console(gcov_list->filename, (char *)buffer, size);

#ifndef GCOV_BUFF_STATIC_ALLOCATE
		free(buffer);
#endif
		gcov_list = gcov_list->next;
		if (gcov_list_first == gcov_list)
		{
			goto coverage_dump_end;
		}
	}
coverage_dump_end:
	GCOV_PRINT("\nGCOV_COVERAGE_DUMP_END\n");

	OS_EXIT_CRITICAL();

	return;
}

/* Initialize the gcov by calling the required constructors */
#ifdef CONFIG_GCOV_STATIC_INIT
void gcov_static_init(void)
{
	extern uintptr_t __init_array_start, __init_array_end;
	uintptr_t func_pointer_start = (uintptr_t)&__init_array_start;
	uintptr_t func_pointer_end = (uintptr_t)&__init_array_end;

	while (func_pointer_start < func_pointer_end)
	{
		void (**p)(void);
		/* get function pointer */
		p = (void (**)(void))func_pointer_start;
		(*p)();
		func_pointer_start += sizeof(p);
	}
}

#endif

#endif
