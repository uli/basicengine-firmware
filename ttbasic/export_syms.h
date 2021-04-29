// unistd time
R(usleep, eb_udelay)

// stdio string formatting
R(printf, c_printf)
S(asprintf)
S(snprintf)
S(sprintf)
S(vasprintf)
S(vsnprintf)
S(vsprintf)

// wrapper that jumps back to BASIC prompt
R(exit, be_exit)

// stdlib memory allocation
S(malloc)
S(free)
S(realloc)
S(calloc)

// eb conio
S(eb_locate)
S(eb_pos_x)
S(eb_pos_y)
S(eb_putch)
S(eb_getch)
S(eb_clrtoeol)
S(eb_cls)
S(eb_puts)
S(eb_show_cursor)
S(eb_char_set)
S(eb_csize_height)
S(eb_csize_width)
S(eb_enable_scrolling)
S(eb_show_cursor)
S(eb_kbhit)
S(eb_last_key_event)

// eb_video
S(eb_rgb)
S(eb_color)

// eb_sys
S(eb_wait)
S(eb_tick)
S(eb_process_events)

// eb_file
S(eb_file_exists)
S(eb_file_size)
S(eb_is_directory)
S(eb_is_file)

S(eb_theme_color)

// stdlib type conversion
S(atoi)

// string functions
S(strcpy)
S(strlen)
S(strcat)
S(strcmp)
S(strncmp)
S(strtol)
S(strtoll)
R(strchr, __builtin_strchr)
R(strstr, __builtin_strstr)
R(strrchr, __builtin_strrchr)
S(strdup)
S(strerror)
S(strspn)
S(strcspn)
S(strncpy)
S(strndup)
S(strcasecmp)

// unistd file ops
S(unlink)
S(mkstemp)
S(rename)
S(mkdir)
S(rmdir)
S(chdir)

// dirent directory functions
#ifdef ALLWINNER_BARE_METAL
S(readdir)
#else
R(readdir, be_readdir)
#endif
S(closedir)
S(opendir)
S(getcwd)

//S(fnmatch)	in newlib, but only built for posix targets

// newlib-style errno wrapper
S(__errno)


//S(stat)	struct stat differs between platforms
//S(realpath)	not in newlib

// stdio stream interface
S(fopen)
S(fclose)
S(fputc)
S(ferror)
S(fgets)
S(fgetc)
S(fputs)
S(fprintf)
S(fread)
S(fseek)
S(ftell)
S(fwrite)
S(fdopen)
//S(fstat)	struct stat differs between platforms
S(fileno)

// stdio streams
S(__getreent)

// unistd file I/O
//S(open)	flags differ between platforms
S(close)
S(read)
S(write)

// memory
S(memcmp)
S(memcpy)
S(memset)
S(memmove)
S(bcopy)
S(bzero)
S(memrchr)

// stdlib environment
// XXX: do these actually work in a useful manner?
S(putenv)
S(getenv)

// stdlib miscellaneous
S(qsort)
R(abs, __builtin_abs)

// unistd getopt
S(getopt)
R(optarg, &optarg)
R(optind, &optind)

// ctype
S(isdigit)
S(isalnum)
S(isalpha)
S(isblank)
S(iscntrl)
S(isgraph)
S(islower)
S(isprint)
S(ispunct)
S(isspace)
S(isupper)
S(isxdigit)
S(toupper)
S(tolower)

// mcurses
// XXX: Some of these have _very_ generic names and might collide with user
// programs...
S(addch)
S(addstr)
S(attrset)
S(clear)
S(clrtoeol)
S(curs_set)
S(endwin)
S(getch)
S(initscr)
S(move)
S(refresh)

// stdlib multi-byte functions
S(mbtowc)
S(wcwidth)

#ifdef __x86_64__
S(__va_start)
S(__va_arg)
#endif

#ifdef __arm__
S(__aeabi_idiv)
S(__aeabi_idivmod)
S(__aeabi_lasr)
S(__aeabi_ldivmod)
S(__aeabi_llsl)
S(__aeabi_llsr)
S(__aeabi_memcpy4)
S(__aeabi_memcpy8)
S(__aeabi_uidiv)
#endif
