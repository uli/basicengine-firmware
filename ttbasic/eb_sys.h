// SPDX-License-Identifier: MIT
// Copyright (c) 2021 Ulrich Hecht

#ifdef __cplusplus
extern "C" {
#endif

void eb_wait(unsigned int ms);
unsigned int eb_tick(void);
unsigned int eb_utick(void);
void eb_process_events(void);
int eb_process_events_check(void);
int eb_process_events_wait(void);
void eb_udelay(unsigned int us);
void eb_set_cpu_speed(int percent);

int eb_install_module(const char *filename);
int eb_load_module(const char *name);
int eb_module_count(void);
const char *eb_module_name(int index);

#ifdef __cplusplus
}
#endif
