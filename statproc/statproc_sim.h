// Used in simulation / playback scenarios to invoke stats process interval handlers

#include <common/sharedif.h>

void sim_before_packet(tval_t ts, sharedif_t *shm);

void sim_after_packet(tval_t ts, sharedif_t *shm);

