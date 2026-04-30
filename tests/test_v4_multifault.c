#include <math.h>
#include <stdio.h>

#include "ss/multifault.h"

static int failed = 0;
#define CHECK(cond) do { if (!(cond)) { fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); failed++; } } while (0)

static void test_empty_chain_passes_through(void) {
    multifault_t mf;
    ss_multifault_init(&mf);
    CHECK(ss_multifault_apply(&mf, 5.5) == 5.5);
}

static void test_chained_clamp_then_spike(void) {
    multifault_t mf;
    ss_multifault_init(&mf);
    int clamp = ss_multifault_add(&mf, SS_FAULT_RANGE_CLAMP);
    int spike = ss_multifault_add(&mf, SS_FAULT_SPIKE);
    mf.stages[clamp].range_low = 0;
    mf.stages[clamp].range_high = 10;
    mf.stages[spike].every_k = 1;
    mf.stages[spike].spike_factor = 3.0;
    /* Input 100 → clamped to 10 → spiked × 3 = 30. */
    CHECK(ss_multifault_apply(&mf, 100.0) == 30.0);
}

static void test_capacity_cap(void) {
    multifault_t mf;
    ss_multifault_init(&mf);
    for (int i = 0; i < SS_MULTIFAULT_MAX; ++i) {
        CHECK(ss_multifault_add(&mf, SS_FAULT_NONE) >= 0);
    }
    /* One more should fail with -1. */
    CHECK(ss_multifault_add(&mf, SS_FAULT_NONE) == -1);
}

static void test_drop_terminates_chain_with_nan(void) {
    multifault_t mf;
    ss_multifault_init(&mf);
    int drop = ss_multifault_add(&mf, SS_FAULT_DROPPED);
    mf.stages[drop].every_k = 1;
    /* Drop forces NaN; downstream stages can't recover from NaN
     * with arithmetic so we just verify NaN propagates. */
    CHECK(isnan(ss_multifault_apply(&mf, 1.0)));
}

int main(void) {
    test_empty_chain_passes_through();
    test_chained_clamp_then_spike();
    test_capacity_cap();
    test_drop_terminates_chain_with_nan();
    printf("multifault: %d failures\n", failed);
    return failed;
}
