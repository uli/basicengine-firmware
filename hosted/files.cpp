#include "SdFat.h"

char full_path[1024];

const char *apsd(const char *p) {
    sprintf(full_path, FS_PREFIX "/sd%s", p);
    printf("FP %s\n",full_path);
    return full_path;
}
const char *apfs(const char *p) {
    sprintf(full_path, FS_PREFIX "/flash%s", p);
    return full_path;
}
