/* See include/ss/fault.h. */

#include "ss/fault.h"

#include <math.h>

void ss_fault_init(fault_t* f, fault_kind_t kind) {
    f->kind = kind;
    f->constant = 0.0;
    f->every_k = 1;
    f->spike_factor = 1.0;
    f->range_low = 0.0;
    f->range_high = 0.0;
    f->call_index = 0;
}

double ss_fault_apply(fault_t* f, double sample) {
    int idx = f->call_index++;
    switch (f->kind) {
        case SS_FAULT_NONE:
            return sample;
        case SS_FAULT_STUCK_AT:
            return f->constant;
        case SS_FAULT_SPIKE:
            if (f->every_k > 0 && idx % f->every_k == 0) return sample * f->spike_factor;
            return sample;
        case SS_FAULT_DROPPED:
            if (f->every_k > 0 && idx % f->every_k == 0) return NAN;
            return sample;
        case SS_FAULT_RANGE_CLAMP:
            if (sample < f->range_low) return f->range_low;
            if (sample > f->range_high) return f->range_high;
            return sample;
    }
    return sample;
}
