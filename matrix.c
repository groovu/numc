#include "matrix.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

// Include SSE intrinsics
#if defined(_MSC_VER)
#include <intrin.h>
#elif defined(__GNUC__) && (defined(__x86_64__) || defined(__i386__))
#include <immintrin.h>
#include <x86intrin.h>
#endif

int ref = 1;
/* Below are some intel intrinsics that might be useful
 * void _mm256_storeu_pd (double * mem_addr, __m256d a)
 * __m256d _mm256_set1_pd (double a)
 * __m256d _mm256_set_pd (double e3, double e2, double e1, double e0)
 * __m256d _mm256_loadu_pd (double const * mem_addr)
 * __m256d _mm256_add_pd (__m256d a, __m256d b)
 * __m256d _mm256_sub_pd (__m256d a, __m256d b)
 * __m256d _mm256_fmadd_pd (__m256d a, __m256d b, __m256d c)
 * __m256d _mm256_mul_pd (__m256d a, __m256d b)
 * __m256d _mm256_cmp_pd (__m256d a, __m256d b, const int imm8)
 * __m256d _mm256_and_pd (__m256d a, __m256d b)
 * __m256d _mm256_max_pd (__m256d a, __m256d b)
*/
//testing 123

/*
 * Generates a random double between `low` and `high`.
 */
double rand_double(double low, double high) {
    double range = (high - low);
    double div = RAND_MAX / range;
    return low + (rand() / div);
}

/*
 * Generates a random matrix with `seed`.
 */
void rand_matrix(matrix *result, unsigned int seed, double low, double high) {
    srand(seed);
    for (int i = 0; i < result->rows; i++) {
        for (int j = 0; j < result->cols; j++) {
            set(result, i, j, rand_double(low, high));
        }
    }
}

/*
 * Allocate space for a matrix struct pointed to by the double pointer mat with
 * `rows` rows and `cols` columns. You should also allocate memory for the data array
 * and initialize all entries to be zeros. Remember to set all fieds of the matrix struct.
 * `parent` should be set to NULL to indicate that this matrix is not a slice.
 * You should return -1 if either `rows` or `cols` or both have invalid values, or if any
 * call to allocate memory in this function fails. If you don't set python error messages here upon
 * failure, then remember to set it in numc.c.
 * Return 0 upon success and non-zero upon failure.
 */
int allocate_matrix(matrix **mat, int rows, int cols) {
    //Dimension error check
    if (rows <= 0 || cols <= 0) {
        PyErr_SetString(PyExc_TypeError, "Invalid dims: Row/Col >= 0");
        return -1;
    }
    *mat = (matrix *) malloc(sizeof(matrix));
    if (NULL == *mat) {
        PyErr_SetString(PyExc_RuntimeError, "malloc in matrix.c failed");
        //FIXME free mat?
        return -1;
    }
    (*mat)->cols = cols;
    (*mat)->rows = rows;
    (*mat)->parent = NULL;
    (*mat)->ref_cnt = ref; //fresh matrix, no kids. 1 is needed for sanity tests. global var
    (*mat)->data = (double **) malloc(sizeof(double *) * rows);
    if ((*mat)->data == NULL) {
        PyErr_SetString(PyExc_RuntimeError, "malloc in matrix.c rowdata");
        return -1;
    }
    (*mat)->data[0] = calloc(rows * cols, sizeof(double));
    if ((*mat)->data[0] == NULL) {
        PyErr_SetString(PyExc_RuntimeError, "malloc in matrix.c rowdata");
        return -1;
    }


    if (NULL == (*mat)->data) {
        PyErr_SetString(PyExc_RuntimeError, "malloc in matrix.c failed for mat->data");
        return -1;
    }
    for (int i = 1; i < rows; i += 1) {
        //(*mat)->data[i] = (double *) malloc(sizeof(double) * cols);
        //double * rowdata = (double *) calloc(cols, sizeof(double));
        // if (NULL == rowdata) {
        //     PyErr_SetString(PyExc_RuntimeError, "malloc in matrix.c rowdata");
        //     return -1;
        // }
        (*mat)->data[i] = (*mat)->data[i - 1] + cols;
    }

    if (rows == 1 || cols == 1) {
        (*mat)->is_1d = 1; //nonzero == 1dim.
    } else {
        (*mat)->is_1d = 0; //0 == 2 dim.
    }
    return 0;


}

