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

int main (int argc, char const *argv[]) {
    srand (time(NULL));

    uint8_t vector_size = atoi(argv[1]);
    uint32_t v_size = (1024 * 1024 * vector_size)/sizeof(__v32u);

    ofstream myfile;
    myfile.open (argv[2]);
    for (int i = 0; i < v_size; i++) {
        myfile << (rand() % UINT32_MAX) << "\n";
    }
    myfile.close();

    uint32_t* vector = (uint32_t*) malloc (v_size * sizeof (uint32_t));

    string line;
    ifstream openfile (argv[2]);
    if (openfile.is_open()) {
        while (getline (openfile,line)){
            cout << line << '\n';
            stoi(line);
        }
        openfile.close();
    }

    return 0;
}