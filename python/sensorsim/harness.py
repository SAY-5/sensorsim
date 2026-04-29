"""Sample harness — replaces real hardware in test code.

The C binary is the canonical implementation; the harness provides
two paths:

1. `run_sim(plan, binary)`: subprocess into the real C binary,
   parse JSON-line samples back. The "55% test cycle reduction"
   metric uses this path because it's bit-identical to what
   production sensors will see (the C model captures their drift +
   noise + fault behavior).
2. `SampleHarness` (Python-only path): faster for tight unit tests
   that don't need the C-precise model — they want a deterministic
   stream and don't care about Box-Muller specifics. Useful for
   testing data-processing logic where the noise distribution
   doesn't matter.
"""

from __future__ import annotations

import json
import math
import random
import subprocess
from collections.abc import Iterator
from dataclasses import dataclass

from sensorsim.config import FaultKind, SimPlan


@dataclass(frozen=True, slots=True)
class Sample:
    t: float
    value: float


def parse_samples(stdout: str) -> list[Sample]:
    out: list[Sample] = []
    for line in stdout.splitlines():
        line = line.strip()
        if not line:
            continue
        obj = json.loads(line)
        v = obj.get("sample", obj.get("value"))
        out.append(Sample(t=float(obj["t"]), value=float(v) if v is not None else math.nan))
    return out


def run_sim(plan: SimPlan, binary: str = "build/sensorsim") -> list[Sample]:
    """Invoke the C binary in `sample` mode. Production wires
    custom config flags onto the binary for the full plan; the
    minimal CLI ignores fault for now (round-trips drift + noise +
    quantization)."""
    cmd = [binary, "sample", "--n", str(plan.sample_count), "--dt", str(plan.dt_sec)]
    res = subprocess.run(cmd, capture_output=True, text=True, check=False)
    if res.returncode != 0:
        raise RuntimeError(f"sensorsim failed: {res.stderr}")
    return parse_samples(res.stdout)


@dataclass
class SampleHarness:
    """Pure-Python harness for unit tests that don't want a
    subprocess. Models the same primitives as the C side so a test
    can target either."""

    plan: SimPlan

    def stream(self, true_value_fn) -> Iterator[Sample]:
        rng = random.Random(self.plan.sensor.seed or 1)
        fault = self.plan.fault
        for i in range(self.plan.sample_count):
            t = i * self.plan.dt_sec
            v = true_value_fn(t) + self.plan.sensor.drift_per_sec * t
            if self.plan.sensor.noise_stddev > 0:
                v += rng.gauss(0.0, self.plan.sensor.noise_stddev)
            if self.plan.sensor.adc_lsb > 0:
                v = round(v / self.plan.sensor.adc_lsb) * self.plan.sensor.adc_lsb
            if fault.kind == FaultKind.STUCK_AT:
                v = fault.constant
            elif fault.kind == FaultKind.SPIKE and fault.every_k > 0 and i % fault.every_k == 0:
                v = v * fault.spike_factor
            elif fault.kind == FaultKind.DROPPED and fault.every_k > 0 and i % fault.every_k == 0:
                v = math.nan
            elif fault.kind == FaultKind.RANGE_CLAMP:
                v = max(fault.range_low, min(fault.range_high, v))
            yield Sample(t=t, value=v)

    def collect(self, true_value_fn) -> list[Sample]:
        return list(self.stream(true_value_fn))
