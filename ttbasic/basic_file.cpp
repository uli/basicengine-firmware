#include "basic.h"
#ifdef H3
#include <fs.h>
#endif

FILEDIR user_files[MAX_USER_FILES];

void SMALL basic_init_file_early() {
  // try to mount SPIFFS, ignore failure (do not format)
#ifdef UNIFILE_USE_OLD_SPIFFS
#ifdef ESP8266_NOWIFI
  SPIFFS.begin(false);
#else
  SPIFFS.begin();
#endif
#elif defined(UNIFILE_USE_FASTROMFS)
  fs.mount();
#elif defined(UNIFILE_USE_NEW_SPIFFS)
  SPIFFS.begin();
#endif

  // Initialize SD card file system
#ifdef ESP32
  bfs.init(5);
#else
  bfs.init(16);  // CS on GPIO16
#endif

#ifdef H3
  // Also done in arch_process_events(), but that's too late for us here.
  sd_detect();
#endif

#ifdef SDL
  char *root = getenv("ENGINEBASIC_ROOT");
  if (root)
    _chdir(root);
#elif defined(H3)
  if (!_chdir(SD_PREFIX))
    bfs.fakeTime();
#else
  if (_chdir(SD_PREFIX)) {
    if (_chdir(FLASH_PREFIX)) {
      // Can't really do anything if this fails, nothing is initialized yet.
    }
  } else
    bfs.fakeTime();
#endif

  loadConfig();
}

void SMALL basic_init_file_late() {
  // Initialize file systems, format SPIFFS if necessary
#ifdef UNIFILE_USE_OLD_SPIFFS
  SPIFFS.begin();
#elif defined(UNIFILE_USE_NEW_SPIFFS)
  SPIFFS.begin(true);
#elif defined(UNIFILE_USE_FASTROMFS)
  if (!fs.mount()) {
    fs.mkfs();
    fs.mount();
  }
#endif
}

int BASIC_INT Basic::get_filenum_param() {
  int32_t f = getparam();
  if (f < 0 || f >= MAX_USER_FILES) {
    E_VALUE(0, MAX_USER_FILES - 1);
    return -1;
  } else
    return f;
}

/***bf fs EOF
Checks whether the end of a file has been reached.
\usage e = EOF(file_num)
\args
@file_num	number of an open file [`0` to `{MAX_USER_FILES_m1}`]
\ret `-1` if the end of the file has been reached, `0` otherwise.
***/
num_t BASIC_INT Basic::neof() {
  int32_t a = get_filenum_param();
  if (!err && user_files[a].f)
    return basic_bool(!feof(user_files[a].f));
  else
    return 0;
}

/***bf fs LOF
Returns the length of a file in bytes.
\usage l = LOF(file_num)
\args
@file_num	number of an open file [`0` to `{MAX_USER_FILES_m1}`]
\ret Length of file.
***/
num_t BASIC_INT Basic::nlof() {
  int32_t a = get_filenum_param();
  if (!err && user_files[a].f) {
    size_t now = ftell(user_files[a].f);
    fseek(user_files[a].f, 0, SEEK_END);
    size_t size = ftell(user_files[a].f);
    fseek(user_files[a].f, now, SEEK_SET);
    return size;
  } else
    return 0;
}

/***bf fs LOC
Returns the current position within a file.
\usage l = LOC(file_num)
\args
@file_num	number of an open file [`0` to `{MAX_USER_FILES_m1}`]
\ret Position at which the next byte will be read or written.
***/
num_t BASIC_INT Basic::nloc() {
  int32_t a = get_filenum_param();
  if (!err && user_files[a].f)
    return ftell(user_files[a].f);
  else
    return 0;
}

