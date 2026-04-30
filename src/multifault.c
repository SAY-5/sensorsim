/* See include/ss/multifault.h. */

#include "ss/multifault.h"

#include <string.h>

void ss_multifault_init(multifault_t* mf) {
    memset(mf, 0, sizeof(*mf));
}

int ss_multifault_add(multifault_t* mf, fault_kind_t kind) {
    if (mf->count >= SS_MULTIFAULT_MAX) return -1;
    ss_fault_init(&mf->stages[mf->count], kind);
    mf->count++;
    return mf->count - 1;
}

double ss_multifault_apply(multifault_t* mf, double sample) {
    double v = sample;
    for (int i = 0; i < mf->count; ++i) {
        v = ss_fault_apply(&mf->stages[i], v);
    }
    return v;
}
