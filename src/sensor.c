/* See include/ss/sensor.h. */

#include "ss/sensor.h"

#include <math.h>
#include <time.h>

uint64_t ss_xorshift64(uint64_t* state) {
    uint64_t x = *state;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    *state = x ? x : 1;
    return x;
}

/* Cycle-counter read via inline assembly. On x86_64 we use rdtsc;
 * on aarch64 we read cntvct_el0; otherwise we fall through to 0
 * and the caller seeds from clock_gettime instead. The asm here
 * is the resume bullet's assembly content — small but explicit. */
uint64_t ss_rdtsc_or_cntvct(void) {
#if defined(__x86_64__)
    uint32_t lo, hi;
    __asm__ volatile("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
#elif defined(__aarch64__)
    uint64_t v;
    __asm__ volatile("mrs %0, cntvct_el0" : "=r"(v));
    return v;
#else
    return 0;
#endif
}

void ss_sensor_init(sensor_t* s, int id, double drift_per_sec,
                    double noise_stddev, double adc_lsb, uint64_t seed) {
    s->id = id;
    s->drift_per_sec = drift_per_sec;
    s->noise_stddev = noise_stddev;
    s->adc_lsb = adc_lsb > 0 ? adc_lsb : 0.0;
    if (seed == 0) {
        /* Mix wall clock + cycle counter so multiple sensors don't
         * collide on identical seeds when constructed together. */
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        seed = (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
        seed ^= ss_rdtsc_or_cntvct();
        seed ^= (uint64_t)id * 0x9E3779B97F4A7C15ULL;
    }
    s->prng_state = seed;
}

/* Box-Muller: draw two uniforms in (0, 1] from xorshift, transform
 * to one Gaussian. We discard the second draw to keep state simple;
 * the cost is negligible in this benchmark. */
static double sample_gaussian(uint64_t* prng) {
    uint64_t a = ss_xorshift64(prng);
    uint64_t b = ss_xorshift64(prng);
    /* Map to (0, 1] — avoid 0 to dodge log(0). */
    double u1 = ((double)a / (double)UINT64_MAX);
    double u2 = ((double)b / (double)UINT64_MAX);
    if (u1 <= 1e-18) u1 = 1e-18;
    return sqrt(-2.0 * log(u1)) * cos(2.0 * 3.14159265358979323846 * u2);
}

double ss_sensor_sample(sensor_t* s, double true_value, double t_sec) {
    double v = true_value + s->drift_per_sec * t_sec;
    if (s->noise_stddev > 0) {
        v += s->noise_stddev * sample_gaussian(&s->prng_state);
    }
    if (s->adc_lsb > 0) {
        v = round(v / s->adc_lsb) * s->adc_lsb;
    }
    return v;
}
