#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
//#include <arm_neon.h>
#include <immintrin.h>
#include "../intrinsics/vima/vima.hpp"

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <bitset>

using namespace std;

void bloom_confirm (int32_t* positives, size_t positives_size, int32_t* entries, size_t entries_size){
    int32_t result = 0;
    int32_t count = 0;
    __m512i vector, entries_vec;
    __mmask16 mask;
    int i, j;

    for (i = 0; i < positives_size; i++){
        vector = _mm512_set1_epi32 (positives[i]);
        for (j = 0; j < entries_size; j += 16){
            entries_vec = _mm512_load_si512 (&entries[j]);
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

int main (int argc, char const *argv[]) {
    srand (time(NULL));

    uint8_t vector_size = atoi(argv[1]);
    uint32_t prob = atoi(argv[2]);
    uint32_t v_size = (1024 * 1024 * vector_size)/sizeof(__v32u) + 10;

    uint32_t* vec1 = (uint32_t*) malloc (v_size/4 * sizeof(uint32_t));
    uint32_t* vec2 = (uint32_t*) malloc (v_size * sizeof(uint32_t));

    uint32_t max = 0;
    for (int i = 0; i < v_size/4; i++) {
        vec1[i] = i + (rand() % UINT32_MAX/5);
        if (vec1[i] > max) max = vec1[i];
    }
    for (int i = 0; i < v_size; i++) {
        if (i % 10 < prob) vec2[i] = vec1[i/4];
        else vec2[i] = max + (rand() % UINT32_MAX/10);
    }

    ofstream myfile, myfile2;
    myfile.open (argv[3]);
    myfile2.open (argv[4]);
    
    for (int i = 0; i <= v_size/4; i++) myfile << vec1[i] << "\n";
    for (int i = 0; i <= v_size; i++) myfile2 << vec2[i] << "\n";

    myfile.close();
    myfile2.close();

    std::cout << "vec1 = " << v_size/4 << " elementos.\n";
    std::cout << "vec2 = " << v_size << " elementos.\n";
    //bloom_confirm (vec2, v_size, vec1, v_size/4);

    /*uint32_t* vector = (uint32_t*) malloc (v_size * sizeof (uint32_t));

    string line;
    ifstream openfile (argv[2]);
    if (openfile.is_open()) {
        while (getline (openfile,line)){
            cout << line << '\n';
            stoi(line);
        }
        openfile.close();
    }*/

    return 0;
}
