R(printf, c_printf)
S(sprintf)
S(strncpy)
S(vsnprintf)

S(exit)

S(malloc)
S(free)

S(eb_locate)
S(eb_putch)
S(eb_getch)
S(eb_clrtoeol)
S(eb_cls)
S(eb_puts)
S(eb_show_cursor)
S(eb_csize_height)
S(eb_csize_width)
S(eb_enable_scrolling)

S(atoi)

S(strcpy)
S(strlen)
S(strcat)
S(strcmp)
S(strncmp)

S(write)

S(unlink)
S(mkstemp)

S(fopen)
S(fclose)
S(fputc)
S(ferror)
S(fgets)
S(fgetc)
S(fputs)
S(fprintf)

R(stdout, &stdout)
R(stdin, &stdin)
R(stderr, &stderr)

S(memcmp)
S(memcpy)

#ifdef __x86_64__
S(__va_start)
S(__va_arg)
#endif
