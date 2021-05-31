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
S(sscanf)

// wrapper that jumps back to BASIC prompt
R(exit, be_exit)

// stdlib memory allocation
S(malloc)
S(free)
S(realloc)
S(calloc)

// eb bg
S(eb_bg_set_size)
S(eb_bg_set_pattern)
S(eb_bg_set_tile_size)
S(eb_bg_set_window)
S(eb_bg_set_priority)
S(eb_bg_enable)
S(eb_bg_disable)
S(eb_bg_off)
S(eb_bg_load)
S(eb_bg_save)
S(eb_bg_move)

S(eb_sprite_off)
S(eb_sprite_set_pattern)
S(eb_sprite_set_size)
S(eb_sprite_enable)
S(eb_sprite_disable)
S(eb_sprite_set_key)
S(eb_sprite_set_priority)
S(eb_sprite_set_frame)
S(eb_sprite_set_opacity)
S(eb_sprite_set_angle)
S(eb_sprite_set_scale_x)
S(eb_sprite_set_scale_y)
S(eb_sprite_set_alpha)
S(eb_sprite_reload)
S(eb_sprite_move)

S(eb_bg_get_tiles)
S(eb_bg_map_tile)
S(eb_bg_set_tiles)
S(eb_bg_set_tile)

S(eb_frameskip)

S(eb_sprite_tile_collision)
S(eb_sprite_collision)
S(eb_sprite_enabled)
S(eb_sprite_x)
S(eb_sprite_y)
S(eb_sprite_w)
S(eb_sprite_h)

S(eb_bg_x)
S(eb_bg_y)
S(eb_bg_win_x)
S(eb_bg_win_y)
S(eb_bg_win_width)
S(eb_bg_win_height)
S(eb_bg_enabled)

S(eb_sprite_frame_x)
S(eb_sprite_frame_y)
S(eb_sprite_flip_x)
S(eb_sprite_flip_y)
S(eb_sprite_opaque)

S(eb_add_bg_layer)
S(eb_remove_bg_layer)

// eb conio
S(eb_char_set)
S(eb_clrtoeol)
S(eb_cls)
S(eb_csize_height)
S(eb_csize_width)
S(eb_enable_escape_codes)
S(eb_enable_scrolling)
S(eb_font)
S(eb_font_by_name)
S(eb_font_count)
S(eb_getch)
S(eb_kbhit)
S(eb_last_key_event)
S(eb_load_font)
S(eb_locate)
S(eb_pos_x)
S(eb_pos_y)
S(eb_putch)
S(eb_puts)
S(eb_show_cursor)
S(eb_show_cursor)
S(eb_term_getch)
S(eb_term_putch)

// eb_video
S(eb_rgb)
S(eb_color)
S(eb_frame)

// eb_sys
S(eb_wait)
S(eb_tick)
S(eb_process_events)
S(eb_set_cpu_speed)

S(eb_add_command)
S(eb_add_numfun)
S(eb_add_strfun)

// eb_file
S(eb_file_exists)
S(eb_file_size)
S(eb_is_directory)
S(eb_is_file)

S(eb_theme_color)

// stdlib type conversion
S(atoi)
S(atol)

// string functions
S(strcpy)
S(strlen)
S(strcat)
S(strcmp)
S(strncmp)
S(strtol)
S(strtoll)
S(strtod)
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
S(fflush)
S(ungetc)

#ifndef ALLWINNER_BARE_METAL	// XXX: should probably be "if not newlib"
S(putchar)
S(puts)
#endif

// stdio streams
S(__getreent)

// unistd file I/O
//S(open)	flags differ between platforms
S(close)
//S(creat)	not in newlib
S(read)
S(write)
S(lseek)	// assumes "whence" defines to be identical across platforms
#ifdef ALLWINNER_BARE_METAL
S(stat)
S(fstat)
//S(lstat)	not in newlib
#else
R(stat, _native_stat)
R(fstat, _native_fstat)
//R(lstat, _native_lstat)
#endif

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
S(random)
S(labs)

// unistd getopt
S(getopt)
R(optarg, &optarg)
R(optind, &optind)

// unistd misc
R(environ, &environ)

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

