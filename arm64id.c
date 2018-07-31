/*-
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2018 Andrew Turner
 * All rights reserved.
 *
 * This software was developed by SRI International and the University of
 * Cambridge Computer Laboratory under DARPA/AFRL contract FA8750-10-C-0237
 * ("CTSRD"), as part of the DARPA CRASH research programme.
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

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/linker_set.h>
#include <sys/ucontext.h>

#include <err.h>
#include <signal.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

typedef uint64_t (*special_reg_read)(uint64_t *);

struct special_reg {
	const char *reg_name;
	special_reg_read reader;
};

SET_DECLARE(special_reg_set, struct special_reg);

#define SPECIAL_REGISTER(name)					\
uint64_t get_##name(uint64_t *);				\
asm(								\
".globl get_"__STRING(name)"		\n"			\
"get_"__STRING(name)":			\n"			\
"	mov	x1, x0			\n"			\
"	mov	x0, #0			\n"			\
"	mrs	x2, "__STRING(name)"	\n"			\
"	str	x2, [x1]		\n"			\
"	ret				\n"			\
);								\
static struct special_reg name ## _entry = {			\
	.reg_name = __XSTRING(name),				\
	.reader = get_ ## name,					\
};								\
DATA_SET(special_reg_set, name ## _entry)

#define SPECIAL_REGISTER_GROUP(n)				\
SPECIAL_REGISTER(S3_0_C0_C ## n ## _0);				\
SPECIAL_REGISTER(S3_0_C0_C ## n ## _1);				\
SPECIAL_REGISTER(S3_0_C0_C ## n ## _2);				\
SPECIAL_REGISTER(S3_0_C0_C ## n ## _3);				\
SPECIAL_REGISTER(S3_0_C0_C ## n ## _4);				\
SPECIAL_REGISTER(S3_0_C0_C ## n ## _5);				\
SPECIAL_REGISTER(S3_0_C0_C ## n ## _6);				\
SPECIAL_REGISTER(S3_0_C0_C ## n ## _7)

SPECIAL_REGISTER_GROUP(0);
SPECIAL_REGISTER_GROUP(1);
SPECIAL_REGISTER_GROUP(2);
SPECIAL_REGISTER_GROUP(3);
SPECIAL_REGISTER_GROUP(4);
SPECIAL_REGISTER_GROUP(5);
SPECIAL_REGISTER_GROUP(6);
SPECIAL_REGISTER_GROUP(7);

static struct special_register_alias {
	const char *name;
	const char *alias;
} reg_aliases[] = {
	{ "S3_0_C0_C0_0", "midr_el1" },
	{ "S3_0_C0_C0_5", "mpidr_el1" },
	{ "S3_0_C0_C0_6", "revidr_el1" },

	{ "S3_0_C0_C1_0", "id_pfr0_el1" },
	{ "S3_0_C0_C1_1", "id_pfr1_el1" },
	{ "S3_0_C0_C1_2", "id_dfr0_el1" },
	{ "S3_0_C0_C1_3", "id_afr0_el1" },
	{ "S3_0_C0_C1_4", "id_mmfr0_el1" },
	{ "S3_0_C0_C1_5", "id_mmfr1_el1" },
	{ "S3_0_C0_C1_6", "id_mmfr2_el1" },
	{ "S3_0_C0_C1_7", "id_mmfr3_el1" },

	{ "S3_0_C0_C2_0", "id_isar0_el1" },
	{ "S3_0_C0_C2_1", "id_isar1_el1" },
	{ "S3_0_C0_C2_2", "id_isar2_el1" },
	{ "S3_0_C0_C2_3", "id_isar3_el1" },
	{ "S3_0_C0_C2_4", "id_isar4_el1" },
	{ "S3_0_C0_C2_5", "id_isar5_el1" },
	{ "S3_0_C0_C2_6", "id_mmfr4_el1" },

	{ "S3_0_C0_C3_0", "mvfr0_el1" },
	{ "S3_0_C0_C3_1", "mvfr1_el1" },
	{ "S3_0_C0_C3_2", "mvfr2_el1" },

	{ "S3_0_C0_C4_0", "id_aa64pfr0_el1" },
	{ "S3_0_C0_C4_1", "id_aa64pfr0_el1" },
	{ "S3_0_C0_C4_4", "id_aa64zfr0_el1" },

	{ "S3_0_C0_C5_0", "id_aa64dfr0_el1" },
	{ "S3_0_C0_C5_1", "id_aa64dfr1_el1" },
	{ "S3_0_C0_C5_4", "id_aa64afr0_el1" },
	{ "S3_0_C0_C5_5", "id_aa64afr1_el1" },

	{ "S3_0_C0_C6_0", "id_aa64isar0_el1" },
	{ "S3_0_C0_C6_1", "id_aa64isar1_el1" },

	{ "S3_0_C0_C7_0", "id_aa64mmfr0_el1" },
	{ "S3_0_C0_C7_1", "id_aa64mmfr1_el1" },
	{ "S3_0_C0_C7_2", "id_aa64mmfr2_el1" },
};

static void
sigill(int signo, siginfo_t *info, void *ctx)
{
	ucontext_t *uap;

	uap = ctx;

	uap->uc_mcontext.mc_gpregs.gp_elr += 4;
	uap->uc_mcontext.mc_gpregs.gp_x[0] = 1;
}


int
main(int argc, char *argv[])
{
	struct sigaction act;
	struct special_reg **sr, *cursr;
	uint64_t reg;

	memset(&act, 0, sizeof(act));
	sigemptyset(&act.sa_mask);
	act.sa_sigaction = sigill;
	act.sa_flags = SA_SIGINFO;

	if (sigaction(SIGILL, &act, NULL) != 0)
		err(1, "sigaction failed");

	SET_FOREACH(sr, special_reg_set) {
		const char *name;
		int i;

		cursr = *sr;

		name = cursr->reg_name;
		for (i = 0; i < nitems(reg_aliases); i++) {
			if (strcmp(name, reg_aliases[i].name) == 0) {
				name = reg_aliases[i].alias;
				break;
			}
		}
		printf("%20s = ", name);

		if (cursr->reader(&reg))
			printf("<invalid>\n");
		else
			printf("0x%016lx\n", reg);
	}

	return (0);
}
