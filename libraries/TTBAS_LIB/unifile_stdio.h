#include <stdio.h>

#ifndef EOF
#define EOF (-1)
#endif

#ifdef __cplusplus
extern "C" {
#endif

FILE *_fopen(const char *path, const char *mode);
int _fclose(FILE *stream);
size_t _fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
size_t _fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);
int _fseek(FILE *stream, int offset, int whence);
int _feof(FILE *stream);
int _getc(FILE *stream);
int _ungetc(int c, FILE *stream);
int _fflush(FILE *stream);
int _ferror(FILE *stream);
void _clearerr(FILE *stream);

#ifdef __cplusplus
}
#endif

#define fopen _fopen
#define fclose _fclose
#define fread _fread
#define fwrite _fwrite
#define fseek _fseek
#define feof _feof
#undef getc
#define getc _getc
#define ungetc _ungetc
#define fflush _fflush
#define ferror _ferror
#define clearerr _clearerr
