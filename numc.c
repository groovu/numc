#include "numc.h"
#include <structmember.h>

PyTypeObject Matrix61cType;

/* Helper functions for initalization of matrices and vectors */

/*
 * Return a tuple given rows and cols
 */
PyObject *get_shape(int rows, int cols) {
  if (rows == 1 || cols == 1) {
    return PyTuple_Pack(1, PyLong_FromLong(rows * cols));
  } else {
    return PyTuple_Pack(2, PyLong_FromLong(rows), PyLong_FromLong(cols));
  }
}
/*
 * Matrix(rows, cols, low, high). Fill a matrix random double values
 */
int init_rand(PyObject *self, int rows, int cols, unsigned int seed, double low,
              double high) {
    matrix *new_mat;
    int alloc_failed = allocate_matrix(&new_mat, rows, cols);
    if (alloc_failed) return alloc_failed;
    rand_matrix(new_mat, seed, low, high);
    ((Matrix61c *)self)->mat = new_mat;
    ((Matrix61c *)self)->shape = get_shape(new_mat->rows, new_mat->cols);
    return 0;
}

/*
 * Matrix(rows, cols, val). Fill a matrix of dimension rows * cols with val
 */
int init_fill(PyObject *self, int rows, int cols, double val) {
    matrix *new_mat;
    int alloc_failed = allocate_matrix(&new_mat, rows, cols);
    if (alloc_failed)
        return alloc_failed;
    else {
        fill_matrix(new_mat, val);
        ((Matrix61c *)self)->mat = new_mat;
        ((Matrix61c *)self)->shape = get_shape(new_mat->rows, new_mat->cols);
    }
    return 0;
}

/*
 * Matrix(rows, cols, 1d_list). Fill a matrix with dimension rows * cols with 1d_list values
 */
int init_1d(PyObject *self, int rows, int cols, PyObject *lst) {
    if (rows * cols != PyList_Size(lst)) {
        PyErr_SetString(PyExc_ValueError, "Incorrect number of elements in list");
        return -1;
    }
    matrix *new_mat;
    int alloc_failed = allocate_matrix(&new_mat, rows, cols);
    if (alloc_failed) return alloc_failed;
    int count = 0;
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            set(new_mat, i, j, PyFloat_AsDouble(PyList_GetItem(lst, count)));
            count++;
        }
    }
    ((Matrix61c *)self)->mat = new_mat;
    ((Matrix61c *)self)->shape = get_shape(new_mat->rows, new_mat->cols);
    return 0;
}

/*
 * Matrix(2d_list). Fill a matrix with dimension len(2d_list) * len(2d_list[0])
 */
int init_2d(PyObject *self, PyObject *lst) {
    int rows = PyList_Size(lst);
    if (rows == 0) {
        PyErr_SetString(PyExc_ValueError,
                        "Cannot initialize numc.Matrix with an empty list");
        return -1;
    }
    int cols;
    if (!PyList_Check(PyList_GetItem(lst, 0))) {
        PyErr_SetString(PyExc_ValueError, "List values not valid");
        return -1;
    } else {
        cols = PyList_Size(PyList_GetItem(lst, 0));
    }
    for (int i = 0; i < rows; i++) {
        if (!PyList_Check(PyList_GetItem(lst, i)) ||
                PyList_Size(PyList_GetItem(lst, i)) != cols) {
            PyErr_SetString(PyExc_ValueError, "List values not valid");
            return -1;
        }
    }
    matrix *new_mat;
    int alloc_failed = allocate_matrix(&new_mat, rows, cols);
    if (alloc_failed) return alloc_failed;
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            set(new_mat, i, j,
                PyFloat_AsDouble(PyList_GetItem(PyList_GetItem(lst, i), j)));
        }
    }
    ((Matrix61c *)self)->mat = new_mat;
    ((Matrix61c *)self)->shape = get_shape(new_mat->rows, new_mat->cols);
    return 0;
}

/*
 * This deallocation function is called when reference count is 0
 */
void Matrix61c_dealloc(Matrix61c *self) {
    deallocate_matrix(self->mat);
    Py_TYPE(self)->tp_free(self);
}

/* For immutable types all initializations should take place in tp_new */
PyObject *Matrix61c_new(PyTypeObject *type, PyObject *args,
                        PyObject *kwds) {
    /* size of allocated memory is tp_basicsize + nitems*tp_itemsize*/
    Matrix61c *self = (Matrix61c *)type->tp_alloc(type, 0);
    return (PyObject *)self;
}

/*
 * This matrix61c type is mutable, so needs init function. Return 0 on success otherwise -1
 */
