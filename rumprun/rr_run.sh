#!/bin/bash
# Usage: ./rr_run.sh application_rr args
# Example: ./rr_run.sh hello.rr -f

# Minimal
#rumprun -S xen -g 'tsc_mode="native";cpus="3"' -id $@

# Just File.
# File must be valid, formatted image:
#   $ dd if=/dev/zero of=hdd.img bs=1m count=20
#   $ mkfs.ext3 detour.img
# Mount on host with: $ sudo mount -o loop detour.img /mnt
#rumprun -S xen -g 'tsc_mode="native";cpus="2-3"' -b ~/data/os_images/detour.img -id $@

# Just Networking
rumprun -S xen -g 'tsc_mode="native";cpus="2-3"' -id -I if,xenif,'bridge=virbr0,mac=00:16:3e:00:00:02' -W if,inet,dhcp $@

# Full Networking/File
#rumprun -S xen -g 'tsc_mode="native";cpus="2-3"' -b ~/data/os_images/detour.img -id -I if,xenif,'bridge=virbr0,mac=00:16:3e:00:00:02' -W if,inet,dhcp $@

