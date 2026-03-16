# `soa` — Structure-of-Arrays ID Map

**Closest impl**: `flat` (identical price index and volume cache)

## What changed

Replaces `array<optional<Order>, 65535>` (AoS, ~786 KB) with four parallel arrays:

| Array | Size | Access pattern |
|---|---|---|
| `qty_by_id[65535]` | 128 KB | Hot in match loop |
| `price_by_id[65535]` | 128 KB | Insert / modify only |
| `side_by_id[65535]` | 64 KB | Insert / modify only |
| `valid_by_id[65535]` | 64 KB | Hot in match loop |

Match-path working set: `valid_by_id` + `qty_by_id` = **192 KB** (fits L2 on most targets) vs. 786 KB for the full AoS structure.

## Expected cache behavior

| Workload | vs `flat` | Reason |
|---|---|---|
| BALANCED / ADD_HEAVY | Better | Hot match-path working set (192 KB) fits L2 |
| MODIFY_HEAVY | Worse | Random-ID modify touches 4 separate cache lines (one per SoA array) instead of 1 |
| QUERY_HEAVY | Identical | Volume array path unchanged |

## Correctness notes

- `valid_by_id` is the liveness bit, not `qty == 0`. Quantity transiently hits 0 mid-match before `valid` is cleared, so using `qty == 0` as the sole liveness check would produce false negatives.
- Partial `modify_order_by_id(id, nonzero_qty)` updates qty + volume but keeps `valid = 1` and the ID in its level vector (FIFO position preserved).
- `lookup_order_by_id` reconstructs `Order` from all four SoA arrays.
