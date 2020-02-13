#!/bin/bash
set -e          # Exit on error
set -o pipefail # Failing programs early in a pipe is an error
set -u          # Unset vars are errors

current_sysrepo_config="$(mktemp)"

function do_exit() {
    $SUPERVISORCTL -c $SUPERVISORDCONF shutdown
    sleep 1
    $SYSREPOCFG -m pdns-server --format=json --import=${current_sysrepo_config}
}

$SYSREPOCFG -m pdns-server --format=json --export=${current_sysrepo_config}
# Import the initial config
$SYSREPOCFG -m pdns-server --format=json --import=${SYSREPO_INITIAL_CONFIG}

$SUPERVISORD -c $SUPERVISORDCONF

trap do_exit ERR EXIT

started=0
for ctr in 0 1 2 3 4 5 6 7 8 9; do
  if $($SUPERVISORCTL -c $SUPERVISORDCONF status pdns-sysrepo | grep -q RUNNING); then
    started=1
    break
  fi
  sleep 1
done

if [ $started -ne 1 ]; then
  exit 1
fi

$NOSETESTS $TEST_INITIAL_CONFIG_PY $TEST_SLAVE_PY

exit $?