#ifndef _GCOV_PORT_H_
#define _GCOV_PORT_H_

#include <stdint.h>
#include "coverage.h"

#include <stdio.h>

#define GCOV_BUFF_STATIC_ALLOCATE
#define GCOV_BUFF_SIZE 16384

#define OS_ENTER_CRITICAL() (void)0
#define OS_EXIT_CRITICAL() (void)0
#define GCOV_PRINT printf

static void dump_on_console(const char *filename, char *ptr, size_t len)
{
	uint32_t iter;

	GCOV_PRINT("\n%c", FILE_START_INDICATOR);
	while (*filename != '\0')
	{
		GCOV_PRINT("%c", *filename++);
	}
	GCOV_PRINT("%c", GCOV_DUMP_SEPARATOR);

	/* Data dump */

	for (iter = 0U; iter < len; iter++)
	{
		GCOV_PRINT(" %02x", (uint8_t)*ptr++);
	}
}

#endif