#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <iostream>
#include <string>
#include <vector>

#define ALPHABET_SIZE 256
#define MAX_SIZE 4096

int *d_labels, *d_haschild, *d_isprefixkey, *s_haschild, *s_louds;
char *s_labels;

int nodes_total;
int nodes_dense;
int nodes_sparse;
int nodes_dense_parent;

int split_level = 3;

struct TrieNode
{
    struct TrieNode *children[ALPHABET_SIZE];
    bool parent = false;
    bool isEndOfWord = false;
    int level = 0;
};

uint32_t select1 (int* bs, int n){
    uint32_t count = 0;
    int i = 0;
    for (i = 0; i < nodes_total; i++){
        if (bs[i] == 1) count++;
        if (count == n) break;
    }
    //printf ("select = %d\n", i);
    return i;
}

uint32_t select0 (int* bs, int n){
    uint32_t count = 0;
    int i = 0;
    for (i = 0; i < nodes_total; i++){
        if (bs[i] == 0) count++;
        if (count == n) break;
    }
    printf ("select0 (%d) = %d\n", n, i);
    return i;
}

uint32_t rank1 (int* bs, int pos){
    int count = 0;
    for (int i = 0; i <= pos; i++){
        if (bs[i] == 1) {
            count++;
        }
    }
    //printf ("rank1 (%d) = %d\n", pos, count);
    return count;
}

uint32_t rank0 (int* bs, int pos){
    uint32_t count = 0;
    for (int i = 0; i <= pos; i++){
        if (bs[i] == 0) count++;
    }
    printf ("rank0 (%d) = %d\n", pos, count);
    return count;
}

char *ltrim(char *str, const char *seps) {
    size_t totrim;
    if (seps == NULL) {
        seps = "\t\n\v\f\r ";
    }
    totrim = strspn (str, seps);
    if (totrim > 0) {
        size_t len = strlen(str);
        if (totrim == len) {
            str[0] = '\0';
        }
        else {
            memmove (str, str + totrim, len + 1 - totrim);
        }
    }
    return str;
}

char *rtrim (char *str, const char *seps) {
    int i;
    if (seps == NULL) {
        seps = "\t\n\v\f\r ";
    }
    i = strlen (str) - 1;
    while (i >= 0 && strchr (seps, str[i]) != NULL) {
        str[i] = '\0';
        i--;
    }
    return str;
}

char *trim (char *str, const char *seps){
    return ltrim (rtrim (str, seps), seps);
}

void buildTrie (TrieNode* root, const char* filename){
    FILE* fp;
    char* line = NULL;
    size_t len = 0;
    ssize_t read;

    fp = fopen(filename, "r");
    if (fp == NULL) exit(EXIT_FAILURE);

    TrieNode* current = root;
    root->parent = true;
    root->isEndOfWord = false;

    while ((read = getline(&line, &len, fp)) != -1) {
        line = trim (line, NULL);
        read = strlen (line);

        int levels = 0;
        for (int i = 0; i < read; i++){
            if (current->children[line[i]] == NULL) {
                if (!current->parent) current->parent = true;
                current->children[line[i]] = (TrieNode*) malloc (sizeof (TrieNode));
                current->children[line[i]]->level = current->level+1;
            }
                        
            current = current->children[line[i]];
            levels++;
        }
        current->isEndOfWord = true;
        current = root;
    }

    fclose(fp);
    if (line) free(line);
}

void printNode (TrieNode* node){
    for (int i = 0; i < ALPHABET_SIZE; i++){
        if (node->children[i] != NULL) {
            printf ("%c, level: %d\n", i%ALPHABET_SIZE, node->children[i]->level);
            printNode (node->children[i]);
        }
    }
}

void convert (TrieNode* node){
    std::vector<TrieNode*> search;
    search.push_back (node);

    int parent_count = 0;
    bool louds = false;
    bool node_count = false;

    TrieNode* current = NULL;

    while (!search.empty()){
        louds = false;
        node_count = false;
        for (int i = 0; i < ALPHABET_SIZE; i++){
            current = search[0]->children[i];
            if (current != NULL) {
                if (current->level <= split_level){
                    if (!node_count){
                        node_count = true;
                        nodes_total++;
                        nodes_dense++;
                    }
                    d_labels[(parent_count * ALPHABET_SIZE) + i] = 1;
                    if (current->parent) {
                        d_haschild [(parent_count * ALPHABET_SIZE) + i] = 1;
                        nodes_dense_parent++;
                    }
                    if (current->isEndOfWord) d_isprefixkey [(parent_count * ALPHABET_SIZE) + i] = 1;
                } else {
                    if (!node_count){
                        node_count = true;
                        nodes_total++;
                        nodes_sparse++;
                    }
                    s_labels[nodes_sparse] = i;
                    if (current->parent) {
                        s_haschild[nodes_sparse] = 1;
                    }

                    if (!louds){
                        s_louds[nodes_sparse] = 1;
                        louds = true;
                    }
                }
                
                search.push_back (current);
            }
        }
        if (search[0]->parent) parent_count++;
        search.erase (search.begin(), search.begin()+1);
    }

    printf ("Nodes: %d\nDense: %d\nDense parents: %d\nSparse: %d\n", nodes_total, nodes_dense, nodes_dense_parent, nodes_sparse);
}

