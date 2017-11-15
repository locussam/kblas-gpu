/**
 * @copyright (c) 2012- King Abdullah University of Science and
 *                      Technology (KAUST). All rights reserved.
 **/


/**
 * @file testing/testing_helper.h

 * KBLAS is a high performance CUDA library for subset of BLAS
 *    and LAPACK routines optimized for NVIDIA GPUs.
 * KBLAS is provided by KAUST.
 *
 * @version 2.0.0
 * @author Wajih Halim Boukaram
 * @author Ali Charara
 * @date 2017-11-13
 **/

#ifndef __TESTING_HELPER_H__
#define __TESTING_HELPER_H__

#include <cuda.h>
#include <cuda_runtime.h>
#include <cuda_runtime_api.h>
#include <cublas_v2.h>
#include <kblas.h>
#include <cusolverDn.h>

#ifdef USE_MAGMA
#include <magma_v2.h>
#endif
#ifdef USE_MKL
#include <mkl.h>
#else
// #include <cblas.h>		TODO: if MKL not set we need to use other libs
// #include <lapacke.h>
#endif
#ifdef USE_OPENMP
#include <omp.h>
#endif
#include <kblas.h>

#ifdef __cplusplus
extern "C" {
#endif

////////////////////////////////////////////////////////////
// Error checking
////////////////////////////////////////////////////////////
#define check_error(ans) { gpuAssert((ans), __FILE__, __LINE__); }
void gpuAssert(cudaError_t code, const char *file, int line);

#define check_cublas_error(ans) { gpuCublasAssert((ans), __FILE__, __LINE__); }
void gpuCublasAssert(cublasStatus_t code, const char *file, int line);

#define check_kblas_error(ans) { gpuKblasAssert((ans), __FILE__, __LINE__); }
void gpuKblasAssert(int code, const char *file, int line);

#define check_cusolver_error(ans) { gpuCusolverAssert((ans), __FILE__, __LINE__); }
void gpuCusolverAssert(cusolverStatus_t code, const char *file, int line);

////////////////////////////////////////////////////////////
// Timers and related stuff
////////////////////////////////////////////////////////////
double gettime(void);

struct GPU_Timer;
typedef GPU_Timer* GPU_Timer_t;

GPU_Timer_t newGPU_Timer(cudaStream_t stream);
void deleteGPU_Timer(GPU_Timer_t timer);
void gpuTimerTic(GPU_Timer_t timer);
void gpuTimerRecordEnd(GPU_Timer_t timer);
double gpuTimerToc(GPU_Timer_t timer);

void avg_and_stdev(double* values, int num_vals, double* avg, double* std_dev, int warmup);

////////////////////////////////////////////////////////////
// Generate array of pointers from a strided array
////////////////////////////////////////////////////////////
void generateDArrayOfPointers(double* original_array, double** array_of_arrays, int stride, int num_arrays, cudaStream_t stream);
void generateSArrayOfPointers(float* original_array, float** array_of_arrays, int stride, int num_arrays, cudaStream_t stream);

////////////////////////////////////////////////////////////
// Allocations
////////////////////////////////////////////////////////////
#define TESTING_MALLOC_CPU( ptr, T, size)                       \
  if ( (ptr = (T*) malloc( (size)*sizeof( T ) ) ) == NULL) {    \
    fprintf( stderr, "!!!! malloc_cpu failed for: %s\n", #ptr ); \
    exit(-1);                                                   \
  }
#define TESTING_MALLOC_DEV( ptr, T, size) check_error( cudaMalloc( (void**)&ptr, (size)*sizeof(T) ) )
#define TESTING_MALLOC_PIN( ptr, T, size) check_error( cudaHostAlloc ( (void**)&ptr, (size)*sizeof( T ), cudaHostAllocPortable  ))

#define TESTING_FREE_CPU(ptr)	{ if( (ptr) ) free( (ptr) ); }
#define TESTING_FREE_DEV(ptr)	check_error( cudaFree( (ptr) ) );

////////////////////////////////////////////////////////////
// Data generation
////////////////////////////////////////////////////////////
void generateDrandom(double* random_data, long num_elements, int num_ops);
void generateSrandom(float* random_data, long num_elements, int num_ops);
void srand_matrix(long rows, long cols, float* A, long LDA);
void drand_matrix(long rows, long cols, double* A, long LDA);
void crand_matrix(long rows, long cols, cuFloatComplex* A, long LDA);
void zrand_matrix(long rows, long cols, cuDoubleComplex* A, long LDA);

void smatrix_make_hpd(int N, float* A, int lda);
void dmatrix_make_hpd(int N, double* A, int lda);
void cmatrix_make_hpd(int N, cuFloatComplex* A, int lda);
void zmatrix_make_hpd(int N, cuDoubleComplex* A, int lda);

// set cond = 0 to use exp decay
void generateDrandomMatrices(
	double* M_strided, int stride_M, double* svals_strided, int stride_S, int rows, int cols,
	double cond, double exp_decay, int seed, int num_ops, int threads
);
void generateSrandomMatrices(
	float* M_strided, int stride_M, float* svals_strided, int stride_S, int rows, int cols,
	float cond, float exp_decay, int seed, int num_ops, int threads
);

////////////////////////////////////////////////////////////
// Result checking
////////////////////////////////////////////////////////////
// Vectors
float sget_max_error(float* ref, float *res, int n, int inc);
double dget_max_error(double* ref, double *res, int n, int inc);
float cget_max_error(cuFloatComplex* ref, cuFloatComplex *res, int n, int inc);
double zget_max_error(cuDoubleComplex* ref, cuDoubleComplex *res, int n, int inc);
// Matrices
float sget_max_error_matrix(float* ref, float *res, long m, long n, long lda);
double dget_max_error_matrix(double* ref, double *res, long m, long n, long lda);
float cget_max_error_matrix(cuFloatComplex* ref, cuFloatComplex *res, long m, long n, long lda);
double zget_max_error_matrix(cuDoubleComplex* ref, cuDoubleComplex *res, long m, long n, long lda);

// float sget_max_error_matrix_uplo(float* ref, float *res, long m, long n, long lda, char uplo);
// double dget_max_error_matrix_symm(double* ref, double *res, long m, long n, long lda, char uplo);

#define printMatrix(m, n, A, lda, out) { \
  for(int r = 0; r < (m); r++){ \
    for(int c = 0; c < (n); c++){ \
      fprintf((out), "%.4e  ", (A)[r + c * (lda)]); \
    } \
    fprintf((out), "\n"); \
  } \
  fprintf((out), "\n"); \
}

////////////////////////////////////////////////////////////
// Command line parser
////////////////////////////////////////////////////////////
#define MAX_NTEST 1000

typedef struct kblas_opts
{
	// matrix size
	int ntest;
	int msize[ MAX_NTEST ];
	int nsize[ MAX_NTEST ];
	int ksize[ MAX_NTEST ];
	int rsize[ MAX_NTEST ];

	// scalars
	int devices[MAX_NGPUS];
	int nstream;
	int ngpu;
	int niter;
	int nruns;
	double      tolerance;
	int check;
	int verbose;
	int nb;
	int db;
	int custom;
	int warmup;
	int time;
	int lapack;
	//int bd[KBLAS_BACKDOORS];
	int batchCount;
	int strided;
	int btest, batch[MAX_NTEST];
	int rtest, rank[MAX_NTEST];
	int omp_numthreads;

	// lapack flags
	char uplo;
	char transA;
	char transB;
	char side;
	char diag;
} kblas_opts;

int parse_opts(int argc, char** argv, kblas_opts *opts);

#ifdef __cplusplus
}
#endif

#endif // __TESTING_HELPER_H__
