#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <arm_neon.h>

#include <iostream>
#include <string>
#include <vector>

using namespace std;

const char* filename;
uint8_t* char_vector;
uint32_t* h_init;
uint32_t no_entries;
uint8_t no_hash_functions;

int seed;

char *ltrim(char *str, const char *seps) {
    size_t totrim;
    if (seps == NULL) {
        seps = "\t\n\v\f\r ";
    }
    totrim = strspn (str, seps);
    if (totrim > 0) {
        size_t len = strlen(str);
        if (totrim == len) {
            str[0] = '\0';
        }
        else {
            memmove (str, str + totrim, len + 1 - totrim);
        }
    }
    return str;
}

char *rtrim (char *str, const char *seps) {
    int i;
    if (seps == NULL) {
        seps = "\t\n\v\f\r ";
    }
    i = strlen (str) - 1;
    while (i >= 0 && strchr (seps, str[i]) != NULL) {
        str[i] = '\0';
        i--;
    }
    return str;
}

char *trim (char *str, const char *seps){
    return ltrim (rtrim (str, seps), seps);
}

uint8x8_t dot_product (uint8_t* src, uint8x8_t idx, uint8x8_t mask_vec){
    uint8x8_t res, aux_vec;
    uint8_t min, i;
    uint8x8_t aux;
    uint16_t accum = 0;

    res = vdup_n_u8 (0);
    min = vminv_u8 (idx);

    i = 0;
    for (int j = 0; j < 8; j++){
        if (mask_vec[j] = 1){
            accum = 0;
            for (i = 0; i < idx[j] - 8; i += 8){
                aux = vld1_u8 (&src[i]);
                //for (int j = 0; j < 8; j++) printf ("aux[%d] = %d\n", i+j, aux[j]);
                accum += vaddlv_u8 (aux);
            }

            aux = vld1_u8 (&src[i]);
            for (; i <= idx[j]; i++) {
                accum += aux[i%8];
                //printf ("aux[%d] = %d\n", i, aux[i%8]);
            }
            res[j] = accum;
        }
    }

    return res;
}

void char_vector_load(){
    FILE* fp;
    char* line = NULL;
    size_t len = 0;
    ssize_t read;

    int pos = 0;

    fp = fopen(filename, "r");
    if (fp == NULL) exit(EXIT_FAILURE);

    while ((read = getline(&line, &len, fp)) != -1 && pos < 8192) {
        line = trim (line, NULL);
        read = strlen (line);

        for (int i = 0; i < read; i++){
            char_vector[pos] = line[i];
            pos++;
        }

        while (pos%4 != 0) pos++;
    }

    fclose(fp);
    if (line) free(line);
}

void compute_hash (uint8x16x4_t* load, uint32x4_t* h1, uint32x4_t* h2, uint32x4_t* h3) {
    uint32x4_t aux1, aux2;
    uint32x4_t* neon_r;
    neon_r = (uint32x4_t*) malloc (4 * sizeof (uint32x4_t));

    aux1 = vdupq_n_u32 (0);
    aux2 = vdupq_n_u32 (0);
    for (int i = 0; i < 4; i++) neon_r[i] = vdupq_n_u32 (0);
    
    for (int i = 0; i < 4; i++){
        neon_r[i][0] = load->val[i][0];
        neon_r[i][1] = load->val[i][1];
        neon_r[i][2] = load->val[i][2];
        neon_r[i][3] = load->val[i][3];
    }

    for (int i = 0; i < 4; i++){
        aux1 = vshlq_n_u32 (*h1, 5);
        aux2 = vshrq_n_u32 (*h1, 7);
        aux1 = vaddq_u32 (aux1, aux2);
        aux1 = vaddq_u32 (aux1, neon_r[i]);
        *h1 = veorq_u32 (*h1, aux1);

        aux1 = vshlq_n_u32 (*h2, 5);
        aux2 = vshrq_n_u32 (*h2, 7);
        aux1 = vaddq_u32 (aux1, aux2);
        aux1 = vaddq_u32 (aux1, neon_r[i]);
        *h2 = veorq_u32 (*h2, aux1);

        aux1 = vshlq_n_u32 (*h3, 5);
        aux2 = vshrq_n_u32 (*h3, 7);
        aux1 = vaddq_u32 (aux1, aux2);
        aux1 = vaddq_u32 (aux1, neon_r[i]);
        *h3 = veorq_u32 (*h3, aux1);
    }
}

void bloom_filter (uint8_t* src, uint8_t* filter, int src_length) {
    uint8x16x4_t load;
    uint32x4_t h1, h2, h3;
    uint32x4_t aux1, aux2;
    uint32x4_t* neon_r;
    neon_r = (uint32x4_t*) malloc (4 * sizeof (uint32x4_t));

    h1 = vdupq_n_u32 (src_length % 5);
    h2 = vdupq_n_u32 (src_length % 34);
    h3 = vdupq_n_u32 (src_length % 12);

    aux1 = vdupq_n_u32 (0);
    aux2 = vdupq_n_u32 (0);
    for (int i = 0; i < 4; i++) neon_r[i] = vdupq_n_u32 (0);
    
    for (uint32_t src_idx = 0; src_idx < src_length; src_idx += 16){
        load = vld4q_u8 (&src[src_idx]);
        for (int i = 0; i < 4; i++){
            neon_r[i][0] = load.val[i][0];
            neon_r[i][1] = load.val[i][1];
            neon_r[i][2] = load.val[i][2];
            neon_r[i][3] = load.val[i][3];
        }
    }

    for (int i = 0; i < 4; i++){
        aux1 = vshlq_n_u32 (h1, 5);
        aux2 = vshrq_n_u32 (h1, 7);
        aux1 = vaddq_u32 (aux1, aux2);
        aux1 = vaddq_u32 (aux1, neon_r[i]);
        h1 = veorq_u32 (h1, aux1);

        aux1 = vshlq_n_u32 (h2, 5);
        aux2 = vshrq_n_u32 (h2, 7);
        aux1 = vaddq_u32 (aux1, aux2);
        aux1 = vaddq_u32 (aux1, neon_r[i]);
        h2 = veorq_u32 (h2, aux1);

        aux1 = vshlq_n_u32 (h3, 5);
        aux2 = vshrq_n_u32 (h3, 7);
        aux1 = vaddq_u32 (aux1, aux2);
        aux1 = vaddq_u32 (aux1, neon_r[i]);
        h3 = veorq_u32 (h3, aux1);
    }

    for (int i = 0; i < 4; i++){
        h1[i] = h1[i] % src_length;
        cout << "h1[" << i << "] = " << h1[i];
        h2[i] = h2[i] % src_length;
        cout << " h2[" << i << "] = " << h2[i];
        h3[i] = h3[i] % src_length;
        cout << " h3[" << i << "] = " << h3[i];
        cout << "\n";
    }
}

int main(int argc, char **argv) {
    filename = argv[1];

    no_entries = 1000;
    no_hash_functions = 3;

    h_init = (uint8_t*) malloc (no_hash_functions * sizeof (uint8_t));

    char_vector = (uint8_t*) malloc (8192 * sizeof(uint8_t));
    
    char_vector_load();
    //neon_load();
    bloom_filter (char_vector, char_vector, 16);
}