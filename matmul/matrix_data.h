#ifndef MATRIX_DATA_H
#define MATRIX_DATA_H

#define MATRIX_M 8
#define MATRIX_N 8
#define MATRIX_K 8

// Static input matrix A
const float matrix_A[MATRIX_M][MATRIX_K] = {
    {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f},
    {8.0f, 7.0f, 6.0f, 5.0f, 4.0f, 3.0f, 2.0f, 1.0f},
    {1.5f, 2.5f, 3.5f, 4.5f, 5.5f, 6.5f, 7.5f, 8.5f},
    {8.5f, 7.5f, 6.5f, 5.5f, 4.5f, 3.5f, 2.5f, 1.5f},
    {2.0f, 0.0f, 2.0f, 0.0f, 2.0f, 0.0f, 2.0f, 0.0f},
    {0.0f, 2.0f, 0.0f, 2.0f, 0.0f, 2.0f, 0.0f, 2.0f},
    {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f},
    {2.0f, 2.0f, 2.0f, 2.0f, 2.0f, 2.0f, 2.0f, 2.0f}
};

// Static input matrix B
const float matrix_B[MATRIX_K][MATRIX_N] = {
    {1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f},
    {0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f},
    {0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f},
    {1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f},
    {2.0f, 1.0f, 0.0f, 2.0f, 1.0f, 0.0f, 2.0f, 1.0f},
    {0.0f, 2.0f, 1.0f, 0.0f, 2.0f, 1.0f, 0.0f, 2.0f},
    {1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f},
    {0.0f, 0.0f, 2.0f, 0.0f, 0.0f, 2.0f, 0.0f, 0.0f}
};

// Output matrix initialized in writable memory
float matrix_C[MATRIX_M][MATRIX_N];

#endif
