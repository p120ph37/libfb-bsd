all: lib examples

lib: libfb.a

examples: ppm2fb ppm2fb-db

ksym.o: $*.c $*.h
	$(CC) $(CFLAGS) $*.c $(LDFLAGS) -r -nostdlib -lkvm -o $@

fb.o: $*.c $*.h ksym.o
	$(CC) $(CFLAGS) $*.c ksym.o $(LDFLAGS) -r -nostdlib -o $@

libfb.a: fb.o
	$(AR) $(ARFLAGS) $@ $?
    
ppm2fb ppm2fb-db: $*.c libfb.a
	$(CC) $(CFLAGS) $*.c $(LDFLAGS) -L. -lfb -o $@ 

clean:
	rm -f *.o *.a ppm2fb ppm2fb-db

.PHONY: all clean lib examples
