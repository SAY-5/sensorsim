"""CLI: read a SimPlan JSON from stdin, run via the harness, print
samples on stdout."""

from __future__ import annotations

import json
import sys

from sensorsim import SampleHarness, SimPlan


def main() -> int:
    raw = sys.stdin.read().strip()
    if not raw:
        print("usage: sensorsim < plan.json", file=sys.stderr)
        return 2
    plan = SimPlan.model_validate(json.loads(raw))
    h = SampleHarness(plan=plan)
    for s in h.stream(lambda _t: 25.0):
        print(json.dumps({"t": s.t, "value": s.value}))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
