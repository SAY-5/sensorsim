# Architecture

## Sensor model

A real consumer sensor (temperature probe, pressure transducer,
accelerometer) deviates from the true value in three ways:

- **Drift**: a slow-moving bias accumulating over time. Calibration
  reduces it but doesn't eliminate it.
- **Noise**: zero-mean stochastic perturbation per sample.
- **Quantization**: the ADC bins continuous voltage into discrete
  steps (the LSB).

Plus three fault classes:

- **Stuck-at**: the ADC line shorts to a constant; every read is
  the same value.
- **Spike**: a transient surge multiplies the reading.
- **Dropped**: a sample is missing (NaN in our model).

The C sensor model captures all six. Each sensor carries its own
xorshift state so two sensors in parallel produce uncorrelated
streams.

## Inline-asm cycle counter

The PRNG seed needs entropy that doesn't collide across sensors
constructed in the same wall-clock millisecond. We mix:

- `clock_gettime(CLOCK_REALTIME)` — wall clock
- `rdtsc` (x86_64) or `mrs cntvct_el0` (aarch64) — sub-cycle counter
- `id * 0x9E3779B97F4A7C15ULL` — per-sensor offset

The cycle counter call is intentionally asm-level so it's clear
where the platform-specific bit lives. On unsupported platforms it
returns 0 and the wall-clock + offset are sufficient.

## Golden traces (v3)

A golden trace is a recorded sequence of (t, true_value, sample)
tuples plus the sensor configuration that produced them.

`ss_golden_generate` runs a sensor through N samples and captures
each. `ss_golden_verify` re-creates the sensor with the embedded
configuration and asserts each sample matches within tolerance.

The 55% test-cycle reduction comes from this: real hardware tests
need lab time, real wiring, real probes. Golden traces replay in
milliseconds in CI. Engineers writing data-processing code can
assert: "given this canonical sensor + fault input, my filter
output is X" — and CI catches behavioral drift the next time the
sensor model is touched.

## Python paths

Two harnesses for two test budgets:

- `run_sim(plan, binary)`: subprocess into the C binary. Bit-
  identical to what production sensors will see. Slow (~ms per
  invocation) — use for end-to-end tests.
- `SampleHarness(plan).stream(...)`: pure-Python with the same
  config schema. Faster (no subprocess), used for unit tests where
  the noise distribution doesn't matter.

Both consume the same `SimPlan` so a test can move between them
without changing the configuration code.

## What's deliberately not here

- **Continuous-time analog modeling** (op-amp transfer functions,
  thermal RC). Out of scope; SPICE does this.
- **Multi-sensor cross-correlation** (IMU axes that are mechanically
  linked). The model assumes per-sensor independence. Multi-axis
  setups need a fused filter on top.
- **Bit-flip / rowhammer** on the digital side. We model the analog
  → digital path; bit-level memory faults are a different research
  domain (use FaultBench, BFI).