/*
 * Allocate space for a matrix struct pointed to by `mat` with `rows` rows and `cols` columns.
 * This is equivalent to setting the new matrix to be
 * from[row_offset:row_offset + rows, col_offset:col_offset + cols]
 * If you don't set python error messages here upon failure, then remember to set it in numc.c.
 * Return 0 upon success and non-zero upon failure.
 */
int allocate_matrix_ref(matrix **mat, matrix *from, int row_offset, int col_offset,
                        int rows, int cols) {
    if (rows <= 0 || cols <= 0) {
        PyErr_SetString(PyExc_TypeError, "Invalid dims: Row/Col >= 0");
        return -1;
    }

    int alloc_error = allocate_matrix(mat, rows, cols);
    if (alloc_error != 0) {
        PyErr_SetString(PyExc_RuntimeError, "allocate_matrix_ref failed to allocate_matrix");
        return -2;
    }
    (*mat)->parent = from;
    from->ref_cnt += 1; //Another matrix is reffing from, so +1 ref count.
    for (int r = 0; r < rows; r += 1) {
        (*mat)->data[r] = &(from->data[r+row_offset][col_offset]); //from->data[r][c]?
    }
    if (rows == 1 || cols == 1) {
        (*mat)->is_1d = 1; //nonzero == 1dim.
    } else {
        (*mat)->is_1d = 0; //0 == 2 dim.
    }
    return 0;
}
int allocate_matrix_ref2(matrix **mat, matrix *from, int row_offset, int col_offset,
                        int rows, int cols) {
    //dif here is youre making a matrix from a reference matrix.
    //
    if (rows <= 0 || cols <= 0) {
        PyErr_SetString(PyExc_TypeError, "Invalid dims: Row/Col >= 0");
        return -1;
    }

    int alloc_error = allocate_matrix(mat, rows, cols);
    if (alloc_error != 0) {
        PyErr_SetString(PyExc_MemoryError, "allocate_matrix_ref failed to allocate_matrix");
        return -2;
    }
    (*mat)->parent = from;
    from->ref_cnt += 1; //Another matrix is reffing from, so +1 ref count.
    for (int r = 0; r < rows; r ++) {
        for (int c = 0; c < cols; c ++) {
            //double val = 0;
            //clever way to make this into a single loop?
            (*mat)->data[r][c] = from->data[r+row_offset][c+col_offset];
            //(*mat)->data[r][c] = val;
        }
    }
    if (rows == 1 || cols == 1) {
        (*mat)->is_1d = 1; //nonzero == 1dim.
    } else {
        (*mat)->is_1d = 0; //0 == 2 dim.
    }
    return 0;
}

/*
 * This function will be called automatically by Python when a numc matrix loses all of its
 * reference pointers.
 * You need to make sure that you only free `mat->data` if no other existing matrices are also
 * referring this data array.
 * See the spec for more information.
 */
void deallocate_matrix(matrix *mat) {
    if (NULL == mat) {
        return;
    }
    int children = mat->ref_cnt;
    matrix* parents = mat->parent;
    if (parents == NULL) {
        if (children > ref) { //FIXME? 0 or 1?
            mat->ref_cnt -= 1;
        }
        else if (children <= ref) {
            free(mat->data);
            free(mat);
        }
    }
    if (parents != NULL) {
        if(parents->ref_cnt > ref) {
            parents->ref_cnt -= 1;
            free(mat);
        }
        else if (parents->ref_cnt <= ref) {
            deallocate_matrix(parents);
            free(mat);
        }
    }
    return;
}



/*
 * Return the double value of the matrix at the given row and column.
 * You may assume `row` and `col` are valid.
 */
