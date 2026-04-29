#include <math.h>
#include <stdio.h>

#include "ss/sensor.h"

static int failed = 0;
#define CHECK(cond) do { if (!(cond)) { fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); failed++; } } while (0)

static void test_zero_drift_zero_noise_returns_quantized_truth(void) {
    sensor_t s;
    ss_sensor_init(&s, 1, 0.0, 0.0, 0.1, 1);
    double v = ss_sensor_sample(&s, 1.234, 0);
    /* round(1.234 / 0.1) * 0.1 = 1.2 */
    CHECK(fabs(v - 1.2) < 1e-9);
}

static void test_drift_accumulates_with_time(void) {
    sensor_t s;
    ss_sensor_init(&s, 1, 0.5 /* drift */, 0.0, 0.0, 1);
    double v0 = ss_sensor_sample(&s, 10.0, 0.0);
    double v10 = ss_sensor_sample(&s, 10.0, 10.0);
    CHECK(fabs(v0 - 10.0) < 1e-9);
    CHECK(fabs(v10 - 15.0) < 1e-9);  /* 10 + 0.5*10 = 15 */
}

static void test_noise_is_zero_mean_for_large_n(void) {
    sensor_t s;
    ss_sensor_init(&s, 1, 0.0, 0.5, 0.0, 7);
    double sum = 0.0;
    int n = 5000;
    for (int i = 0; i < n; ++i) sum += ss_sensor_sample(&s, 0.0, 0.0);
    double mean = sum / n;
    /* With std=0.5 and n=5000, the SE of the mean is ~0.0071. We
     * give 5 sigma headroom. */
    CHECK(fabs(mean) < 0.05);
}

static void test_seeds_diverge(void) {
    sensor_t a, b;
    ss_sensor_init(&a, 1, 0.0, 1.0, 0.0, 1);
    ss_sensor_init(&b, 2, 0.0, 1.0, 0.0, 2);
    double sa = ss_sensor_sample(&a, 0.0, 0.0);
    double sb = ss_sensor_sample(&b, 0.0, 0.0);
    /* Two seeds should produce different first samples. */
    CHECK(fabs(sa - sb) > 1e-9);
}

static void test_xorshift_is_deterministic(void) {
    uint64_t a = 1, b = 1;
    for (int i = 0; i < 1000; ++i) {
        CHECK(ss_xorshift64(&a) == ss_xorshift64(&b));
    }
}

int main(void) {
    test_zero_drift_zero_noise_returns_quantized_truth();
    test_drift_accumulates_with_time();
    test_noise_is_zero_mean_for_large_n();
    test_seeds_diverge();
    test_xorshift_is_deterministic();
    printf("sensor: %d failures\n", failed);
    return failed;
}
