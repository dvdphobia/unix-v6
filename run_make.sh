#!/bin/bash
cd /workspaces/unix-v6
make clean > /tmp/make_clean.log 2>&1
make run > /tmp/make_run.log 2>&1 &
sleep 2
tail -100 /tmp/make_run.log