int Matrix61c_init(PyObject *self, PyObject *args, PyObject *kwds) {
    /* Generate random matrices */
    if (kwds != NULL) {
        PyObject *rand = PyDict_GetItemString(kwds, "rand");
        if (!rand) {
            PyErr_SetString(PyExc_TypeError, "Invalid arguments");
            return -1;
        }
        if (!PyBool_Check(rand)) {
            PyErr_SetString(PyExc_TypeError, "Invalid arguments");
            return -1;
        }
        if (rand != Py_True) {
            PyErr_SetString(PyExc_TypeError, "Invalid arguments");
            return -1;
        }

        PyObject *low = PyDict_GetItemString(kwds, "low");
        PyObject *high = PyDict_GetItemString(kwds, "high");
        PyObject *seed = PyDict_GetItemString(kwds, "seed");
        double double_low = 0;
        double double_high = 1;
        unsigned int unsigned_seed = 0;

        if (low) {
            if (PyFloat_Check(low)) {
                double_low = PyFloat_AsDouble(low);
            } else if (PyLong_Check(low)) {
                double_low = PyLong_AsLong(low);
            }
        }

        if (high) {
            if (PyFloat_Check(high)) {
                double_high = PyFloat_AsDouble(high);
            } else if (PyLong_Check(high)) {
                double_high = PyLong_AsLong(high);
            }
        }

        if (double_low >= double_high) {
            PyErr_SetString(PyExc_TypeError, "Invalid arguments");
            return -1;
        }

        // Set seed if argument exists
        if (seed) {
            if (PyLong_Check(seed)) {
                unsigned_seed = PyLong_AsUnsignedLong(seed);
            }
        }

        PyObject *rows = NULL;
        PyObject *cols = NULL;
        if (PyArg_UnpackTuple(args, "args", 2, 2, &rows, &cols)) {
            if (rows && cols && PyLong_Check(rows) && PyLong_Check(cols)) {
                return init_rand(self, PyLong_AsLong(rows), PyLong_AsLong(cols), unsigned_seed, double_low,
                                 double_high);
            }
        } else {
            PyErr_SetString(PyExc_TypeError, "Invalid arguments");
            return -1;
        }
    }
    PyObject *arg1 = NULL;
    PyObject *arg2 = NULL;
    PyObject *arg3 = NULL;
    if (PyArg_UnpackTuple(args, "args", 1, 3, &arg1, &arg2, &arg3)) {
        /* arguments are (rows, cols, val) */
        if (arg1 && arg2 && arg3 && PyLong_Check(arg1) && PyLong_Check(arg2) && (PyLong_Check(arg3)
                || PyFloat_Check(arg3))) {
            if (PyLong_Check(arg3)) {
                return init_fill(self, PyLong_AsLong(arg1), PyLong_AsLong(arg2), PyLong_AsLong(arg3));
            } else
                return init_fill(self, PyLong_AsLong(arg1), PyLong_AsLong(arg2), PyFloat_AsDouble(arg3));
        } else if (arg1 && arg2 && arg3 && PyLong_Check(arg1) && PyLong_Check(arg2) && PyList_Check(arg3)) {
            /* Matrix(rows, cols, 1D list) */
            return init_1d(self, PyLong_AsLong(arg1), PyLong_AsLong(arg2), arg3);
        } else if (arg1 && PyList_Check(arg1) && arg2 == NULL && arg3 == NULL) {
            /* Matrix(rows, cols, 1D list) */
            return init_2d(self, arg1);
        } else if (arg1 && arg2 && PyLong_Check(arg1) && PyLong_Check(arg2) && arg3 == NULL) {
            /* Matrix(rows, cols, 1D list) */
            return init_fill(self, PyLong_AsLong(arg1), PyLong_AsLong(arg2), 0);
        } else {
            PyErr_SetString(PyExc_TypeError, "Invalid arguments");
            return -1;
        }
    } else {
        PyErr_SetString(PyExc_TypeError, "Invalid arguments");
        return -1;
    }
}

/*
 * List of lists representations for matrices
 */
PyObject *Matrix61c_to_list(Matrix61c *self) {
    int rows = self->mat->rows;
    int cols = self->mat->cols;
    PyObject *py_lst = NULL;
    if (self->mat->is_1d) {  // If 1D matrix, print as a single list
        py_lst = PyList_New(rows * cols);
        int count = 0;
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                PyList_SetItem(py_lst, count, PyFloat_FromDouble(get(self->mat, i, j)));
                count++;
            }
        }
    } else {  // if 2D, print as nested list
        py_lst = PyList_New(rows);
        for (int i = 0; i < rows; i++) {
            PyList_SetItem(py_lst, i, PyList_New(cols));
            PyObject *curr_row = PyList_GetItem(py_lst, i);
            for (int j = 0; j < cols; j++) {
                PyList_SetItem(curr_row, j, PyFloat_FromDouble(get(self->mat, i, j)));
            }
        }
    }
    return py_lst;
}

