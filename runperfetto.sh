#!/bin/bash

# Note: 
# Tracebox binary starts traced, traced_probe services and invoke perfetto
# You'll need it in your path.
# Download and build instructions here: https://perfetto.dev/docs/quickstart/linux-tracing

if [ -v 1 ]; then
  tracebox --system-sockets --txt --config perfetto-smallconfig.pbtxt -o $1
else
  echo "Pass in output file name: <trace>.trace"
fi


