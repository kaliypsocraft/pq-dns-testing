#include <arpa/inet.h>
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

#include "sandbox.h"
#include "question.h"
#include "packedrr.h"
#include "rrfrag.h"
#include "resource_record.h"

#define RRFRAGHEADER 15 // 14 for the fields, + 1 for the name
#define RRHEADER 10
#define DNSMESSAGEHEADER DNSHEADERSIZE
uint32_t MAXUDP = 1232;
uint32_t our_addr;
uint32_t is_resolver = false;

int rrfs_copied = 0;
bool BYPASS = false;


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
PartialRR *find_partialrr(PartialDNSMessage *pm, uint16_t rrid) {
  if (pm == NULL) {
    return NULL;
  }
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
// Obtain fragments and reconstruct them in order to verify later on
void copy_section(PartialDNSMessage *pm, PackedRR **msgsection, uint16_t sec_len) {

    RRFrag *rrfrag = msgsection[0]->data.rrfrag;
    printf("rrfrag->fragsize: %d\n", rrfrag->fragsize);
    for (int i = 0; i < sec_len; i++) {
        uint16_t rrid = rrfrag[i].rrid;
        uint32_t curidx = rrfrag[i].curidx;
        uint16_t fragsize = rrfrag[i].fragsize;
        // Find the associated record somewhere in the partialDNSMessage
        //printf("iteration: %d | rrid: %d\n",i, rrid);
        PartialRR *prr = find_partialrr(pm, rrid);
        // prr->bytes_received = 100000;
        /**/
        if (prr->rrid == 0) {
            prr->rrid = rrid;
        }


        // Sanity check that we aren't overwriting anything we shouldn't
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
            prr->bytes = malloc(sizeof(unsigned char) * rrfrag->fragsize);
            prr->bytes_capacity = rrfrag->fragsize;
            prr->headers = malloc(sizeof(FragHeader)); // Fix this
            prr->headers_capacity = 1;

            if (prr->bytes == NULL) {
                printf("Error allocating bytes in prr\n");
                fflush(stdout);
                exit(-1);
            }
            if (prr->headers == NULL) {
                printf("Error allocating headers in prr\n");
                fflush(stdout);
                exit(-1);
            }

            prr->rrsize = rrfrag->rrsize;
        }
        /*
        for (uint16_t j = blockidx; j < lastblockidx; j++) {
            if (prr->block_markers[j] != BLOCKRECVD) {
                printf("block wasn't waiting for data\n");
                // ERROR();
            }
        }
        */
    //prr->bytes_received = 1000;
        //printf("prr->bytes_capacity: %ld\n", prr->bytes_capacity);
        //printf("prr->bytes_received: %ld\n", prr->bytes_received);
        while (prr->bytes_received + rrfrag->fragsize > prr->bytes_capacity) {
            // Increase byte array size dynamically as needed
            prr->bytes_capacity *= 2;
            prr->bytes = realloc(prr->bytes, sizeof(unsigned char) * prr->bytes_capacity);
            
            if (prr->bytes == NULL) {
                printf("Error reallocating bytes in prr\n");
                fflush(stdout);
                exit(-1);
            }
        }

        while (prr->num_headers + 1 > prr->headers_capacity) {
            // Double headers if necessary
            prr->headers_capacity *= 2;
            prr->headers = realloc(prr->headers, prr->headers_capacity * (sizeof(FragHeader)));

            if (prr->headers == NULL) {
                printf("Error reallocating headers in prr\n");
                fflush(stdout);
                exit(-1);
            }
        }
        //printf("rrfrag->fragsize: %d\n", rrfrag->fragsize);
        //memcpy(prr->bytes + prr->bytes_received, rrfrag->fragdata, rrfrag->fragsize);
        //printf("Here!\n");
        
        prr->bytes_received += rrfrag->fragsize;
        prr->headers[prr->num_headers].frag_size = rrfrag->fragsize;
        prr->headers[prr->num_headers].frag_start = rrfrag->curidx;
        prr->num_headers++;

        for (uint16_t j = blockidx; j < lastblockidx; j++) {
            prr->block_markers[j] = BLOCKRECVD;
        }

        prr->blocks_received += lastblockidx - blockidx;

        if (prr->expected_blocks == prr->blocks_received) {
            prr->is_complete = true;
            /*
            for (uint16_t j = 0; j < prr->expected_blocks; j++) {
                assert(prr->block_markers[j] == BLOCKRECVD);
            }
            */

            unsigned char *unsorted_prr = prr->bytes;
            prr->bytes = malloc(sizeof(unsigned char) * prr->bytes_capacity);

            if (prr->bytes == NULL) {
                printf("Error allocating bytes in prr\n");
                fflush(stdout);
                exit(-1);
            }

            uint32_t byteidx = 0;
            for (int i = 0; i < prr->num_headers; i++) {
                memcpy(prr->bytes + prr->headers[i].frag_start, unsorted_prr + byteidx, prr->headers[i].frag_size);
                byteidx += prr->headers[i].frag_size;
            }

            free(unsorted_prr);
            free(prr->headers);
            prr->headers = NULL;
        }
    }
}