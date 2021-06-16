#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
//#include <arm_neon.h>
#include <immintrin.h>
#include "../../intrinsics/vima/vima.hpp"

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <bitset>

#define VECTOR_SIZE 2048

using namespace std;

void __attribute__ ((noinline)) ORCS_tracing_start() {
    asm volatile ("nop");
}

void __attribute__ ((noinline)) ORCS_tracing_stop() {
    asm volatile ("nop");
}

uint32_t* bloom_create (uint32_t n, float p, size_t *size, size_t *functions){
    *size = ceil((n * log(p)) / log(1 / pow(2, log(2))));
    *functions = round((*size / n) * log(2));

    uint32_t* bf = (uint32_t*) malloc (*size/32 * sizeof (uint32_t));
    for (int i = 0; i < *size/32; i++) bf[i] = 0;

    return bf;
}

void bloom_confirm (uint32_t* positives, size_t positives_size, uint32_t* entries, size_t entries_size){
    int result = 0;
    for (int i = 0; i < positives_size; i++){
        for (int j = 0; j < entries_size; j++){
            if (positives[i] == entries[j]) {
                result++;
                break;
            }
        }
    }
    std::cout << result << " positivos reais.\n";
}

void bloom_chk(uint32_t* entries, size_t entries_size, uint32_t* bloom_filter, size_t bloom_filter_size, uint32_t* factors, uint32_t* shift_m, size_t functions, uint32_t* output, uint32_t* output_count){
    int calc = 0, bf_pos = 0, bit_pos = 0, res = 0;
    bool pos;

    *output_count = 0;
	for (int i = 0; i < entries_size; i++){
        pos = true;
		for (int j = 0; j < functions && pos; j++){
            calc = 0;
			calc = entries[i] * factors[j];
			calc <<= shift_m[j];
			calc %= bloom_filter_size;
			bf_pos = calc >> 5;
			bit_pos = 1 << (calc & 31);
            res = bloom_filter[bf_pos] & bit_pos;
            if (res == 0){
                pos = false;
                break;
            }
		}
        if (pos) {
            output[*output_count] = entries[i];
            *output_count += 1;
        }
	}
}

void bloom_set(uint32_t* entries, size_t entries_size, uint32_t* bloom_filter, size_t bloom_filter_size, uint32_t* factors, uint32_t* shift_m, size_t functions){
    int calc = 0, bf_pos = 0, bit_pos = 0;
	for (int i = 0; i < entries_size; i++){
		for (int j = 0; j < functions; j++){
			calc = 0;
			calc = entries[i] * factors[j];
            calc <<= shift_m[j];
            calc %= bloom_filter_size;
            bf_pos = calc >> 5;
            bit_pos = 1 << (calc & 31);
            bloom_filter[bf_pos] |= bit_pos;
		}
	}
}

void populate_vector (uint32_t* vector, size_t v_size, uint32_t value){
    for (int i = 0; i < v_size; i++) vector[i] = value;
}

void populate_vector (uint32_t* vector, size_t v_size){
    for (int i = 0; i < v_size; i++) vector[i] = rand() % (v_size*100);
}

int main (__v32s argc, char const *argv[]){
    ORCS_tracing_stop();

    srand(time(NULL));
    uint32_t vector_size;
    uint32_t *bitmap, *o_orderkey, *l_orderkey, *filter_vec;
    uint32_t prime_numbers[] = {23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71, 73, 79, 83, 89, 97};
    vector_size = atoi(argv[1]);
    
    
    __v32u v_size = (1024 * 1024 * vector_size)/sizeof(__v32u);
    o_orderkey = (uint32_t*) malloc ((uint32_t) v_size/4 * sizeof (uint32_t));
    l_orderkey = (uint32_t*) malloc (v_size * sizeof (uint32_t));

    populate_vector (o_orderkey, v_size/4);
    populate_vector (l_orderkey, v_size);

    size_t bloom_filter_size = 0;
    size_t hash_functions = 0;
    double p = 0.0001;
    uint32_t output_count = 0;
    
    uint32_t *bloom_filter = bloom_create ((uint32_t) v_size/4, p, &bloom_filter_size, &hash_functions);
    uint32_t *output = (uint32_t*) malloc (v_size * sizeof (uint32_t));
    uint32_t *hash_function_factors = (uint32_t*) malloc (hash_functions * sizeof (uint32_t));    
    uint32_t *shift_amounts = (uint32_t*) malloc (hash_functions * sizeof (uint32_t));

    std::cout << "v_size = " << v_size << "\n";
    std::cout << "bf_size = " << bloom_filter_size << "\n";
    std::cout << "functions = " << hash_functions << "\n";

    for (int i = 0; i < hash_functions; i++) {
        hash_function_factors[i] = prime_numbers[i % 15];
        shift_amounts[i] = i;
    }

    ORCS_tracing_start();

    bloom_set (o_orderkey, (uint32_t) v_size/4, bloom_filter, bloom_filter_size, hash_function_factors, shift_amounts, hash_functions);
    
    bloom_chk (l_orderkey, v_size, bloom_filter, bloom_filter_size, hash_function_factors, shift_amounts, hash_functions, output, &output_count);
    std::cout << output_count << " positives.\n";

    bloom_confirm (output, output_count, o_orderkey, v_size/4);

    free (o_orderkey);
    free (l_orderkey);
    free (hash_function_factors);
    free (shift_amounts);
}