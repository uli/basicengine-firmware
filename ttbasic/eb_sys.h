#ifdef __cplusplus
extern "C" {
#endif

void eb_wait(unsigned int ms);
unsigned int eb_tick(void);
void eb_process_events(void);
void eb_udelay(unsigned int us);
void eb_set_cpu_speed(int percent);

#ifdef __cplusplus
}
#endif
