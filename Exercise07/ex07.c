#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>
#include <assert.h>
#include <stdint.h>

typedef enum {Red, Black} Color; 

typedef struct TreeNode {
    unsigned long value;
    struct TreeNode* left; // left child
    struct TreeNode* right; // right child
    Color color;
} RBNode, *RedBlackTree;

RBNode* NullNode = NULL; 

RedBlackTree Initialize() {
    if (NullNode == NULL) {
        NullNode = malloc(sizeof(*NullNode));
        NullNode->left = NullNode;
        NullNode->right = NullNode;
        NullNode->color = Black;
        NullNode->value = 0;
    }

    RedBlackTree RBTree = malloc(sizeof(RBNode)); 
    RBTree->value = 0;    
    RBTree->left = NullNode;
    RBTree->right = NullNode;
    RBTree->color = Black;

    return RBTree;
}

RedBlackTree RotateLeft(RedBlackTree node) {
    RBNode* temp_left = malloc(sizeof(RBNode));
    temp_left = node->left;
    node->left = temp_left->right;
    temp_left->right = node;

    return temp_left;
}

RedBlackTree RotateRight(RedBlackTree node) {
    RBNode* temp_right = malloc(sizeof(RBNode));
    temp_right = node->right;
    node->right = temp_right->left;
    temp_right->left = node;

    return temp_right;
}

static RedBlackTree Rotate(RBNode* Parent, unsigned long int key) {
    if (key < Parent->value) {
        if (key < Parent->left->value) {
            return Parent->left = RotateLeft(Parent->left);
        }
        else
            return Parent->left = RotateRight(Parent->left);

    }
    else
        return Parent->right = key > Parent->right->value ?
        RotateRight(Parent->right) : RotateLeft(Parent->right);
}

static RedBlackTree node, parent, grandparent, greatgrandparent;

static void HandleCaseRedChildren(RedBlackTree RBTree, unsigned long int key) { // Both children are red nodes

    node->color = Red;
    node->left->color = node->right->color = Black;

    if (parent->color == Red) {
        grandparent->color = Red;
        if ((key < grandparent->value) != (key < parent->value))
            parent = Rotate(grandparent, key);
        node = Rotate(greatgrandparent, key);
    }
    RBTree->right->color = Black;
}

RedBlackTree Add(RedBlackTree RBTree, unsigned long int key) { // Insert a key in tree

    node = parent = grandparent = greatgrandparent = RBTree;
    NullNode->value = key;

    while (node->value != key) {
        greatgrandparent = grandparent;
        grandparent = parent;
        parent = node;
        if (key < node->value)
            node = node->left;
        else
            node = node->right;
        if (node->left->color == Red && node->right->color == Red)
            HandleCaseRedChildren(RBTree, key);
    }

    if (node != NullNode)
        return NullNode;

    node = malloc(sizeof(RBNode));
    node->value = key;
    node->left = node->right = NullNode;

    if (key < parent->value)
        parent->left = node;
    else
        parent->right = node;  
    HandleCaseRedChildren(RBTree, key);
    return RBTree;
}

static void InOrder(RedBlackTree RBTree) { // Print elements in order
    if (RBTree != NullNode) {
        InOrder(RBTree->left);
        printf("%ld ", RBTree->value);
        InOrder(RBTree->right);
    }
}

void PrintTree(RedBlackTree RBTree) {
    InOrder(RBTree->right);
}

static void Search(RedBlackTree RBTree, int key) {
    if (RBTree->value == key && RBTree != NullNode) {
        // printf("Found");
    } else if (RBTree != NullNode) {
        Search(RBTree->left, key);
        Search(RBTree->right, key);
    }
}

void UpdateStream(RedBlackTree RBTree, float ratio, int operations) {
    srand(time(NULL));
    int search_ops = operations * ((1 - ratio) * 10);
    int insert_ops = operations * ratio * 10;
    while (insert_ops) {
        Add(RBTree, rand() % 100000 + 100);
        insert_ops--;
    }
    while(search_ops) {
        Search(RBTree, rand() % 10000);
        search_ops--;
    }
}

void random_knuth_array(int N, int arr[]) {
    int in, im;

    im = 0;
    for (in = 0; in < N && im < N; ++in) {
        int rn = N - in;
        int rm = N - im;
        if (rand() % rn < rm) {
            arr[im++] = in + 1;
        }
    }
    // permutate
    for (int i = N-1; i >= 0; --i){
        //generate a random number [0, n-1]
        int j = rand() % (i+1);

        //swap the last element with element at random index
        int temp = arr[i];
        arr[i] = arr[j];
        arr[j] = temp;
    }
    assert(im == N);
}

int main(int argc, char* argv[]) {
    if (argc != 3) printf("Usage: <program> <array length> <operation ratio [0..1]");
    const int array_length = atoi(argv[1]);
    const float ratio = atof(argv[2]);
 
    int* array = (int *)malloc(sizeof(int) * array_length);
    random_knuth_array(array_length, array);

    RedBlackTree RBT = Initialize();
    for (int i = 0; i < sizeof(int) * array_length / sizeof(int); i++)
    {
        RBT = Add(RBT, array[i]);
    }

    int operations[7] = {100000, 500000, 1e6, 5e6, 10e6, 25e6, 50e6};
    for (int i = 0; i < 7; i++) {
        clock_t begin_reference = clock();
        UpdateStream(RBT, ratio, operations[i]);
        clock_t end_reference = clock();
        double time_spent_reference = (double)(end_reference - begin_reference) / CLOCKS_PER_SEC;
        printf("Operations %i, Operations per second %i\n", 
                operations[i], (int)(operations[i] / time_spent_reference));
    }

    return 0;
}

// Compile and run with clang ex02.c -O3 -o ex02.out && ./ex02.out 10 0.1