PyObject *Matrix61c_class_to_list(Matrix61c *self, PyObject *args) {
    PyObject *mat = NULL;
    if (PyArg_UnpackTuple(args, "args", 1, 1, &mat)) {
        if (!PyObject_TypeCheck(mat, &Matrix61cType)) {
            PyErr_SetString(PyExc_TypeError, "Argument must of type numc.Matrix!");
            return NULL;
        }
        Matrix61c* mat61c = (Matrix61c*)mat;
        return Matrix61c_to_list(mat61c);
    } else {
        PyErr_SetString(PyExc_TypeError, "Invalid arguments");
        return NULL;
    }
}

/*
 * Add class methods
 */
PyMethodDef Matrix61c_class_methods[] = {
    {"to_list", (PyCFunction)Matrix61c_class_to_list, METH_VARARGS, "Returns a list representation of numc.Matrix"},
    {NULL, NULL, 0, NULL}
};

/*
 * Matrix61c string representation. For printing purposes.
 */
PyObject *Matrix61c_repr(PyObject *self) {
    PyObject *py_lst = Matrix61c_to_list((Matrix61c *)self);
    return PyObject_Repr(py_lst);
}

/* NUMBER METHODS */
//errors:
//typeerror if invalid dims
//runtime error if memory allocs fail.

/*
 * Add the second numc.Matrix (Matrix61c) object to the first one. The first operand is
 * self, and the second operand can be obtained by casting `args`.
 */
PyObject *Matrix61c_add(Matrix61c* self, PyObject* args) {
    // add self to args and return.
    //check if args is Matrixtype?
    if(!(PyObject_TypeCheck(args, &Matrix61cType)) || !(PyObject_TypeCheck(self, &Matrix61cType))) {
        PyErr_SetString(PyExc_TypeError, "a or b are not numc.Matrix type");
        return NULL;    
    }
    Matrix61c *mat = (Matrix61c *) args;

    Matrix61c *rv = (Matrix61c *) Matrix61c_new(&Matrix61cType, NULL, NULL);
    //rv->mat = new_mat?
    int row = self->mat->rows;
    int col = self->mat->cols;
    int rvcode = allocate_matrix(&rv->mat, row, col);
    //do I need to error check again?
    if (rvcode == -1) {
        Matrix61c_dealloc(rv);
        //from matrix, PyErr_SetString(Py)
        return NULL;
    }
    rv->shape = get_shape(row, col);
    int code = add_matrix(rv->mat, self->mat, mat->mat);
    if (code != 0) {
        Matrix61c_dealloc(rv);
        return NULL;
    }
    return (PyObject *) rv;
}

/*
 * Substract the second numc.Matrix (Matrix61c) object from the first one. The first operand is
 * self, and the second operand can be obtained by casting `args`.
 */
PyObject *Matrix61c_sub(Matrix61c* self, PyObject* args) {
    if(!(PyObject_TypeCheck(args, &Matrix61cType)) || !(PyObject_TypeCheck(self, &Matrix61cType))) {
        PyErr_SetString(PyExc_TypeError, "a or b are not numc.Matrix type");
        return NULL;    
    }
    Matrix61c *mat = (Matrix61c *) args;

    Matrix61c *rv = (Matrix61c *) Matrix61c_new(&Matrix61cType, NULL, NULL);
    //rv->mat = new_mat?
    int row = self->mat->rows;
    int col = self->mat->cols;
    int rvcode = allocate_matrix(&rv->mat, row, col);
    //do I need to error check again?
    if (rvcode == -1) {
        Matrix61c_dealloc(rv);
        //from matrix, PyErr_SetString(Py)
        return NULL;
    }
    rv->shape = get_shape(row, col);
    int code = sub_matrix(rv->mat, self->mat, mat->mat);
    if (code != 0) {
        Matrix61c_dealloc(rv);
        return NULL;
    }
    return (PyObject *) rv;
}

/*
 * NOT element-wise multiplication. The first operand is self, and the second operand
 * can be obtained by casting `args`.
 */
PyObject *Matrix61c_multiply(Matrix61c* self, PyObject *args) {
    if(!(PyObject_TypeCheck(args, &Matrix61cType)) || !(PyObject_TypeCheck(self, &Matrix61cType))) {
        PyErr_SetString(PyExc_TypeError, "a or b are not numc.Matrix type");
        return NULL;    
    }
    Matrix61c *mat = (Matrix61c *) args;

    Matrix61c *rv = (Matrix61c *) Matrix61c_new(&Matrix61cType, NULL, NULL);
    //rv->mat = new_mat?
    int row = self->mat->rows;
    int col = mat->mat->cols;
    int rvcode = allocate_matrix(&rv->mat, row, col);
    //do I need to error check again?
    if (rvcode == -1) {
        Matrix61c_dealloc(rv);
        //from matrix, PyErr_SetString(Py)
        return NULL;
    }
    rv->shape = get_shape(row, col);
    int code = mul_matrix(rv->mat, self->mat, mat->mat);
    if (code < 0) {
        Matrix61c_dealloc(rv);
        return NULL;
    }
    return (PyObject *) rv;
}

