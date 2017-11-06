#include <algorithm> 	// for std::min
#include "kblas.h"
#include "kblas_struct.h"
#include "kblas_gpu_util.ch"
#include "batch_transpose.h"

#define TRANSPOSE_TILE_DIM      32
#define TRANSPOSE_BLOCK_ROWS    8
#define TRANSPOSE_LOAD(m)       __ldg(&(m))

#define KBLAS_TRANSPOSE_FAILURE			-1
#define KBLAS_TRANSPOSE_SUCCESS			 0

template<class T, class T_ptr>
__global__ 
void transpose_kernel(int m, int n, T_ptr matrix_data, int ldm, int stride_m, T_ptr transpose_data, int ldt, int stride_t, int op_start, int ops)
{
    __shared__ T tile[TRANSPOSE_TILE_DIM][TRANSPOSE_TILE_DIM + 1];
    
    int x = blockIdx.x * TRANSPOSE_TILE_DIM + threadIdx.x;
    int y = blockIdx.y * TRANSPOSE_TILE_DIM + threadIdx.y;
    int op_index = op_start + blockIdx.z;
    
    if(op_index >= ops) return;
    
    T* matrix = getOperationPtr<T>(matrix_data, op_index, stride_m);
    T* transpose = getOperationPtr<T>(transpose_data, op_index, stride_t);
    
    #pragma unroll
    for (int j = 0; j < TRANSPOSE_TILE_DIM; j += TRANSPOSE_BLOCK_ROWS)
        if(x < m && y + j < n)
            tile[threadIdx.y + j][threadIdx.x] = TRANSPOSE_LOAD(matrix[x + (y + j) * ldm]);

    __syncthreads();
    
    x = blockIdx.y * TRANSPOSE_TILE_DIM + threadIdx.x; 
    y = blockIdx.x * TRANSPOSE_TILE_DIM + threadIdx.y;
    
    #pragma unroll
    for (int j = 0; j < TRANSPOSE_TILE_DIM; j += TRANSPOSE_BLOCK_ROWS)
        if(y + j < m && x < n)
            transpose[x + (y + j) * ldt] = tile[threadIdx.x][threadIdx.y + j];
}

template<class T, class T_ptr>
int batch_transpose_template(kblasHandle_t handle, int m, int n, T_ptr matrix_data, int ldm, int stride_m, T_ptr transpose_data, int ldt, int stride_t, int ops)
{
	int ops_per_kernel = 32768;
		
    int block_rows = iDivUp(m, TRANSPOSE_TILE_DIM);
    int block_cols = iDivUp(n, TRANSPOSE_TILE_DIM);
    
	dim3 blockDim(TRANSPOSE_TILE_DIM, TRANSPOSE_BLOCK_ROWS, 1);
    dim3 gridDim(block_rows, block_cols, ops_per_kernel);
	
	int op_start = 0;

	while(op_start < ops)
    {
		gridDim.z = std::min(ops_per_kernel, ops - op_start);
		transpose_kernel<T, T_ptr><<< gridDim, blockDim, 0, handle->stream >>>(m, n, matrix_data, ldm, stride_m, transpose_data, ldt, stride_t, op_start, ops);
		op_start += ops_per_kernel;
	}	
    
    check_error_ret( cudaGetLastError(), KBLAS_TRANSPOSE_FAILURE );
	return KBLAS_TRANSPOSE_SUCCESS;
}

// Strided interface
int kblasDtranspose_batch_strided(kblasHandle_t handle, int m, int n, double* matrix_strided, int ldm, int stride_m, double* transpose_strided, int ldt, int stride_t, int ops)
{
	return batch_transpose_template<double, double*>(handle, m, n, matrix_strided, ldm, stride_m, transpose_strided, ldt, stride_t, ops);
}

int kblasStranspose_batch_strided(kblasHandle_t handle, int m, int n, float* matrix_strided, int ldm, int stride_m, float* transpose_strided, int ldt, int stride_t, int ops)
{
	return batch_transpose_template<float, float*>(handle, m, n, matrix_strided, ldm, stride_m, transpose_strided, ldt, stride_t, ops);
}

// Array of pointers interface
int kblasDtranspose_batch(kblasHandle_t handle, int m, int n, double** matrix_ptrs, int ldm, double** transpose_ptrs, int ldt, int ops)
{
	return batch_transpose_template<double, double**>(handle, m, n, matrix_ptrs, ldm, 0, transpose_ptrs, ldt, 0, ops);
}

int kblasStranspose_batch(kblasHandle_t handle, int m, int n, float** matrix_ptrs, int ldm, float** transpose_ptrs, int ldt, int ops)
{
	return batch_transpose_template<float, float**>(handle, m, n, matrix_ptrs, ldm, 0, transpose_ptrs, ldt, 0, ops);
}