/***bf fs DIR$
Returns the next entry of an open directory.
\usage d$ = DIR$(dir_num)
\args
@dir_num	number of an open directory
\ret Returns the directory entry as the value of the function.

Returns an empty string if the end of the directory has been reached.

Also returns the entry's size in `RET(0)` and `0` or `1` in RET(1)
depending on whether the entry is a directory itself.
\error
An error is generated if `dir_num` is not open or not a directory.
***/
BString Basic::sdir() {
  int32_t fnum = getparam();
  if (err)
    goto out;
  if (fnum < 0 || fnum >= MAX_USER_FILES) {
    E_VALUE(0, MAX_USER_FILES - 1);
out:
    return BString(F(""));
  }
  if (!user_files[fnum].d && !user_files[fnum].f) {
    err = ERR_FILE_NOT_OPEN;
    goto out;
  }
  if (!user_files[fnum].d) {
    err = ERR_NOT_DIR;
    goto out;
  }

  // read entries, skip "." and ".."
  dirent *dir_entry;
  do {
    dir_entry = _readdir(user_files[fnum].d);
  } while (dir_entry && (!strcmp_P(dir_entry->d_name, PSTR(".")) ||
                         !strcmp_P(dir_entry->d_name, PSTR(".."))));

  if (!dir_entry)
    return BString();

  struct stat st;
  _stat((user_files[fnum].dir_name + BString(F("/")) +
       BString(dir_entry->d_name)).c_str(), &st);

  retval[0] = st.st_size;
  retval[1] = dir_entry->d_type == DT_DIR;

  return BString(dir_entry->d_name);
}

/***bf fs INPUT$
Returns a string of characters read from a specified file.
\usage dat$ = INPUT$(len, [#]file_num)
\args
@len		number of bytes to read
@file_num	number of an open file [`0` to `{MAX_USER_FILES_m1}`]
\ret Data read from file.
\ref INPUT
***/
BString Basic::sinput() {
  int32_t len, fnum;
  BString value;
  ssize_t rd;

  if (checkOpen())
    goto out;
  if (getParam(len, I_COMMA))
    goto out;
  if (*cip == I_SHARP)
    ++cip;
  if (getParam(fnum, 0, MAX_USER_FILES - 1, I_CLOSE))
    goto out;
  if (!user_files[fnum].f) {
    err = ERR_FILE_NOT_OPEN;
    goto out;
  }
  if (!value.reserve(len)) {
    err = ERR_OOM;
    goto out;
  }
  rd = fread(value.begin(), 1, len, user_files[fnum].f);
  if (rd < 0) {
    err = ERR_FILE_READ;
    goto out;
  }
  value.resetLength(rd);
out:
  return value;
}

/***bc fs CMD
Redirect screen I/O to a file.
\desc
`CMD` allows to redirect standard text output or input operations to
files.

Redirection can be disabled by replacing the file number with `OFF`. `CMD
OFF` turns off both input and output redirection.
\usage
CMD <INPUT|OUTPUT> file_num
CMD <INPUT|OUTPUT> OFF
CMD OFF
\args
@file_num	number of an open file to redirect to [`0` to `{MAX_USER_FILES_m1}`]
\note
Redirection will automatically be reset if a file redirected to is closed
(either explicitly or implicitly, by opening a new file using the currently
used file number), or when returning to the command prompt.
\ref OPEN CLOSE
***/
void Basic::icmd() {
  bool is_input;
  int32_t redir;
  if (*cip == I_OUTPUT) {
    is_input = false;
    ++cip;
  } else if (*cip == I_INPUT) {
    is_input = true;
    ++cip;
  } else if (*cip == I_OFF) {
    ++cip;
    redirect_input_file = -1;
    redirect_output_file = -1;
    return;
  } else {
    SYNTAX_T("exp INPUT, OUTPUT or OFF");
    return;
  }

  if (*cip == I_OFF) {
    ++cip;
    if (is_input)
      redirect_input_file = -1;
    else
      redirect_output_file = -1;
    return;
  } else
    getParam(redir, 0, MAX_USER_FILES, I_NONE);
  if (!user_files[redir].f) {
    err = ERR_FILE_NOT_OPEN;
    if (is_input)
      redirect_input_file = -1;
    else
      redirect_output_file = -1;
  } else {
    if (is_input)
      redirect_input_file = redir;
    else
      redirect_output_file = redir;
  }
}

/***bc fs CHDIR
Changes the current directory.
\usage CHDIR directory$
\args
@directory$	path to the new current directory
\ref CWD$()
***/
void Basic::ichdir() {
  BString new_cwd;
  if (!(new_cwd = getParamFname())) {
    return;
  }
  if (_chdir(new_cwd.c_str()))
    err = ERR_FILE_OPEN;
}

