#include <boost/tokenizer.hpp>
#include <boost/algorithm/string.hpp>
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

#define VECTOR_SIZE 16

using namespace std;

void __attribute__ ((noinline)) ORCS_tracing_start() {
    asm volatile ("nop");
    asm volatile ("nop");
    asm volatile ("nop");
    asm volatile ("nop");
    asm volatile ("nop");
    asm volatile ("nop");
    asm volatile ("nop");
    asm volatile ("nop");
    asm volatile ("nop");
    asm volatile ("nop");
    asm volatile ("nop");
}

void __attribute__ ((noinline)) ORCS_tracing_stop() {
    asm volatile ("nop");
    asm volatile ("nop");
    asm volatile ("nop");
    asm volatile ("nop");
    asm volatile ("nop");
    asm volatile ("nop");
    asm volatile ("nop");
    asm volatile ("nop");
    asm volatile ("nop");
    asm volatile ("nop");
    asm volatile ("nop");
}

uint32_t castDate2Int (string date){
    uint32_t day, month, year;
    uint32_t result;

    if (std::sscanf(date.c_str(), "%d-%d-%d", &year, &month, &day) != 3){
		cout << "Error to parse date string: " << date.c_str() << "." << endl;
    } else result = 10000 * year + 100 * month + day;
    return result;
}

void loadDateColumn (int* data_vector, uint32_t v_size, string file_path, int column){
    ifstream data(file_path);
    string line;
    vector<string> tokens;
    int count = 0;

    while (getline (data, line)){
        boost::split (tokens, line, boost::is_any_of("|"));
        //cout << tokens[column] << "\n";
        data_vector[count++] = castDate2Int (tokens[column]);
        if (count == v_size) break;
    }
}

void loadIntegerColumn (int* data_vector, uint32_t v_size, string file_path, int column){
    ifstream data(file_path);
    string line;
    vector<string> tokens;
    int count = 0;

    while (getline (data, line)){
        boost::split (tokens, line, boost::is_any_of("|"));
        //cout << tokens[column] << "\n";
        data_vector[count++] = stoi (tokens[column]);
        if (count == v_size) break;
    }
}

void bloom_chk (int32_t *input_keys, size_t input_size, int32_t *mask_factors, int32_t* shift_m, int32_t *bloom_filter, size_t bloom_filter_size, int32_t* output, size_t *output_count){
	__mmask16 k = _mm512_int2mask (0xFFFF);
	__m512i mask_1 = _mm512_set1_epi32(1);
	__m512i mask_31 = _mm512_set1_epi32(31);
	__m512i mask_0 = _mm512_set1_epi32(0);
	__m512i key, val, fun;
	__m512i mul_factors = _mm512_loadu_si512 ((__m512i*) mask_factors);
	__m512i shift_amounts = _mm512_loadu_si512 ((__m512i*) shift_m);

	uint32_t* aux_vec1 = (uint32_t*) malloc (VECTOR_SIZE * sizeof(uint32_t));
	size_t i = 0, j = 16, o = 0;
	do {
		key = _mm512_mask_loadu_epi32(key, k, &input_keys[i]);
		i += j;
		fun = _mm512_mask_xor_epi32(fun, k, fun, fun);
		__m512i fac = _mm512_permutexvar_epi32(fun, mul_factors);
        __m512i shi = _mm512_permutexvar_epi32(fun, shift_amounts);
		__m512i bit = _mm512_sllv_epi32(_mm512_mullo_epi32(key, fac), shi);
		_mm512_storeu_si512 (aux_vec1, bit);
		for (int i = 0; i < VECTOR_SIZE; i++) aux_vec1[i] %= bloom_filter_size;
		bit = _mm512_loadu_si512 (aux_vec1);
		__m512i bit_div = _mm512_srli_epi32(bit, 5);// = _mm512_i32gather_epi32(_mm512_srli_epi32(bit, 5), bloom_filter, 4);
        bit_div = _mm512_i32gather_epi32 (bit_div, bloom_filter, 4);

        //scalar gather
        //_mm512_storeu_si512 (aux_vec1, bit_div);
		//for (int i = 0; i < VECTOR_SIZE; i++) {
		//	printf ("aux_vec1[%d] = %u -> ", i, aux_vec1[i]);
		//	aux_vec1[i] = bloom_filter[aux_vec1[i]];
		//	printf ("%u\n", aux_vec1[i]);
		//}
		//bit_div = _mm512_loadu_si512 (aux_vec1);
        
		__m512i bit_mod = _mm512_sllv_epi32(mask_1, _mm512_and_epi32(bit, mask_31));
		k = _mm512_test_epi32_mask(bit_div, bit_mod);
		fun = _mm512_add_epi32(fun, mask_1);
		__mmask16 kk = _mm512_mask_cmpeq_epi32_mask(k, fun, mul_factors);
		_mm512_mask_store_epi32(&output[j], kk, key);
		k = _mm512_kor(_mm512_knot(k), kk);

		j = 0;
		int aux = _mm512_mask2int(k);
		j = _mm_popcnt_u32 (aux);
		*output_count += j;
	} while (i + j <= input_size);
}

