/* v4: composite fault patterns.
 *
 * v3's fault model is one-fault-at-a-time. Real sensor failures
 * compose: a stuck-at can fire a few times *and* drop samples *and*
 * spike out-of-range. v4 adds MultiFault that chains N independent
 * fault stages so a single sample passes through every active
 * pattern in order.
 *
 * Composition order matters: a stuck-at OVERWRITES the value, so
 * stuck-at after a spike means the spike is lost. We document this
 * by exposing the order as a deliberate config knob.
 */

#ifndef SS_MULTIFAULT_H
#define SS_MULTIFAULT_H

#include "ss/fault.h"

#define SS_MULTIFAULT_MAX 4

typedef struct multifault {
    fault_t stages[SS_MULTIFAULT_MAX];
    int count;
} multifault_t;

void ss_multifault_init(multifault_t* mf);
int ss_multifault_add(multifault_t* mf, fault_kind_t kind);
double ss_multifault_apply(multifault_t* mf, double sample);

#endif
