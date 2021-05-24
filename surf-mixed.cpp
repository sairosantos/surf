#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <iostream>
#include <string>
#include <vector>

#define ALPHABET_SIZE 256

struct Info {
    int real;
    int hash;
};

struct TrieNode
{
    struct TrieNode *children[ALPHABET_SIZE];
    bool parent = false;
    bool isEndOfWord = false;
    int level = 0;
    struct Info info;
};

int *d_labels, *d_haschild, *d_isprefixkey, *s_haschild, *s_louds;
char *s_labels;

Info *d_values, *s_values;

int sparse_chars;
int dense_parents;
int dense_chars;
int dense_nodes;
int max_level;
int seed;
int* pos_level;
int* count_level;

const char* filename;

int split_level;

int shift_add_xor (std::string input){
    int h = 0;
    if (seed == 0){
        srand (time(NULL));
        seed = rand() % 256;
    }

    h = seed;

    for (int i = 0; i < input.length(); i++){
        h = h ^ ((h << 3) + (h >> 2) + input[i]);
    }

    return h & 0b1111111;
}

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
    node->info.hash = 0;
    node->info.real = 0;
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
                current->children[line[i]]->children['$']->info.hash = current->children[line[i]]->info.hash;
            }
                        
            current = current->children[line[i]];
            levels++;
        }
        std::string str(line);
        current->info.hash = shift_add_xor (str);
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
                        d_values [(parent_count * ALPHABET_SIZE) + i].real = current->info.real;
                        d_values [(parent_count * ALPHABET_SIZE) + i].hash = current->info.hash;
                    }
                } else {
                    s_labels[sparse_count] = i;
                    if (current->parent) s_haschild[sparse_count] = 1;
                    if (!louds){
                        s_louds[sparse_count] = 1;
                        louds = true;
                    }
                    s_values[sparse_count].real = current->info.real;
                    s_values[sparse_count].hash = current->info.hash;
                    sparse_count++;
                }
                
                search.push_back (current);
            }
        }
        if (search[0]->parent) parent_count++;
        search.erase (search.begin(), search.begin()+1);
    }
}

void printDense() {
    for (int i = 0; i < dense_parents * ALPHABET_SIZE; i++){
        if (d_labels[i] == 1 || d_haschild[i] == 1 || d_isprefixkey[i] == 1){
            printf ("%c, D-Labels[%d] = %d, D-HasChild[%d] = %d, D-isPrefixKey[%d] = %d, D-Values[%d] = %d (hash) %d (real)\n", i%ALPHABET_SIZE, i, d_labels[i], i, d_haschild[i], i, d_isprefixkey[i], i, d_values[i].hash, d_values[i].real);
        }
    }
}