/*
 * Negates the given numc.Matrix.
 */
PyObject *Matrix61c_neg(Matrix61c* self) {
    if(!(PyObject_TypeCheck(self, &Matrix61cType))) {
        PyErr_SetString(PyExc_TypeError, "a is not numc.Matrix type");
        return NULL;    
    }
    //Matrix61c *mat = (Matrix61c *) args;

    Matrix61c *rv = (Matrix61c *) Matrix61c_new(&Matrix61cType, NULL, NULL);
    //rv->mat = new_mat?
    int row = self->mat->rows;
    int col = self->mat->cols;
    int rvcode = allocate_matrix(&rv->mat, row, col);
    //do I need to error check again?
    if (rvcode == -1) {
        Matrix61c_dealloc(rv);
        //from matrix, PyErr_SetString(Py)
        return NULL;
    }
    rv->shape = get_shape(row, col);
    int code = neg_matrix(rv->mat, self->mat);
    if (code != 0) {
        //FIXME what errors could possibly occur with abs and neg?
        //it modifies a valid matrix in place.  is this correct?
        Matrix61c_dealloc(rv);
        return NULL;
    }
    return (PyObject *) rv;
}

/*
 * Take the element-wise absolute value of this numc.Matrix.
 */
PyObject *Matrix61c_abs(Matrix61c *self) {
    if(!(PyObject_TypeCheck(self, &Matrix61cType))) {
        PyErr_SetString(PyExc_TypeError, "a is not numc.Matrix type");
        return NULL;    
    }
    //Matrix61c *mat = (Matrix61c *) args;

    Matrix61c *rv = (Matrix61c *) Matrix61c_new(&Matrix61cType, NULL, NULL);
    //rv->mat = new_mat?
    int row = self->mat->rows;
    int col = self->mat->cols;
    int rvcode = allocate_matrix(&rv->mat, row, col);
    //do I need to error check again?
    if (rvcode == -1) {
        Matrix61c_dealloc(rv);
        //from matrix, PyErr_SetString(Py)
        return NULL;
    }
    rv->shape = get_shape(row, col);
    int code = abs_matrix(rv->mat, self->mat);
    if (code != 0) {
        //FIXME what errors could possibly occur with abs and neg?
        //it modifies a valid matrix in place.  is this correct?
        Matrix61c_dealloc(rv);
        return NULL;
    }
    return (PyObject *) rv;
}

/*
 * Raise numc.Matrix (Matrix61c) to the `pow`th power. You can ignore the argument `optional`.
 */
PyObject *Matrix61c_pow(Matrix61c *self, PyObject *pow, PyObject *optional) {
    //FIXME error check pow, how?
    if(!(PyLong_Check(pow))) {
        PyErr_SetString(PyExc_TypeError, "pow is not a PyObject");
        return NULL;    
    }
    int power = PyLong_AsLong(pow);
    if(!(PyObject_TypeCheck(self, &Matrix61cType))) {
        PyErr_SetString(PyExc_TypeError, "a is not numc.Matrix type");
        return NULL;    
    }
    //Matrix61c *mat = (Matrix61c *) args;

    Matrix61c *rv = (Matrix61c *) Matrix61c_new(&Matrix61cType, NULL, NULL);
    //rv->mat = new_mat?
    int row = self->mat->rows;
    int col = self->mat->cols;
    int rvcode = allocate_matrix(&rv->mat, row, col);
    //do I need to error check again?
    if (rvcode == -1) {
        Matrix61c_dealloc(rv);
        //from matrix, PyErr_SetString(Py)
        return NULL;
    }
    rv->shape = get_shape(row, col);
    int code = pow_matrix(rv->mat, self->mat, power);
    if (code < 0) {
        //errors should be set already in matrix.c.  all except abs, neg.
        Matrix61c_dealloc(rv);
        return NULL;
    }
    return (PyObject *) rv;
}

/*
 * Create a PyNumberMethods struct for overloading operators with all the number methods you have
 * define. You might find this link helpful: https://docs.python.org/3.6/c-api/typeobj.html
 */
PyNumberMethods Matrix61c_as_number = {
    //what is this sorcery.  how does it know add is +?
    (binaryfunc) Matrix61c_add,
    (binaryfunc) Matrix61c_sub,
    (binaryfunc) Matrix61c_multiply,
    NULL,
    NULL,
    (ternaryfunc) Matrix61c_pow,
    (unaryfunc) Matrix61c_neg,
    NULL,
    (unaryfunc) Matrix61c_abs
    
};
    // (binaryfunc) Matrix61c_add,
    // (binaryfunc) Matrix61c_multiply


/* INSTANCE METHODS */

/*
 * Given a numc.Matrix self, parse `args` to (int) row, (int) col, and (double/int) val.
 * Return None in Python (this is different from returning null).
 */