/***bf fs CWD$
Returns the current working directory.
\usage dir$ = CWD$()
\ret Current working directory.
\ref CHDIR
***/
BString Basic::scwd() {
  if (checkOpen() || checkClose())
    return BString();
  char cwd[64];
  if (_getcwd(cwd, 64))
    return BString(cwd);
  else {
    err = ERR_LONG_PATH;
    return BString();
  }
}

/***bc fs OPEN
Opens a file or directory.
\usage
OPEN file$ [FOR <INPUT|OUTPUT|APPEND|DIRECTORY>] AS [#]file_num
\args
@file$		name of the file or directory
@file_num	file number to be used [`0` to `{MAX_USER_FILES_m1}`]
\sec MODES
* `*INPUT*` opens the file for reading. (This is the default if no mode is
  specified.)
* `*OUTPUT*` empties the file and opens it for writing.
* `*APPEND*` opens the file for writing while keeping its content.
* `*DIRECTORY*` opens `file$` as a directory.
\ref CLOSE DIR$() INPUT INPUT$() PRINT SEEK
***/
void Basic::iopen() {
  BString filename;
  const char *flags = "r";
  int32_t filenum;

  if (!(filename = getParamFname()))
    return;

  if (*cip == I_FOR) {
    ++cip;
    switch (*cip++) {
    case I_OUTPUT:	flags = "w"; break;
    case I_INPUT:	flags = "r"; break;
    case I_APPEND:	flags = "a"; break;
    case I_DIRECTORY:	flags = NULL; break;
    default:		SYNTAX_T("exp file mode"); return;
    }
  }

  if (*cip++ != I_AS) {
    E_SYNTAX(I_AS);
    return;
  }
  if (*cip == I_SHARP)
    ++cip;

  if (getParam(filenum, 0, MAX_USER_FILES - 1, I_NONE))
    return;

  if (user_files[filenum].f) {
    fclose(user_files[filenum].f);
    if (redirect_output_file == filenum)
      redirect_output_file = -1;
    if (redirect_input_file == filenum)
      redirect_input_file = -1;
    user_files[filenum].f = NULL;
  } else if (user_files[filenum].d) {
    _closedir(user_files[filenum].d);
    user_files[filenum].d = NULL;
    user_files[filenum].dir_name = BString();
  }

  FILEDIR f = { NULL, NULL, "" };
  if (flags == NULL) {
    f.d = _opendir(filename.c_str());
    char cwd[64];
    if (_getcwd(cwd, 64) == NULL) {
      err = ERR_LONG_PATH;
      if (f.d)
        _closedir(f.d);
      return;
    }
    f.dir_name = BString(cwd) + BString(F("/")) + filename;
  } else
    f.f = fopen(filename.c_str(), flags);
  if (!f.f && !f.d)
    err = ERR_FILE_OPEN;

  user_files[filenum] = f;
}

/***bc fs CLOSE
Closes an open file or directory.
\usage CLOSE [#]file_num
\args
@file_num	number of an open file or directory [`0` to `{MAX_USER_FILES_m1}`]
\ref OPEN
***/
void Basic::iclose() {
  int32_t filenum;

  if (*cip == I_SHARP)
    ++cip;

  if (getParam(filenum, 0, MAX_USER_FILES - 1, I_NONE))
    return;

  if (!user_files[filenum].f && !user_files[filenum].d)
    err = ERR_FILE_NOT_OPEN;
  else {
    if (user_files[filenum].f) {
      fclose(user_files[filenum].f);
      if (redirect_output_file == filenum)
        redirect_output_file = -1;
      if (redirect_input_file == filenum)
        redirect_input_file = -1;
      user_files[filenum].f = NULL;
    } else {
      _closedir(user_files[filenum].d);
      user_files[filenum].d = NULL;
    }
  }
}

