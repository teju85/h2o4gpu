/*!
 * Copyright 2018 H2O.ai, Inc.
 * License   Apache License Version 2.0 (see LICENSE for details)
 */
#pragma once

#include "../data/ffm/data.h"
#include "../../common/logger.h"

namespace ffm {

typedef struct Params {
  int verbose = 0;

  float learningRate = 0.2;
  float regLambda = 0.00002;

  int nIter = 10;
  int batchSize = 1000;

  size_t numRows = 0;
  size_t numNodes = 0;
  size_t numFeatures = 0;
  size_t numFields = 0;
  int k = 4;

  bool normalize = true;
  bool autoStop = false;

  int seed = 0;

  // For CPU number of threads to be used
  // For GPU number of GPUs to be used
  int nGpus = 1;

  void printParams() {
    log_verbose(verbose, "learningRate = %f \n regLambda = %f \n nIter = %d \n batchSize = %d \n numRows = %d \n numFeatures = %d \n numFields = %d \n k = %d", learningRate, regLambda, nIter, batchSize, numRows, numFeatures, numFields, k);
  }

} Params;

void ffm_fit_float(size_t *features, size_t* fields, float* values, int *labels, float *scales, size_t *positions, float *w, Params &_param);
void ffm_fit_double(size_t *features, size_t* fields, double* values, int *labels, double *scales, size_t *positions, double *w, Params &_param);

void ffm_predict_float(size_t *features, size_t* fields, float* values, float *scales, size_t* positions, float *predictions, float *w, Params &_param);
void ffm_predict_double(size_t *features, size_t* fields, double* values, double *scales, size_t* positions, double *predictions, double *w, Params &_param);

}
