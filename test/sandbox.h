#ifndef SANDBOX_H
#define SANDBOX_H

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


typedef struct FragHeader {
    size_t frag_size; // The size of the block location of the fragment in the final RR
    uint32_t frag_start; // The starting location of the fragment in the final RR
} FragHeader;


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



int init_partialrr(PartialRR **prr);

PartialRR *find_partialrr(PartialDNSMessage *pm, uint16_t rrid);

void copy_section(PartialDNSMessage *pm, PackedRR **msgsection, uint16_t sec_len);

#endif