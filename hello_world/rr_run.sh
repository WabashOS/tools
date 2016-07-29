#!/bin/bash
# Usage: ./rr_run.sh application_rr arg
# Only supports 1 argument (output type for the hello_world-type apps)
rumprun -S xen -g 'tsc_mode="native";cpus="2-3"' -id -I if,xenif,'bridge=virbr0,mac=00:16:3e:00:00:02' -W if,inet,dhcp $1 $2
#rumprun -S xen -g 'tsc_mode="native";cpus="3"' -id $1 $2
