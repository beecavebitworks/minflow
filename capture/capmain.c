
typedef unsigned char  u_char;
typedef unsigned int   u_int;
typedef unsigned short u_short;

#include <pcap/pcap.h>
#include <signal.h>
#include <unistd.h>
#include <capture/pcap32.h>

int snaplen=72;		// TODO: what about IPV6?
char ebuf[32];
char device[16]="eth0";
static void *pd;

void info(int val)
{
  // TODO: print stats
}

void get_libpcap_stats(void *pd, int *recv, int *ifdrops, int *kdrops)
{
  struct pcap_stat stat;

  if (pcap_stats(pd, &stat) < 0) {
    //(void)fprintf(stderr, "pcap_stats: %s\n", pcap_geterr(pd));
    *recv=0;
    *kdrops=0;
    *ifdrops=0;
    return;
  }

  *recv = stat.ps_recv;
  *kdrops = stat.ps_drop;
  *ifdrops = stat.ps_ifdrop;

}

uint32_t tlast_report = 0;

extern int on_packet(pcap_pkthdr32_t *hdr, char *pkt, int pktnum);
extern void shm_update_capstats(int recv, int ifdrops, int kdrops);

void callback(u_char *user, const struct pcap_pkthdr *h, const u_char *bytes)
{
  pcap_pkthdr32_t hdr;
  hdr.ts.tv_sec = h->ts.tv_sec;
  hdr.ts.tv_usec = h->ts.tv_usec;
  hdr.caplen = h->caplen;
  hdr.len = h->len;

  if (tlast_report == 0) {
    tlast_report = h->ts.tv_sec;
  } else if ((h->ts.tv_sec - tlast_report ) >= 1) {
    int recv, ifdrops, kdrops;
    get_libpcap_stats(pd, &recv, &ifdrops, &kdrops);
    shm_update_capstats(recv,ifdrops, kdrops);
    tlast_report = h->ts.tv_sec;
  }
 
  on_packet(&hdr,(char *)bytes,0);
}

void go_live_pcap()
{
  int pflag = 0;
  int status;

		*ebuf = '\0';
                pd = pcap_open_live(device, snaplen, !pflag, 1000, ebuf);
                if (pd == NULL){
                        printf("ERROR:%s", ebuf);
                        return;
                }
                else if (*ebuf)
                        printf("WARN:%s", ebuf);
#ifdef LINUX
        (void)setsignal(SIGTERM, cleanup);
        (void)setsignal(SIGINT, cleanup);
        /* Cooperate with nohup(1) */
#endif // LINUX


                //setup_sigsegv();


                /*
                 * Let user own process after socket has been opened.
                 */
                setuid(getuid());

		// TODO: init

                int cnt=0;
                int userdata;
                status = pcap_loop(pd, cnt, callback, (u_char*)&userdata);
                printf( "pcap_loop done" );

                if (status == -1) {
                        /*
                        * Error.  Report it.
                        */
                        (void)fprintf(stderr, "pcap_loop: %s\n",
                        pcap_geterr(pd));
                }
                if (1) { //RFileName == NULL) {
                        /*
                        * We're doing a live capture.  Report the capture
                        * statistics.
                        */
                        info(1);
                }

                pcap_close(pd);

}


