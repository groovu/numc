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

int ref = 0;
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
        PyErr_SetString(PyExc_MemoryError, "malloc in matrix.c failed");
        return -1;
    }
    (*mat)->cols = cols;
    (*mat)->rows = rows;
    (*mat)->parent = NULL;
    //global ref var.  1 passes make tests, 0 does not.
    (*mat)->ref_cnt = ref; //fresh matrix, no kids. 1 is needed for sanity tests.
    //what about the rest of the struct?  data, is_1d, ref_cnt?
    //(*mat)->data = (double **) malloc(sizeof(double) * cols * rows);

    (*mat)->data = (double **) malloc(sizeof(double *) * rows);
    if (NULL == (*mat)->data) {
        PyErr_SetString(PyExc_MemoryError, "malloc in matrix.c failed for mat->data");
        return -1;
    }

    for (int i = 0; i < rows; i += 1) {
        //(*mat)->data[i] = (double *) malloc(sizeof(double) * cols);
        double * rowdata = (double *) malloc(sizeof(double) * cols);
        if (NULL == rowdata) {
            PyErr_SetString(PyExc_MemoryError, "malloc in matrix.c rowdata");
            return -1;
        }
        (*mat)->data[i] = rowdata;
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
    //rows, cols, ref count taken care of by allocate_matrix
    (*mat)->parent = from;
    //from matrix update
    from->ref_cnt += 1; //Another matrix is reffing from, so +1 ref count.

    //data allocate, you can use arr[x][y];
    //for x
    //for y
    //mat[x][y] = from[x+offset][y+offset]?
//    (*mat)->data[0][0] = from->data[0][0]; 
    //(*mat)->data[0][0] = 0;
    //double test = from->data[0][0];
    //from->data[0][0] = 0;
    //printf("\n%d", );


    for (int r = 0; r < rows; r ++) {
        for (int c = 0; c < cols; c ++) {
            double val = 0;
            val = from->data[r+row_offset][c+col_offset];
            //val = 5.5;
            (*mat)->data[r][c] = val;
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
    // only free mat.data if no other matrices are referencing mat.  use a counter on mat?
    //i have no parent, and my ref == 1 (myself), once I lose my ref ptr, delete self.
    //i have a parent, 

    //matrix loses all ref's in python.
    //how do the parents play a role?
    if (NULL == mat) {
        return;
    }
    int children = mat->ref_cnt;
    matrix* parents = mat->parent;

    // if (children == 0 && parents == NULL) {
    //     free(mat->data);
    //     free(mat);
    // }
    // else if (children > 0 && parents == NULL) {

    // }
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

    //matrix* test = malloc(sizeof(matrix));
    // is this enough?  free(mat.data) frees ptrs to row data, do I need to free the (mat.data[i])?
    //ya valgrind says I alloc as much as I free, but testing shows adding the above line still yields the same results.
    return;
    //idk why this shit is crashing.  try filling in the other fxns first.
}
    //need a main dealloc to release mat, then a recursive loop on parents to update their ref_cnt.



/*
 * Return the double value of the matrix at the given row and column.
 * You may assume `row` and `col` are valid.
 */
double get(matrix *mat, int row, int col) {
    /* TODO: YOUR CODE HERE */
    //int index = (mat->cols) * row + col;
    //double * row_data = mat->data;
    //double val = row_data[index];
    ////double val = mat->data[row][col];
    //return val;
    return mat->data[row][col];
}

/*
 * Set the value at the given row and column to val. You may assume `row` and
 * `col` are valid
 */
void set(matrix *mat, int row, int col, double val) {
    /* TODO: YOUR CODE HERE */
    // int index = (mat->cols) * row + col;
    // double * row_data = mat->data;
    // row_data[index] = val;
    //*(mat->data[index]) = val;
    mat->data[row][col] = val;

}

/*
 * Set all entries in mat to val
 */
void fill_matrix(matrix *mat, double val) {
    /* TODO: YOUR CODE HERE */
    // int size = mat->rows * mat->cols;
    // for (int i = 0; i < size; i ++) {
    //     *(mat->data[i]) = val;
    // }
    for (int r = 0; r < mat->rows; r++) {
        for (int c = 0; c < mat->cols; c++) {
            mat->data[r][c] = val;
        }
    }
}

/*
 * Store the result of adding mat1 and mat2 to `result`.
 * Return 0 upon success and a nonzero value upon failure.
 */
int add_matrix(matrix *result, matrix *mat1, matrix *mat2) {
    //Error check
    //dim check
    //if (mat1->rows != mat2->cols || mat1->cols != mat2->rows) {
    if (NULL == mat1 || NULL == mat2) {
        PyErr_SetString(PyExc_TypeError, "add_matrix: null input matrices");
        return -3;
    }
    if (mat1->rows != mat2->rows || mat1->cols != mat2->cols) {
        PyErr_SetString(PyExc_TypeError, "Mtrx add dimension mismatch error");
        return -2;
    }//null check


    for (int r = 0; r < result->rows; r++) {
        for (int c = 0; c < result->cols; c++) {
            result->data[r][c] = mat1->data[r][c] + mat2->data[r][c];
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
        PyErr_SetString(PyExc_TypeError, "Mtrx sub dimension mismatch error");
        return -4;
    }//null check


    for (int r = 0; r < result->rows; r++) {
        for (int c = 0; c < result->cols; c++) {
            result->data[r][c] = mat1->data[r][c] - mat2->data[r][c];
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
        PyErr_SetString(PyExc_TypeError, "Mtrx mul dimension mismatch error");
        return -6;
    }
    for (int r = 0; r < mat1->rows; r++) { //can we assume dims are good?
        for (int c = 0; c < mat2->cols; c++) {
            int sum = 0;
            for (int i = 0; i < mat1->cols; i++) {
                sum += mat1->data[r][i] * mat2->data[i][c];
            }
            result->data[r][c] = sum;
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
    if (mat->rows != mat->cols || result->rows != mat->cols) {
        PyErr_SetString(PyExc_TypeError, "pow_matrix not square");
        return -9;
    }
    if (pow < 1) {
        PyErr_SetString(PyExc_TypeError, "pow_matrix negative power");
        return -10;
    } else if (pow == 0) {
        //return identitity?
    } else if (pow == 1) {
        return mat;
    }
    //error check?  make sure its square?
    //matrix * middle = result; //copies ?
    //allocate_matrix(&middle, mat->rows, mat->cols);
    //mul_matrix(middle, mat, mat);
    mul_matrix(result, mat, mat);
    for (int i = 2; i < pow; i++) {
        mul_matrix(result, result, mat);
        //middle = result;
    }
    return 0;
}

/*
 * Store the result of element-wise negating mat's entries to `result`.
 * Return 0 upon success and a nonzero value upon failure.
 */
int neg_matrix(matrix *result, matrix *mat) {
    /* TODO: YOUR CODE HERE */
    //FIXME add error checking.
    for (int r = 0; r < mat->rows; r++) { //can we assume dims are good?
        for (int c = 0; c < mat->cols; c++) {
            result->data[r][c] = mat->data[r][c] * -1;
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


