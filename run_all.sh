#!/usr/bin/env bash
# run_all.sh — sweep all 64 Sniper simulation cells (4 × 4 × 4)
#
# Usage: ./run_all.sh [--dry-run]
#
#   --dry-run  Print the commands that would run without executing them.
#
# Requires SNIPER_ROOT to be set (e.g. export SNIPER_ROOT=/opt/sniper).
# Results land in results/<impl>/<policy>/<workload>/.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
RUN_SNIPER="${SCRIPT_DIR}/run_sniper.sh"

DRY_RUN=0
if [[ "${1:-}" == "--dry-run" ]]; then
    DRY_RUN=1
fi

if [[ -z "${SNIPER_ROOT:-}" ]]; then
    echo "ERROR: SNIPER_ROOT is not set. Export the path to your Sniper installation." >&2
    exit 1
fi

IMPLS=(reference map_vec array flat)
POLICIES=(always_miss lru srrip nmru)
WORKLOADS=(BALANCED ADD_HEAVY MODIFY_HEAVY QUERY_HEAVY)

TOTAL=$(( ${#IMPLS[@]} * ${#POLICIES[@]} * ${#WORKLOADS[@]} ))
CELL=0
FAILED=0
FAILED_CELLS=()

for IMPL in "${IMPLS[@]}"; do
    for POLICY in "${POLICIES[@]}"; do
        for WORKLOAD in "${WORKLOADS[@]}"; do
            CELL=$(( CELL + 1 ))
            echo ""
            echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
            echo " Cell ${CELL}/${TOTAL}: impl=${IMPL}  policy=${POLICY}  workload=${WORKLOAD}"
            echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

            if [[ "$DRY_RUN" -eq 1 ]]; then
                echo "[dry-run] ${RUN_SNIPER} ${IMPL} ${POLICY} ${WORKLOAD}"
                continue
            fi

            if "${RUN_SNIPER}" "${IMPL}" "${POLICY}" "${WORKLOAD}"; then
                echo " -> OK"
            else
                FAILED=$(( FAILED + 1 ))
                FAILED_CELLS+=("${IMPL}/${POLICY}/${WORKLOAD}")
                echo " -> FAILED (continuing)" >&2
            fi
        done
    done
done

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
if [[ "$DRY_RUN" -eq 1 ]]; then
    echo "Dry run complete. ${TOTAL} cells would be run."
elif [[ "$FAILED" -eq 0 ]]; then
    echo "All ${TOTAL} cells completed successfully."
else
    echo "${FAILED}/${TOTAL} cells FAILED:"
    for cell in "${FAILED_CELLS[@]}"; do
        echo "  - ${cell}"
    done
    exit 1
fi
