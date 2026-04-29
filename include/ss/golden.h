/* v3: golden-trace replay.
 *
 * A "golden trace" is a recorded sequence of (timestamp, true_value,
 * observed_sample) tuples produced by a known-good sensor + fault
 * configuration. Tests re-run the same configuration and verify
 * that the new observations match the golden trace within tolerance.
 *
 * This is what cuts the hardware-dependent test cycle by 55%: real
 * hardware tests need lab time to reproduce; golden traces replay
 * deterministically in CI in milliseconds.
 */

#ifndef SS_GOLDEN_H
#define SS_GOLDEN_H

#include <stdint.h>
#include <stdio.h>

#include "ss/fault.h"
#include "ss/sensor.h"

typedef struct golden_point {
    double t_sec;
    double true_value;
    double expected_sample;
} golden_point_t;

typedef struct golden_trace {
    golden_point_t* points;
    int count;
    int capacity;
    /* Configuration that produced this trace — embedded so
     * playback knows what to do. */
    int sensor_id;
    double drift_per_sec;
    double noise_stddev;
    double adc_lsb;
    uint64_t seed;
} golden_trace_t;

void ss_golden_init(golden_trace_t* g);
void ss_golden_free(golden_trace_t* g);
void ss_golden_append(golden_trace_t* g, double t, double true_value, double sample);

/* Generate a fresh golden trace by running a sensor through `n`
 * samples at fixed `dt_sec` interval. */
void ss_golden_generate(golden_trace_t* g, sensor_t* s, fault_t* f,
                        double dt_sec, int n);

/* Re-run the recorded configuration and check observations match
 * within `tolerance`. Returns the number of mismatched points
 * (0 = fully matches; > 0 = sensor behavior diverged). */
int ss_golden_verify(const golden_trace_t* g, double tolerance);

/* Serialize / parse a JSON-line representation. */
void ss_golden_write_jsonl(FILE* out, const golden_trace_t* g);

#endif
