#!/bin/bash
set -x
sudo xentrace -c 3 -T 10 -D trace.bin
(cat trace.bin | xentrace_format ~/repos/xen46/tools/xentrace/formats > trace.txt)
