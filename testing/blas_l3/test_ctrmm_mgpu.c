/**
 * @copyright (c) 2012- King Abdullah University of Science and
 *                      Technology (KAUST). All rights reserved.
 **/


/**
 * @file testing/blas_l3/test_ctrmm_mgpu.c

 * KBLAS is a high performance CUDA library for subset of BLAS
 *    and LAPACK routines optimized for NVIDIA GPUs.
 * KBLAS is provided by KAUST.
 *
 * @version 2.0.0
 * @author Ali Charara
 * @date 2017-11-13
 **/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <sys/time.h>
#include <cuda.h>
#include <cuda_runtime.h>
#include <cuda_runtime_api.h>
#include "cublas_v2.h"

#include "kblas.h"
#include "testing_utils.h"
#include "test_trmm_mgpu.ch"

extern int kblas_trmm_ib_cublas;
extern bool kblas_trmm_use_custom;

//==============================================================================================
int main(int argc, char** argv)
{
  
  kblas_opts opts;
  if(!parse_opts( argc, argv, &opts )){
    USAGE;
    return -1;
  }
  
  cublasHandle_t cublas_handle;
  cublasCreate(&cublas_handle);
  
  double alpha = 0.29;
  kblas_trmm_ib_cublas = opts.nb;
  kblas_trmm_use_custom = (bool)opts.custom;
  test_trmm<double>(opts, alpha, cublas_handle);
  
  cublasDestroy(cublas_handle);
}


