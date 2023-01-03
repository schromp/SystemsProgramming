/*
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */
#include "cachelab.h"
#include <stdio.h>

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/*
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded.
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N]) {
  if (M == 32 && N == 32) {

    int row, col;
    int blocksize_row = 8;
    int blocksize_col = 8;

    for (row = 0; row < N; row += blocksize_row) {
      for (col = 0; col < M; col += blocksize_col) {

        for (int blockrow = row; blockrow < row + blocksize_row; blockrow++) {
          for (int blockcol = col; blockcol < col + blocksize_col; blockcol++) {

            // deffering the diagonal until after line is done
            if (blockrow == blockcol) {
              continue;
            } else {
              B[blockcol][blockrow] = A[blockrow][blockcol];
            }
          }
          if (row == col) {
            B[blockrow][blockrow] = A[blockrow][blockrow];
          }
        }
      }
    }
  } else if (M == 64 && N == 64) {

    int row, col;
    int blocksize_row = 4;
    int blocksize_col = 4;

    for (row = 0; row < N; row += blocksize_row) {
      for (col = 0; col < M; col += blocksize_col) {

        for (int blockrow = row; blockrow < row + blocksize_row; blockrow++) {
          for (int blockcol = col; blockcol < col + blocksize_col; blockcol++) {

            // deffering the diagonal until after line is done
            if (blockrow == blockcol) {
              continue;
            } else {
              B[blockcol][blockrow] = A[blockrow][blockcol];
            }
          }
          if (row == col) {
            B[blockrow][blockrow] = A[blockrow][blockrow];
          }
        }
      }
    }
  } else {

    int tmp = 0;
    int tmp_pos = 0;
    int blocksize_row = 16;
    int blocksize_col = 16;

    for (int row = 0; row < N; row += blocksize_row) {
      for (int col = 0; col < M; col += blocksize_col) {

        for (int row_block = row;
             (row_block < row + blocksize_row) && (row_block < N);
             row_block++) {
          for (int col_block = col;
               (col_block < col + blocksize_col) && (col_block < M);
               col_block++) {

            if (row_block != col_block) {
              B[col_block][row_block] = A[row_block][col_block];
            } else {
              tmp = A[row_block][col_block];
              tmp_pos = row_block;
            }
            if (row_block == col_block) {
              B[tmp_pos][tmp_pos] = tmp;
            }
          }
        }
      }
    }
  }
}

/*
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started.
 */

/*
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N]) {
  int i, j, tmp;

  for (i = 0; i < N; i++) {
    for (j = 0; j < M; j++) {
      tmp = A[i][j];
      B[j][i] = tmp;
    }
  }
}

char trans_32_desc[] = "perfectly working 32 by 32 transpose";
void trans_32(int M, int N, int A[N][M], int B[M][N]) {

  int row, col;
  int blocksize_row = 8;
  int blocksize_col = 8;

  for (row = 0; row < N; row += blocksize_row) {
    for (col = 0; col < M; col += blocksize_col) {

      for (int blockrow = row; blockrow < row + blocksize_row; blockrow++) {
        for (int blockcol = col; blockcol < col + blocksize_col; blockcol++) {

          // deffering the diagonal until after line is done
          if (blockrow == blockcol) {
            continue;
          } else {
            B[blockcol][blockrow] = A[blockrow][blockcol];
          }
        }
        if (row == col) {
          B[blockrow][blockrow] = A[blockrow][blockrow];
        }
      }
    }
  }
}

char trans_64_desc[] = "first try on a 64 line transpose";
void trans_64(int M, int N, int A[N][M], int B[M][N]) {

  int row, col;
  int blocksize_row = 4;
  int blocksize_col = 4;

  for (row = 0; row < N; row += blocksize_row) {
    for (col = 0; col < M; col += blocksize_col) {

      for (int blockrow = row; blockrow < row + blocksize_row; blockrow++) {
        for (int blockcol = col; blockcol < col + blocksize_col; blockcol++) {

          // deffering the diagonal until after line is done
          if (blockrow == blockcol) {
            continue;
          } else {
            B[blockcol][blockrow] = A[blockrow][blockcol];
          }
        }
        if (row == col) {
          B[blockrow][blockrow] = A[blockrow][blockrow];
        }
      }
    }
  }
}

char trans_as_desc[] = "first try on a asymetric line transpose";
void trans_as(int M, int N, int A[N][M], int B[M][N]) {

  int tmp = 0;
  int tmp_pos = 0;
  int blocksize_row = 16;
  int blocksize_col = 16;

  for (int row = 0; row < N; row += blocksize_row) {
    for (int col = 0; col < M; col += blocksize_col) {

      for (int row_block = row;
           (row_block < row + blocksize_row) && (row_block < N); row_block++) {
        for (int col_block = col;
             (col_block < col + blocksize_col) && (col_block < M);
             col_block++) {

          if (row_block != col_block) {
            B[col_block][row_block] = A[row_block][col_block];
          } else {
            tmp = A[row_block][col_block];
            tmp_pos = row_block;
          }
          if (row_block == col_block) {
            B[tmp_pos][tmp_pos] = tmp;
          }
        }
      }
    }
  }
}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions() {
  /* Register your solution function */
  registerTransFunction(transpose_submit, transpose_submit_desc);

  /* Register any additional transpose functions */
  // registerTransFunction(trans, trans_desc);
  // registerTransFunction(trans_32, trans_32_desc);
  // registerTransFunction(trans_64, trans_64_desc);
  registerTransFunction(trans_as, trans_as_desc);
}

/*
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N]) {
  int i, j;

  for (i = 0; i < N; i++) {
    for (j = 0; j < M; ++j) {
      if (A[i][j] != B[j][i]) {
        return 0;
      }
    }
  }
  return 1;
}
