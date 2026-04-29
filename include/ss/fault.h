/* Fault injection.
 *
 * Wraps a sensor with a programmable fault pattern: a `fault_t`
 * decides per-sample whether to mutate the value (stuck-at, spike,
 * dropped/NaN) before returning it.
 *
 * v3 lets users configure fault patterns by sample-index ranges,
 * stochastic probabilities, or both. The shape is composable so
 * future patterns (clock jitter, partial nibble corruption) plug
 * in by adding cases to `fault_apply`.
 */

#ifndef SS_FAULT_H
#define SS_FAULT_H

#include "ss/sensor.h"

typedef enum fault_kind {
    SS_FAULT_NONE = 0,
    SS_FAULT_STUCK_AT,    /* every sample → constant value */
    SS_FAULT_SPIKE,       /* every k-th sample is multiplied by spike_factor */
    SS_FAULT_DROPPED,     /* every k-th sample is NaN */
    SS_FAULT_RANGE_CLAMP, /* clip to [low, high] */
} fault_kind_t;

typedef struct fault {
    fault_kind_t kind;
    double constant;        /* for STUCK_AT */
    int every_k;            /* for SPIKE / DROPPED */
    double spike_factor;    /* for SPIKE */
    double range_low, range_high;  /* for RANGE_CLAMP */
    int call_index;         /* internal counter */
} fault_t;

void ss_fault_init(fault_t* f, fault_kind_t kind);

/* Apply the fault to `sample`. `index` is the per-sensor sample
 * counter (the caller increments). Returns the (possibly mutated)
 * value. NaN signals a dropped sample. */
double ss_fault_apply(fault_t* f, double sample);

#endif
