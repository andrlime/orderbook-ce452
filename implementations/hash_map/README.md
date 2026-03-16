# `hash_map` — Unordered Map Price Index

**Closest impl**: `array` (same AoS ID map; replaces `std::map` with `std::unordered_map`)

## What changed

`SideOrders` is now `unordered_map<PriceType, vector<IdType>>`:

- O(1) average price lookup vs O(log n) RB-tree traversal.
- `best_bid` / `best_ask` hints added (copied from `flat`) because `unordered_map` has no ordered iteration for the sweep. The sweep walks the flat `volumeMap` array and only calls `unordered_map::find` when `volumeMap[p] > 0`.
- `reserve(4096)` in constructor prevents rehash during the seeding phase (≈ 2000 active levels).
- Empty level vectors are erased from the map after a level is fully matched.

## Memory vs `flat`

`flat` pre-allocates `65536 × sizeof(vector) × 2 ≈ 3 MB` of level-vector storage regardless of how many price levels are populated. `hash_map` allocates only ~2000 bucket entries for the active price range — smaller resident footprint at runtime.

## Expected cache behavior

| Workload | vs `array` | vs `flat` | Reason |
|---|---|---|---|
| ADD_HEAVY | Better | Slightly worse | No RB-tree traversal on insert; hash overhead vs. direct index |
| MODIFY_HEAVY | Same | Same | Modify path uses only `idToOrdersMap[id]`, not the hash map |
| QUERY_HEAVY | Identical | Identical | Volume array path unchanged |
| BALANCED | Better | Slightly worse | Between `array` and `flat` |

## Correctness notes

- `best_bid` / `best_ask` may lag empty levels after cancels. The sweep loop advances `p` via `volumeMap[p] == 0` check — the hash map is never consulted for empty levels.
- `operator[]` on insert auto-creates an empty vector if the price is new — correct behavior.
- Stale IDs (from `modify_order_by_id`) are cleaned up lazily during the next match sweep, same as `flat` and `array`.
