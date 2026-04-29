"""Closed Pydantic schemas for sensor + fault + simulation plans."""

from __future__ import annotations

from enum import StrEnum

from pydantic import BaseModel, Field, field_validator


class FaultKind(StrEnum):
    NONE = "none"
    STUCK_AT = "stuck_at"
    SPIKE = "spike"
    DROPPED = "dropped"
    RANGE_CLAMP = "range_clamp"


class FaultConfig(BaseModel):
    model_config = {"extra": "forbid"}
    kind: FaultKind = FaultKind.NONE
    constant: float = 0.0
    every_k: int = 1
    spike_factor: float = 1.0
    range_low: float = 0.0
    range_high: float = 0.0


class SensorConfig(BaseModel):
    model_config = {"extra": "forbid"}
    id: int = Field(ge=0)
    drift_per_sec: float = 0.0
    noise_stddev: float = 0.0
    adc_lsb: float = 0.0
    seed: int = 0


class SimPlan(BaseModel):
    model_config = {"extra": "forbid"}
    sensor: SensorConfig
    fault: FaultConfig = FaultConfig()
    sample_count: int = Field(ge=1)
    dt_sec: float = Field(gt=0)

    @field_validator("dt_sec")
    @classmethod
    def _dt_reasonable(cls, v: float) -> float:
        if v > 1.0:
            raise ValueError("dt_sec > 1.0 disallowed; sensor sampling is sub-second")
        return v