/***bc fs SEEK
Sets the file position for the next read or write.
\usage SEEK file_num, position
\args
@file_num	number of an open file [`0` to `{MAX_USER_FILES_m1}`]
@position	position where the next read or write should occur
\error
The command will generate an error if `file_num` is not open or the operation
is unsuccessful.
\ref INPUT INPUT$() PRINT
***/
void Basic::iseek() {
  int32_t filenum, pos;

  if (*cip == I_SHARP)
    ++cip;

  if (getParam(filenum, 0, MAX_USER_FILES - 1, I_COMMA))
    return;

  if (!user_files[filenum].f)
    err = ERR_FILE_NOT_OPEN;

  size_t now = ftell(user_files[filenum].f);
  fseek(user_files[filenum].f, 0, SEEK_END);
  size_t max = ftell(user_files[filenum].f);
  fseek(user_files[filenum].f, now, SEEK_SET);

  if (getParam(pos, 0, max, I_NONE))
    return;

  if (fseek(user_files[filenum].f, pos, SEEK_SET))
    err = ERR_FILE_SEEK;
}

/***bc fs FILES
Displays the contents of the current or a specified directory.
\usage FILES [filespec$]
\args
@filespec$	a filename or path, may include wildcard characters +
                [default: all files in the current directory]
\ref CHDIR CWD$()
***/
void Basic::ifiles() {
  BString fname;
  char wildcard[SD_PATH_LEN];
  char *wcard = NULL;
  char *ptr = NULL;
  uint8_t flgwildcard = 0;
  int16_t rc;

  if (!is_strexp())
    fname = "";
  else
    fname = getParamFname();

  if (fname.length() > 0) {
    for (int8_t i = fname.length() - 1; i >= 0; i--) {
      if (fname[i] == '/') {
        ptr = &fname[i];
        break;
      }
      if (fname[i] == '*' || fname[i] == '?' || fname[i] == '.')
        flgwildcard = 1;
    }
    if (ptr != NULL && flgwildcard == 1) {
      strcpy(wildcard, ptr + 1);
      wcard = wildcard;
      *(ptr + 1) = 0;
    } else if (ptr == NULL && flgwildcard == 1) {
      strcpy(wildcard, fname.c_str());
      wcard = wildcard;
      fname = "";
    }
  }

  rc = bfs.flist((char *)fname.c_str(), wcard, sc0.getWidth() / 14);
  if (rc == SD_ERR_INIT) {
    err = ERR_SD_NOT_READY;
  } else if (rc == SD_ERR_OPEN_FILE) {
    err = ERR_FILE_OPEN;
  }
}

/***bc fs MKDIR
Creates a directory.
\usage MKDIR directory$
\args
@directory$	path to the new directory
***/
void Basic::imkdir() {
  uint8_t rc;
  BString fname;

  if (!(fname = getParamFname())) {
    return;
  }

  rc = _mkdir(fname.c_str(), 0755);
  if (rc == SD_ERR_INIT) {
    err = ERR_SD_NOT_READY;
  } else if (rc == SD_ERR_OPEN_FILE) {
    err = ERR_BAD_FNAME;
  }
}

/***bc fs RMDIR
Deletes a directory.
\usage RMDIR directory$
\args
@directory$	name of directory to be deleted
\bugs
Does not support wildcard patterns.
\ref MKDIR
***/
void Basic::irmdir() {
  BString fname;
  uint8_t rc;

  if (!(fname = getParamFname())) {
    return;
  }

  rc = _rmdir(fname.c_str());
  if (rc == SD_ERR_INIT) {
    err = ERR_SD_NOT_READY;
  } else if (rc == SD_ERR_OPEN_FILE) {
    err = ERR_BAD_FNAME;
  }
}

/***bc fs RENAME
Changes the name of a file or directory.
\usage RENAME old$ TO new$
\args
@old$	current file name
@new$	new file name
\bugs
Does not support wildcard patterns.
***/
void Basic::irename() {
  bool rc;

  BString old_fname = getParamFname();
  if (err)
    return;
  if (*cip != I_TO) {
    E_SYNTAX(I_TO);
    return;
  }
  cip++;
  BString new_fname = getParamFname();
  if (err)
    return;

  rc = _rename(old_fname.c_str(), new_fname.c_str());
  if (rc)
    err = ERR_FILE_WRITE;
}