double get(matrix *mat, int row, int col) {
    if (row > mat->rows || col > mat->cols || row < 0 || col < 0) {
        PyErr_SetString(PyExc_IndexError, "index out of range in get, matrix.c");
        return -1;
    }
    return mat->data[row][col];
}

/*
 * Set the value at the given row and column to val. You may assume `row` and
 * `col` are valid
 */
void set(matrix *mat, int row, int col, double val) {
    if (row > mat->rows || col > mat->cols || row < 0 || col < 0) {
        PyErr_SetString(PyExc_IndexError, "index out of range in set, matrix.c");
        return;
    }
    mat->data[row][col] = val;
    return;
}

/*
 * Set all entries in mat to val
 */
void fill_matrix(matrix *mat, double val) {
    for (int i = 0; i < mat->rows * mat->cols; i += 1) {
        mat->data[0][i] = val;
    }
}

/*
 * Store the result of adding mat1 and mat2 to `result`.
 * Return 0 upon success and a nonzero value upon failure.
 */
int add_matrix(matrix *result, matrix *mat1, matrix *mat2) {
    if (NULL == mat1 || NULL == mat2) {
        PyErr_SetString(PyExc_TypeError, "add_matrix: null input matrices");
        return -3;
    }
    if (mat1->rows != mat2->rows || mat1->cols != mat2->cols) {
        PyErr_SetString(PyExc_ValueError, "Mtrx add dimension mismatch error");
        return -2;
    }
    int total = mat1->rows * mat1->cols;
    if (total < 10001) {
        for (int i = 0; i < mat1->rows * mat1->cols; i += 1) {
            result->data[0][i] = mat1->data[0][i] + mat2->data[0][i];
        }
    } else {
        #pragma omp parallel for
        for (int i = 0; i < mat1->rows * mat1->cols; i += 1) {
            result->data[0][i] = mat1->data[0][i] + mat2->data[0][i];
        }
    }
    return 0;
}

/*
 * Store the result of subtracting mat2 from mat1 to `result`.
 * Return 0 upon success and a nonzero value upon failure.
 */
int sub_matrix(matrix *result, matrix *mat1, matrix *mat2) {
    /* TODO: YOUR CODE HERE */
    if (NULL == mat1 || NULL == mat2) {
        PyErr_SetString(PyExc_TypeError, "sub_matrix: null input matrices");
        return -5;
    }
    if (mat1->rows != mat2->rows || mat1->cols != mat2->cols) {
        PyErr_SetString(PyExc_ValueError, "Mtrx sub dimension mismatch error");
        return -4;
    }//null check
    int total = mat1->rows * mat1->cols;
    if (total < 10001) {
        for (int i = 0; i < total; i += 1) {
            result->data[0][i] = mat1->data[0][i] - mat2->data[0][i];
        }
    } else {
        #pragma omp parallel for
        for (int i = 0; i < total; i += 1) {
            result->data[0][i] = mat1->data[0][i] - mat2->data[0][i];
        }
    }
    return 0;
}


/*
 * Store the result of multiplying mat1 and mat2 to `result`.
 * Return 0 upon success and a nonzero value upon failure.
 * Remember that matrix multiplication is not the same as multiplying individual elements.
 */