// math
S(sqrt)
S(pow)
S(sin)
S(cos)
S(log)
S(log10)
S(asin)
S(acos)
S(atan)
S(asinh)
S(acosh)
S(ceil)
S(floor)
S(fabs)
S(j0)
S(j1)
S(y0)
S(y1)
S(exp)
S(cbrt)
S(sinh)
S(cosh)
S(tan)
S(tanh)
S(atanh)
S(erf)
S(erfc)

// time
S(time)
S(gettimeofday)
S(localtime)

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
S(__aeabi_memmove)
S(__aeabi_uidiv)
S(__aeabi_d2lz)
S(__aeabi_uidivmod)
#endif

// sts_mixer
S(sts_mixer_init)
S(sts_mixer_shutdown)
S(sts_mixer_get_active_voices)
S(sts_mixer_play_sample)
S(sts_mixer_play_stream)
S(sts_mixer_stop_voice)
S(sts_mixer_stop_sample)
S(sts_mixer_stop_stream)
S(sts_mixer_mix_audio)
S(eb_get_mixer)

// stb_image
S(stbi_load_from_memory)
S(stbi_load_from_callbacks)
S(stbi_load)
S(stbi_load_from_file)
S(stbi_load_gif_from_memory)
S(stbi_load_16_from_memory)
S(stbi_load_16_from_callbacks)
S(stbi_load_16)
S(stbi_load_from_file_16)
S(stbi_is_hdr_from_callbacks)
S(stbi_is_hdr_from_memory)
S(stbi_is_hdr)
S(stbi_is_hdr_from_file)
S(stbi_failure_reason)
S(stbi_image_free)
S(stbi_info_from_memory)
S(stbi_info_from_callbacks)
S(stbi_is_16_bit_from_memory)
S(stbi_is_16_bit_from_callbacks)
S(stbi_info)
S(stbi_info_from_file)
S(stbi_is_16_bit)
S(stbi_is_16_bit_from_file)
S(stbi_set_unpremultiply_on_load)
S(stbi_convert_iphone_png_to_rgb)
S(stbi_set_flip_vertically_on_load)
S(stbi_zlib_decode_malloc_guesssize)
S(stbi_zlib_decode_malloc_guesssize_headerflag)
S(stbi_zlib_decode_malloc)
S(stbi_zlib_decode_buffer)
S(stbi_zlib_decode_noheader_malloc)
S(stbi_zlib_decode_noheader_buffer)

// stb_image_resize
S(stbir_resize)
S(stbir_resize_float)
S(stbir_resize_float_generic)
S(stbir_resize_region)
S(stbir_resize_subpixel)
S(stbir_resize_uint16_generic)
S(stbir_resize_uint8)
S(stbir_resize_uint8_generic)
S(stbir_resize_uint8_srgb)
S(stbir_resize_uint8_srgb_edgemode)

// stb_image_write
S(stbi_flip_vertically_on_write)
S(stbi_write_bmp)
S(stbi_write_bmp_to_func)
S(stbi_write_hdr)
S(stbi_write_hdr_to_func)
S(stbi_write_jpg)
S(stbi_write_jpg_to_func)
S(stbi_write_png)
S(stbi_write_png_to_func)
S(stbi_write_tga)
S(stbi_write_tga_to_func)

// utf8.h
S(utf8casecmp)
S(utf8cat)
S(utf8chr)
S(utf8cmp)
S(utf8cpy)
S(utf8cspn)
S(utf8dup)
S(utf8len)
S(utf8nlen)
S(utf8ncasecmp)
S(utf8ncat)
S(utf8ncmp)
S(utf8ncpy)
S(utf8ndup)
S(utf8pbrk)
S(utf8rchr)
S(utf8size)
S(utf8size_lazy)
S(utf8nsize_lazy)
S(utf8spn)
S(utf8str)
S(utf8casestr)
S(utf8valid)
S(utf8nvalid)
S(utf8makevalid)
S(utf8codepoint)
S(utf8codepointcalcsize)
S(utf8codepointsize)
S(utf8catcodepoint)
S(utf8islower)
S(utf8isupper)
S(utf8lwr)
S(utf8upr)
S(utf8lwrcodepoint)
S(utf8uprcodepoint)
S(utf8rcodepoint)
S(utf8dup_ex)
S(utf8ndup_ex)
