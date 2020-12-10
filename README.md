# numc

Overall: In this project, I recreated 1d and 2d matrix functions in C.  These functions are similar to numpy matrix functions.  The matrices support the arithmetic functions: add, sub, mul, abs, neg, and pow.  The matrices also support indexing and slicing, and the operations on these elements.

Task 1:  In this task, we implemented matrix functions in C.  The matrix data structure includes the rows and columns of the matrix, a flag indicating if the matrix is 1d, and reference counter, and pointer to their parent, and a double pointer to its data.

Rather than using a double for loop to initialize the matrix data, I used a trick to utilize one "long" row to represent all of the data for a matrix.  So instead of pointing each row to individual arrays, I create one long array, and point each row to a specific position in the long array.  This should improve cache miss rate, since all the data resides on this long array instead of being spread out in mutliple row arrays.

Task 2: In this task, we setup our configuration for Python to interact with C.  We simply set up the arguments for a module of our C code to be setup whenever we start the program.

Task 3: In this task, I wrote the code that interfaces between Python and C.  Numc uses a Python interface but utilizes the C code.  The purpose of this is that Python is easy to use and includes an interactive mode, while the C code provides performance enhancements that Python lacks.

However, this needs to be setup.  Simple index slicing in Python does not exist in C, so we recreate this using our interface.  My interface supports accessing matrix elements using indices, slices, and tuples of the two.  (A[i], A[i:j], A[i/i:j, k/k:l]).  Indices were simple; if it was A was 2d, you just return a pointer to the position of that row, and for 1d, you return the value at the index.  Slices were a bit more complicated, but using a built in function, I was able to extract the information from the slices and feed the values to allocate_matrix_ref.  Tuples of slices we're much more complicated.  For a while I thought I needed to transpose the matrix, so that I could reuse my previous functions.  (Matrix[slice,slice], the second slice would grab a column/s instead of a row/s.)

This task ended up being an extension of Task 1.  If your implementation in Task 1 was perfect, Task 3 is a breeze since all you are doing is mapping the Python interface functions to the C functions.  The error checks in Task 1 did not prevent my errors in Task 3.  For example, I had an implementation of allocate_matrix_ref that made a new matrix and filled the values from the reference matrix.  This passed the sanity checks in Task 1, but ended up being completely wrong.  Obvious failure on my part for not understanding, but it ended up taking a long time to realize the failure in my logic.  

The most important function here was the allocate_matrix_ref fxn.  My issues with Matrix[slice,slice] ended up being a simple problem.  Instead of tranposing any matrices, I just had to be clever with my pointer manipulation and correctly set my offsets.  I believed that if I improperly pointed a reference matrix at an old matrix, and the dimensions were different, I would possibly be given the reference matrix access to the old matrix outside of it's own bounds.  But that was the point of all of the error checks, to ensure that reference matrices cannot access data outside of its own bounds, while still being accessible to the parent matrix.

This process ended up taking the most amount of time for me due to the context switching between Python and C.  C is a language that has strict rules and nuances that requires users to read documentation.  If I could make a suggestion, the project should all be in one language or somehow streamlined.  I believe Task 4 is supposed to be the meat of the project, but I likely spent 1/2 to 3/4's of my time on Task 3.  

Task 4:  In Task 4, I optimized my C code using various methods.  Below are the methods for each function.

allocate_matrix: Optimized how the mat->data structure is initialized.

allocate_matrix_ref: No optimizations.

deallocate_matrix: No optimizations.

get: No optimizations.

set: No optimizations.

fill_matrix: Blocking of size two was used to speed up filling of matrices.

add_matrix: I utilized pragma omp parallel and unrolled the method into 16 blocks.

sub_matrix:  I utilized pragma omp parallel and unrolled the method into 16 blocks.

mul_matrix: Using tips from Piazza and Discord, I implemented mul by transposing the second matrix, used memset to clear data instead of the fill function, and used pragma omp parallel for matrices with more than 10000 elements.  I also used the ikj matrix multiplication method.  Instead of going through each element of the two arrays, I start with an element from the first array, then work through the second array,

pow_matrix: Similar to mul_matrix, I split the pow into three cases.  Power of 0 returns an identity matrix.  Power of 1 multiplies the identity matrix with the original matrix.  Power of 2 utilizes mul_matrix.  For powers of 3 and more, instead of recursively calling mul, I copied the code from mul into a for loop so that extra frames are not created.  I also added omp_set_num_threads(4) at the start of the loop then return it to 8 at the end of the loop. 

neg_matrix: I utilized praga omp parallel and unrolled the method into 17 blocks.

abs_matrix: I utilized praga omp parallel and unrolled the method into 17 blocks.

I did not use any SIMD instructions for any of my optimizations.  Whenever I implemented SIMD instructions, my unittest would pass for the specific test, but when running the full test, it would segfault.  I am unsure of what causes these issues, but I opted not to figure this out due to running out of time.

Testing:  Using the template as a guide, I duplicated many tests with various matrix sizes.  Overall, larger matrices improved dramatically with my optimizations, but small matrices tended to range from 0.5 to 4 times increase in performance.  

Notes:  If I had more time, I would implement SIMD instructions in my above functions.  OpenMP is super OP, adding one line ended up speeding up many functions dramatically.



