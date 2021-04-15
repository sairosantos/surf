#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <arm_neon.h>

#include <iostream>
#include <string>
#include <vector>

#define ALPHABET_SIZE 256
#define VECTOR_SIZE 8192

using namespace std;

uint8_t *d_labels, *d_haschild, *d_isprefixkey, *s_haschild, *s_louds;
char *s_labels;

const char* filename;
uint8_t* char_vector;

int sparse_chars;
int dense_parents;
int dense_chars;
int dense_nodes;
int max_level;
int seed;
int* pos_level;
int* count_level;

int max_haschild;

int split_level = 3;

struct TrieNode
{
    struct TrieNode *children[ALPHABET_SIZE];
    bool parent = false;
    bool isEndOfWord = false;
    int level = 0;
    int hash = 0;
};

int select1 (uint8_t* bs, int n, int size){
    if (n <= 0) return -1;
    int count = 0, i = 0;
    for (i = 0; i < size; i++){
        if (bs[i] == 1) count++;
        if (count == n) break;
    }
    return i;
}

int rank1 (uint8_t* bs, int pos){
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
    node->hash = 0;
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
                    }
                } else {
                    s_labels[sparse_count] = i;
                    if (current->parent) s_haschild[sparse_count] = 1;
                    if (!louds){
                        s_louds[sparse_count] = 1;
                        louds = true;
                    }
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
    for (int i = 0; i < dense_chars * ALPHABET_SIZE; i++){
        if (d_labels[i] == 1 || d_haschild[i] == 1 || d_isprefixkey[i] == 1){
            printf ("%c, D-Labels[%d] = %d, D-HasChild[%d] = %d, D-isPrefixKey[%d] = %d\n", i%ALPHABET_SIZE, i, d_labels[i], i, d_haschild[i], i, d_isprefixkey[i]);
        }
    }
}