/***bc fs REMOVE
Deletes a file from storage.

WARNING: `REMOVE` does not ask for confirmation before deleting the
         specified file.

WARNING: Do not confuse with `DELETE`, which deletes lines from the
program in memory.
\usage REMOVE file$
\args
@file$	file to be deleted
\bugs
Does not support wildcard patterns.
\ref DELETE
***/
void Basic::iremove() {
  BString fname;

  if (!(fname = getParamFname())) {
    return;
  }

  bool rc = _remove(fname.c_str());
  if (rc) {
    err = ERR_FILE_WRITE;  // XXX: something more descriptive?
    return;
  }
}

/***bc fs COPY
Copies a file.
\usage COPY file$ TO new_file$
\args
@file$		file to be copied
@new_file$	copy of the file to be created
\bugs
Does not support wildcard patterns.
***/
void Basic::icopy() {
  uint8_t rc;

  BString old_fname = getParamFname();
  if (err)
    return;
  if (*cip != I_TO) {
    E_SYNTAX(I_TO);
    return;
  }
  cip++;
  BString new_fname = getParamFname();
  if (err)
    return;

  rc = bfs.fcopy(old_fname.c_str(), new_fname.c_str());
  if (rc)
    err = ERR_FILE_WRITE;
}

/***bf fs COMPARE
Compares two files.
\usage result = COMPARE(file1$, file2$)
\args
@file1$	name of first file to compare
@file2$ name of second file to compare
\ret
If both files are equal, the return value is `0`. Otherwise, the return
value is `-1` or `1`.

If the files are not equal, the sign of the result is determined by the sign
of the difference between the first pair of bytes that differ in `file1$`
and `file2$`.
***/
num_t Basic::ncompare() {
  if (checkOpen())
    return 0;
  BString one = getParamFname();
  if (err)
    return 0;
  if (*cip++ != I_COMMA) {
    E_SYNTAX(I_COMMA);
    return 0;
  }
  BString two = getParamFname();
  if (err || checkClose())
    return 0;
  return bfs.compare(one.c_str(), two.c_str());
}

/***bc fs BSAVE
Saves an area of memory to a file.
\usage BSAVE file$, addr, len
\args
@file$	name of binary file
@addr	memory start address
@len	number of bytes to be saved
\note
`BSAVE` will use 32-bit memory accesses if `addr` and `len` are
multiples of 4, and 8-bit accesses otherwise.

While the `BSAVE` command does a basic sanity check of the given
parameters, it cannot predict any system malfunctions resulting
from its use. Use with caution.
\ref BLOAD
***/
void SMALL Basic::ibsave() {
  uint32_t vadr, len;
  BString fname;
  uint8_t rc;

  if (!(fname = getParamFname())) {
    return;
  }

  if (*cip != I_COMMA) {
    E_SYNTAX(I_COMMA);
    return;
  }
  cip++;
  if (getParam(vadr, I_COMMA))
    return;  // アドレスの取得
  if (getParam(len, I_NONE))
    return;  // データ長の取得

  // アドレスの範囲チェック
  if (!sanitize_addr(vadr, 0) || !sanitize_addr(vadr + len, 0)) {
    err = ERR_RANGE;
    return;
  }

  // ファイルオープン
  rc = bfs.tmpOpen((char *)fname.c_str(), 1);
  if (rc) {
    err = rc;
    return;
  }

  if ((vadr & 3) == 0 && (len & 3) == 0) {
    // データの書込み
    for (uint32_t i = 0; i < len; i += 4) {
      uint32_t c = *(uint32_t *)(unsigned long)vadr;
      if (bfs.tmpWrite((char *)&c, 4)) {
        err = ERR_FILE_WRITE;
        goto DONE;
      }
      vadr += 4;
    }
  } else {
    // データの書込み
    for (uint32_t i = 0; i < len; i++) {
      if (bfs.putch(*(uint8_t *)(unsigned long)vadr)) {
        err = ERR_FILE_WRITE;
        goto DONE;
      }
      vadr++;
    }
  }

DONE:
  bfs.tmpClose();
}

