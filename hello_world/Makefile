
hello : hello.c
	cc --std=gnu99 --static -O2 -o hello hello.c

hello.rr : hello.c
	x86_64-rumprun-netbsd-gcc --std=gnu99 --static -O3 -o hello.rr.bin hello.c
	rumprun-bake xen_pv hello.rr hello.rr.bin
	rm hello.rr.bin

hello_pth : hello_pth.c
	cc -pthread --std=gnu99 --static -O2 -o hello_pth hello_pth.c

hello_pth.rr : hello_pth.c
	x86_64-rumprun-netbsd-gcc --std=gnu99 --static -O3 -o hello.rr.bin hello_pth.c
	rumprun-bake xen_pv hello_pth.rr hello.rr.bin
	rm hello.rr.bin

clean:
	rm -f hello
	rm -f hello.rr
	rm -f hello.out
