/* See include/ss/golden.h. */

#include "ss/golden.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

void ss_golden_init(golden_trace_t* g) {
    memset(g, 0, sizeof(*g));
}

void ss_golden_free(golden_trace_t* g) {
    free(g->points);
    g->points = NULL;
    g->count = 0;
    g->capacity = 0;
}

void ss_golden_append(golden_trace_t* g, double t, double true_value, double sample) {
    if (g->count == g->capacity) {
        int cap = g->capacity == 0 ? 64 : g->capacity * 2;
        g->points = (golden_point_t*)realloc(g->points, sizeof(golden_point_t) * (size_t)cap);
        g->capacity = cap;
    }
    g->points[g->count++] = (golden_point_t){t, true_value, sample};
}

void ss_golden_generate(golden_trace_t* g, sensor_t* s, fault_t* f,
                        double dt_sec, int n) {
    g->sensor_id = s->id;
    g->drift_per_sec = s->drift_per_sec;
    g->noise_stddev = s->noise_stddev;
    g->adc_lsb = s->adc_lsb;
    g->seed = s->prng_state;
    for (int i = 0; i < n; ++i) {
        double t = (double)i * dt_sec;
        double truth = 25.0 + 0.5 * sin(t);  /* canonical "warming + oscillating" signal */
        double sample = ss_sensor_sample(s, truth, t);
        sample = ss_fault_apply(f, sample);
        ss_golden_append(g, t, truth, sample);
    }
}

int ss_golden_verify(const golden_trace_t* g, double tolerance) {
    sensor_t s;
    fault_t f;
    ss_sensor_init(&s, g->sensor_id, g->drift_per_sec, g->noise_stddev, g->adc_lsb, g->seed);
    ss_fault_init(&f, SS_FAULT_NONE);
    int mismatches = 0;
    for (int i = 0; i < g->count; ++i) {
        const golden_point_t* p = &g->points[i];
        double sample = ss_sensor_sample(&s, p->true_value, p->t_sec);
        sample = ss_fault_apply(&f, sample);
        if (isnan(p->expected_sample) && isnan(sample)) continue;
        double delta = fabs(sample - p->expected_sample);
        if (delta > tolerance) ++mismatches;
    }
    return mismatches;
}

void ss_golden_write_jsonl(FILE* out, const golden_trace_t* g) {
    for (int i = 0; i < g->count; ++i) {
        fprintf(out,
                "{\"t\":%.9f,\"truth\":%.6f,\"sample\":%.6f}\n",
                g->points[i].t_sec, g->points[i].true_value, g->points[i].expected_sample);
    }
}
