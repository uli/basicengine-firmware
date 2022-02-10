// SPDX-License-Identifier: MIT
// Copyright (c) 2021 Ulrich Hecht

#include "eb_file.h"
#include "basic.h"

EBAPI int eb_file_exists(const char *name)
{
    struct stat st;
    if (_stat(name, &st))
        return 0;
    else
        return 1;
}

EBAPI int eb_is_directory(const char *name)
{
    struct stat st;
    if (_stat(name, &st))
        return 0;
    else
        return S_ISDIR(st.st_mode);
}

EBAPI int eb_is_file(const char *name)
{
    struct stat st;
    if (_stat(name, &st))
        return 0;
    else
        return S_ISREG(st.st_mode);
}

EBAPI int eb_file_size(const char *name)
{
    struct stat st;
    if (_stat(name, &st))
        return -1;
    else
        return st.st_size;
}

#include <miniz.h>

EBAPI int eb_unzip(const char *filename, int verbose) {
  if (!eb_file_exists(filename)) {
    err = ERR_FILE_OPEN;
    return -1;
  }

  mz_zip_archive zip_archive = {};

  int status = mz_zip_reader_init_file(&zip_archive, filename, 0);
  if (!status) {
    err = ERR_FORMAT;
    err_expected = _("not a ZIP file");
    return -1;
  }

  if (!mz_zip_validate_archive(&zip_archive, 0)) {
    err = ERR_FORMAT;
    err_expected = _("broken ZIP file");
    mz_zip_reader_end(&zip_archive);
    return -1;
  }

  int file_count = 0;
  for (int i = 0; i < (int)mz_zip_reader_get_num_files(&zip_archive); i++) {
    mz_zip_archive_file_stat file_stat;
    if (!mz_zip_reader_file_stat(&zip_archive, i, &file_stat)) {
      printf("mz_zip_reader_file_stat() failed!\n");
      mz_zip_reader_end(&zip_archive);
      err = ERR_FORMAT;
      err_expected = _("ZIP file read error");
      return -1;
    }

    if (verbose)
      c_printf("%s\n", file_stat.m_filename);

    if (mz_zip_reader_is_file_a_directory(&zip_archive, i)) {
      mkdir(file_stat.m_filename, 0755);
    } else {
      status = mz_zip_reader_extract_file_to_file(&zip_archive, file_stat.m_filename, file_stat.m_filename, 0);
      if (!status) {
        err = ERR_IO;
        err_expected = _("file could not be extracted");
        mz_zip_reader_end(&zip_archive);
        return -1;
      }
      ++file_count;
    }
  }
  if (verbose)
    c_printf(_("%d files extracted\n"), file_count);

  return 0;
}
