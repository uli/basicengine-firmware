R(printf, c_printf)

R(usleep, delayMicroseconds)

S(asprintf)
S(snprintf)
S(sprintf)
S(strncpy)
S(vasprintf)
S(vsnprintf)
S(vsprintf)

R(exit, be_exit)

S(malloc)
S(free)
S(realloc)
S(calloc)

S(eb_locate)
S(eb_pos_x)
S(eb_pos_y)
S(eb_putch)
S(eb_getch)
S(eb_clrtoeol)
S(eb_cls)
S(eb_puts)
S(eb_show_cursor)
S(eb_csize_height)
S(eb_csize_width)
S(eb_enable_scrolling)
S(eb_show_cursor)
S(eb_kbhit)
S(eb_last_key_event)

S(eb_rgb)
S(eb_color)

S(eb_wait)
S(eb_tick)

S(eb_file_exists)
S(eb_file_size)
S(eb_is_directory)
S(eb_is_file)

S(eb_theme_color)

S(atoi)

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
S(strndup)
S(strcasecmp)

S(write)

S(unlink)
S(mkstemp)
S(rename)
S(mkdir)
S(rmdir)

S(__errno)

S(readdir)
S(closedir)
S(opendir)
S(getcwd)
S(chdir)
//S(stat)	struct stat differs between platforms
//S(realpath)	not in newlib

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

//S(open)	flags differ between platforms
S(close)
S(read)
S(write)

R(stdout, &stdout)
R(stdin, &stdin)
R(stderr, &stderr)

S(memcmp)
S(memcpy)
S(memset)
S(memmove)
S(bcopy)
S(bzero)
S(memrchr)

S(putenv)
S(getenv)

S(qsort)
R(abs, __builtin_abs)

S(getopt)
R(optarg, &optarg)
R(optind, &optind)

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

S(mbtowc)
S(wcwidth)

#ifdef __x86_64__
S(__va_start)
S(__va_arg)
#endif
