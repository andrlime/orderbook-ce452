#!/usr/bin/env bash
# run_sniper.sh — run a single Sniper simulation cell
#
# Usage: ./run_sniper.sh <impl> <policy> <workload>
#
#   impl     : reference | map_vec | array | flat
#   policy   : lru | srrip | nmru | always_miss
#   workload : BALANCED | ADD_HEAVY | MODIFY_HEAVY | QUERY_HEAVY
#
# Requires SNIPER_ROOT to be set (e.g. export SNIPER_ROOT=/opt/sniper).

set -euo pipefail

IMPL="${1:-}"
POLICY="${2:-}"
WORKLOAD="${3:-}"

# ── Validate arguments ──────────────────────────────────────────────────────
if [[ -z "$IMPL" || -z "$POLICY" || -z "$WORKLOAD" ]]; then
    echo "Usage: $0 <impl> <policy> <workload>" >&2
    echo "  impl     : reference | map_vec | array | flat" >&2
    echo "  policy   : lru | srrip | nmru | always_miss" >&2
    echo "  workload : BALANCED | ADD_HEAVY | MODIFY_HEAVY | QUERY_HEAVY" >&2
    exit 1
fi

if [[ -z "${SNIPER_ROOT:-}" ]]; then
    echo "ERROR: SNIPER_ROOT is not set. Export the path to your Sniper installation." >&2
    exit 1
fi

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CFG="${SCRIPT_DIR}/sniper/${POLICY}.cfg"

if [[ ! -f "$CFG" ]]; then
    echo "ERROR: Config not found: $CFG" >&2
    exit 1
fi

# ── Build ───────────────────────────────────────────────────────────────────
echo "==> Building: impl=${IMPL}  workload=${WORKLOAD}"
"${SCRIPT_DIR}/build.sh" "${IMPL}" bench "${WORKLOAD}"

BINARY="${SCRIPT_DIR}/build/${IMPL}/bench"
if [[ ! -x "$BINARY" ]]; then
    echo "ERROR: bench binary not found at ${BINARY}" >&2
    exit 1
fi

# ── Run Sniper ───────────────────────────────────────────────────────────────
OUTPUT_DIR="${SCRIPT_DIR}/results/${IMPL}/${POLICY}/${WORKLOAD}"
mkdir -p "${OUTPUT_DIR}"

echo "==> Simulating: impl=${IMPL}  policy=${POLICY}  workload=${WORKLOAD}"
echo "    cfg    : ${CFG}"
echo "    output : ${OUTPUT_DIR}"
echo "    binary : ${BINARY}"

"${SNIPER_ROOT}/run-sniper" \
    -c "${CFG}" \
    -o "${OUTPUT_DIR}" \
    -- "${BINARY}"

echo "==> Done. Results in ${OUTPUT_DIR}"
