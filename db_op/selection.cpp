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
#include <string>

#define VECTOR_SIZE 2048

using namespace std;

uint32_t castDate2Int (string date){
    uint32_t day, month, year;
    uint32_t result;

    if (std::sscanf(date.c_str(), "%d-%d-%d", &year, &month, &day) != 3){
		cout << "Error to parse date string: " << date.c_str() << "." << endl;
    } else result = 10000 * year + 100 * month + day;
    return result;
}

void loadColumn (uint32_t* data_vector, uint32_t v_size, string file_path, int column){
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

void populate_vector (uint32_t* vector, size_t v_size){
    for (int i = 0; i < v_size; i++) vector[i] = rand() % UINT32_MAX;
}

int main (__v32s argc, char const *argv[]){
    uint32_t vector_size;
    uint32_t filter = castDate2Int ("1994-01-01");
    uint32_t *bitmap, *vector1, *filter_vec;
    vector_size = atoi(argv[1]);
    
    __v32u v_size = (1024 * vector_size)/sizeof(__v32u);
    bitmap = (uint32_t*) malloc (v_size * sizeof (uint32_t));
    for (int j = 0; j < v_size; j += VM2KI) _vim2K_imovu (1, &bitmap[j]);

    filter_vec = (uint32_t*) malloc (v_size * sizeof (uint32_t));
    for (int j = 0; j < v_size; j += VM2KI) _vim2K_imovu (filter, &filter_vec[j]);

    vector1 = (uint32_t*) malloc (v_size * sizeof (uint32_t));
    srand (time(NULL));

    loadColumn (vector1, v_size, "/home/srsantos/Experiment/tpch-dbgen/data/lineitem.tbl", 10);

    for (int i = 0; i < v_size; i += VECTOR_SIZE){
        _vim2K_isltu (&filter_vec[i], &vector1[i], &bitmap[i]);
    }

    /*for (int i = 0; i < v_size; ++i){
        std::cout << bitmap[i];
    }*/

    free (bitmap);
    free (vector1);
    free (filter_vec);
}
