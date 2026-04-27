/*-
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 1999 John D. Polstra
 * Copyright (c) 1999,2001 Peter Wemm <peter@FreeBSD.org>
 * All rights reserved.
 * Copyright (c) 2023 Jessica Clarke <jrtc27@FreeBSD.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef LINKER_SET_H
#define LINKER_SET_H

#if defined(__APPLE__)
#include <mach-o/dyld.h>
#include <mach-o/getsect.h>
#endif

#define	LS_STRING(x)		#x
#define	LS_XSTRING(x)		LS_STRING(x)
#define	LS_CONCAT1(a, b)	a ## b
#define	LS_CONCAT(a, b)		LS_CONCAT1(a, b)

#define	LS_USED			__attribute__((__used__))

#if defined(__APPLE__)
#define	LS_SECTION(x)		__attribute__((__section__("__DATA,set_" #x)))
#else
#define	LS_SECTION(x)		__attribute__((__section__(x)))
#define LS_WEAK(sym)		asm(".weak " LS_XSTRING(sym))
#define	LS_WEAK_SYMBOL		__attribute__((__weak__))
#endif

/*
 * The following macros are used to declare global sets of objects, which
 * are collected by the linker into a `linker_set' as defined below.
 * For ELF, this is done by constructing a separate segment for each set.
 */

#if defined(__powerpc64__) && (!defined(_CALL_ELF) || _CALL_ELF == 1)
/*
 * ELFv1 pointers to functions are actaully pointers to function
 * descriptors.
 *
 * Move the symbol pointer from ".text" to ".data" segment, to make
 * the GCC compiler happy:
 */
#define	LS_MAKE_SET_CONST
#else
#define	LS_MAKE_SET_CONST const
#endif

/*
 * Private macros, not to be used outside this header file.
 */

/*
 * The userspace address sanitizer inserts redzones around global variables,
 * violating the assumption that linker set elements are packed.
 */
#if defined(__SANITIZE_ADDRESS__)
#define	LS_NOASAN	__attribute__((no_sanitize("address")))
#elif defined(__has_feature)
# if __has_feature(address_sanitizer)
#define	LS_NOASAN	__attribute__((no_sanitize("address")))
# endif
#endif
#if !defined(LS_NOASAN)
#define	LS_NOASAN
#endif

#if defined(__APPLE__)
#define LS_MAKE_SET_QV(set, sym, qv)				\
	static void const * qv					\
	LS_NOASAN						\
	__set_##set##_sym_##sym	LS_SECTION(set)			\
	LS_USED = &(sym)

static inline __pure2 uint8_t *
__set_getsectiondata(const char *segname, const char *sectname,
    unsigned long *size)
{
	uint32_t image_count, image_index;
	const struct mach_header *mh;
	uint8_t *ret;

	image_count = _dyld_image_count();
	for (image_index = 0; image_index < image_count; ++image_index) {
		mh = _dyld_get_image_header(image_index);
		if (mh == NULL)
			continue;

		ret = getsectiondata((const struct mach_header_64 *)mh,
		    segname, sectname, size);
		if (ret != NULL)
			return (ret);
	}

	return (NULL);
}

#define LS_SET_RANGE(set)	({					\
	unsigned long __set_size;					\
	char *__set_data;						\
	__set_data = __set_getsectiondata("__DATA",			\
	    "set_" #set, &__set_size);					\
	(struct {							\
		LS_CONCAT(__typeof_set_,set)	**begin;		\
		LS_CONCAT(__typeof_set_,set)	**limit;		\
	}){								\
		.begin = (LS_CONCAT(__typeof_set_,set) **)__set_data,	\
		.limit = (LS_CONCAT(__typeof_set_,set) **)(__set_data +	\
		    __set_size)						\
	};								\
})
#else
#define LS_MAKE_SET_QV(set, sym, qv)				\
	LS_WEAK(LS_CONCAT(__start_ls_set_,set));			\
	LS_WEAK(LS_CONCAT(__stop_ls_set_,set));			\
	static void const * qv					\
	LS_NOASAN						\
	ls_set_##set##_sym_##sym LS_SECTION("ls_set_" #set)	\
	LS_USED = &(sym)
#endif
#define LS_MAKE_SET(set, sym)	LS_MAKE_SET_QV(set, sym, LS_MAKE_SET_CONST)

/*
 * Public macros.
 */
#define LS_TEXT_SET(set, sym)	LS_MAKE_SET(set, sym)
#define LS_DATA_SET(set, sym)	LS_MAKE_SET(set, sym)
#define LS_DATA_WSET(set, sym)	LS_MAKE_SET_QV(set, sym, )
#define LS_BSS_SET(set, sym)	LS_MAKE_SET(set, sym)
#define LS_ABS_SET(set, sym)	LS_MAKE_SET(set, sym)
#define LS_SET_ENTRY(set, sym)	LS_MAKE_SET(set, sym)

/*
 * Initialize before referring to a given linker set.
 */
#if defined(__APPLE__)
#define LS_SET_DECLARE(set, ptype)					\
	typedef ptype LS_CONCAT(__typeof_set_,set)

#define LS_SET_BEGIN(set)						\
	(LS_SET_RANGE(set).begin)
#define LS_SET_LIMIT(set)						\
	(LS_SET_RANGE(set).limit)
#else
#define LS_SET_DECLARE(set, ptype)					\
	extern ptype LS_WEAK_SYMBOL *LS_CONCAT(__start_ls_set_,set);	\
	extern ptype LS_WEAK_SYMBOL *LS_CONCAT(__stop_ls_set_,set)

#define LS_SET_BEGIN(set)						\
	(&LS_CONCAT(__start_ls_set_,set))
#define LS_SET_LIMIT(set)						\
	(&LS_CONCAT(__stop_ls_set_,set))
#endif

/*
 * Iterate over all the elements of a set.
 *
 * Sets always contain addresses of things, and "pvar" points to words
 * containing those addresses.  Thus is must be declared as "type **pvar",
 * and the address of each set item is obtained inside the loop by "*pvar".
 */
#define LS_SET_FOREACH(pvar, set)					\
	for (pvar = LS_SET_BEGIN(set); pvar < LS_SET_LIMIT(set); pvar++)

#define LS_SET_ITEM(set, i)						\
	((LS_SET_BEGIN(set))[i])

/*
 * Provide a count of the items in a set.
 */
#define LS_SET_COUNT(set)						\
	(LS_SET_LIMIT(set) - LS_SET_BEGIN(set))

#endif	/* LINKER_SET_H */
