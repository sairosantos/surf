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

int main (__v32s argc, char const *argv[]){
    ORCS_tracing_stop();
    uint32_t vector_size, VIMA_VECTOR_SIZE;
    uint32_t filter = 15;
    uint32_t *bitmap, *vector1, *vector2, *filter_vec, *result;
    uint32_t prime_numbers[] = {23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71, 73, 79, 83, 89, 97};
    vector_size = atoi(argv[1]);
    VIMA_VECTOR_SIZE = atoi(argv[2]);
    
    __v32u v_size = (1024 * 1024 * vector_size)/sizeof(__v32u);
    bitmap = (uint32_t*) malloc (v_size * sizeof (uint32_t));

    if (VIMA_VECTOR_SIZE == 2048){
        for (int i = 0; i < v_size; i += VIMA_VECTOR_SIZE) _vim2K_imovu (1, &bitmap[i]);
        filter_vec = (uint32_t*) malloc (VIMA_VECTOR_SIZE * sizeof (uint32_t));
        _vim2K_imovu (filter, filter_vec);
    }

    if (VIMA_VECTOR_SIZE == 64){
        for (int i = 0; i < v_size; i += VIMA_VECTOR_SIZE) _vim64_imovu (1, &bitmap[i]);
        filter_vec = (uint32_t*) malloc (VIMA_VECTOR_SIZE * sizeof (uint32_t));
        _vim64_imovu (filter, filter_vec);
    }

    result = (uint32_t*) malloc (v_size * sizeof (uint32_t));
    vector1 = (uint32_t*) malloc (v_size * sizeof (uint32_t));
    vector2 = (uint32_t*) malloc (v_size * sizeof (uint32_t));
    
    loadIntegerColumn (vector1, v_size, "/home/srsantos/Experiment/tpch-dbgen/data/lineitem.tbl", 4);
    loadIntegerColumn (vector2, v_size, "/home/srsantos/Experiment/tpch-dbgen/data/lineitem.tbl", 5);

    if (VIMA_VECTOR_SIZE == 2048){
        for (int i = 0; i < v_size; i += VIMA_VECTOR_SIZE)  _vim2K_isltu (filter_vec, &vector1[i], &bitmap[i]);
    }

    if (VIMA_VECTOR_SIZE == 64){
        for (int i = 0; i < v_size; i += VIMA_VECTOR_SIZE)  _vim64_isltu (filter_vec, &vector1[i], &bitmap[i]);
    }

    if (VIMA_VECTOR_SIZE == 2048){
        ORCS_tracing_start();
        for (int i = 0; i < v_size; i += VIMA_VECTOR_SIZE){
            _vim2K_ilmku (&vector2[i], &bitmap[i], &result[i]);
        }
        ORCS_tracing_stop();
    }

    if (VIMA_VECTOR_SIZE == 64){
        ORCS_tracing_start();
        for (int i = 0; i < v_size; i += VIMA_VECTOR_SIZE){
            _vim64_ilmku (&vector2[i], &bitmap[i], &result[i]);
        }
        ORCS_tracing_stop();
    }

    std::cout << vector1[v_size-1];
    std::cout << vector2[v_size-1];
    std::cout << bitmap[VIMA_VECTOR_SIZE-1];
    std::cout << filter_vec[VIMA_VECTOR_SIZE-1];
    std::cout << result[v_size-1];

    free (bitmap);
    free (vector1);
    free (filter_vec);
}