int* bloom_create (int n, float p, size_t *size, size_t *functions){
    *size = ceil((n * log(p)) / log(1 / pow(2, log(2))));
    *functions = round((*size / n) * log(2));

    int* bf = (int*) aligned_alloc (32, *size/32 * sizeof (int));

    return bf;
}

void bloom_set(int32_t* entries, size_t entries_size, int32_t* bloom_filter, size_t bloom_filter_size, int32_t* factors, int32_t* shift_m, size_t functions){
    __mmask16 mask_k = _mm512_int2mask (0xFFFF);
	__m512i mask_31 = _mm512_set1_epi32(31);
	__m512i mask_1 = _mm512_set1_epi32(1);
	__m512i mask_0 = _mm512_set1_epi32(0);
	__m512i facts = _mm512_loadu_si512((__m512i*) factors);
	__m512i shift = _mm512_loadu_si512((__m512i*) shift_m);
	uint32_t *aux_vec1 = (uint32_t*) malloc (VECTOR_SIZE * sizeof(uint32_t));
	uint32_t *aux_vec2 = (uint32_t*) malloc (VECTOR_SIZE * sizeof(uint32_t));

    __m512i key, aux, fun, fac, bit, bit_div, bit_mod;
	
    for (int i = 0; i < entries_size; i += VECTOR_SIZE){
	    __m512i key = _mm512_mask_loadu_epi32(key, mask_k, &entries[i]);
		for (int j = 0; j < functions; j++){
            bit = key;
            fac = _mm512_permutexvar_epi32(facts, fun);
            bit = _mm512_mullo_epi32 (bit, fac);
            bit = _mm512_sllv_epi32 (bit, shift);
	    _mm512_storeu_si512	 (aux_vec1, bit);
	    for (int i = 0; i < VECTOR_SIZE; i++) {
		//printf ("%u ", aux_vec1[i]);
		aux_vec1[i] %= bloom_filter_size;
	    }
	    //printf ("\n");
            bit = _mm512_loadu_si512(aux_vec1);
            bit_div = _mm512_srli_epi32 (bit, 5);
            //bit_div = _mm512_i32gather_epi32 (bit_div, bloom_filter, 4);
            bit_mod = _mm512_and_epi32 (bit, mask_31);
            bit = _mm512_sllv_epi32 (mask_1, bit_mod);
            //bit = _mm512_or_epi32 (bit_div, bit);

	    _mm512_storeu_si512 (aux_vec1, bit);
	    _mm512_storeu_si512 (aux_vec2, bit_div);
	    for (int i = 0; i < VECTOR_SIZE; i++) {
		//printf ("bloom_filter[%u] = %u\n", aux_vec2[i], bloom_filter[aux_vec2[i]]);
		bloom_filter[aux_vec2[i]] = bloom_filter[aux_vec2[i]] | aux_vec1[i];
		//printf ("bloom_filter[%u] = %u\n", aux_vec2[i], bloom_filter[aux_vec2[i]]);
	    }
	    //printf ("\n");
            //_mm512_i32scatter_epi32 (bloom_filter, bit_div, bit, 4);
            fun = _mm512_add_epi32 (mask_1, fun);
        }
    }
}

