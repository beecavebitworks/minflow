all: rminflo minflocap statprocd

CFILES=capture/readcap.c net/iputils.c net/streamkey.c net/parse.c
CFILES+=utils/hashtable.c utils/sort.c utils/mgarray.c common/shmdata.c utils/stringbuf.c
CFILES+=net/pktinfo.c common/tval.c utils/ipc.c export/export.c capture/cleanup.c

## does not work on Mac:
## utils/sigsegv.c

SIMFILES+=statproc/statproc_sim.c statproc/statproc_handlers.c statproc/statproc_utils.c

CFLAGS=-g -std=c11

# rminflo only reads pcap files
# minflocap does live capture
# statprocd processes stats from minflocap

rminflo: $(CFILES) $(SIMFILES) capture/main.c
	cc $(CFLAGS) -DNOPCAP -I . -o $@ $^

minflocap: $(CFILES) $(SIMFILES) capture/main.c capture/capmain.c
	cc $(CFLAGS) -I . -o $@ $^ ../libpcap/libpcap.a

statprocd: $(CFILES) $(SIMFILES) statproc/main.c
	cc $(CFLAGS) -I . -o $@ $^

mga_test: test/mga_test.c utils/mgarray.c
	cc $(CFLAGS) -I . -o $@ $^

sort_test: test/sort_test.c utils/sort.c
	cc $(CFLAGS) -I . -o $@ $^

streamkey_test: test/streamkey_test.c net/streamkey.c
	cc $(CFLAGS) -I . -o $@ $^

statproc_utils_test: test/misc.c
	cc $(CFLAGS) -I . -o $@ $^

clean:
	rm -f *.o minflocap statprocd rminflo ./*_test
	rm -rf ./*.dSYM
