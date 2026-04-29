from __future__ import annotations

import math

import pytest

from sensorsim import (
    FaultConfig,
    SampleHarness,
    SensorConfig,
    SimPlan,
    parse_samples,
)
from sensorsim.config import FaultKind


def _plan(**kw) -> SimPlan:
    base = {
        "sensor": SensorConfig(id=1, seed=42),
        "sample_count": 50,
        "dt_sec": 0.01,
    }
    base.update(kw)
    return SimPlan(**base)


def test_clean_harness_returns_truth() -> None:
    h = SampleHarness(plan=_plan())
    samples = h.collect(lambda _t: 5.0)
    assert all(s.value == 5.0 for s in samples)


def test_drift_accumulates() -> None:
    plan = _plan(sensor=SensorConfig(id=1, seed=1, drift_per_sec=1.0))
    h = SampleHarness(plan=plan)
    samples = h.collect(lambda _t: 0.0)
    # At t=0.49 (last sample), value ≈ 0.49.
    assert abs(samples[-1].value - 0.49) < 1e-6


def test_stuck_at_fault_overrides_value() -> None:
    fault = FaultConfig(kind=FaultKind.STUCK_AT, constant=42.0)
    h = SampleHarness(plan=_plan(fault=fault))
    samples = h.collect(lambda _t: 5.0)
    assert all(s.value == 42.0 for s in samples)


def test_dropped_fault_emits_nan_at_period() -> None:
    fault = FaultConfig(kind=FaultKind.DROPPED, every_k=5)
    h = SampleHarness(plan=_plan(fault=fault))
    samples = h.collect(lambda _t: 1.0)
    nan_indices = [i for i, s in enumerate(samples) if math.isnan(s.value)]
    assert nan_indices == [0, 5, 10, 15, 20, 25, 30, 35, 40, 45]


def test_parse_samples_round_trips_jsonl() -> None:
    text = '{"t":0.0,"value":1.5}\n{"t":0.01,"value":1.6}\n'
    samples = parse_samples(text)
    assert len(samples) == 2
    assert samples[0].value == 1.5


def test_invalid_dt_rejected() -> None:
    with pytest.raises(ValueError):
        SimPlan(sensor=SensorConfig(id=1), sample_count=10, dt_sec=2.0)


def test_invalid_sample_count_rejected() -> None:
    with pytest.raises(ValueError):
        SimPlan(sensor=SensorConfig(id=1), sample_count=0, dt_sec=0.01)
