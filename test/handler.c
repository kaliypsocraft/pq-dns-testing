#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include "sandbox.h"
#include <assert.h>

#define BLOCKSIZE 1024

void init_partial_rr(PartialRR *prr) {
    prr->rrid = 0;
    prr->block_markers = NULL;
    prr->bytes = NULL;
    prr->rrsize = 0;
    prr->blocks_received = 0;
    prr->bytes_received = 0;
    prr->expected_blocks = 0;
    prr->is_complete = false;
}

void init_rr_frag(RRFrag *rrfrag) {
    rrfrag->name = NULL; 
    rrfrag->type = 0; 
    rrfrag->fragsize = 0;
    rrfrag->curidx = 0;
    rrfrag->rrsize = 0;
    rrfrag->rrid = 0;
    rrfrag->fragdata = NULL; 
}

void init_packed_rr(PackedRR *packed_rr) {
    packed_rr->isRRFrag = false;
    packed_rr->data.rrfrag = NULL; 
}

void init_partial_dns_message(PartialDNSMessage *pm) {
    sem_init(&pm->lock, 0, 1);
    pm->identification = 0;
    pm->identification_set = false;
    pm->flags = 0;
    pm->flags_set = false;
    pm->qdcount = 0;
    pm->qdcount_set = false;
    pm->ancount = 0;
    pm->ancount_set = false;
    pm->nscount = 0;
    pm->nscount_set = false;
    pm->arcount = 0;
    pm->arcount_set = false;
    pm->question_section = NULL; 
    pm->questions_done = 0;
    pm->answers_section = NULL; 
    pm->answers_done = 0;
    pm->authoritative_section = NULL; 
    pm->authoritative_done = 0;
    pm->additional_section = NULL; 
    pm->additionals_done = 0;
}

void send_packets(PartialDNSMessage *pm, PackedRR **msgsection, uint16_t sec_len) {
    clock_t start_time = clock();
    // So far it is entering here and there is a segmentation fault
    printf("I am called from here!\n");
    copy_section(pm, msgsection, sec_len);
    clock_t end_time = clock();
    double time_taken = (double)(end_time - start_time) / CLOCKS_PER_SEC;
    printf("Time taken for copy_section: %f seconds\n", time_taken);
}

int main(int argc, char *argv[]) {
	
	if (argc < 2) {
        printf("\n\nIncorrect usage: ./arrf_[dynamic|static] <fragSize>");
		exit(1);
    } 

	int fragSize = atoi(argv[1]);


    PartialDNSMessage pm = {
        .ancount = 1,
        .nscount = 0,
        .arcount = 0,
        .answers_section = malloc(sizeof(PartialRR*) * fragSize),
        .authoritative_section = NULL,
        .additional_section = NULL,
        .answers_done = 0,
        .authoritative_done = 0,
        .additionals_done = 0
    };

    if (!pm.answers_section) {
        printf("Error allocating answers_section\n");
        return -1;
    }

    pm.answers_section[0] = malloc(sizeof(PartialRR));
    if (!pm.answers_section[0]) {
        printf("Error allocating PartialRR\n");
        free(pm.answers_section);
        return -1;
    }
    memset(pm.answers_section[0], 0, sizeof(PartialRR));

    unsigned char fragdata[fragSize];

	for (int i = 0; i < fragSize; i++) {
		fragdata[i] = i + 1;
	}

    PackedRR **msgsection = malloc(sizeof(PackedRR*) * fragSize);
	if (!msgsection) {
        printf("Error allocating msgsection\n");
        free(pm.answers_section[0]);
        free(pm.answers_section);
        return -1;
    }
    
	RRFrag *rrfrag = malloc(sizeof(RRFrag) * fragSize);
	if (!rrfrag) {
		printf("Error allocating\n");
		exit(1);
	}
    for (int i = 0; i < fragSize; i++) {
        rrfrag[i] = (RRFrag){
            .rrid = 0,
            .curidx = i,
            .fragsize = fragSize,
            .rrsize = fragSize,
            .fragdata = fragdata
        };
	}
        
	msgsection[0] = &(PackedRR){
		.isRRFrag = true,
		.data.rrfrag = rrfrag
	};
	printf("rrfrag->fragsize: %d\n", msgsection[0]->data.rrfrag->fragsize);
    send_packets(&pm, msgsection, fragSize);
	
    return 0;
}


