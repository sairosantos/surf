#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <iostream>
#include <string>
#include <vector>

#define ALPHABET_SIZE 256

int *d_labels, *d_haschild, *d_isprefixkey, *d_values, *s_haschild, *s_louds, *s_values;
char *s_labels;

int sparse_chars;
int dense_parents;
int dense_chars;
int dense_nodes;
int max_level;
int* pos_level;

int split_level = 3;

struct TrieNode
{
    struct TrieNode *children[ALPHABET_SIZE];
    bool parent = false;
    bool isEndOfWord = false;
    int level = 0;
    int real = 0;
};

int select1 (int* bs, int n, int size){
    if (n <= 0) return -1;
    int count = 0, i = 0;
    for (i = 0; i < size; i++){
        if (bs[i] == 1) count++;
        if (count == n) break;
    }
    return i;
}

int rank1 (int* bs, int pos){
    int count = 0;
    for (int i = 0; i <= pos; i++){
        if (bs[i] == 1) count++;
    }
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

TrieNode* new_node() {
    TrieNode* node = (TrieNode*) malloc (sizeof (TrieNode));
    node->parent = false;
    node->isEndOfWord = false;
    node->level = 0;
    node->real = 0;
    for (int i = 0; i < ALPHABET_SIZE; i++) node->children[i] = NULL;
    return node;
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

void convert (TrieNode* node){
    std::vector<TrieNode*> search;
    search.push_back (node);

    int parent_count = 0;
    bool louds = false;
    int sparse_count = 0;

    TrieNode* current = NULL;

    while (!search.empty()){
        louds = false;
        for (int i = 0; i < ALPHABET_SIZE; i++){
            if (search[0]->children[i] != NULL) {
                current = search[0]->children[i];
                if (current->level <= split_level){
                    d_labels[(parent_count * ALPHABET_SIZE) + i] = 1;
                    if (current->parent) {
                        d_haschild [(parent_count * ALPHABET_SIZE) + i] = 1;
                    }
                    if (current->isEndOfWord) {
                        d_isprefixkey [(parent_count * ALPHABET_SIZE) + i] = 1;
                        d_values [(parent_count * ALPHABET_SIZE) + i] = current->real;
                    }
                } else {
                    s_labels[sparse_count] = i;
                    if (current->parent) s_haschild[sparse_count] = 1;
                    if (!louds){
                        s_louds[sparse_count] = 1;
                        louds = true;
                    }
                    if (current->real != 0) s_values[sparse_count] = current->real;
                    sparse_count++;
                }
                
                search.push_back (current);
            }
        }
        if (search[0]->parent) parent_count++;
        search.erase (search.begin(), search.begin()+1);
    }
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

        if (read > max_level) max_level = read;

        int levels = 0;
        for (int i = 0; i < read; i++){
            if (current->children[line[i]] == NULL) {
                if (!current->parent) {
                    current->parent = true;
                }
                current->children[line[i]] = new_node();
                current->children[line[i]]->level = current->level+1;
            } else if (current->children[line[i]]->isEndOfWord) {
                if (!current->children[line[i]]->parent) {
                    current->children[line[i]]->parent = true;
                }
                current->children[line[i]]->isEndOfWord = false;
                current->children[line[i]]->children['$'] = new_node();
                current->children[line[i]]->children['$']->level = current->children[line[i]]->level+1;
                current->children[line[i]]->children['$']->isEndOfWord = true;
                current->children[line[i]]->children['$']->real = current->children[line[i]]->real;
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

void printDense() {
    for (int i = 0; i < dense_parents * ALPHABET_SIZE; i++){
        if (d_labels[i] == 1 || d_haschild[i] == 1 || d_isprefixkey[i] == 1){
            printf ("%c, D-Labels[%d] = %d, D-HasChild[%d] = %d, D-isPrefixKey[%d] = %d, D-Values[%d] = %d\n", i%ALPHABET_SIZE, i, d_labels[i], i, d_haschild[i], i, d_isprefixkey[i], i, d_values[i]);
        }
    }
}

void printSparse() {
    for (int i = 0; i < sparse_chars; i++){
        if (s_labels[i] > 0){
            printf ("S-Labels[%d] = %c, S-HasChild[%d] = %d, S-LOUDS[%d] = %d, S-Values[%d] = %d\n", i, s_labels[i], i, s_haschild[i], i, s_louds[i], i, s_values[i]);
        }
    }
}

void printTrie (TrieNode* node, std::string prefix){
    for (int i = 0; i < ALPHABET_SIZE; i++){
        if (node->children[i] != NULL){
            printTrie (node->children[i], prefix + (char) i);
        }
    }

    if (node->isEndOfWord) {
        std::cout << prefix;
        printf (" %d", node->real);
        std::cout << "\n";
    }
}

int lookupDense (std::string string, int* value_hash, int* cutoff){
    int i = 0, pos = 0, child = 0;
    int limit = (split_level < string.length() ? split_level : string.length());

    for (i = 0; i < limit; i++){
        *cutoff = i;
        pos = child + string[i];
        pos_level[i] = pos;
        if (d_isprefixkey[pos] == 1) {
            *value_hash = d_values[pos];
            return 0;
        }
        if (d_labels[pos] == 0) return -1; //prefixo não existe na trie
        child = ALPHABET_SIZE * rank1 (d_haschild, pos);
    }

    if (i < split_level){
        if (d_isprefixkey[pos] == 1) {
            *value_hash = d_values[pos];
            return 0;
        }
        if (d_isprefixkey[child + '$'] == 1){
            *value_hash = d_values[child + '$'];
            return 0;
        }
        return -1;
    } else if (i == string.length() && d_isprefixkey[pos] == 1) return 0;
    return (child/ALPHABET_SIZE - dense_nodes) + 1;//o tamanho do prefixo é >= ao split_value, vamos para o sparse
}

int lookupSparse (std::string string, int dense, int* value_hash, int* cutoff){
    int i = 0, pos = select1 (s_louds, dense, sparse_chars);
    bool found = false, child = false;
    if (string.length() == 0 && s_labels[pos] == '$') {
        *value_hash = s_values[pos];
        *cutoff = split_level;
        return 0;
    }
    for (i = 0; i < string.length() && pos < sparse_chars; i++){
        *cutoff = i + split_level;
        do {
            if (string[i] == s_labels[pos]) {
                pos_level[split_level + i] = pos;
                *value_hash = s_values[pos];
                break;
            } else pos++;
        } while (s_louds[pos] == 0);
        if (s_haschild[pos] == 1) {
            pos = select1 (s_louds, (rank1 (s_haschild, pos) + dense_parents) + 1 - dense_nodes, sparse_chars);
        }
        else break;
    }
    if (i == string.length()) return (found ? 0 : 1); //percorremos toda a string e chegamos ao fim
    if (!child) return 0; //paramos pois chegamos a uma folha, resultado positivo, possivelmente falso.
    return 1;
}

bool lookup (std::string input){
    int result = -1;
    int cutoff = 0;
    int value_real = 0;
    int input_real = 0;
    result = lookupDense (input, &value_real, &cutoff);
    if (result > 0) result = lookupSparse (input.substr (split_level, input.length()), result, &value_real, &cutoff);
    if (cutoff < input.length()) input_real = input.substr (cutoff + 1, input.length())[0];
    return (value_real == input_real);
}

void cleanMaxArray() {
    for (int i = 0; i < max_level; i++) pos_level[i] = 0;
}

void printMaxArray() {
    for (int i = 0; i < max_level; i++) printf ("pos_level[%d] = %d\n", i, pos_level[i]);
}

void keyboardInput() {
    std::string input;
    std::string end = "end";
    std::string op = "";

    std::cout << "a - exact key search | b - lower bound search: ";
    std::cin >> op;
    
    while (input.compare(end) != 0) {
        std::cin >> input;
        if (input.compare ("") && input.compare (end)) {
            if (!op.compare ("a")){
                if (lookup (input)) std::cout << "YES\n";
                else std::cout << "NO\n";
            } else if (!op.compare ("b")){
                cleanMaxArray();
                //lowerBound (input);
            } else {
                std::cout << "invalid option";
                input = "end";
            }
        }
    }
}

void deleteTrie (TrieNode* node){
    for (int i = 0; i < ALPHABET_SIZE; i++){
        if (node->children[i] != NULL) deleteTrie (node->children[i]);
    }
    free (node);
}

int surfReal (TrieNode* node){
    int children = 0;
    if (!node->parent) return 0;
    for (int i = 0; i < ALPHABET_SIZE; i++){
        if (node->children[i] != NULL) {
            children++;
            children += surfReal (node->children[i]);
        }
    }

    if (children <= 1) {
        for (int i = 0; i < ALPHABET_SIZE; i++) {
            if (node->children[i] != NULL) {
                deleteTrie (node->children[i]);
                node->children[i] = NULL;
                node->parent = false;
                node->isEndOfWord = true;
                node->real = i;
                return 0;
            }
        }
    }

    return children;
}

void specs (TrieNode* node){
    std::vector<TrieNode*> search;
    search.push_back (node);

    TrieNode* current = NULL;
    bool node_count = false;

    while (!search.empty()){
        node_count = false;
        for (int i = 0; i < ALPHABET_SIZE; i++){
            if (search[0]->children[i] != NULL) {
                current = search[0]->children[i];
                if (current->level <= split_level){
                    if (!node_count) {
                        node_count = true;
                        dense_nodes++;
                    }
                    if (current->parent) dense_parents++;
                    dense_chars++;
                } else sparse_chars++;
                
                search.push_back (current);
            }
        }
        search.erase (search.begin(), search.begin()+1);
    }
}

void test (const char* filename){
    FILE* fp;
    char* line = NULL;
    size_t len = 0;
    ssize_t read;

    fp = fopen(filename, "r");
    if (fp == NULL) exit(EXIT_FAILURE);

    while ((read = getline(&line, &len, fp)) != -1) {
        line = trim (line, NULL);
        read = strlen (line);

        std::string str(line);
        std::cout << str;
        if (lookup (str)) std::cout << " YES\n";
        else break;
    }

    fclose(fp);
    if (line) free(line);
}

int main(int argc, char **argv) {
    TrieNode* root = new_node();
    root->level = 0;

    TrieNode* current = root;
    root->level = 0;

    max_level = 0;

    buildTrie (root, argv[1]);
    surfReal (root);

    specs (root);

    printTrie (root, "");

    d_labels = (int*) malloc (dense_parents * ALPHABET_SIZE * sizeof (int));
    d_haschild = (int*) malloc (dense_parents * ALPHABET_SIZE * sizeof (int));
    d_isprefixkey = (int*) malloc (dense_parents * ALPHABET_SIZE * sizeof (int));
    d_values = (int*) malloc (dense_parents * ALPHABET_SIZE * sizeof (int));
    for (int i = 0; i < dense_parents * ALPHABET_SIZE; i++){
        d_labels[i] = 0;
        d_haschild[i] = 0;
        d_isprefixkey[i] = 0;
        d_values[i] = 0;
    }

    s_labels = (char*) malloc (sparse_chars * sizeof (char));
    s_haschild = (int*) malloc (sparse_chars * sizeof (int));
    s_louds = (int*) malloc (sparse_chars * sizeof (int));
    s_values = (int*) malloc (sparse_chars * sizeof (int));
    memset (s_labels, 0, sizeof(s_labels));
    for (int i = 0; i < sparse_chars; i++) {
        s_haschild[i] = 0;
        s_louds[i] = 0;
        s_values[i] = 0;
    }
    
    pos_level = (int*) malloc (max_level * sizeof (int));
    for (int i = 0; i < max_level; i++) pos_level[i] = 0;

    convert (root);
    deleteTrie (root);

    //printDense();
    //printSparse();

    //keyboardInput();
    test (argv[1]);
    
    free (d_labels);
    free (d_haschild);
    free (d_isprefixkey);
    free (s_labels);
    free (s_haschild);
    free (s_louds);
}