PyObject *Matrix61c_set_value(Matrix61c *self, PyObject* args) {
    //type err if args != 3, or val != float/int.
    //indexerr ir oor.  taken care of by matrix.c
    PyObject *row = NULL;
    PyObject *col = NULL;
    PyObject *val = NULL;
    int arg_len = PyObject_Length(args);
    if (arg_len != 3) { //FIXME, this should catch errors, but I still segfault if I do a.get(x,x,x).
        PyErr_SetString(PyExc_TypeError, "Incorrect number of arguments.");
        return NULL;
    }
    if (PyArg_UnpackTuple(args, "args", 0, 10, &row, &col, &val)) { //FIXME, what are the actual values needed?
        //FIXME if (val != float or int, error.)
        //check here or in matrix.c?
        //PyFloat_AsDouble(val);
        if (!PyFloat_AsDouble(val)) {
            PyErr_SetString(PyExc_TypeError, "val is not valid type");
            return NULL;
        }
        set(self->mat, PyLong_AsLong(row), PyLong_AsLong(col), PyFloat_AsDouble(val));
        return Py_None;
    } else {
        PyErr_SetString(PyExc_TypeError, "Invalid arguments");
        return NULL;
    }
}

/*
 * Given a numc.Matrix `self`, parse `args` to (int) row and (int) col.
 * Return the value at the `row`th row and `col`th column, which is a Python
 * float/int.
 */
PyObject *Matrix61c_get_value(Matrix61c *self, PyObject* args) {
    //type err if args len != 2 or wrong types.  (not ints?  can it be more?
    //index err taken care of matrix.c)
    PyObject *row = NULL;
    PyObject *col = NULL;
    PyObject *val = NULL;
    int arg_len = PyObject_Length(args);
    //printf("arg_len %d\n", arg_len);
    //printf("%d", arg_len);
    //char *s = PyString_AsString(args);
    if (arg_len != 2) { //FIXME, this should catch errors, but I still segfault if I do a.get(x,x,x).
        PyErr_SetString(PyExc_TypeError, "Incorrect number of arguments.");
        return NULL;
    }
    if (PyArg_UnpackTuple(args, "args", 2, 2, &row, &col)) {
        val = PyFloat_FromDouble(get(self->mat, PyLong_AsLong(row), PyLong_AsLong(col)));
    } else {
        PyErr_SetString(PyExc_TypeError, "Invalid arguments");
        return NULL;
    }
    return val;
}


/*
 * Create an array of PyMethodDef structs to hold the instance methods.
 * Name the python function corresponding to Matrix61c_get_value as "get" and Matrix61c_set_value
 * as "set"
 * You might find this link helpful: https://docs.python.org/3.6/c-api/structures.html
 */
PyMethodDef Matrix61c_methods[] = {
    /* TODO: YOUR CODE HERE */
    {"set", (PyCFunction) Matrix61c_set_value, METH_VARARGS, NULL},
    {"get", (PyCFunction) Matrix61c_get_value, METH_VARARGS, NULL},
    {NULL, NULL, 0, NULL}
};

/* INDEXING */

/*
 * Given a numc.Matrix `self`, index into it with `key`. Return the indexed result.
 */
