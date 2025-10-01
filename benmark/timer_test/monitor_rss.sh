#!/bin/bash

if [ $# -lt 2 ]; then
    echo "Usage: $0 <program> <log_file>"
    echo "Example: $0 ./sw_task_oneshot sw_task_rss_log.txt"
    exit 1
fi

PROGRAM=$1
LOGFILE=$2

$PROGRAM &
PID=$!
echo "Process PID: $PID"

echo "Time(s) RSS(KB)" > "$LOGFILE"

for i in $(seq 0 59); do
    RSS=$(ps -o rss= -p $PID)
    echo "$i $RSS"
    echo "$i $RSS" >> "$LOGFILE"
    sleep 1
    if ! ps -p $PID > /dev/null; then
        echo "Process ended at $i seconds."
        break
    fi
done

kill $PID 2>/dev/null

echo "RSS log saved to $LOGFILE"