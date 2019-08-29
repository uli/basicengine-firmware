#ifdef __cplusplus
extern "C" {
#endif

//main soundcard routines
extern char* AU_search(unsigned int config);
extern unsigned int AU_cardbuf_space(void);
extern void AU_start(void);
extern void AU_stop(void);
extern void AU_close(void);
extern void AU_setrate(unsigned int *fr, unsigned int *bt, unsigned int *ch);
extern void AU_setmixer_all(unsigned int vol);
extern void AU_writedata(char *pcm, long len, unsigned int look);


#ifdef __cplusplus
}
#endif