PyObject *Matrix61c_subscript(Matrix61c* self, PyObject* key) {
    PyObject *keyrow = NULL;
    PyObject *keycol = NULL;
    int row = self->mat->rows;
    int col = self->mat->cols;

    int dim = 2; //default 2d matrix.
    if (PyArg_UnpackTuple(self->shape, "dims",0, 10,&keyrow, &keycol)) {//unpack tuple.
    } else {
        PyErr_SetString(PyExc_TypeError, "bad args in subscript unpacking tuple1.");
        return NULL;
    }
    if (self->mat->is_1d) {  //sorta redundant.  get rid of dim eventually.
        dim = 1;
    }
    int slicecheck = PySlice_Check(key); //checks what type of key was given.
    int intcheck = PyLong_Check(key);
    int tuplecheck = PyTuple_Check(key);
    if (tuplecheck) { //tuple, recursive call to set_subscript for each item.
        if (PyObject_Length(key) > 2) { //error, too many args in tuple.
            PyErr_SetString(PyExc_TypeError, "Invalid arguments: Tuple len > 2");
            return NULL;
        }
        if (dim == 1) { //error, no tuples for 1d.
            PyErr_SetString(PyExc_TypeError, "1D matrices only support single slice!");
            return NULL;
        }
        PyObject *item1 = PyTuple_GetItem(key, 0); PyObject *item2 = PyTuple_GetItem(key, 1); 
        PyObject *res = NULL;
        res = Matrix61c_subscript(self, item1); 
        if (!PySlice_Check(item1)) {//mat[notslice, blah]
            return Matrix61c_subscript((Matrix61c *) res, item2);
        }
        if (PyLong_Check(item2)) { //return column item2 from slice.;
            int col_item = PyLong_AsLong(item2);
            Matrix61c *rv = (Matrix61c *) Matrix61c_new(&Matrix61cType, NULL, NULL);
            int rvcode = allocate_matrix_ref(&rv->mat, self->mat, 0, col_item, ((Matrix61c *)res)->mat->rows, 1);
            if (rvcode != 0) {
                PyErr_SetString(PyExc_RuntimeError, "allocate_matrix_ref failed in _subscript");
                return NULL;
                //FIXME, you need to free stuff.
            }
            rv->shape = get_shape(((Matrix61c *)res)->mat->rows, 1);
            return (PyObject *) rv;
        }
        if (PySlice_Check(item2)) {//slice, slice.
            PyObject *resrow = NULL;
            PyObject *rescol = NULL;
            int dim2 = 2;
            //((Matrix61c *)res)->shape;
            PyArg_UnpackTuple(((Matrix61c *)res)->shape, "dims",0, 10,&resrow, &rescol);//unpack res1 shape.
            //FIXME, make above an if statement to error check.
            if (rescol == NULL) {// checks if 1d matrix
               dim2 = 1;
            }
            Py_ssize_t start = (Py_ssize_t) NULL;
            Py_ssize_t stop = (Py_ssize_t) NULL;
            Py_ssize_t step = (Py_ssize_t) NULL;
            Py_ssize_t slicelen = (Py_ssize_t) NULL;
            int sliceinfo = PySlice_GetIndicesEx(item2, 100, &start, &stop, &step, &slicelen); //FIXME wtf should arg2 be?
            //printf("slice len %ld", slicelen);
            if (sliceinfo != 0) {//failed to read slice.
                PyErr_SetString(PyExc_RuntimeError, "failed up unpack slice");
                return NULL;
            }
            if (step > 1 || start == stop) { //slice invalid.
                PyErr_SetString(PyExc_ValueError, "Invalid arguments: in Pyslice_check(item2)");
                return NULL;
            }
            if (slicelen == 1 && dim2 == 1) {
                PyObject *val = NULL;
                if (start >= ((Matrix61c *)res)->mat->cols) {
                    PyErr_SetString(PyExc_IndexError, "Index out of range.");
                    return NULL;
                }
                val = PyFloat_FromDouble(get(((Matrix61c *)res)->mat, 0, start));
                return val;
            }
            if (dim2 == 1) {
                Matrix61c *rv = (Matrix61c *) Matrix61c_new(&Matrix61cType, NULL, NULL);
                if (start > stop) {
                    start -= 1;
                }
                int rvcode = allocate_matrix_ref(&rv->mat, ((Matrix61c *)res)->mat, 0, start, 1, slicelen);
                if (rvcode != 0) {
                    PyErr_SetString(PyExc_RuntimeError, "allocate_matrix_ref failed in _subscript");
                    return NULL;
                }
                rv->shape = get_shape(1, slicelen);
                return (PyObject *) rv;
            }
            Matrix61c *rv = (Matrix61c *) Matrix61c_new(&Matrix61cType, NULL, NULL);
            int rvcode = allocate_matrix_ref(&rv->mat, ((Matrix61c *)res)->mat, 0, start, PyLong_AsLong(resrow), slicelen);
            if (rvcode != 0) {
                PyErr_SetString(PyExc_RuntimeError, "allocate_matrix_ref failed in subscript 1");
                return NULL;
            }
            rv->shape = get_shape(PyLong_AsLong(resrow), slicelen);
            return (PyObject *) rv;
            return Matrix61c_subscript((Matrix61c *) res, item2);

            //CHRIST THIS IS SLOPPY.
            //have res = mat[item1]
            //res rows, res cols
            //start, stop, step, slice len.
            //unpack item2 slice.
            // if res is 1d
                //if item2 slice is 1, return val
                    //get(res->mat, 0, start)?
                //else grab cols start to stop, return mat.
                    //alloc_ref(rvmat, resmat, 0, start, 1, slicelen.)
            //if res is 2d
                //if item2 slice is 1, ret column/mat.
                //else grab cols start to stop, return mat.
        }
    }
    if (intcheck) { //if key is int.
        int index = PyLong_AsLong(key);
        if (dim == 1) {
            PyObject *val = NULL;
            if (index >= col || index < 0) {
                PyErr_SetString(PyExc_IndexError, "Index out of range.");
                return NULL;
            }
            val = PyFloat_FromDouble(get(self->mat, 0, index));
            return val;
        }
        //else 2d
        if (index >= row || index < 0) {
            PyErr_SetString(PyExc_IndexError, "Index out of range.");
            return NULL;
        }
        Matrix61c *rv = (Matrix61c *) Matrix61c_new(&Matrix61cType, NULL, NULL);
        int rvcode = allocate_matrix_ref(&rv->mat, self->mat, PyLong_AsLong(key), 0, 1, col);
        if (rvcode != 0) {
            PyErr_SetString(PyExc_RuntimeError, "allocate_matrix_ref failed in _subscript");
            return NULL;
        }
        rv->shape = get_shape(1, col);
        return (PyObject *) rv;
    }
    if (slicecheck) { //if key is slice
        Py_ssize_t start = (Py_ssize_t) NULL;
        Py_ssize_t stop = (Py_ssize_t) NULL;
        Py_ssize_t step = (Py_ssize_t) NULL;
        Py_ssize_t slicelen = (Py_ssize_t) NULL;
        //int sliceinfo = PySlice_Unpack(key, &start, &stop, &step);
        int sliceinfo = PySlice_GetIndicesEx(key, col, &start, &stop, &step, &slicelen);
        if (sliceinfo != 0) {
            PyErr_SetString(PyExc_RuntimeError, "failed up unpack slice");
            return NULL;
        }
        //printf("slice start: %ld, stop: %ld, step: %ld, slicelen: %ld", start, stop, step, slicelen);
        if (step > 1 || start == stop) {
            PyErr_SetString(PyExc_ValueError, "Invalid arguments: Slice starts and ends at same index, or step > 1");
            return NULL;
        }
        if (slicelen == 1 && dim == 1) {
            PyObject *val = NULL;
            if (start >= col) {
                PyErr_SetString(PyExc_IndexError, "Index out of range.");
                return NULL;
            }
            val = PyFloat_FromDouble(get(self->mat, 0, start));
            return val;
        }
        if (dim == 1) {
            Matrix61c *rv = (Matrix61c *) Matrix61c_new(&Matrix61cType, NULL, NULL);
            if (start > stop) {
                start -= 1;
            }
            int rvcode = allocate_matrix_ref(&rv->mat, self->mat, 0, start, 1, slicelen);
            if (rvcode != 0) {
                PyErr_SetString(PyExc_RuntimeError, "allocate_matrix_ref failed in _subscript");
                return NULL;
            }
            rv->shape = get_shape(1, slicelen);
            return (PyObject *) rv;
        }
        //else 2d. jsut grab the rows we need.
        //printf("slicing 2d");
        Matrix61c *rv = (Matrix61c *) Matrix61c_new(&Matrix61cType, NULL, NULL);
        int rvcode = allocate_matrix_ref(&rv->mat, self->mat, start, 0, slicelen, col);
        if (rvcode != 0) {
            PyErr_SetString(PyExc_RuntimeError, "allocate_matrix_ref failed in _subscript");
            return NULL;
            }
        rv->shape = get_shape(slicelen, col);
        return (PyObject *) rv;
        //if stepsize != 1; value error.
        //if len slice <1; error.

    }

    return NULL; //shouldn't ever reach here.;
}



