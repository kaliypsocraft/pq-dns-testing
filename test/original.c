
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <assert.h>

#include "original.h"
#include "question.h"
#include "packedrr.h"
#include "rrfrag.h"
#include "resource_record.h"


char *
itoa(uint16_t in) {
	char *res = NULL;
	int num_bytes = snprintf(NULL, 0, "%hu", in) + 1;
	res = malloc(sizeof(char) * num_bytes);
	snprintf(res, num_bytes , "%hu", in);
	return res;
}

void
ERROR(void) {
	assert(false);
}


#define BLOCKSIZE 32

#define BLOCKRECVD 0
#define BLOCKFREE -1
#define BLOCKWAITING 1
PartialRR *find_partialrr(PartialDNSMessage *pm, uint16_t rrid) {
  if (pm == NULL) {
    return NULL;
  }
  //printf("rrid: %u\n", rrid);
  //printf("Length: %d\n", length);
  for (uint16_t i = 0; i < pm->ancount; i++) {
    if (pm->answers_section[i]->rrid == rrid) {
      return pm->answers_section[i];
    }
  }
  for (uint16_t i = 0; i < pm->nscount; i++) {
    if (pm->authoritative_section[i]->rrid == rrid) {
      return pm->authoritative_section[i];
    }
  }
  for (uint16_t i = 0; i < pm->arcount; i++) {
    if (pm->additional_section[i]->rrid == rrid) {
      return pm->additional_section[i];
    }
  }
  printf("Could not find!\n");
  return NULL;
}

int init_partialrr(PartialRR **prr) {
    assert(*prr == NULL);
    PartialRR *_prr = malloc(sizeof(PartialRR));
    if (_prr == NULL) {
        return -1;
    }
    _prr->bytes = NULL;
    _prr->headers = NULL;
    _prr->num_headers = 0;
    _prr->headers_capacity = 0;
    _prr->rrsize = 0;
    _prr->bytes_received = 0;
    _prr->is_complete = false;
    _prr->rrid = 0;
    _prr->blocks_received = 0;
    _prr->expected_blocks = 0;
    _prr->block_markers = NULL;
    *prr = _prr;
    return 0;
}

void copy_section(PartialDNSMessage *pm, PackedRR **msgsection, uint16_t sec_len) {

      RRFrag *rrfrag = msgsection[0]->data.rrfrag;
      for (int i = 0; i < sec_len; i++) {
          uint16_t rrid = rrfrag[i].rrid;
          uint32_t curidx = rrfrag[i].curidx;
          uint16_t fragsize = rrfrag[i].fragsize;
          //printf("rrfrag->rrid: %d, rrfrag->curidx: %d rrfrag->fragsize: %d\n", rrfrag->rrid, rrfrag->curidx, rrfrag->fragsize);
          // Find the associated record somewhere in the partialDNSMessage
          PartialRR *prr = find_partialrr(pm, rrid);
          if (prr->rrid == 0) {
              prr->rrid = rrid;
          }
          // Sanity check that we aren't overwriting anything we shouldn't.
          uint16_t blockidx = curidx / (double)BLOCKSIZE;
          uint16_t lastblockidx = blockidx + ceil(fragsize / (double)BLOCKSIZE);
          uint16_t totalblocks = ceil(rrfrag->rrsize / (double)BLOCKSIZE);
          if (prr->block_markers == NULL) {
              prr->block_markers = malloc(sizeof(int8_t) * totalblocks);
              for (uint16_t j = 0; j < totalblocks; j++) {
                  prr->block_markers[j] = BLOCKFREE;
              }
              prr->expected_blocks = totalblocks;
          }
          if (prr->bytes == NULL) {
              prr->bytes = malloc(sizeof(unsigned char) * rrfrag->rrsize);
              if (prr->bytes == NULL) {
                  printf("Error allocating bytes in prr\n");
                  fflush(stdout);
                  exit(-1);
              }
              prr->rrsize = rrfrag->rrsize;
          }
          /*
          for (uint16_t j = blockidx; j < lastblockidx; j++) {
              if (prr->block_markers[j] != BLOCKRECVD) {
                  printf("block wasn't waiting for data\n");
                  ERROR();
              }
          }
          */
          //memcpy(prr->bytes + rrfrag->curidx, rrfrag->fragdata, rrfrag->fragsize);
          for (uint16_t j = blockidx; j < lastblockidx; j++) {
              prr->block_markers[j] = BLOCKRECVD;
          }
          prr->blocks_received += lastblockidx - blockidx;
          prr->bytes_received += rrfrag->fragsize;
          if (prr->expected_blocks == prr->blocks_received) {
              prr->is_complete = true;
              for (uint16_t j = 0; j < prr->expected_blocks; j++) {
                  assert(prr->block_markers[j] == BLOCKRECVD);
              }
          }
        }
      
}	

