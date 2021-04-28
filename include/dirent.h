#ifdef __cplusplus
extern "C" {
#endif

#include "fatfs/ff.h"

struct dirent {
	unsigned char d_type;
	char d_name[256];	/* filename */
};

#define DT_DIR	4
#define DT_REG	8

DIR *opendir(const char *name);
int closedir(DIR *dirp);
struct dirent *readdir(DIR *dir);

#ifdef __cplusplus
}
#endif
