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
}

void __attribute__ ((noinline)) ORCS_tracing_stop() {
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

uint32_t* bloom_create (uint32_t n, float p, size_t *size, size_t *functions){
    *size = ceil((n * log(p)) / log(1 / pow(2, log(2))));
    *functions = round((*size / n) * log(2));

    //std::cout << *size << " bits.\n";
    //std::cout << *functions << " functions.\n";

    uint32_t* bf = (uint32_t*) malloc (*size/32 * sizeof (uint32_t));
    for (int i = 0; i < *size/32; i++) bf[i] = 0;

    return bf;
}

void bloom_set(uint32_t* entries, size_t entries_size, uint32_t* bloom_filter, size_t bloom_filter_size, uint32_t* factors, uint32_t* shift_m, size_t functions){
    uint32_t *mask_1, *mask_31;
    uint32_t *shift_vec, *shift5_vec; 
    uint32_t *key, *aux;
    uint32_t *fun, *fac;
    uint32_t *bit, *bit_div, *bit_mod;

    key = (uint32_t*) malloc (VECTOR_SIZE * sizeof(uint32_t));
    aux = (uint32_t*) malloc (VECTOR_SIZE * sizeof(uint32_t));
    fun = (uint32_t*) malloc (VECTOR_SIZE * sizeof(uint32_t));
    fac = (uint32_t*) malloc (VECTOR_SIZE * sizeof(uint32_t));
    bit = (uint32_t*) malloc (VECTOR_SIZE * sizeof(uint32_t));
    bit_div = (uint32_t*) malloc (VECTOR_SIZE * sizeof(uint32_t));
    bit_mod = (uint32_t*) malloc (VECTOR_SIZE * sizeof(uint32_t));
    mask_1 = (uint32_t*) malloc (VECTOR_SIZE * sizeof(uint32_t));
    mask_31 = (uint32_t*) malloc (VECTOR_SIZE * sizeof(uint32_t));
    shift_vec = (uint32_t*) malloc (VECTOR_SIZE * sizeof(uint32_t));
    shift5_vec = (uint32_t*) malloc (VECTOR_SIZE * sizeof(uint32_t));

    _vim2K_imovu (1, mask_1);
    _vim2K_imovu (31, mask_31);
    _vim2K_imovu (5, shift5_vec);
    
    for (int i = 0; i < entries_size; i += VECTOR_SIZE) {
        _vim2K_ilmku (&entries[i], mask_1, key); //load new entries according to the mask.
        _vim2K_irmku (fun, mask_1); //take the entries of the hash counters back to 0 if they were reset.
        for (int j = 0; j < functions; j++){
            _vim2K_icpyu (key, bit);
            _vim2K_ipmtu (factors, fun, fac);
            _vim2K_ipmtu (shift_m, fun, shift_vec);
            _vim2K_imulu (bit, fac, bit); //first step of hash
            _vim2K_isllu (bit, shift_vec, bit); //second step of hash
            _vim2K_imodu (bit, bloom_filter_size, bit); //which bit within the bloom filter to set!
            _vim2K_isrlu (bit, shift5_vec, bit_div); //which integer inside the bloom filter?
            _vim2K_iandu (bit, mask_31, bit_mod); //which bit inside that integer?
            _vim2K_isllu (mask_1, bit_mod, bit); //bits are positioned!
            _vim2K_iscou (bit, bit_div, bloom_filter);
            _vim2K_iaddu (fun, mask_1, fun);
        }
    };

    free (key);
    free (aux);
    free (fun);
    free (fac);
    free (bit);
    free (bit_div);
    free (bit_mod);
    free (mask_1);
    free (mask_31);
    free (shift_vec);
    free (shift5_vec);
}

void bloom_chk(uint32_t* entries, size_t entries_size, uint32_t* bloom_filter, size_t bloom_filter_size, uint32_t* factors, uint32_t* shift_m, size_t functions, uint32_t* output, uint32_t* output_count){
    uint32_t *mask_k, *mask_kk; //mask that says which entries to continue calculating
    uint32_t *mask_0, *mask_1, *mask_31;
    uint32_t *shift_vec, *shift5_vec; 
    uint32_t *key;
    uint32_t *fun, *fac, *fun_max;
    uint32_t *bit, *bit_div, *bit_mod;

    key = (uint32_t*) malloc (VECTOR_SIZE * sizeof(uint32_t));
    fun = (uint32_t*) malloc (VECTOR_SIZE * sizeof(uint32_t));
    fac = (uint32_t*) malloc (VECTOR_SIZE * sizeof(uint32_t));
    fun_max = (uint32_t*) malloc (VECTOR_SIZE * sizeof(uint32_t));
    bit = (uint32_t*) malloc (VECTOR_SIZE * sizeof(uint32_t));
    bit_div = (uint32_t*) malloc (VECTOR_SIZE * sizeof(uint32_t));
    bit_mod = (uint32_t*) malloc (VECTOR_SIZE * sizeof(uint32_t));
    mask_k = (uint32_t*) malloc (VECTOR_SIZE * sizeof(uint32_t));
    mask_kk = (uint32_t*) malloc (VECTOR_SIZE * sizeof(uint32_t));
    mask_0 = (uint32_t*) malloc (VECTOR_SIZE * sizeof(uint32_t));
    mask_1 = (uint32_t*) malloc (VECTOR_SIZE * sizeof(uint32_t));
    mask_31 = (uint32_t*) malloc (VECTOR_SIZE * sizeof(uint32_t));
    shift_vec = (uint32_t*) malloc (VECTOR_SIZE * sizeof(uint32_t));
    shift5_vec = (uint32_t*) malloc (VECTOR_SIZE * sizeof(uint32_t));

    _vim2K_imovu (0, key);
    _vim2K_imovu (1, mask_k);
    _vim2K_imovu (0, mask_kk);
    _vim2K_imovu (0, mask_0);
    _vim2K_imovu (1, mask_1);
    _vim2K_imovu (31, mask_31);
    _vim2K_imovu (5, shift5_vec);
    _vim2K_imovu (functions - 1, fun_max);
    int i = 0;
    *output_count = 0;
    uint32_t j = VECTOR_SIZE;

    for (int i = 0; i <= entries_size; ) {
        _vim2K_ilmku (&entries[i], mask_k, key); //load new entries according to the mask.
        i += j;
        _vim2K_irmku (fun, mask_k); //take the entries of the hash counters back to 0 if they were reset.
        _vim2K_icpyu (key, bit);
        _vim2K_ipmtu (factors, fun, fac);
        _vim2K_ipmtu (shift_m, fun, shift_vec);
        _vim2K_imulu (bit, fac, bit); //first step of hash
        _vim2K_isllu (bit, shift_vec, bit); //second step of hash
        _vim2K_imodu (bit, bloom_filter_size, bit); //which bit within the bloom filter to set!
        _vim2K_isrlu (bit, shift5_vec, bit_div); //which integer inside the bloom filter?
        _vim2K_iandu (bit, mask_31, bit_mod); //which bit inside that integer?
        _vim2K_isllu (mask_1, bit_mod, bit); //bits are positioned!
        _vim2K_igtru (bloom_filter, bit_div, bit_div); //we gather those integers from the bloom filter!
        _vim2K_iandu (bit, bit_div, bit); //check if the bits are set
        _vim2K_icmqu (bit, mask_0, mask_k); //if they're not, those entries don't go on
        _vim2K_icmqu (fun, fun_max, mask_kk); //which entries have gotten to the last hash function?
        
        _vim2K_idptu (mask_kk, &j);
        if (j > 0) {
            _vim2K_ismku (key, mask_kk, &output[*output_count]);
            *output_count += j;
        }

        _vim2K_iorun (mask_k, mask_kk, mask_k); //which ones passed that also? those are ready and must be added to the result.
        _vim2K_idptu (mask_k, &j);
        _vim2K_iaddu (fun, mask_1, fun);
    };
    
    free (key);
    free (fun);
    free (fac);
    free (fun_max);
    free (bit);
    free (bit_div);
    free (bit_mod);
    free (mask_k);
    free (mask_kk);
    free (mask_0);
    free (mask_1);
    free (mask_31);
    free (shift_vec);
    free (shift5_vec);
}

void bloom_confirm_scalar (uint32_t* positives, size_t positives_size, uint32_t* entries, size_t entries_size){
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

void bloom_confirm (uint32_t* positives, size_t positives_size, uint32_t* entries, size_t entries_size){
    if (positives_size < VECTOR_SIZE) {
        bloom_confirm_scalar (positives, positives_size, entries, entries_size);
        return;
    }
    uint32_t result = 0;
    uint32_t count = 0;
    uint32_t* vector = (uint32_t*) malloc (VECTOR_SIZE * sizeof (uint32_t));
    uint32_t* check = (uint32_t*) malloc (VECTOR_SIZE * sizeof (uint32_t));
    _vim2K_imovu (0, check);
    for (int i = 0; i < positives_size; i++){
        _vim2K_imovu (positives[i], vector);
        for (int j = 0; j < entries_size; j += VECTOR_SIZE){
            count = 0;
            _vim2K_icmqu (vector, &entries[j], check);
            _vim2K_idptu (check, &count);
            if (count > 0){
                result++;
                break;
            }
        }
    }
    std::cout << result << " positivos reais.\n";
}

void populate_vector (uint32_t* vector, size_t v_size, string filename){
    uint32_t count = 0;
    string line;
    ifstream openfile (filename);
    if (openfile.is_open()) {
        while (getline (openfile,line) && count <= v_size) vector[count++] = stoi(line);
        openfile.close();
    }
    if (count < v_size){
        for (int i = count; i < v_size; i++) vector[i] = rand() % (v_size*100);
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

    //loadIntegerColumn (o_orderkey, (uint32_t) v_size/4, "/home/srsantos/Experiment/tpch-dbgen/data/orders.tbl", 1);
    //loadIntegerColumn (l_orderkey, v_size, "/home/srsantos/Experiment/tpch-dbgen/data/lineitem.tbl", 1);

    populate_vector (o_orderkey, v_size/4, argv[2]);
    populate_vector (l_orderkey, v_size, argv[3]);

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

    bloom_set (o_orderkey, (uint32_t) v_size/4, bloom_filter, bloom_filter_size, hash_function_factors, shift_amounts, hash_functions);
    
    ORCS_tracing_start();

    bloom_chk (l_orderkey, v_size, bloom_filter, bloom_filter_size, hash_function_factors, shift_amounts, hash_functions, output, &output_count);
    std::cout << output_count << " positives.\n";

    ORCS_tracing_stop();

    //bloom_confirm (output, output_count, o_orderkey, v_size/4);
    //bloom_confirm_scalar (output, output_count, o_orderkey, v_size/4);
    //bloom_confirm_scalar (l_orderkey, v_size, o_orderkey, v_size/4);

    

    free (o_orderkey);
    free (l_orderkey);
    free (hash_function_factors);
    free (shift_amounts);
}
