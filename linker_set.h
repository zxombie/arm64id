/*-
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 1999 John D. Polstra
 * Copyright (c) 1999,2001 Peter Wemm <peter@FreeBSD.org>
 * All rights reserved.
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

#define	LS_STRING(x)		#x
#define	LS_XSTRING(x)		LS_STRING(x)
#define	LS_CONCAT1(a, b)	a ## b
#define	LS_CONCAT(a, b)		LS_CONCAT1(a, b)

#define	LS_SECTION(x)		__attribute__((__section__(x)))
#define	LS_USED			__attribute__((__used__))
#define LS_WEAK(sym)		asm(".weak " LS_XSTRING(sym))
#define	LS_WEAK_SYMBOL		__attribute__((__weak__))

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
#define	LS_NOASAN	__attribute__((no_sanitize("address")))

#define LS_MAKE_SET_QV(set, sym, qv)				\
	LS_WEAK(LS_CONCAT(__start_ls_set_,set));			\
	LS_WEAK(LS_CONCAT(__stop_ls_set_,set));			\
	static void const * qv					\
	LS_NOASAN						\
	ls_set_##set##_sym_##sym LS_SECTION("ls_set_" #set)	\
	LS_USED = &(sym)
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
#define LS_SET_DECLARE(set, ptype)					\
	extern ptype LS_WEAK_SYMBOL *LS_CONCAT(__start_ls_set_,set);	\
	extern ptype LS_WEAK_SYMBOL *LS_CONCAT(__stop_ls_set_,set)

#define LS_SET_BEGIN(set)						\
	(&LS_CONCAT(__start_ls_set_,set))
#define LS_SET_LIMIT(set)						\
	(&LS_CONCAT(__stop_ls_set_,set))

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
