#ifdef __cplusplus
extern "C" {
#endif

int eb_file_exists(const char *name);
int eb_is_directory(const char *name);
int eb_is_file(const char *name);
int eb_file_size(const char *name);

#ifdef __cplusplus
}
#endif
