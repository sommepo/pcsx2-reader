#!/usr/bin/env bash
# Runs a command with a hard memory limit, so that a runaway process ends itself instead of
# exhausting this shared machine.
#   tools/reader/limit.sh [-m SIZE] command [arguments...]
# SIZE is a systemd size such as 2G; the default is READER_MEMORY_MAX or 4G. The command runs
# in its own transient scope: if it goes over the limit, only that scope is killed.
set -euo pipefail
limit="${READER_MEMORY_MAX:-4G}"
if [ "${1:-}" = "-m" ]; then
  limit="$2"
  shift 2
fi
if [ $# -eq 0 ]; then
  echo "usage: limit.sh [-m SIZE] command [arguments...]" >&2
  exit 64
fi
if ! systemd-run --user --scope --quiet -p MemoryMax=1G -- true 2>/dev/null; then
  echo "limit.sh: no systemd user scope available, refusing to run without a memory limit" >&2
  echo "          (set READER_NO_MEMORY_LIMIT=1 to run anyway)" >&2
  [ "${READER_NO_MEMORY_LIMIT:-}" = 1 ] && exec "$@"
  exit 69
fi
exec systemd-run --user --scope --quiet --collect \
  -p MemoryMax="$limit" -p MemorySwapMax=0 -- "$@"
