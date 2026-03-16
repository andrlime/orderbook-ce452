# `flat_eager` — Eager Deletion Variant

**Closest impl**: `flat` (identical data structures)

## What changed

Only the deletion strategy differs from `flat`.

In `flat`, `modify_order_by_id(id, 0)` sets `idToOrdersMap[id] = nullopt` but leaves the stale `IdType` in the level vector, cleaned up lazily during the next match sweep.

`flat_eager` immediately does `std::find + erase` on the level vector in `modify_order_by_id`, at O(level_depth ≈ 16) cost. As a result:

- **Every ID in a level vector is guaranteed live** — the match loop has no stale-ID branch.
- The `!slot.has_value()` early-return in `modify_order_by_id` handles the case where the order was already fully matched by the engine before the cancel arrives (Test 34 — noop, no double-erase).

## Expected cache behavior

| Workload | vs `flat` | Reason |
|---|---|---|
| ADD_HEAVY | Identical | Zero cancels — eager path never triggered |
| MODIFY_HEAVY | Worse | 3 cancels/round × O(16) vector scan + cache misses on cold level vector |
| BALANCED | Approximately equal | Cancel overhead and branch savings cancel out |
| QUERY_HEAVY | Identical | Volume array path unchanged |

## Correctness notes

- Only `new_quantity == 0` triggers the eager erase from the level vector. Partial qty reductions (`new_quantity > 0`) must **not** erase — FIFO order must be preserved (Test 16).
- When the match loop fully consumes a resting order (`resting.quantity == 0`), it sets `slot = nullopt` and erases the ID from the vector. Subsequent `modify_order_by_id(id, 0)` sees `!slot.has_value()` and returns — no double-erase (Test 34).
