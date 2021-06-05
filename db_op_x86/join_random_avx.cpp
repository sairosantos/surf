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

#define VECTOR_SIZE 2048

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

const uint64_t perm[256] = {0x0706050403020100ull,
     0x0007060504030201ull, 0x0107060504030200ull, 0x0001070605040302ull,
     0x0207060504030100ull, 0x0002070605040301ull, 0x0102070605040300ull,
     0x0001020706050403ull, 0x0307060504020100ull, 0x0003070605040201ull,
     0x0103070605040200ull, 0x0001030706050402ull, 0x0203070605040100ull,
     0x0002030706050401ull, 0x0102030706050400ull, 0x0001020307060504ull,
     0x0407060503020100ull, 0x0004070605030201ull, 0x0104070605030200ull,
     0x0001040706050302ull, 0x0204070605030100ull, 0x0002040706050301ull,
     0x0102040706050300ull, 0x0001020407060503ull, 0x0304070605020100ull,
     0x0003040706050201ull, 0x0103040706050200ull, 0x0001030407060502ull,
     0x0203040706050100ull, 0x0002030407060501ull, 0x0102030407060500ull,
     0x0001020304070605ull, 0x0507060403020100ull, 0x0005070604030201ull,
     0x0105070604030200ull, 0x0001050706040302ull, 0x0205070604030100ull,
     0x0002050706040301ull, 0x0102050706040300ull, 0x0001020507060403ull,
     0x0305070604020100ull, 0x0003050706040201ull, 0x0103050706040200ull,
     0x0001030507060402ull, 0x0203050706040100ull, 0x0002030507060401ull,
     0x0102030507060400ull, 0x0001020305070604ull, 0x0405070603020100ull,
     0x0004050706030201ull, 0x0104050706030200ull, 0x0001040507060302ull,
     0x0204050706030100ull, 0x0002040507060301ull, 0x0102040507060300ull,
     0x0001020405070603ull, 0x0304050706020100ull, 0x0003040507060201ull,
     0x0103040507060200ull, 0x0001030405070602ull, 0x0203040507060100ull,
     0x0002030405070601ull, 0x0102030405070600ull, 0x0001020304050706ull,
     0x0607050403020100ull, 0x0006070504030201ull, 0x0106070504030200ull,
     0x0001060705040302ull, 0x0206070504030100ull, 0x0002060705040301ull,
     0x0102060705040300ull, 0x0001020607050403ull, 0x0306070504020100ull,
     0x0003060705040201ull, 0x0103060705040200ull, 0x0001030607050402ull,
     0x0203060705040100ull, 0x0002030607050401ull, 0x0102030607050400ull,
     0x0001020306070504ull, 0x0406070503020100ull, 0x0004060705030201ull,
     0x0104060705030200ull, 0x0001040607050302ull, 0x0204060705030100ull,
     0x0002040607050301ull, 0x0102040607050300ull, 0x0001020406070503ull,
     0x0304060705020100ull, 0x0003040607050201ull, 0x0103040607050200ull,
     0x0001030406070502ull, 0x0203040607050100ull, 0x0002030406070501ull,
     0x0102030406070500ull, 0x0001020304060705ull, 0x0506070403020100ull,
     0x0005060704030201ull, 0x0105060704030200ull, 0x0001050607040302ull,
     0x0205060704030100ull, 0x0002050607040301ull, 0x0102050607040300ull,
     0x0001020506070403ull, 0x0305060704020100ull, 0x0003050607040201ull,
     0x0103050607040200ull, 0x0001030506070402ull, 0x0203050607040100ull,
     0x0002030506070401ull, 0x0102030506070400ull, 0x0001020305060704ull,
     0x0405060703020100ull, 0x0004050607030201ull, 0x0104050607030200ull,
     0x0001040506070302ull, 0x0204050607030100ull, 0x0002040506070301ull,
     0x0102040506070300ull, 0x0001020405060703ull, 0x0304050607020100ull,
     0x0003040506070201ull, 0x0103040506070200ull, 0x0001030405060702ull,
     0x0203040506070100ull, 0x0002030405060701ull, 0x0102030405060700ull,
     0x0001020304050607ull, 0x0706050403020100ull, 0x0007060504030201ull,
     0x0107060504030200ull, 0x0001070605040302ull, 0x0207060504030100ull,
     0x0002070605040301ull, 0x0102070605040300ull, 0x0001020706050403ull,
     0x0307060504020100ull, 0x0003070605040201ull, 0x0103070605040200ull,
     0x0001030706050402ull, 0x0203070605040100ull, 0x0002030706050401ull,
     0x0102030706050400ull, 0x0001020307060504ull, 0x0407060503020100ull,
     0x0004070605030201ull, 0x0104070605030200ull, 0x0001040706050302ull,
     0x0204070605030100ull, 0x0002040706050301ull, 0x0102040706050300ull,
     0x0001020407060503ull, 0x0304070605020100ull, 0x0003040706050201ull,
     0x0103040706050200ull, 0x0001030407060502ull, 0x0203040706050100ull,
     0x0002030407060501ull, 0x0102030407060500ull, 0x0001020304070605ull,
     0x0507060403020100ull, 0x0005070604030201ull, 0x0105070604030200ull,
     0x0001050706040302ull, 0x0205070604030100ull, 0x0002050706040301ull,
     0x0102050706040300ull, 0x0001020507060403ull, 0x0305070604020100ull,
     0x0003050706040201ull, 0x0103050706040200ull, 0x0001030507060402ull,
     0x0203050706040100ull, 0x0002030507060401ull, 0x0102030507060400ull,
     0x0001020305070604ull, 0x0405070603020100ull, 0x0004050706030201ull,
     0x0104050706030200ull, 0x0001040507060302ull, 0x0204050706030100ull,
     0x0002040507060301ull, 0x0102040507060300ull, 0x0001020405070603ull,
     0x0304050706020100ull, 0x0003040507060201ull, 0x0103040507060200ull,
     0x0001030405070602ull, 0x0203040507060100ull, 0x0002030405070601ull,
     0x0102030405070600ull, 0x0001020304050706ull, 0x0607050403020100ull,
     0x0006070504030201ull, 0x0106070504030200ull, 0x0001060705040302ull,
     0x0206070504030100ull, 0x0002060705040301ull, 0x0102060705040300ull,
     0x0001020607050403ull, 0x0306070504020100ull, 0x0003060705040201ull,
     0x0103060705040200ull, 0x0001030607050402ull, 0x0203060705040100ull,
     0x0002030607050401ull, 0x0102030607050400ull, 0x0001020306070504ull,
     0x0406070503020100ull, 0x0004060705030201ull, 0x0104060705030200ull,
     0x0001040607050302ull, 0x0204060705030100ull, 0x0002040607050301ull,
     0x0102040607050300ull, 0x0001020406070503ull, 0x0304060705020100ull,
     0x0003040607050201ull, 0x0103040607050200ull, 0x0001030406070502ull,
     0x0203040607050100ull, 0x0002030406070501ull, 0x0102030406070500ull,
     0x0001020304060705ull, 0x0506070403020100ull, 0x0005060704030201ull,
     0x0105060704030200ull, 0x0001050607040302ull, 0x0205060704030100ull,
     0x0002050607040301ull, 0x0102050607040300ull, 0x0001020506070403ull,
     0x0305060704020100ull, 0x0003050607040201ull, 0x0103050607040200ull,
     0x0001030506070402ull, 0x0203050607040100ull, 0x0002030506070401ull,
     0x0102030506070400ull, 0x0001020305060704ull, 0x0405060703020100ull,
     0x0004050607030201ull, 0x0104050607030200ull, 0x0001040506070302ull,
     0x0204050607030100ull, 0x0002040506070301ull, 0x0102040506070300ull,
     0x0001020405060703ull, 0x0304050607020100ull, 0x0003040506070201ull,
     0x0103040506070200ull, 0x0001030405060702ull, 0x0203040506070100ull,
     0x0002030405060701ull, 0x0102030405060700ull, 0x0001020304050607ull
};

