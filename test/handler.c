#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include "sandbox.h"
#include <assert.h>

#define BLOCKSIZE 1024

/*
typedef struct RRFrag {
    char *name;
    uint16_t type; 
    uint16_t fragsize;
    uint32_t curidx;
    uint32_t rrsize;
    uint16_t rrid;
    unsigned char *fragdata;
} RRFrag;

typedef struct PackedRR {
    bool isRRFrag;
    union {
        ResourceRecord *rr; 
        RRFrag *rrfrag;
    } data;
} PackedRR;

typedef struct PartialRR {
    uint16_t rrid;
    int8_t *block_markers; 
    unsigned char *bytes;
    uint32_t rrsize;
    uint32_t blocks_received;
    uint32_t bytes_received;
    uint32_t expected_blocks;
    bool is_complete;
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

*/
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

int main() {
    PartialDNSMessage pm;
    init_partial_dns_message(&pm);
    uint16_t sec_len = 10; 
    PackedRR **msgsection = malloc(sizeof(PackedRR*) * sec_len);
     PartialDNSMessage pm = {
        .ancount = 1,
        .nscount = 0,
        .arcount = 0,
        .answers_section = malloc(sizeof(PartialRR*) * 1),
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

    unsigned char fragdata[] = {1, 2, 3, 4, 5};
    /*
    RRFrag rrfrag = {
        .rrid = 1,
        .curidx = 0,
        .fragsize = sizeof(fragdata),
        .rrsize = sizeof(fragdata),
        .fragdata = fragdata
    };
    */

    PackedRR *msgsection[50];

    for (int i = 0; i < 50; i++) {
        RRFrag rrfrag = {
            .rrid = (i + 1),
            .curidx = i,
            .fragsize = sizeof(fragdata),
            .rrsize = sizeof(fragdata),
            .fragdata = fragdata
        };
        
        msgsection[i] = &(PackedRR) {
            .isRRFrag = true,
            .data.rrfrag = &rrfrag
        };
        
    }
    // TODO: Initialise multiple of this
    // Each array space is 20 bytes
    // Therefore if we need to test what occurs with a 1024 bytes for example, we just create an array 
    // of 1024/20 elements
    /*
    PackedRR *msgsection[] = {
        &(PackedRR) {
            .isRRFrag = true,
            .data.rrfrag = &rrfrag
        }
    };
    */
    /*
    for (uint16_t i = 0; i < sec_len; i++) {
        msgsection[i] = malloc(sizeof(PackedRR));
        init_packed_rr(msgsection[i]);
        msgsection[i]->isRRFrag = (i % 2 == 0);
        if (msgsection[i]->isRRFrag) {
            msgsection[i]->data.rrfrag = malloc(sizeof(RRFrag));
            init_rr_frag(msgsection[i]->data.rrfrag);
            msgsection[i]->data.rrfrag->rrid = i;
            msgsection[i]->data.rrfrag->fragsize = 256; 
            msgsection[i]->data.rrfrag->fragdata = malloc(256); 
        } else {
            msgsection[i]->data.rr = NULL; 
        }
    }
    */
   
    send_packets(&pm, msgsection, sec_len);
    /*
    for (uint16_t i = 0; i < sec_len; i++) {
        if (msgsection[i]->isRRFrag) {
            free(msgsection[i]->data.rrfrag->fragdata); 
            free(msgsection[i]->data.rrfrag); 
        }
        free(msgsection[i]);
    }
    free(msgsection);
    */

    return 0;
}