void printSparse() {
    for (int i = 0; i < sparse_chars; i++){
        if (s_labels[i] > 0){
            printf ("S-Labels[%d] = %c, S-HasChild[%d] = %d, S-LOUDS[%d] = %d, S-Values[%d] = %d (hash) %d (real)\n", i, s_labels[i], i, s_haschild[i], i, s_louds[i], i, s_values[i].hash, s_values[i].real);
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
        printf (" %d", node->info.hash);
        std::cout << "\n";
    }
}

int lookupDense (std::string string, int* value_hash, int* value_real, int* cutoff){
    int i = 0, pos = 0, child = 0;
    int limit = (split_level < string.length() ? split_level : string.length());

    for (i = 0; i < limit; i++){
        *cutoff = i;
        pos = child + string[i];
        pos_level[i] = pos;
        if (d_isprefixkey[pos] == 1) {
            *value_hash = d_values[pos].hash;
            *value_real = d_values[pos].real;
            return 0;
        }
        if (d_labels[pos] == 0) return -1; //prefixo não existe na trie
        child = ALPHABET_SIZE * rank1 (d_haschild, pos);
    }

    if (i < split_level){
        if (d_isprefixkey[pos] == 1) {
            *value_hash = d_values[pos].hash;
            *value_real = d_values[pos].real;
            return 0;
        }
        if (d_isprefixkey[child + '$'] == 1){
            *value_hash = d_values[child + '$'].hash;
            *value_real = d_values[child + '$'].real;
            return 0;
        }
        return -1;
    } else if (i == string.length() && d_isprefixkey[pos] == 1) return 0;
    return (child/ALPHABET_SIZE - dense_nodes) + 1;//o tamanho do prefixo é >= ao split_value, vamos para o sparse
}

int lookupSparse (std::string string, int dense, int* value_hash, int* value_real, int* cutoff){
    int i = 0, pos = select1 (s_louds, dense, sparse_chars);
    bool found = false, child = false;
    if (string.length() == 0 && s_labels[pos] == '$') {
        *value_hash = s_values[pos].hash;
        *value_real = s_values[pos].real;
        *cutoff = split_level;
        return 0;
    }
    for (i = 0; i < string.length() && pos < sparse_chars; i++){
        *cutoff = i + split_level;
        do {
            if (string[i] == s_labels[pos]) {
                pos_level[split_level + i] = pos;
                *value_hash = s_values[pos].hash;
                *value_real = s_values[pos].real;
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
    int input_hash = shift_add_xor (input), value_hash = 0, input_real = 0, value_real = 0, cutoff = 0;
    result = lookupDense (input, &value_hash, &value_real, &cutoff);
    if (result > 0){
        result = lookupSparse (input.substr (split_level, input.length()), result, &value_hash, &value_real, &cutoff);
    }
    if (cutoff < input.length()) input_real = input.substr (cutoff + 1, input.length())[0];
    return (value_hash == input_hash) && (value_real == input_real);
}

void searchAll (std::vector<std::string>* keys, char* current, int level){
    int i = 0;
    int new_level = 0;

    for (i = level; i < max_level; i++) current[i] = 0;
    if (level < split_level){
        for (i = pos_level[level]; i % ALPHABET_SIZE != 0; i++){
            if (d_labels[i] == 1) current[level] = i % ALPHABET_SIZE;
            if (d_haschild[i] == 1) {
                new_level = (ALPHABET_SIZE * rank1 (d_haschild, i)) + 1;
                pos_level[level + 1] = new_level;
                searchAll (keys, current, level + 1);
            }
            if (d_isprefixkey[i] == 1) {
                for (int j = level + 1; j < max_level; j++) {
                    current[j] = 0;
                    pos_level[j] = 0;
                }
                std::string str(current);
                keys->push_back (str);
            }
        }
    } else {
        if (level == split_level) i = select1 (s_louds, (pos_level[level]/ALPHABET_SIZE - dense_nodes) + 1, sparse_chars);
        else i = pos_level[level];
        do {
            current[level] = s_labels[i];
            if (s_haschild[i] == 1) {
                pos_level[level + 1] = select1 (s_louds, (rank1 (s_haschild, i) + dense_parents) + 1 - dense_nodes, sparse_chars);
                searchAll (keys, current, level + 1);
            } else {
                for (int j = level + 1; j < max_level; j++) {
                    current[j] = 0;
                    pos_level[j] = 0;
                }
                std::string str(current);
                keys->push_back (str);
            }
            i++;
        } while (s_louds[i] != 1 && i < sparse_chars);
    }
}

void cleanMaxArray() {
    for (int i = 0; i < max_level; i++) pos_level[i] = 0;
}

void printMaxArray() {
    for (int i = 0; i < max_level; i++) printf ("pos_level[%d] = %d\n", i, pos_level[i]);
}

void lowerBound (std::string input){
    std::vector<std::string> keys;
    char* key = (char*) malloc (max_level * sizeof(char));
    memset (key, 0, sizeof (key));
    lookup (input);
    searchAll (&keys, key, 0);
    for (int i = 0; i < keys.size(); i++) {
        std::cout << keys[i] + "\n";
    }

    free (key);
}

void deleteTrie (TrieNode* node){
    for (int i = 0; i < ALPHABET_SIZE; i++){
        if (node->children[i] != NULL) deleteTrie (node->children[i]);
    }
    free (node);
}

int surfMixed (TrieNode* node){
    int children = 0;
    if (!node->parent) return 0;
    for (int i = 0; i < ALPHABET_SIZE; i++){
        if (node->children[i] != NULL) {
            children++;
            children += surfMixed (node->children[i]);
        }
    }

    if (children <= 1) {
        for (int i = 0; i < ALPHABET_SIZE; i++) {
            if (node->children[i] != NULL) {
                node->info.hash = node->children[i]->info.hash;
                node->info.real = i;
                deleteTrie (node->children[i]);
                node->children[i] = NULL;
                node->parent = false;
                node->isEndOfWord = true;
                return 0;
            }
        }
    }

    count_level[node->level]++;

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

void keyboardInput() {
    std::string input;
    std::string end = "end";
    std::string op = "";

    std::cout << "a - exact key search | b - lower bound search | c - test all: ";
    std::cin >> op;

    if (op < "a" || op > "c") return;
    if (!op.compare ("c")){
        test (filename);
        exit (EXIT_SUCCESS);
    } 
    while (input.compare(end) != 0) {
        std::cin >> input;
        if (input.compare ("") && input.compare (end)) {
            if (!op.compare ("a")){
                if (lookup (input)) std::cout << "YES\n";
                else std::cout << "NO\n";
            } else if (!op.compare ("b")){
                cleanMaxArray();
                lowerBound (input);
            }
        }
    }
}

int main(int argc, char **argv) {
    TrieNode* root = new_node();
    root->level = 0;

    TrieNode* current = root;
    root->level = 0;

    max_level = 0;
    filename = argv[1];
    
    buildTrie (root, filename);
    count_level = (int*) malloc (max_level*sizeof(int));
    pos_level = (int*) malloc (max_level * sizeof (int));
    for (int i = 0; i < max_level; i++) {
        count_level[i] = 0;
        pos_level[i] = 0;
    }

    surfMixed (root);

    int dense_size = 0, sparse_size = 0, i = 0;
    for (split_level = 0; split_level < max_level; split_level++){
        for (i = 0; i < split_level; i++) dense_size += count_level[i];
        for (i = split_level; i < max_level; i++) sparse_size += count_level[i];
        if (dense_size * 64 > sparse_size) break;
    }

    specs (root);

    d_labels = (int*) malloc (dense_parents * ALPHABET_SIZE * sizeof (int));
    d_haschild = (int*) malloc (dense_parents * ALPHABET_SIZE * sizeof (int));
    d_isprefixkey = (int*) malloc (dense_parents * ALPHABET_SIZE * sizeof (int));
    d_values = (Info*) malloc (dense_parents * ALPHABET_SIZE * sizeof (Info));
    for (int i = 0; i < dense_parents * ALPHABET_SIZE; i++){
        d_labels[i] = 0;
        d_haschild[i] = 0;
        d_isprefixkey[i] = 0;
        d_values[i].hash = 0;
        d_values[i].real = 0;
    }

    s_labels = (char*) malloc (sparse_chars * sizeof (char));
    s_haschild = (int*) malloc (sparse_chars * sizeof (int));
    s_louds = (int*) malloc (sparse_chars * sizeof (int));
    s_values = (Info*) malloc (sparse_chars * sizeof (Info));
    memset (s_labels, 0, sizeof(s_labels));
    for (int i = 0; i < sparse_chars; i++) {
        s_haschild[i] = 0;
        s_louds[i] = 0;
        s_values[i].hash = 0;
        s_values[i].real = 0;
    }
    
    pos_level = (int*) malloc (max_level * sizeof (int));
    for (int i = 0; i < max_level; i++) pos_level[i] = 0;

    convert (root);
    deleteTrie (root);

    //printDense();
    //printSparse();

    keyboardInput();
    //test (argv[1]);
    
    free (d_labels);
    free (d_haschild);
    free (d_isprefixkey);
    free (d_values);
    free (s_labels);
    free (s_haschild);
    free (s_louds);
    free (s_values);
}