void bloom_confirm (uint32_t* positives, size_t positives_size, uint32_t* entries, size_t entries_size){
    uint32_t result = 0;
    uint32_t count = 0;
    uint32_t* vector = (uint32_t*) malloc (VECTOR_SIZE * sizeof (uint32_t));
    uint32_t* check = (uint32_t*) malloc (VECTOR_SIZE * sizeof (uint32_t));
    uint32_t* mask_0 = (uint32_t*) malloc (VECTOR_SIZE * sizeof (uint32_t));
    _vim2K_imovu (0, mask_0);
    for (int i = 0; i < positives_size; i++){
        _vim2K_imovu (positives[i], vector);
        for (int j = 0; j < entries_size; j += VECTOR_SIZE){
            _vim2K_icmqu (vector, &entries[j], check);
            _vim2K_icmqu (check, mask_0, check);
            _vim2K_idptu (check, &count);
            if (count > 0){
                result++;
                break;
            }
        }
    }
    //std::cout << result << " positivos reais.\n";
}

void populate_vector (int* vector, size_t v_size, string filename){
    int count = 0;
    string line;
    ifstream openfile (filename);
    if (openfile.is_open()) {
        while (getline (openfile,line)) vector[count++] = stoi(line);
        openfile.close();
    }
    if (count < v_size){
        for (int i = count; i < v_size; i++) vector[i] = rand() % UINT32_MAX;
    }
}

void populate_vector (int* vector, size_t v_size, int value){
    for (int i = 0; i < v_size; i++) vector[i] = value;
}

void populate_vector (int* vector, size_t v_size){
    for (int i = 0; i < v_size; i++) vector[i] = rand() % UINT32_MAX;
}

int main (__v32s argc, char const *argv[]){
    ORCS_tracing_stop();

    srand(time(NULL));
    int vector_size;
    int *bitmap, *o_orderkey, *l_orderkey, *filter_vec;
    int prime_numbers[] = {23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71, 73, 79, 83, 89, 97};
    vector_size = atoi(argv[1]);
    
    int32_t v_size = (1024 * 1024 * vector_size)/sizeof(int);
    o_orderkey = (int*) aligned_alloc (32, (int) v_size/4 * sizeof (int));
    l_orderkey = (int*) aligned_alloc (32, v_size * sizeof (int));

    std::cout << "v_size = " << v_size*4 << "\n";

    //loadIntegerColumn (o_orderkey, v_size, "/home/srsantos/Experiment/tpch-dbgen/data/orders.tbl", 1);
    //loadIntegerColumn (l_orderkey, v_size * 4, "/home/srsantos/Experiment/tpch-dbgen/data/lineitem.tbl", 1);

    populate_vector (o_orderkey, (int) v_size/4, 5);
    populate_vector (l_orderkey, v_size, 6);

    size_t bloom_filter_size = 0;
    size_t hash_functions = 0;
    double p = 0.000001;
    size_t output_count = 0;
    
    int *bloom_filter = bloom_create ((int) v_size/4, p, &bloom_filter_size, &hash_functions);
    int *output = (int*) malloc (v_size * sizeof (int));
    int *hash_function_factors = (int*) malloc (hash_functions * sizeof (int));    
    int *shift_amounts = (int*) malloc (hash_functions * sizeof (int));

    std::cout << "bloom_filter_size = " << bloom_filter_size << "\n";
    std::cout << "hash_functions = " << hash_functions << "\n";

    for (int i = 0; i < hash_functions; i++) {
        hash_function_factors[i] = prime_numbers[i % 15];
        shift_amounts[i] = i;
    }

    ORCS_tracing_start();

    bloom_set (o_orderkey, (int) v_size/4, bloom_filter, bloom_filter_size, hash_function_factors, shift_amounts, hash_functions);
    //bloom_chk (vector1, v_size, bloom_filter, bloom_filter_size, hash_function_factors, shift_amounts, hash_functions, output, &output_count);
    //std::cout << output_count << " positives.\n";

    for (int i = 0; i < bloom_filter_size/32; i++) printf ("%u ", bloom_filter[i]);
    printf ("\n");

    bloom_chk(l_orderkey, v_size, hash_function_factors, shift_amounts, bloom_filter, bloom_filter_size, output, &output_count);
    std::cout << output_count << " positives.\n";

    //bloom_confirm (output, output_count, o_orderkey, v_size);

    //std::cout << bloom_filter[bloom_filter_size-1];
    //std::cout << l_orderkey[(v_size*4)-1];
    //std::cout << output[output_count];

    free (o_orderkey);
    free (l_orderkey);
    //free (bloom_filter);
    //free (output);
    free (hash_function_factors);
    free (shift_amounts);
}