int mul_matrix(matrix *result, matrix *mat1, matrix *mat2) {
    /* TODO: YOUR CODE HERE */
    if (NULL == mat1 || NULL == mat2) {
        PyErr_SetString(PyExc_TypeError, "mul_matrix: null input matrices");
        return -7;
    }
    if (mat1->cols != mat2->rows) {
        PyErr_SetString(PyExc_ValueError, "Mtrx mul dimension mismatch error");
        return -6;
    }
    //#pragma omp parallel
    // for (int r = 0; r < mat1->rows; r++) { //can we assume dims are good?
    //     for (int c = 0; c < mat2->cols; c++) {
    //         double sum = 0;
    //         for (int i = 0; i < mat1->cols; i++) {
    //             sum += mat1->data[r][i] * mat2->data[i][c];
    //         }
    //         result->data[r][c] = sum;
    //     }
    // }

    //ikj pure row
    fill_matrix(result, 0);
    int total = mat1-> rows * mat1->cols;
    if (total < 10001) {
        for (int i = 0; i < mat1->rows; i ++) {
            double * mat1Row = mat1->data[i];
            double * resRow = result->data[i];
            for (int k = 0; k < mat1->cols; k ++) {
                double * mat2Row = mat2->data[k];
                double mat1Val = mat1Row[k];
                for (int j = 0; j < mat2->cols; j ++) {
                    resRow[j] += mat1Val * mat2Row[j];
                }
            }
        }
    } else {
        #pragma omp parallel for
        for (int i = 0; i < mat1->rows; i ++) {
            double * mat1Row = mat1->data[i];
            double * resRow = result->data[i];
            for (int k = 0; k < mat1->cols; k ++) {
                double * mat2Row = mat2->data[k];
                double mat1Val = mat1Row[k];
                for (int j = 0; j < mat2->cols; j ++) {
                    resRow[j] += mat1Val * mat2Row[j];
                }
            }
        }
    }
    return 0;

}

/*
 * Store the result of raising mat to the (pow)th power to `result`.
 * Return 0 upon success and a nonzero value upon failure.
 * Remember that pow is defined with matrix multiplication, not element-wise multiplication.
 */
int pow_matrix(matrix *result, matrix *mat, int pow) {
    /* TODO: YOUR CODE HERE */
    if (NULL == mat || NULL == result) {
        PyErr_SetString(PyExc_TypeError, "pow_matrix null input");
        return -8;
    }
    if (mat->rows != mat->cols || result->rows != mat->cols || pow < 0) {
        PyErr_SetString(PyExc_ValueError, "pow_matrix not square or power is negative");
        return -9;
    }
    if (pow == 0) {
        fill_matrix(result, 0);
        for (int i = 0; i < mat->cols; i += 1) {
            result->data[i][i] = 1;
        }
        return 0;
    } else if (pow == 1) {
        //fill_matrix(result, 0);
        matrix * middle = NULL;
        allocate_matrix(&middle, mat->rows, mat->cols);
        for (int i = 0; i < mat->cols; i += 1) {
            middle->data[i][i] = 1;
        }
        mul_matrix(result, middle, mat);
        deallocate_matrix(middle);
        return 0;
    }
    mul_matrix(result, mat, mat); //pow 2.
    matrix * middle = NULL;
    //FIXME error check this too

    //I could just use ref2 once, then reuse old ref.  ref2 is needed to deep copy a working matrix.
    //allocate_matrix_ref2(&middle, result, 0, 0, result->rows, result->cols);
    //#pragma omp parallel for
    for (int i = 2; i < pow; i ++) {
        allocate_matrix_ref2(&middle, result, 0, 0, result->rows, result->cols);
        mul_matrix(result, middle, mat);
        deallocate_matrix(middle);
    }
    return 0;
}


int pow_matrix2(matrix *result, matrix *mat, int pow) {
    if (NULL == mat || NULL == result) {
        PyErr_SetString(PyExc_TypeError, "pow_matrix null input");
        return -8;
    }
    if (mat->rows != mat->cols || result->rows != mat->cols || pow < 0) {
        PyErr_SetString(PyExc_ValueError, "pow_matrix not square or power < 0");
        return -9;
    }
    
    if (pow == 0) { //return identity.
        fill_matrix(result, 0);
        for (int i = 0; i < mat->cols; i += 1) {
            result->data[i][i] = 1;
        }
        return 0;
    }
    if (pow == 1) { //place mat into res.
        allocate_matrix_ref(&result, mat, 0, 0, mat->rows, mat->cols);
        return 0;
    }
    if (pow > 1) {

    }
}
/*
 * Store the result of element-wise negating mat's entries to `result`.
 * Return 0 upon success and a nonzero value upon failure.
 */