/*
 * Given a numc.Matrix `self`, index into it with `key`, and set the indexed result to `v`.
 */
int Matrix61c_set_subscript(Matrix61c* self, PyObject *key, PyObject *v) {
    //so v can be list, nested lists, or an int/float.

    PyObject * rv = NULL;
    rv = Matrix61c_subscript(self, key); //get slice.
    //int slicecheck = PySlice_Check(key); //checks what type of key was given.
    //int intcheck = PyLong_Check(key);
    //int tuplecheck = PyTuple_Check(key);
    if (rv == NULL) { //if slice fails, end.
        PyErr_SetString(PyExc_ValueError, "subscript failed. possibly bad slice in set_sub");
        return -1;
    }
    int isMat = PyObject_TypeCheck(rv, &Matrix61cType); //check if rv is mat, if not, it must have been a float. right?
    int islist = PyList_Check(v); //check if v is a list.
    //printf("isMat %ld, pyListCheck: %ld\n", isMat, islist);
    if ((islist && !isMat) || (!islist && isMat)) { //matrix can only be updated by list, and single val only updated by val.
        PyErr_SetString(PyExc_TypeError, "Value is not valid.  Either the rv or v is wrong.");
        return -1;
    }
    //if rv[key] = mat
        //if mat 1d, v must be 1d list.
        //if mat 2d, v must be 2d list.
    //else if rv[key] = val
        //v must be val.
    // if (tuplecheck) {
    //     printf("handle tuples seperately lol");
    //     return -1;
    // }
    if (isMat && islist) { //rv[key] = mat, v = list
        //yoinked from init_2d; idt it matters what the og matrix is.  as long as rv returns valid.
        int rvd = ((Matrix61c *)rv)->mat->is_1d; //checks if rv is 1d.
        //decode key to get start and end indices. FIXME.  updates i to j, instead of 0 to col.
        //init_2d start
        //printf("rvd: %d\n", rvd);
        if (rvd == 1) { //rv[key] = 1d
            //need to check that list is not nested lists. FIXME
            int rows = ((Matrix61c *)rv)->mat->rows;
            int cols = ((Matrix61c *)rv)->mat->cols; //this makes it redundant. improve?
            //old or cond: || PyList_Size(v) != self->mat->cols; why?
            if (rows * cols != PyList_Size(v)  || PyList_Check(PyList_GetItem(v,1)) != 0) { //PyList_checks if v is 2d.
                //printf("rows %d, cols %d, pylistsize %d\n", rows, cols, PyList_Size(v));
                PyErr_SetString(PyExc_ValueError, "Incorrect number of elements in list 1d, or v is 2d.");
                return -1;
            }
            int count = 0;
            for (int i = 0; i < rows; i++) {
                for (int j = 0; j < cols; j++) {
                    //I should be updated rv, instead of mat.  updates on rv should be reflected by mat.
                    //but it's not updating...
                    //printf("loop\n");
                    //printf("og mat %ld, rv mat %ld\n", get(self->mat, i, j), get(((Matrix61c *)rv)->mat,i, j));

                    set(((Matrix61c *)rv)->mat, i, j, PyFloat_AsDouble(PyList_GetItem(v, count)));

                    //printf("i:%d, j:%d, PyList v:%f\n", i, j, PyFloat_AsDouble(PyList_GetItem(v,count)));
                    //printf("after update\n");
                    //printf("og mat %ld, rv mat %ld\n", get(self->mat, i, j), get(((Matrix61c *)rv)->mat,i, j));
                    count++;
                }
            }
            return 0;      
        }
        else if (rvd == 0) {//rv = 2d
            int rows = PyList_Size(v);
            if (rows == 0) {
                PyErr_SetString(PyExc_ValueError,
                                "Cannot initialize numc.Matrix with an empty list");
                return -1;
            }
            int cols;
            if (!PyList_Check(PyList_GetItem(v, 0))) {
                PyErr_SetString(PyExc_ValueError, "List values not valid.  v input was 1d");
                return -1; //errors cause 1d.  need another case.
            } else {
                cols = PyList_Size(PyList_GetItem(v, 0));
            }
            for (int i = 0; i < rows; i++) {
                //|| PyList_Size(PyList_GetItem(v, i)) != self->mat->cols point of this again?
                if (!PyList_Check(PyList_GetItem(v, i)) || PyList_Size(PyList_GetItem(v, i)) != cols) {
                    PyErr_SetString(PyExc_ValueError, "List values not valid 2 in 2d. 111");
                    return -1;
                }
            }
            for (int i = 0; i < rows; i++) {
                for (int j = 0; j < cols; j++) {
                    set(((Matrix61c *)rv)->mat, i, j, PyFloat_AsDouble(PyList_GetItem(PyList_GetItem(v, i), j)));            
                    }
            }
            return 0;
        }
    }
    if (!isMat && !islist) {//rv = val, v = val

    }
    return -1; //should never get herE?
}

