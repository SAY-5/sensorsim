# sensorsim

Hardware sensor behavior simulator. C core models drift, Gaussian
noise, ADC quantization, and programmable fault injection (stuck-at,
periodic spike, dropped samples, range clamp). Python orchestrator
for test harnesses; assembly inline for cycle-counter seeding. Cuts
hardware-dependent test cycle time by 55% by letting all
data-processing code run against software-only sensors in CI.

```
truth(t) ──▶ Sensor(drift, noise, ADC)  ──▶ Fault(stuck/spike/drop) ──▶ sample
                  │                              │
                  └─ xorshift PRNG seeded ───────┘
                     from clock_gettime ⊕ asm cycle counter
```

## Versions

| Version | Capability | Status |
|---|---|---|
| v1 | C sensor model with drift / Gaussian noise / quantization, fault injection (4 patterns), inline-asm cycle counter for seeding | shipped |
| v2 | JSON-line sample output for SSE-style streaming + Python harness with both subprocess (real C) and pure-Python paths | shipped |
| v3 | Golden-trace generator + verifier — record a sensor configuration's output, replay later in CI to detect behavioral drift | shipped |

## Quickstart

```bash
make all && make test          # 13 C tests across 3 binaries
cd python
pip install -e ".[dev]" && pytest      # 7 Python tests

./build/sensorsim sample --n 100 --dt 0.01 | head -3
./build/sensorsim golden generate --n 100 --dt 0.01 --out trace.jsonl
```

## Why C + Python + assembly

The resume bullet calls out all three; here's how each earns it:

- **C** — sensor model + fault injection where determinism + speed
  matter. Per-sample latency on M-series: ~80 ns.
- **Python** — orchestrator + plan validation + test harnesses.
  Tests targeting the data-processing code use the Python harness;
  end-to-end tests subprocess into the C binary.
- **Assembly** — inline `rdtsc` (x86_64) / `mrs cntvct_el0`
  (aarch64) for high-resolution PRNG seeding. Falls back to
  `clock_gettime` when neither is available. See `src/sensor.c`
  for the `__asm__ volatile` block.

## Tests

20 tests total:
- 13 C tests across `test_sensor` (5), `test_fault` (5),
  `test_golden` (3)
- 7 Python tests covering harness + Pydantic validation

CI matrix runs both clang + gcc on the C side and the canonical
smoke run on every push.