void printSparse() {
    for (int i = 0; i < sparse_chars; i++){
        if (s_labels[i] > 0){
            printf ("S-Labels[%d] = %c, S-HasChild[%d] = %d, S-LOUDS[%d] = %d\n", i, s_labels[i], i, s_haschild[i], i, s_louds[i]);
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
        printf (" %d", node->hash);
        std::cout << "\n";
    }
}

int lookupDense (std::string string){
    int i = 0, pos = 0, child = 0;
    int limit = (split_level < string.length() ? split_level : string.length());

    for (i = 0; i < limit; i++){
        pos = child + string[i];
        pos_level[i] = pos;
        if (d_isprefixkey[pos] == 1) {
            return 0;
        }
        if (d_labels[pos] == 0) return -1; //prefixo não existe na trie
        child = ALPHABET_SIZE * rank1 (d_haschild, pos);
    }

    if (i < split_level){
        if (d_isprefixkey[pos] == 1) {
            return 0;
        }
        if (d_isprefixkey[child + '$'] == 1){
            return 0;
        }
        return -1;
    } else if (i == string.length() && d_isprefixkey[pos] == 1) return 0;
    return (child/ALPHABET_SIZE - dense_nodes) + 1;//o tamanho do prefixo é >= ao split_value, vamos para o sparse
}

int lookupSparse (std::string string, int dense){
    int i = 0, pos = select1 (s_louds, dense, sparse_chars);
    bool found = false, child = false;
    if (string.length() == 0 && s_labels[pos] == '$') {
        return 0;
    }
    for (i = 0; i < string.length() && pos < sparse_chars; i++){
        do {
            if (string[i] == s_labels[pos]) {
                found = true;
                pos_level[split_level + i] = pos;
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
    result = lookupDense (input);
    if (result == 0) {
        return true;
    } else if (result > 0){
        result = lookupSparse (input.substr (split_level, input.length()), result);
        if (result == 0) return true;
        else return false;
    }
    return false;
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

void deleteTrie (TrieNode* node){
    for (int i = 0; i < ALPHABET_SIZE; i++){
        if (node->children[i] != NULL) deleteTrie (node->children[i]);
    }
    free (node);
}

int surfBase (TrieNode* node){
    int children = 0;
    if (!node->parent) return 0;
    for (int i = 0; i < ALPHABET_SIZE; i++){
        if (node->children[i] != NULL) {
            children++;
            children += surfBase (node->children[i]);
        }
    }

    if (children <= 1) {
        for (int i = 0; i < ALPHABET_SIZE; i++) {
            if (node->children[i] != NULL) {
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

uint8x8_t dot_product (uint8_t* src, uint8x8_t idx, uint8x8_t mask_vec){
    uint8x8_t res, aux_vec;
    uint8_t min, i;
    uint8x8_t aux;
    uint16_t accum = 0;

    res = vdup_n_u8 (0);
    min = vminv_u8 (idx);

    i = 0;
    for (int j = 0; j < 8; j++){
        if (mask_vec[j] = 1){
            accum = 0;
            for (i = 0; i < idx[j] - 8; i += 8){
                aux = vld1_u8 (&src[i]);
                //for (int j = 0; j < 8; j++) printf ("aux[%d] = %d\n", i+j, aux[j]);
                accum += vaddlv_u8 (aux);
            }

            aux = vld1_u8 (&src[i]);
            for (; i <= idx[j]; i++) {
                accum += aux[i%8];
                //printf ("aux[%d] = %d\n", i, aux[i%8]);
            }
            res[j] = accum;
        }
    }

    return res;
}

uint8x8_t gather (uint8_t* src, uint8x8_t idx, uint8x8_t mask_vec){
    uint8x8_t res;
    uint8_t min = UINT8_MAX, max = 0, load = 0;
    uint16_t sum = 0;
    uint8x8_t min_vec, max_vec;
    uint8x16_t aux;
    uint8_t aux_count;
    
    load = min = vminv_u8 (idx);
    min_vec = vdup_n_u8 (min);
    idx = vsub_u8 (idx, min_vec);

    do {
        //printf ("min = %d\n", min);
        aux = vld1q_u8 (&src[load]); //carregando os 16 elementos
        //for (int i = 0; i < 16; i++) printf ("aux[%d] = %d ", load+i, aux[i]);
        //printf ("\n");
        //for (int i = 0; i < 8; i++) printf ("idx[%d] = %d | ", i, idx[i]);
        //printf ("\n");

        for (int i = 0; i < 8; i++){
            if (mask_vec[i] == 1) {
                if (idx[i] < 16) {
                    res[i] = aux[idx[i]];
                    idx[i] = 0;
                    mask_vec[i] = 0;
                } else idx[i] -= 16;
            }
        }

        sum = vaddlv_u8 (mask_vec);
        //printf ("sum = %d\n", sum);

        for (int i = 0; i < 8; i++) {
            if (idx[i] > 0) {
                aux_count = idx[i];
                break;
            }
        }

        //printf ("aux_count = %d\n", aux_count);
        //printf ("aux_count/16 = %d\n", aux_count/16);

        load += 16 * (aux_count/16 + 1);
        //for (int i = 0; i < 8; i++) printf ("res[%d] = %d ", i, res[i]);
    } while (sum != 0);
    
    return res;
}

void char_vector_load(){
    FILE* fp;
    char* line = NULL;
    size_t len = 0;
    ssize_t read;

    int pos = 0;

    fp = fopen(filename, "r");
    if (fp == NULL) exit(EXIT_FAILURE);

    while ((read = getline(&line, &len, fp)) != -1 && pos < VECTOR_SIZE) {
        line = trim (line, NULL);
        read = strlen (line);

        for (int i = 0; i < read; i++){
            char_vector[pos] = line[i];
            pos++;
        }

        while (pos%4 != 0) pos++;
    }

    fclose(fp);
    if (line) free(line);
}

void neon_load(){
    uint8x8_t vector_a, vector_b, vector_c, vector_d;
    uint8x8x4_t result;

    int main_vector = 0;
    for (int i = 0; i < 8; i += 2){
        result = vld4_u8 (&char_vector[main_vector]);
        vector_a[i] = result.val[0][0];
        vector_b[i] = result.val[1][0];
        vector_c[i] = result.val[2][0];
        vector_d[i] = result.val[3][0];
        vector_a[i+1] = result.val[0][1];
        vector_b[i+1] = result.val[1][1];
        vector_c[i+1] = result.val[2][1];
        vector_d[i+1] = result.val[3][1];
        main_vector += 8;
    }

    for (int i = 0; i < 8; i++){
        cout << "vector_a[" << i << "] = " << vector_a[i];
        cout << " vector_b[" << i << "] = " << vector_b[i];
        cout << " vector_c[" << i << "] = " << vector_c[i];
        cout << " vector_d[" << i << "] = " << vector_d[i] << "\n";
    }

    uint8x8_t vec_mul = vdup_n_u8 (255);
    uint8x8_t lookup_mask = vdup_n_u8 (1);
    for (int i = 0; i < 8; i++) printf ("lookup_mask[%d] = %d | ", i, lookup_mask[i]);
    printf ("\n");
    uint8x8_t result_mask = vdup_n_u8 (0);
    result_mask = gather (d_isprefixkey, vector_a, lookup_mask);
    //for (int i = 0; i < 8; i++) printf ("result_mask[%d] = %d | ", i, result_mask[i]);
    lookup_mask = dot_product (d_haschild, vector_a, result_mask);
    for (int i = 0; i < 8; i++) printf ("lookup_mask[%d] = %d | ", i, lookup_mask[i]);
    printf ("\n");
    //lookup_mask = vmull_u8 (lookup_mask, vec_mul);
    for (int i = 0; i < 8; i++) printf ("lookup_mask[%d] = %d | ", i, lookup_mask[i]);
    printf ("\n");
    lookup_mask = vadd_u8 (lookup_mask, vec_mul);
    for (int i = 0; i < 8; i++) printf ("lookup_mask[%d] = %d | ", i, lookup_mask[i]);
    printf ("\n");
}

void bloom_filter (uint8_t* src, uint8_t* filter, int src_length) {
    uint8x16x4_t load;
    uint32x4_t h1, h2, h3;
    uint32x4_t aux1, aux2;
    uint32x4_t* neon_r;
    neon_r = (uint32x4_t*) malloc (4 * sizeof (uint32x4_t));

    h1 = vdupq_n_u32 (src_length % 5);
    h2 = vdupq_n_u32 (src_length % 34);
    h3 = vdupq_n_u32 (src_length % 12);

    aux1 = vdupq_n_u32 (0);
    aux2 = vdupq_n_u32 (0);
    for (int i = 0; i < 4; i++) neon_r[i] = vdupq_n_u32 (0);
    
    for (uint32_t src_idx = 0; src_idx < src_length; src_idx += 16){
        load = vld4q_u8 (&src[src_idx]);
        for (int i = 0; i < 4; i++){
            neon_r[i][0] = load.val[i][0];
            neon_r[i][1] = load.val[i][1];
            neon_r[i][2] = load.val[i][2];
            neon_r[i][3] = load.val[i][3];
        }
    }

    for (int i = 0; i < 4; i++){
        aux1 = vshlq_n_u32 (h1, 5);
        aux2 = vshrq_n_u32 (h1, 7);
        aux1 = vaddq_u32 (aux1, aux2);
        aux1 = vaddq_u32 (aux1, neon_r[i]);
        h1 = veorq_u32 (h1, aux1);

        aux1 = vshlq_n_u32 (h2, 5);
        aux2 = vshrq_n_u32 (h2, 7);
        aux1 = vaddq_u32 (aux1, aux2);
        aux1 = vaddq_u32 (aux1, neon_r[i]);
        h2 = veorq_u32 (h2, aux1);

        aux1 = vshlq_n_u32 (h3, 5);
        aux2 = vshrq_n_u32 (h3, 7);
        aux1 = vaddq_u32 (aux1, aux2);
        aux1 = vaddq_u32 (aux1, neon_r[i]);
        h3 = veorq_u32 (h3, aux1);
    }

    for (int i = 0; i < 4; i++){
        h1[i] = h1[i] % src_length;
        cout << "h1[" << i << "] = " << h1[i];
        h2[i] = h2[i] % src_length;
        cout << " h2[" << i << "] = " << h2[i];
        h3[i] = h3[i] % src_length;
        cout << " h3[" << i << "] = " << h3[i];
        cout << "\n";
    }
}

int main(int argc, char **argv) {
    TrieNode* root = new_node();
    root->level = 0;

    TrieNode* current = root;
    root->level = 0;

    max_haschild = 0;
    max_level = 0;
    filename = argv[1];

    char_vector = (uint8_t*) malloc (VECTOR_SIZE * sizeof(uint8_t));
    
    buildTrie (root, filename);
    count_level = (int*) malloc (max_level*sizeof(int));
    pos_level = (int*) malloc (max_level * sizeof (int));
    for (int i = 0; i < max_level; i++) {
        count_level[i] = 0;
        pos_level[i] = 0;
    }

    surfBase (root);

    int dense_size = 0, sparse_size = 0, i = 0;
    for (split_level = 0; split_level < max_level; split_level++){
        for (i = 0; i < split_level; i++) dense_size += count_level[i];
        for (i = split_level; i < max_level; i++) sparse_size += count_level[i];
        if (dense_size * 64 > sparse_size) break;
    }

    specs (root);

    d_labels = (uint8_t*) malloc (dense_chars * ALPHABET_SIZE * sizeof (uint8_t));
    d_haschild = (uint8_t*) malloc (dense_chars * ALPHABET_SIZE * sizeof (uint8_t));
    d_isprefixkey = (uint8_t*) malloc (dense_chars * ALPHABET_SIZE * sizeof (uint8_t));
    for (int i = 0; i < dense_chars * ALPHABET_SIZE; i++){
        d_labels[i] = 0;
        d_haschild[i] = 0;
        d_isprefixkey[i] = 0;
    }

    s_labels = (char*) malloc (sparse_chars * sizeof (char));
    s_haschild = (uint8_t*) malloc (sparse_chars * sizeof (uint8_t));
    s_louds = (uint8_t*) malloc (sparse_chars * sizeof (uint8_t));
    memset (s_labels, 0, sizeof(s_labels));
    for (int i = 0; i < sparse_chars; i++) {
        s_haschild[i] = 0;
        s_louds[i] = 0;
    }

    convert (root);
    deleteTrie (root);

    //printDense();
    //printSparse();

    //keyboardInput();
    //test (argv[1]);

    char_vector_load();
    //neon_load();
    bloom_filter (char_vector, d_labels, 16);

    free (pos_level);
    free (count_level);

    free (d_labels);
    free (d_haschild);
    free (d_isprefixkey);
    free (s_labels);
    free (s_haschild);
    free (s_louds);
}