/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DEBUG_GCOV_H_
#define ZEPHYR_INCLUDE_DEBUG_GCOV_H_

#ifdef __cplusplus
extern "C" {
#endif

#if defined(CONFIG_COVERAGE) && defined(CONFIG_GCOV_STATIC_INIT)
void gcov_static_init(void);
#else
#define gcov_static_init() ((void)0)
#endif

#ifdef CONFIG_COVERAGE
void gcov_coverage_dump(void);
#else
#define gcov_coverage_dump() ((void)0)
#endif

#ifdef __cplusplus
}
#endif

#endif	/* ZEPHYR_INCLUDE_DEBUG_GCOV_H_ */
