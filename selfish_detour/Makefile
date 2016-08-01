
detour : detour.c hrtimer.h x86_64-gcc-rdtsc.h calibrate.h
	cc --std=gnu99 --static -O2 -o detour detour.c

detour.rr : detour.c hrtimer.h x86_64-gcc-rdtsc.h calibrate.h
	x86_64-rumprun-netbsd-gcc --std=gnu99 --static -O3 -o detour.rr.bin detour.c
	rumprun-bake xen_pv detour.rr detour.rr.bin
	rm detour.rr.bin

clean: 
	rm -rf detour.rr
	rm -rf detour
