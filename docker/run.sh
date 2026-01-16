#!/usr/bin/env sh
set -eu

cmd="${1:-main}"

case "$cmd" in
  main)
    exec /work/build/main
    ;;
  test|tests)
    exec ctest --test-dir /work/build --output-on-failure
    ;;
  shell)
    exec /bin/bash
    ;;
  *)
    exec "$@"
    ;;
esac
