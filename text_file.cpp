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
    uint32_t v_size = (1024 * 128 * vector_size)/sizeof(__v32u);

    uint32_t* vec1 = (uint32_t*) malloc (v_size/4 * sizeof(uint32_t));
    uint32_t* vec2 = (uint32_t*) malloc (v_size * sizeof(uint32_t));

    for (int i = 0; i < v_size/4; i++) vec1[i] = i + v_size;
    for (int i = 0; i < v_size; i++) {
        if (i % 10 == 0) vec2[i] = vec1[i/4];
        else vec2[i] = i;
    }

    int count = 0;
    for (int i = 0; i < v_size/4; i++){
        for (int j = 0; j < v_size; j++){
            if (vec1[i] == vec2[j]) {
                count++;
                break;
            }
        }
    }

    std::cout << "vec1 = " << v_size/4 << " elementos.\n";
    std::cout << "vec2 = " << v_size << " elementos.\n";
    std::cout << count << " elementos iguais.\n";

    /*ofstream myfile;
    ofstream myfile2;
    int number = 0;

    myfile.open (argv[2]);
    myfile2.open (argv[3]);
    for (int i = 0; i < v_size; i++) {
        number = (rand() % v_size*10);
        myfile << number << "\n";
        if (i % 10 != 0) number = (rand() % v_size*10);
        myfile2 << number << "\n";
    }
    myfile.close();
    myfile2.close();*/

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
