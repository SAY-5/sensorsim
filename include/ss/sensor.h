/* Sensor model.
 *
 * A `sensor_t` produces samples deterministically given (true_value,
 * timestamp, seed). The output incorporates:
 *
 *  - sampling drift: linear bias drift_per_sec * t
 *  - signal noise: zero-mean Gaussian (Box-Muller from a per-sensor
 *    PRNG so multiple sensors don't share a stream)
 *  - quantization: round to ADC LSB
 *
 * The PRNG is a 64-bit xorshift seeded from the sensor's `seed` field;
 * the seed seeded from clock_gettime + a per-sensor offset on init,
 * but the test path passes an explicit seed so trials are
 * reproducible.
 */

#ifndef SS_SENSOR_H
#define SS_SENSOR_H

#include <stdint.h>

typedef struct sensor {
    int id;
    double drift_per_sec;   /* linear bias added per second of t */
    double noise_stddev;    /* Gaussian noise std-dev */
    double adc_lsb;         /* quantization step */
    uint64_t prng_state;    /* xorshift state */
} sensor_t;

/* Initialize. Seed is a 64-bit value; pass 0 to draw from
 * clock_gettime. */
void ss_sensor_init(sensor_t* s, int id, double drift_per_sec,
                    double noise_stddev, double adc_lsb, uint64_t seed);

/* Read one sample for `true_value` at time `t_sec` (since the
 * sensor's epoch). The sensor's PRNG advances. */
double ss_sensor_sample(sensor_t* s, double true_value, double t_sec);

/* Internal: xorshift step. Exposed for tests + the assembly
 * timestamp helper. */
uint64_t ss_xorshift64(uint64_t* state);

/* Inline-assembly timestamp counter. Used as a high-resolution
 * seed source on platforms where clock_gettime resolution might be
 * coarse. Falls back to 0 if the architecture isn't supported. */
uint64_t ss_rdtsc_or_cntvct(void);

#endif