uint32_t castDate2Int (string date){
    uint32_t day, month, year;
    uint32_t result;

    if (std::sscanf(date.c_str(), "%d-%d-%d", &year, &month, &day) != 3){
		cout << "Error to parse date string: " << date.c_str() << "." << endl;
    } else result = 10000 * year + 100 * month + day;
    return result;
}

void loadDateColumn (uint32_t* data_vector, uint32_t v_size, string file_path, int column){
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

void loadIntegerColumn (uint32_t* data_vector, uint32_t v_size, string file_path, int column){
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

void bloom_chk(int32_t *keys, int32_t *vals, size_t tuples, int32_t *filter,
                         int32_t factors[8], size_t functions, uint8_t log_filter_size,
                         int32_t *keys_out, int32_t *vals_out) {
	size_t j, m, i = 0, o = 0, t = tuples - 8;
	__m256i facts = _mm256_loadu_si256((__m256i*) factors);
	__m128i shift = _mm_cvtsi32_si128(32 - log_filter_size);
	__m256i mask_k = _mm256_set1_epi32(functions);
	__m256i mask_31 = _mm256_set1_epi32(31);
	__m256i mask_1 = _mm256_set1_epi32(1);
	__m256i mask_0 = _mm256_set1_epi32(0);
	// non-constant registers
	__m256i key, val, fun, inv = _mm256_cmpeq_epi32(mask_1, mask_1);
	if (tuples >= 8) do {
		// load new items (mask_0 out reloads)
		__m256i new_key = _mm256_maskload_epi32(&keys[i], inv);
		// reset old items
		fun = _mm256_andnot_si256(inv, fun);
		key = _mm256_andnot_si256(inv, key);
		// pick hash function
		__m256i fact = _mm256_permutevar8x32_epi32(facts, fun);
		fun = _mm256_add_epi32(fun, mask_1);
		// combine old with new items
		key = _mm256_or_si256(key, new_key);
		// hash the keys and check who is almost dmask_1
		__m256i hash = _mm256_mullo_epi32(key, fact);
		__m256i last = _mm256_cmpeq_epi32(fun, mask_k);
		hash = _mm256_srl_epi32(hash, shift);
		// check bitmap
		__m256i div_32 = _mm256_srli_epi32(hash, 5);
		div_32 = _mm256_i32gather_epi32(filter, div_32, 4);
		__m256i mod_32 = _mm256_and_si256(hash, mask_31);
		mod_32 = _mm256_sllv_epi32(mask_1, mod_32);
		div_32 = _mm256_and_si256(div_32, mod_32);
		inv = _mm256_cmpeq_epi32(div_32, mask_0);
		// branch to print winners
		if (!_mm256_testz_si256(div_32, last)) {
			__m256i mask = _mm256_andnot_si256(inv, last);
			m = _mm256_movemask_ps(_mm256_castsi256_ps(mask));
			__m128i perm_half = _mm_loadl_epi64((__m128i*) &perm[m ^ 255]);
			__m256i perm = _mm256_cvtepi8_epi32(perm_half);
			mask = _mm256_permutevar8x32_epi32(mask, perm);
			__m256i key_out = _mm256_permutevar8x32_epi32(key, perm);
			_mm256_maskstore_epi32(&keys_out[o], mask, key_out);
			o += _mm_popcnt_u64(m);
		}
		// permute to get rid of losers (and winners)
		inv = _mm256_or_si256(inv, last);
		m = _mm256_movemask_ps(_mm256_castsi256_ps(inv));
		__m128i perm_half = _mm_loadl_epi64((__m128i*) &perm[m]);
		__m256i perm = _mm256_cvtepi8_epi32(perm_half);
		inv = _mm256_permutevar8x32_epi32(inv, perm);
		fun = _mm256_permutevar8x32_epi32(fun, perm);
		key = _mm256_permutevar8x32_epi32(key, perm);
		i += _mm_popcnt_u64(m);
	} while (i <= t);
	// copy last items
	int32_t l_keys[8];
	j = _mm256_movemask_ps(_mm256_castsi256_ps(inv));
	j = 8 - _mm_popcnt_u64(j);
	assert(i + j <= tuples);
	for (i += j ; i != tuples ; ++i, ++j) {
		l_keys[j] = keys[i];
	}
	// process remaining items with scalar code
	for (i = 0 ; i != j ; ++i) {
		int32_t key1 = l_keys[i];
		size_t f = 0;
		do {
			uint32_t h = key1 * factors[f++];
			h >>= 32 - log_filter_size;
			asm goto("btl	%1, (%0)\n\t"
			         "jnc	%l[failed]"
			:: "r"(filter), "r"(h)
			: "cc" : failed);
		} while (f != functions);
		keys_out[o++] = key1;
failed:;
    }
}

int* bloom_create (int n, float p, size_t *size, size_t *functions){
    *size = ceil((n * log(p)) / log(1 / pow(2, log(2))));
    *functions = round((*size / n) * log(2));

    //std::cout << *size << " bits.\n";
    //std::cout << *functions << " functions.\n";

    int* bf = (int*) aligned_alloc (32, *size/32 * sizeof (int));

    return bf;
}

void bloom_set(int32_t* entries, size_t entries_size, int32_t* bloom_filter, size_t bloom_filter_size, int32_t* factors, int32_t* shift_m, size_t functions){
    __m256i facts = _mm256_loadu_si256((__m256i*) factors);
	__m128i shift = _mm_cvtsi32_si128(32 - 7);
	__m256i mask_k = _mm256_set1_epi32(functions);
	__m256i mask_31 = _mm256_set1_epi32(31);
	__m256i mask_1 = _mm256_set1_epi32(1);
	__m256i mask_0 = _mm256_set1_epi32(0);
	// non-constant registers
	__m256i key, val, fun, inv = _mm256_cmpeq_epi32(mask_1, mask_1);

    for (int i = 0; i < entries_size; i += VECTOR_SIZE){
        __m256i new_key = _mm256_maskload_epi32(&entries[i], inv);
		// reset old items
		fun = _mm256_andnot_si256(inv, fun);
		key = _mm256_andnot_si256(inv, key);
		// pick hash function
		__m256i fact = _mm256_permutevar8x32_epi32(facts, fun);
		fun = _mm256_add_epi32(fun, mask_1);
		// combine old with new items
		key = _mm256_or_si256(key, new_key);
		// hash the keys and check who is almost dmask_1
		__m256i hash = _mm256_mullo_epi32(key, fact);
		__m256i last = _mm256_cmpeq_epi32(fun, mask_k);
		hash = _mm256_srl_epi32(hash, shift); //hash result
		// check bitmap
		__m256i div_32 = _mm256_srli_epi32(hash, 5); //divide by 32, integer positions
		div_32 = _mm256_i32gather_epi32(bloom_filter, div_32, 4); //gather integers
		hash = _mm256_and_si256(hash, mask_31); //mod 31, bit position
		hash = _mm256_sllv_epi32(mask_1, hash); //shift by that amount, this is the result
		
        __m256i res = _mm256_or_si256(div_32, hash);
		for (int i = 0; i < 8; i++) bloom_filter[div_32[i]] = res[i];
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

    //loadDateColumn (o_orderkey, v_size, "/home/srsantos/Experiment/tpch-dbgen/data/orders.tbl", 1);
    //loadDateColumn (l_orderkey, v_size * 4, "/home/srsantos/Experiment/tpch-dbgen/data/lineitem.tbl", 1);

    populate_vector (o_orderkey, (int) v_size/4);
    populate_vector (l_orderkey, v_size);

    size_t bloom_filter_size = 0;
    size_t hash_functions = 0;
    double p = 0.000001;
    int output_count = 0;
    
    int *bloom_filter = bloom_create ((int) v_size/4, p, &bloom_filter_size, &hash_functions);
    int *output = (int*) malloc (v_size * sizeof (int));
    int *hash_function_factors = (int*) malloc (hash_functions * sizeof (int));    
    int *shift_amounts = (int*) malloc (hash_functions * sizeof (int));

    for (int i = 0; i < hash_functions; i++) {
        hash_function_factors[i] = prime_numbers[i % 15];
        shift_amounts[i] = i;
    }

    ORCS_tracing_start();

    bloom_set (o_orderkey, (int) v_size/4, bloom_filter, bloom_filter_size, hash_function_factors, shift_amounts, hash_functions);
    //bloom_chk (vector1, v_size, bloom_filter, bloom_filter_size, hash_function_factors, shift_amounts, hash_functions, output, &output_count);
    //std::cout << output_count << " positives.\n";

    bloom_chk(l_orderkey, NULL, v_size, bloom_filter, hash_function_factors, hash_functions, 0, output, NULL);
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
