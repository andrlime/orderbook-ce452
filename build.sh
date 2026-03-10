#!/usr/bin/env bash
# Usage: ./build.sh [impl] [test|bench|obj|all] [workload]
#   impl     — folder under implementations/ (default: reference)
#   mode     — test | bench | obj | all (default: test)
#   workload — BALANCED | ADD_HEAVY | MODIFY_HEAVY | QUERY_HEAVY (default: BALANCED)

set -euo pipefail

IMPL="${1:-reference}"
MODE="${2:-test}"
WORKLOAD="${3:-BALANCED}"

BUILD_DIR="build/${IMPL}"
CMAKE_BUILD_DIR="${BUILD_DIR}/cmake"

cmake_configure() {
    cmake -S . -B "${CMAKE_BUILD_DIR}" \
        -G "Unix Makefiles" \
        -DCMAKE_BUILD_TYPE=Release \
        -DIMPL="${IMPL}" \
        -DWORKLOAD="${WORKLOAD}" \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
        ${SNIPER_ROOT:+-DSNIPER_ROOT="${SNIPER_ROOT}"}
}

build_target() {
    cmake --build "${CMAKE_BUILD_DIR}" --target "$1" -- -j"$(sysctl -n hw.logicalcpu 2>/dev/null || nproc)"
}

cmake_configure

case "${MODE}" in
  test)
    build_target run_tests
    echo "--- Running tests (impl=${IMPL}) ---"
    "${CMAKE_BUILD_DIR}/run_tests"
    ;;
  bench)
    build_target bench
    mkdir -p "${BUILD_DIR}"
    cp "${CMAKE_BUILD_DIR}/bench" "${BUILD_DIR}/bench"
    echo "Benchmark binary: ${BUILD_DIR}/bench"
    ;;
  obj)
    build_target engine_impl
    mkdir -p "${BUILD_DIR}"
    ARCHIVE=$(find "$(pwd)/${CMAKE_BUILD_DIR}" -name "libengine_impl.a" | head -1)
    # ar x --output is GNU-only; extract into a temp dir instead (portable).
    EXTRACT_TMP=$(mktemp -d)
    (cd "${EXTRACT_TMP}" && ar x "${ARCHIVE}")
    OBJ=$(find "${EXTRACT_TMP}" -name "*.o" | head -1)
    cp "${OBJ}" "${BUILD_DIR}/engine.o"
    rm -rf "${EXTRACT_TMP}"
    echo "Object file: ${BUILD_DIR}/engine.o"
    ;;
  all)
    build_target run_tests
    build_target bench
    mkdir -p "${BUILD_DIR}"
    cp "${CMAKE_BUILD_DIR}/bench" "${BUILD_DIR}/bench"
    echo "--- Running tests (impl=${IMPL}) ---"
    "${CMAKE_BUILD_DIR}/run_tests"
    echo "Benchmark binary: ${BUILD_DIR}/bench"
    ;;
  *)
    echo "Unknown mode: ${MODE}. Use test|bench|obj|all." >&2
    exit 1
    ;;
esac