void printDense() {
    for (int i = 0; i < MAX_SIZE; i++){
        if (d_labels[i] == 1){
            printf ("%c, D-Labels[%d] = %d, D-HasChild[%d] = %d, D-isPrefixKey[%d] = %d\n", i%ALPHABET_SIZE, i, d_labels[i], i, d_haschild[i], i, d_isprefixkey[i]);
        }
    }
}

void printSparse() {
    for (int i = 0; i < MAX_SIZE; i++){
        if (s_labels[i] > 0){
            printf ("S-Labels[%d] = %c, S-HasChild[%d] = %d, S-LOUDS[%d] = %d\n", i, s_labels[i], i, s_haschild[i], i, s_louds[i]);
        }
    }
}

int lookupDense (std::string string){
    int i = 0, pos = 0, child = 0;
    int limit = (split_level < string.length() ? split_level : string.length());

    int max_level_pos = ALPHABET_SIZE;
    int max_level_pos_hc = 0;

    int pos_hc = 0;
    int node = 0;

    for (i = 0; i < limit; i++){
        pos = child + string[i];
        if (d_labels[pos] == 0) return -1;
        pos_hc = rank1 (d_haschild, pos);
        child = ALPHABET_SIZE * pos_hc; //para a próxima iteração

        node = pos_hc - max_level_pos_hc;
        
        max_level_pos_hc = rank1 (d_haschild, max_level_pos);
        max_level_pos = ALPHABET_SIZE + (rank1 (d_haschild, max_level_pos) * ALPHABET_SIZE);
    }
    if (i == string.length() && d_isprefixkey[pos] == 1) return 0;
    else if (i == split_level) return node;
    return -1;
}

int lookupSparse (std::string string, int dense){
    int i = 0, pos = select1 (s_louds, dense);
    bool found = false;
    for (int i = 0; i < string.length(); i++){
        do {
            //printf ("string = %c, s_labels[%d] = %c\n", string[i], pos, s_labels[pos]);
            if (string[i] == s_labels[pos]) {
                found = true;
                break;
            } else {
                pos++;
                found = false;
            }
        } while (s_louds[pos] == 0);
        //printf ("S-Labels[%d] = %c, S-HasChild[%d] = %u, S-LOUDS[%d] = %u\n", pos, s_labels[pos], pos, s_haschild[pos], pos, s_louds[pos]);
        if (!found) return -1;
        else pos = select1 (s_louds, (rank1 (s_haschild, pos) + nodes_dense_parent) + 1 - nodes_dense);
    }
    return 0;
}

void keyboardInput() {
    std::string input;
    std::string end = "end";
    int result = false;

    while (input.compare(end) != 0) {
        std::cin >> input;
        if (input.compare ("") && input.compare (end)) {
            result = lookupDense (input);
            //printf ("result = %d\n", result);
            if (result == 0) {
                std::cout << "YES";
            } else if (result > 0) {
                result = lookupSparse (input.substr (split_level, input.length()), result);
                if (result == 0) std::cout << "YES";
                else std::cout << "NO";
            } else std::cout << "NO";
            std::cout << "\n";
        }
    }
}

int main(int argc, char **argv) {
    TrieNode* root = (TrieNode*) malloc (sizeof (TrieNode));
    root->level = 0;

    TrieNode* current = root;
    root->level = 0;

    d_labels = (int*) malloc (MAX_SIZE * ALPHABET_SIZE * sizeof (int));
    d_haschild = (int*) malloc (MAX_SIZE * ALPHABET_SIZE * sizeof (int));
    d_isprefixkey = (int*) malloc (MAX_SIZE * ALPHABET_SIZE * sizeof (int));
    for (int i = 0; i < MAX_SIZE * ALPHABET_SIZE; i++){
        d_labels[i] = 0;
        d_haschild[i] = 0;
        d_isprefixkey[i] = 0;
    }

    s_labels = (char*) malloc (MAX_SIZE * sizeof (char));
    s_haschild = (int*) malloc (MAX_SIZE * sizeof (int));
    s_louds = (int*) malloc (MAX_SIZE * sizeof (int));
    memset (s_labels, 0, sizeof(s_labels));
    for (int i = 0; i < MAX_SIZE; i++) {
        s_haschild[i] = 0;
        s_louds[i] = 0;
    }

    buildTrie (root, "words.txt");
    convert (root);
    printDense();
    printSparse();

    printf ("Há %d nós no Dense e %d no Sparse.\n", nodes_dense, nodes_sparse);

    keyboardInput();
}