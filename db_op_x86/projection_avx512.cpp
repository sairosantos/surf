#include <boost/tokenizer.hpp>
#include <boost/algorithm/string.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
//#include <arm_neon.h>
#include <immintrin.h>

#include <iostream>
#include <fstream>
#include <string>
#include <vector>

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

void loadDateColumn (int* data_vector, int v_size, string file_path, int column){
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

void loadIntegerColumn (int* data_vector, int v_size, string file_path, int column){
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

int main (int argc, char const *argv[]){
    ORCS_tracing_stop();
    int vector_size;
    int filter = 15;
    int *vector1, *vector2, *result;
    __m512i vec_a, vec_b, mask_0;
    __mmask16 vec_dst;

    vector_size = atoi(argv[1]);
    
    int v_size = (1024 * 1024 * vector_size)/sizeof(int);

    result = (int*) aligned_alloc (32, v_size * sizeof (int));
    vector1 = (int*) aligned_alloc (32, v_size * sizeof (int));
    vector2 = (int*) aligned_alloc (32, v_size * sizeof (int));
    
    loadIntegerColumn (vector1, v_size, "/home/srsantos/Experiment/tpch-dbgen/data/lineitem.tbl", 4);
    loadIntegerColumn (vector2, v_size, "/home/srsantos/Experiment/tpch-dbgen/data/lineitem.tbl", 5);

    ORCS_tracing_start();

    mask_0 = _mm512_set1_epi32 (0);
    vec_a = _mm512_set1_epi32 (filter);
    for (int i = 0; i < v_size; i += 16){
        vec_b = _mm512_load_si512 ((__m512i *) &vector1[i]);
        vec_dst = _mm512_cmpgt_epi32_mask (vec_b, vec_a);
        vec_b =_mm512_mask_load_epi32 (mask_0, vec_dst, (__m512i *) &vector2[i]);
        _mm512_store_epi32 ((__m512i *) &result[i], vec_b);
    }

    std::cout << vector1[v_size-1] << " ";
    std::cout << vector2[v_size-1] << " ";
    std::cout << result[v_size-1] << " ";

    free (vector1);
    free (vector2);
}
