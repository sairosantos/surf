#include <boost/tokenizer.hpp>
#include <boost/algorithm/string.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <immintrin.h>

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <string>

using namespace std;

uint32_t castDate2Int (string date){
    uint32_t day, month, year;
    uint32_t result;

    if (std::sscanf(date.c_str(), "%d-%d-%d", &year, &month, &day) != 3){
		cout << "Error to parse date string: " << date.c_str() << "." << endl;
    } else result = 10000 * year + 100 * month + day;
    return result;
}

void loadColumn (int* data_vector, uint32_t v_size, string file_path, int column){
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

void populate_vector (uint32_t* vector, size_t v_size){
    for (int i = 0; i < v_size; i++) vector[i] = rand() % UINT32_MAX;
}

int main (int argc, char const *argv[]){
    ORCS_tracing_stop();

    uint32_t vector_size;
    uint32_t filter = 15;
    int *bitmap, *vector1, *filter_vec;
    __m256i vec_a, vec_b, vec_dst;
    
    vector_size = atoi(argv[1]);
    int v_size = (1024 * 1024 * vector_size)/sizeof(int);
    bitmap = (int*) aligned_alloc (32, v_size * sizeof (int));
    for (int j = 0; j < v_size; j += 8) {
        vec_a = _mm256_load_si256 ((__m256i *) &bitmap[j]);
        vec_a = _mm256_set1_epi32(1);
        _mm256_store_si256 ((__m256i *) &bitmap[j], vec_a);
    }

    filter_vec = (int*) aligned_alloc (32, v_size * sizeof (int));
    for (int j = 0; j < v_size; j += 8){
        vec_a = _mm256_load_si256 ((__m256i *) &filter_vec[j]);
        vec_a = _mm256_set1_epi32(filter);
        _mm256_store_si256 ((__m256i *) &filter_vec[j], vec_a);
    }

    vector1 = (int*) aligned_alloc (32, v_size * sizeof (int));
    loadColumn (vector1, v_size, "/home/srsantos/Experiment/tpch-dbgen/data/lineitem.tbl", 4);
    ORCS_tracing_start();

    for (int i = 0; i < v_size; i += 8){
        vec_a = _mm256_load_si256 ((__m256i *) &filter_vec[i]);
        vec_b = _mm256_load_si256 ((__m256i *) &vector1[i]);
        vec_dst = _mm256_cmpgt_epi32 (vec_b, vec_a);
        _mm256_store_si256 ((__m256i *) &bitmap[i], vec_dst);
    }

    std::cout << filter_vec[v_size-1] << " ";
    std::cout << vector1[v_size-1] << " ";
    std::cout << bitmap[v_size-1] << " ";

    free (bitmap);
    free (vector1);
    free (filter_vec);
}
