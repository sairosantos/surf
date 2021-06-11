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
#define PRINT_SET 0
#define PRINT_CHECK 0

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

void printInt (int integer, std::string string){
	std::bitset<32> x(integer);
	std::cout << string << ": " << x << " \n";
}

void printAVX (__m512i vec, std::string string){
	int32_t* store = (int32_t*) malloc (16 * sizeof(int32_t));

	_mm512_storeu_si512 (store, vec);
	std::cout << string << ": ";
	for (int i = 0; i < 3; i++) {
		//char a = -58;
		std::bitset<32> x(store[i]);
		std::cout << x << " ";//" (" << store[i] << ") ";
		//printf ("%u ", store[i]);
	}
	printf ("\n");

	free (store);
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

void bloom_chk (int32_t *input_keys, size_t input_size, size_t functions, int32_t *mask_factors, int32_t* shift_m, int32_t *bloom_filter, size_t bloom_filter_size, int32_t* output, size_t *output_count){
	__mmask16 k = _mm512_int2mask (0xFFFF);
	__m512i mask_1 = _mm512_set1_epi32(1);
	__m512i mask_31 = _mm512_set1_epi32(31);
	__m512i mask_0 = _mm512_set1_epi32(0);
	__m512i key, val, fun;
	__m512i mul_factors = _mm512_loadu_si512 ((__m512i*) mask_factors);
	__m512i shift_amounts = _mm512_loadu_si512 ((__m512i*) shift_m);
	__m512i fun_max = _mm512_set1_epi32(functions - 1);

	#if PRINT_CHECK
		printAVX (mask_1, "mask_1  ");
		printAVX (mask_31, "mask_31 ");
		printAVX (mask_0, "mask_0  ");
		printAVX (mul_factors, "mul_fact");
		printAVX (shift_amounts, "shift_am");
		printf ("---------------------------------------------------------------------------\n");
	#endif

	uint32_t* aux_vec1 = (uint32_t*) malloc (VECTOR_SIZE * sizeof(uint32_t));
	size_t i = 0, j = 16, o = 0;
	do {
		key = _mm512_mask_loadu_epi32(key, k, &input_keys[i]); //carregamos novas entradas nos espaços liberados na rodada anterior
		i += j;
		fun = _mm512_mask_xor_epi32(fun, k, fun, fun); //setamos em qual função hash as entradas novas estão (0)
		__m512i fac = _mm512_permutexvar_epi32(fun, mul_factors); //buscamos os fatores multiplicatiovs
        	__m512i shi = _mm512_permutexvar_epi32(fun, shift_amounts); //buscamos as quantidades de shift
		__m512i bit = _mm512_mullo_epi32(key, fac); //multiplicação

		#if PRINT_CHECK
			printAVX (key, "key     ");
			printAVX (fun, "fun     ");
			printAVX (fac, "fac     ");
			printAVX (shi, "shi     ");
			printAVX (bit, "res_mul ");
		#endif
		bit = _mm512_sllv_epi32 (bit,shi); //shift
		#if PRINT_CHECK
			printAVX (bit, "res_hash");
		#endif
		_mm512_storeu_si512 (aux_vec1, bit);
		for (int i = 0; i < VECTOR_SIZE; i++) aux_vec1[i] %= bloom_filter_size; //mod pelo tamanho do bloom filter
		bit = _mm512_loadu_si512 (aux_vec1);
		__m512i bit_div = _mm512_srli_epi32(bit, 5); //dividimos por 32 para chegar aos índices dentro do BF
		#if PRINT_CHECK
			printAVX (bit, "res_mod ");
			printAVX (bit_div, "indices ");
		#endif
        bit_div = _mm512_i32gather_epi32 (bit_div, bloom_filter, 4); //buscamos os inteiros
       
		__m512i bit_mod = _mm512_sllv_epi32(mask_1, _mm512_and_epi32(bit, mask_31)); //descobrimos qual é o bit equivalente no inteiro e posicionamos o bit
		k = _mm512_test_epi32_mask(bit_div, bit_mod); //comparação; sim significa que o essa entrada continua para a próxima função hash
		#if PRINT_CHECK
			printAVX (bit_div, "bit_div ");
			printAVX (bit_mod, "bit_mod ");
		#endif
		__mmask16 kk = _mm512_mask_cmpeq_epi32_mask(k, fun, fun_max); //vemos quais entradas chegaram ao índice final
		#if PRINT_CHECK
			printInt (_mm512_mask2int (k), "k       ");
			printInt (_mm512_mask2int (kk), "kk      ");
			printInt (_mm512_mask2int (_mm512_knot (k)), "inv-k   ");
			printf   ("j = %lu, output_count = %lu\n", j, *output_count);
		#endif
		_mm512_mask_storeu_epi32(&output[*output_count], kk, key); //armazenamos as que estavam no índice final e passaram

		j = 0;
		int aux = _mm512_mask2int(kk); //essas vão sair porque deram positivo no último nível
		j = _mm_popcnt_u32 (aux);
		*output_count += j;
		aux = _mm512_mask2int(_mm512_knot(k)); //essas vão sair porque deram negativo em algum nível
		j += _mm_popcnt_u32 (aux);
		k = _mm512_kor(_mm512_knot(k), kk);
		#if PRINT_CHECK
			printAVX (bit_div, "gather  ");
			printAVX (bit_mod, "bitinint");
			printInt (aux, "k       ");
			printInt (_mm512_mask2int(kk), "kk      ");
			printAVX (fun,     "mt_fac+1");
			printf ("i = %lu, j = %lu, output_count = %lu\n", i, j, *output_count);
			printf ("---------------------------------------------------------------------------\n");
		#endif
		fun = _mm512_add_epi32(fun, mask_1); //somamos 1 a todas aos índices de função hash
	} while (i + j <= input_size);
}

int* bloom_create (int n, float p, size_t *size, size_t *functions){
    *size = ceil((n * log(p)) / log(1 / pow(2, log(2))));
    *functions = round((*size / n) * log(2));

    int* bf = (int*) aligned_alloc (32, *size/32 * sizeof (int));
    for (int i = 0; i < *size/32; i++) bf[i] = 0;

    return bf;
}

void bloom_set(int32_t* entries, size_t entries_size, int32_t* bloom_filter, size_t bloom_filter_size, int32_t* factors, int32_t* shift_m, size_t functions){
    __mmask16 mask_k = _mm512_int2mask (0xFFFF);
	__m512i mask_31 = _mm512_set1_epi32(31);
	__m512i mask_1 = _mm512_set1_epi32(1);
	__m512i mask_0 = _mm512_set1_epi32(0);
	__m512i facts = _mm512_loadu_si512((__m512i*) factors);
	__m512i shift = _mm512_loadu_si512((__m512i*) shift_m);
	#if PRINT_SET
		printAVX (mask_31, "mask_31 ");
		printAVX (mask_1, "mask_1  ");
		printAVX (mask_0, "mask_0  ");
		printAVX (facts, "facts   ");
		printAVX (shift, "shift   ");
		printf ("-----------------------------------------------------------\n");
	#endif
	uint32_t *aux_vec1 = (uint32_t*) malloc (VECTOR_SIZE * sizeof(uint32_t));
	uint32_t *aux_vec2 = (uint32_t*) malloc (VECTOR_SIZE * sizeof(uint32_t));

    __m512i key, aux, fun, fac, bit, bit_div, bit_mod;
	
    for (int i = 0; i < entries_size; i += VECTOR_SIZE){
	    __m512i key = _mm512_mask_loadu_epi32(key, mask_k, &entries[i]);
	    fun = _mm512_set1_epi32(0);
		for (int j = 0; j < functions; j++){
            bit = key;
            fac = _mm512_permutexvar_epi32(fun, facts);
            bit = _mm512_mullo_epi32 (bit, fac);
	    #if PRINT_SET
		printAVX (key, "entries ");
		printAVX (fac, "mult_fac");
		printAVX (bit, "res_mult");
	    #endif
            bit = _mm512_sllv_epi32 (bit, shift);
	    #if PRINT_SET
	    	printAVX (bit, "res_hash");
	    #endif
	    _mm512_storeu_si512	 (aux_vec1, bit);
	    for (int i = 0; i < VECTOR_SIZE; i++) aux_vec1[i] %= bloom_filter_size;
	    //printf ("\n");
            bit = _mm512_loadu_si512(aux_vec1);
            bit_div = _mm512_srli_epi32 (bit, 5);
            //bit_div = _mm512_i32gather_epi32 (bit_div, bloom_filter, 4);
            bit_mod = _mm512_and_epi32 (bit, mask_31);
	    #if PRINT_SET
		printAVX (bit, "res_mod ");
		printAVX (bit_div, "div32   ");
	   	printAVX (bit_mod, "bitinint");
	    #endif
            bit = _mm512_sllv_epi32 (mask_1, bit_mod);
	    #if PRINT_SET
	    	printAVX (bit, "position");
		__m512i aux_avx = _mm512_i32gather_epi32 (bit_div, bloom_filter, 4);
            	printAVX (aux_avx, "gather  ");
	    #endif
            //bit = _mm512_or_epi32 (bit_div, bit);

	    _mm512_storeu_si512 (aux_vec1, bit);
	    _mm512_storeu_si512 (aux_vec2, bit_div);
	    for (int i = 0; i < VECTOR_SIZE; i++) {
		//printf ("bloom_filter[%u] = %u\n", aux_vec2[i], bloom_filter[aux_vec2[i]]);
		bloom_filter[aux_vec2[i]] = bloom_filter[aux_vec2[i]] | aux_vec1[i];
		//printf ("bloom_filter[%u] = %u\n", aux_vec2[i], bloom_filter[aux_vec2[i]]);
	    }
	    fun = _mm512_add_epi32 (mask_1, fun);

	    #if PRINT_SET
		printAVX (fun, "functn+1");
	    	aux_avx = _mm512_i32gather_epi32 (bit_div, bloom_filter, 4);
            	printAVX (aux_avx, "gather  ");
		printf ("-----------------------------------------------------------\n");
	    #endif

	    //printf ("\n");
            //_mm512_i32scatter_epi32 (bloom_filter, bit_div, bit, 4);
        }
    }
}

void bloom_confirm (int32_t* positives, size_t positives_size, int32_t* entries, size_t entries_size){
    int32_t result = 0;
    int32_t count = 0;
    __m512i vector, entries_vec;
    __mmask16 mask;
    int i, j;

    for (i = 0; i < positives_size; i++){
        vector = _mm512_set1_epi32 (positives[i]);
        for (j = 0; j < entries_size; j += VECTOR_SIZE){
            entries_vec = _mm512_loadu_si512 (&entries[j]);
            mask = _mm512_cmpeq_epi32_mask (vector, entries_vec);
	    count = _mm512_mask2int (mask);
            if (count > 0){
                result++;
                break;
            }
        }
    }

    std::cout << result << " positivos reais.\n";
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

    std::cout << "v_size = " << v_size << "\n";

    //loadIntegerColumn (o_orderkey, (uint32_t) v_size/4, "/home/srsantos/Experiment/tpch-dbgen/data/orders.tbl", 1);
    //loadIntegerColumn (l_orderkey, v_size, "/home/srsantos/Experiment/tpch-dbgen/data/lineitem.tbl", 1);

    populate_vector (o_orderkey, (int) v_size/4);
    populate_vector (l_orderkey, v_size);

    size_t bloom_filter_size = 0;
    size_t hash_functions = 0;
    double p = 0.001;
    size_t output_count = 0;
    
    int *bloom_filter = bloom_create ((int) v_size/4, p, &bloom_filter_size, &hash_functions);
    int *output = (int*) aligned_alloc (32, v_size * sizeof (int));
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

    //for (int i = 0; i < bloom_filter_size/32; i++) printf ("%u ", bloom_filter[i]);
    //printf ("\n");

    bloom_chk(l_orderkey, v_size, hash_functions, hash_function_factors, shift_amounts, bloom_filter, bloom_filter_size, output, &output_count);
    std::cout << output_count << " positives.\n";

    bloom_confirm (output, output_count, o_orderkey, v_size/4);

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