/***bc fs BLOAD
Loads a binary file to memory.
\usage BLOAD file$ TO addr[, len]
\args
@file$	name of binary file
@addr	memory address to be loaded to
@len	number of bytes to be loaded [default: whole file]
\note
`BLOAD` will use 32-bit memory accesses if `addr` and `len` are
multiples of 4, and 8-bit accesses otherwise.

While the `BLOAD` command does a basic sanity check of the given
parameters, it cannot predict any system malfunctions resulting
from its use. Use with caution.
\ref BSAVE
***/
void SMALL Basic::ibload() {
  uint32_t vadr;
  int32_t len = -1;
  int32_t c;
  BString fname;
  uint8_t rc;

  if (!(fname = getParamFname())) {
    return;
  }

  if (*cip != I_TO) {
    E_SYNTAX(I_TO);
    return;
  }
  cip++;
  if (getParam(vadr, I_NONE))
    return;  // アドレスの取得
  if (*cip == I_COMMA) {
    cip++;
    if (getParam(len, I_NONE))
      return;  // データ長の取得
  }

  // ファイルオープン
  rc = bfs.tmpOpen((char *)fname.c_str(), 0);
  if (rc == SD_ERR_INIT) {
    err = ERR_SD_NOT_READY;
    return;
  } else if (rc == SD_ERR_OPEN_FILE) {
    err = ERR_FILE_OPEN;
    return;
  }

  if (len == -1)
    len = bfs.tmpSize();

  // アドレスの範囲チェック
  if (!sanitize_addr(vadr, 0) || !sanitize_addr(vadr + len, 0)) {
    err = ERR_RANGE;
    goto DONE;
  }

  if ((vadr & 3) == 0 && (len & 3) == 0) {
    for (ssize_t i = 0; i < len; i += 4) {
      uint32_t c;
      if (bfs.tmpRead((char *)&c, 4) < 0) {
        err = ERR_FILE_READ;
        goto DONE;
      }
      *((uint32_t *)(unsigned long)vadr) = c;
      vadr += 4;
    }
  } else {
    // データの読込み
    for (ssize_t i = 0; i < len; i++) {
      c = bfs.read();
      if (c < 0) {
        err = ERR_FILE_READ;
        goto DONE;
      }
      *((uint8_t *)(unsigned long)(vadr++)) = c;
    }
  }

DONE:
  bfs.tmpClose();
  return;
}

/***bc fs TYPE
Writes the contents of a file to the screen. When a screen's worth of text
has been printed, it pauses and waits for a key press.
\usage TYPE file$
\args
@file$	name of the file
***/
void Basic::itype() {
  //char fname[SD_PATH_LEN];
  //uint8_t rc;
  int32_t line = 0;
  uint8_t c;

  BString fname;
  int8_t rc;

  if (!(fname = getParamFname())) {
    return;
  }

  while (1) {
    rc = bfs.textOut((char *)fname.c_str(), line, sc0.getHeight());
    if (rc < 0) {
      err = -rc;
      return;
    } else if (rc == 0) {
      break;
    }

    PRINT_P("== More?('Y' or SPACE key) =="); newline();
    c = c_getch();
    if (c != 'y' && c != 'Y' && c != 32)
      break;
    line += sc0.getHeight();
  }
}

/***bc fs FORMAT
Formats the specified file system.

WARNING: This command will erase any files stored on the formatted file
system, including system files and the saved configuration, if any.

IMPORTANT: Formatting the SD card file system does not currently work.
\usage FORMAT <"/flash"|"/sd">
\bugs
SD card formatting is currently broken; use another device to format SD
cards for use with the BASIC Engine.
***/
void SdFormat();
void Basic::iformat() {
#ifdef UNIFILE_USE_OLD_SPIFFS
  BString target = getParamFname();
  if (err)
    return;

  if (target == BString(F("/flash"))) {
    PRINT_P("This will ERASE ALL DATA on the internal flash file system!\n");
    PRINT_P("ARE YOU SURE? (Y/N) ");
    BString answer = getstr();
    if (answer != "Y" && answer != "y") {
      PRINT_P("Aborted\n");
      return;
    }
    PRINT_P("Formatting... ");
    if (SPIFFS.format())
      PRINT_P("Success!\n");
    else
      PRINT_P("Failed.");
  } else if (target == "/sd") {
    SdFormat();
  } else {
    err = ERR_NOT_SUPPORTED;
  }
#else
  err = ERR_NOT_SUPPORTED;
#endif
}
