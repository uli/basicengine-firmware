// SPDX-License-Identifier: MIT
// Copyright (c) 2021 Ulrich Hecht

#ifdef __cplusplus
extern "C" {
#endif

#include <libtcc.h>

TCCState *eb_tcc_new(int output_type);
void eb_tcc_initialize_symbols(TCCState *tcc);
int eb_tcc_link(TCCState *tcc, const char *name, int output_type);

#define EXPORT_FUNC(f) void __export_ ##f(void) __attribute__((alias(#f)));
#define EXPORT_VAR(v) extern char __export_##v __attribute__((alias(#v)));

#ifdef __cplusplus
}
#endif