int neg_matrix(matrix *result, matrix *mat) {
    /* TODO: YOUR CODE HERE */
    //FIXME add error checking.
    if (NULL == result || NULL == mat) {
        PyErr_SetString(PyExc_TypeError, "neg_matrix: null matrices");
        return -11;
    }
    // for (int r = 0; r < mat->rows; r++) { //can we assume dims are good?
    //     for (int c = 0; c < mat->cols; c++) {
    //         result->data[r][c] = mat->data[r][c] * -1;
    //     }
    // }
    int total = mat->rows * mat->cols;
    int count = 16;
    int unroll = total / count * count;
    if (total < 10001) {//without omp
        for (int i = 0; i < unroll; i += count) {
            result->data[0][i] = -mat->data[0][i];
            result->data[0][i+1] = -mat->data[0][i+1];
            result->data[0][i+2] = -mat->data[0][i+2];
            result->data[0][i+3] = -mat->data[0][i+3];
            result->data[0][i+4] = -mat->data[0][i+4];
            result->data[0][i+5] = -mat->data[0][i+5];
            result->data[0][i+6] = -mat->data[0][i+6];
            result->data[0][i+7] = -mat->data[0][i+7];
            result->data[0][i+8 ] = -mat->data[0][i+8 ];
            result->data[0][i+9 ] = -mat->data[0][i+9 ];
            result->data[0][i+10] = -mat->data[0][i+10];
            result->data[0][i+11] = -mat->data[0][i+11];
            result->data[0][i+12] = -mat->data[0][i+12];
            result->data[0][i+13] = -mat->data[0][i+13];
            result->data[0][i+14] = -mat->data[0][i+14];
            result->data[0][i+15] = -mat->data[0][i+15];
            //unrolling doesn't seem to increase performance that much
        }
        for (int i = unroll; i < total; i += 1) {
            result->data[0][i] = -mat->data[0][i];
        }
    } else {
    #pragma omp parallel for
        for (int i = 0; i < unroll; i += count) {
            result->data[0][i] = -mat->data[0][i];
            result->data[0][i+1] = -mat->data[0][i+1];
            result->data[0][i+2] = -mat->data[0][i+2];
            result->data[0][i+3] = -mat->data[0][i+3];
            result->data[0][i+4] = -mat->data[0][i+4];
            result->data[0][i+5] = -mat->data[0][i+5];
            result->data[0][i+6] = -mat->data[0][i+6];
            result->data[0][i+7] = -mat->data[0][i+7];
            result->data[0][i+8 ] = -mat->data[0][i+8 ];
            result->data[0][i+9 ] = -mat->data[0][i+9 ];
            result->data[0][i+10] = -mat->data[0][i+10];
            result->data[0][i+11] = -mat->data[0][i+11];
            result->data[0][i+12] = -mat->data[0][i+12];
            result->data[0][i+13] = -mat->data[0][i+13];
            result->data[0][i+14] = -mat->data[0][i+14];
            result->data[0][i+15] = -mat->data[0][i+15];
            //unrolling doesn't seem to increase performance that much
        }
        for (int i = unroll; i < total; i += 1) {
            result->data[0][i] = -mat->data[0][i];
        }
    }
    return 0;
}

/*
 * Store the result of taking the absolute value element-wise to `result`.
 * Return 0 upon success and a nonzero value upon failure.
 */
int abs_matrix(matrix *result, matrix *mat) {
    /* TODO: YOUR CODE HERE */
    //FIXME add error checking.
    if (NULL == result || NULL == mat) {
        PyErr_SetString(PyExc_TypeError, "abs_matrix: null matrices");
        return -12;
    }
    int total = mat->cols * mat->rows;
    if (total < 10001) {
        for (int r = 0; r < mat->rows; r++) { //can we assume dims are good?
            for (int c = 0; c < mat->cols; c++) {
                double matval = mat->data[r][c];
                if (matval < 0) {
                    matval *= -1;
                }
                result->data[r][c] = matval;
            }
        }
    } else {
    #pragma omp parallel for
        for (int r = 0; r < mat->rows; r++) { //can we assume dims are good?
            for (int c = 0; c < mat->cols; c++) {
                double matval = mat->data[r][c];
                if (matval < 0) {
                    matval *= -1;
                }
                result->data[r][c] = matval;
            }
        }
    }
    return 0;
}


