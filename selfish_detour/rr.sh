#!/bin/bash
rumprun -S xen -g 'tsc_mode="native";cpus="3";mem="512"' -b /home/xarc/vm_configs/detour.img,/ -id detour.rr -n 40000 -o f
