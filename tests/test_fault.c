#include <math.h>
#include <stdio.h>

#include "ss/fault.h"

static int failed = 0;
#define CHECK(cond) do { if (!(cond)) { fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); failed++; } } while (0)

static void test_none_passes_through(void) {
    fault_t f;
    ss_fault_init(&f, SS_FAULT_NONE);
    for (int i = 0; i < 10; ++i) CHECK(ss_fault_apply(&f, 1.5) == 1.5);
}

static void test_stuck_at_returns_constant(void) {
    fault_t f;
    ss_fault_init(&f, SS_FAULT_STUCK_AT);
    f.constant = 7.0;
    CHECK(ss_fault_apply(&f, 1.0) == 7.0);
    CHECK(ss_fault_apply(&f, 99.0) == 7.0);
}

static void test_spike_every_kth(void) {
    fault_t f;
    ss_fault_init(&f, SS_FAULT_SPIKE);
    f.every_k = 3;
    f.spike_factor = 10.0;
    /* indices 0, 3, 6, ... get spiked. */
    CHECK(ss_fault_apply(&f, 1.0) == 10.0);  /* idx 0 */
    CHECK(ss_fault_apply(&f, 1.0) == 1.0);   /* idx 1 */
    CHECK(ss_fault_apply(&f, 1.0) == 1.0);   /* idx 2 */
    CHECK(ss_fault_apply(&f, 1.0) == 10.0);  /* idx 3 */
}

static void test_dropped_emits_nan(void) {
    fault_t f;
    ss_fault_init(&f, SS_FAULT_DROPPED);
    f.every_k = 2;
    CHECK(isnan(ss_fault_apply(&f, 1.0)));   /* idx 0 */
    CHECK(ss_fault_apply(&f, 1.0) == 1.0);   /* idx 1 */
    CHECK(isnan(ss_fault_apply(&f, 1.0)));   /* idx 2 */
}

static void test_range_clamp_clips(void) {
    fault_t f;
    ss_fault_init(&f, SS_FAULT_RANGE_CLAMP);
    f.range_low = 0.0;
    f.range_high = 1.0;
    CHECK(ss_fault_apply(&f, -5.0) == 0.0);
    CHECK(ss_fault_apply(&f, 0.5) == 0.5);
    CHECK(ss_fault_apply(&f, 100.0) == 1.0);
}

int main(void) {
    test_none_passes_through();
    test_stuck_at_returns_constant();
    test_spike_every_kth();
    test_dropped_emits_nan();
    test_range_clamp_clips();
    printf("fault: %d failures\n", failed);
    return failed;
}
