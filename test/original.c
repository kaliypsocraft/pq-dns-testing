
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

typedef struct FragHeader {
    size_t frag_size; // The size of the block location of the fragment in the final RR
    uint32_t frag_start; // The starting location of the fragment in the final RR
} FragHeader;

int main(int argc, char **argv) {
    
    return 0;
}

typedef struct PartialRR {
    unsigned char *bytes;
    FragHeader *headers;
    uint16_t num_headers;
    uint16_t headers_capacity;
    size_t rrsize;
    size_t bytes_received;
    size_t bytes_capacity;
    bool is_complete;
    uint16_t rrid; // 0 will represent an uninitialized PartialRR. There must be no rr's with rrid 0
    size_t blocks_received;
    size_t expected_blocks;
    int8_t *block_markers; // -1 == not requested, 0 == received, 1 == requested but not received
} PartialRR;
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


typedef struct PartialDNSMessage {
	sem_t lock;
	uint16_t identification;
	bool identification_set;
	uint16_t flags;
	bool flags_set;
	uint16_t qdcount;
	bool qdcount_set;
	uint16_t ancount;
	bool ancount_set;
	uint16_t nscount;
	bool nscount_set;
	uint16_t arcount;
	bool arcount_set;
	Question **question_section;
	uint16_t questions_done;
	PartialRR **answers_section;
	uint16_t answers_done;
	PartialRR **authoritative_section;
	uint16_t authoritative_done;
	PartialRR **additional_section;
	uint16_t additionals_done;
} PartialDNSMessage;


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
	for (uint16_t i = 0; i < sec_len; i++) {
        RRFrag *rrfrag = msgsection[i];
        uint16_t rrid = rrfrag->rrid;
        uint32_t curidx = rrfrag->curidx;
        uint16_t fragsize = rrfrag->fragsize;
        // Find the associated record somewhere in the partialDNSMessage
        PartialRR *prr = find_partialrr(pm, rrid);
        if (prr->rrid == 0) {
            prr->rrid = rrid;
        }
        // Find the associated record somewhere in the partialDNSMessage
        PartialRR *prr = find_partialrr(pm, rrid);

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

        for (uint16_t j = blockidx; j < lastblockidx; j++) {
            if (prr->block_markers[j] == BLOCKRECVD) {
                printf("block wasn't waiting for data\n");
                ERROR();
            }
        }

        while (prr->bytes_received + rrfrag->fragsize > prr->bytes_capacity) {
            // Increase byte array size dynamically as needed
            prr->bytes_capacity *= 2;
            realloc(prr->bytes, sizeof(unsigned char) * prr->bytes_capacity);

            if (prr->bytes == NULL) {
                printf("Error reallocating bytes in prr\n");
                fflush(stdout);
                exit(-1);
            }
        }

        while (prr->num_headers + 1 > prr->headers_capacity) {
            // Double headers if necessary
            prr->headers_capacity *= 2;
            realloc(prr->headers, prr->headers_capacity * (sizeof(FragHeader)));

            if (prr->headers == NULL) {
                printf("Error reallocating headers in prr\n");
                fflush(stdout);
                exit(-1);
            }
        }

        memcpy(prr->bytes + prr->bytes_received, rrfrag->fragdata, rrfrag->fragsize);
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

            for (uint16_t j = 0; j < prr->expected_blocks; j++) {
                assert(prr->block_markers[j] == BLOCKRECVD);
            }

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