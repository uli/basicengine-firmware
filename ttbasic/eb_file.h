// SPDX-License-Identifier: MIT
// Copyright (c) 2021 Ulrich Hecht

#ifdef __cplusplus
extern "C" {
#endif

int eb_file_exists(const char *name);
int eb_is_directory(const char *name);
int eb_is_file(const char *name);
int eb_file_size(const char *name);
int eb_unzip(const char *filename, int verbose);

#ifdef __cplusplus
}
#endif