PyMappingMethods Matrix61c_mapping = {
    NULL,
    (binaryfunc) Matrix61c_subscript,
    (objobjargproc) Matrix61c_set_subscript,
};

/* INSTANCE ATTRIBUTES*/
PyMemberDef Matrix61c_members[] = {
    {
        "shape", T_OBJECT_EX, offsetof(Matrix61c, shape), 0,
        "(rows, cols)"
    },
    {NULL}  /* Sentinel */
};

PyTypeObject Matrix61cType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "numc.Matrix",
    .tp_basicsize = sizeof(Matrix61c),
    .tp_dealloc = (destructor)Matrix61c_dealloc,
    .tp_repr = (reprfunc)Matrix61c_repr,
    .tp_as_number = &Matrix61c_as_number,
    .tp_flags = Py_TPFLAGS_DEFAULT |
    Py_TPFLAGS_BASETYPE,
    .tp_doc = "numc.Matrix objects",
    .tp_methods = Matrix61c_methods,
    .tp_members = Matrix61c_members,
    .tp_as_mapping = &Matrix61c_mapping,
    .tp_init = (initproc)Matrix61c_init,
    .tp_new = Matrix61c_new
};


struct PyModuleDef numcmodule = {
    PyModuleDef_HEAD_INIT,
    "numc",
    "Numc matrix operations",
    -1,
    Matrix61c_class_methods
};

/* Initialize the numc module */
PyMODINIT_FUNC PyInit_numc(void) {
    PyObject* m;

    if (PyType_Ready(&Matrix61cType) < 0)
        return NULL;

    m = PyModule_Create(&numcmodule);
    if (m == NULL)
        return NULL;

    Py_INCREF(&Matrix61cType);
    PyModule_AddObject(m, "Matrix", (PyObject *)&Matrix61cType);
    printf("CS61C Fall 2020 Project 4: numc imported!\n");
    fflush(stdout);
    return m;
}