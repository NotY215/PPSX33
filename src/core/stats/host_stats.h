#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

void host_stats_init(void);
void host_stats_sample(void);          /* call periodically from progress_hw */
void host_stats_write_log_section(void); /* after main log — peaks */

uint64_t host_stats_peak_working_set(void);
uint32_t host_stats_peak_cpu_permille(void); /* 0..1000 = 0..100.0% */
uint64_t host_stats_peak_io_read(void);
uint64_t host_stats_peak_io_write(void);
uint64_t host_stats_present_frames(void);
void host_stats_note_present(void);

#ifdef __cplusplus
}
#endif
