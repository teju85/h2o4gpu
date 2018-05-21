/*!
 * Copyright 2018 H2O.ai, Inc.
 * License   Apache License Version 2.0 (see LICENSE for details)
 */
#include "ffm.h"
#include "../../common/timer.h"
#include <numeric>
#include <algorithm>

namespace ffm {

template<typename T>
FFM<T>::FFM(Params &params) : params(params), trainer(params) {}

template<typename T>
FFM<T>::FFM(Params & params, T *weights) : params(params), trainer(weights, params) {}

template<typename T>
void FFM<T>::fit(const Dataset<T> &dataset) {
  this->trainer.setDataset(dataset);

  Timer timer;

  for (int epoch = 1; epoch <= this->params.nIter; epoch++) {
    timer.tic();
    trainer.oneEpoch(true);
    timer.toc();
    log_debug(params.verbose, "Epoch took %f.", timer.pop());
    if (trainer.earlyStop()) {
      break;
    }
  }
}

template<typename T>
void FFM<T>::predict(const Dataset<T> &dataset, T *predictions) {
  this->trainer.setDataset(dataset);
  trainer.predict(predictions);
}

/**
 * C API method
 */
void ffm_fit_float(int* features, int* fields, float* values, int *labels, float *scales, int *rowPositions, float *w, Params &_param) {
  log_debug(_param.verbose, "Converting %d float rows into a dataset.", _param.numRows);
  Dataset<float> dataset(_param.numFields, _param.numFeatures, _param.numRows, _param.numNodes, features, fields, values, labels, scales, rowPositions);
  FFM<float> ffm(_param);
  _param.printParams();
  log_debug(_param.verbose, "Running FFM fit for float.");
  Timer timer;
  timer.tic();
  ffm.fit(dataset);
  timer.toc();
  log_debug(_param.verbose, "Float fit took %f.", timer.pop());
  ffm.trainer.model->copyTo(w);
}

void ffm_fit_double(int* features, int* fields, double* values, int *labels, double *scales, int *rowPositions, double *w, Params &_param) {
  log_debug(_param.verbose, "Converting %d double rows into a dataset.", _param.numRows);
  Dataset<double> dataset(_param.numFields, _param.numFeatures, _param.numRows, _param.numNodes, features, fields, values, labels, scales, rowPositions);
  FFM<double> ffm(_param);
  _param.printParams();
  log_debug(_param.verbose, "Running FFM fit for double.");
  ffm.fit(dataset);
  ffm.trainer.model->copyTo(w);
}

void ffm_predict_float(int *features, int* fields, float* values, float *scales, int* rowPositions, float *predictions, float *w, Params &_param) {
  log_debug(_param.verbose, "Converting %d float rows into a dataset for predictions.", _param.numRows);
  Dataset<float> dataset(_param.numFields, _param.numFeatures, _param.numRows, _param.numNodes, features, fields, values, nullptr, scales, rowPositions);
  FFM<float> ffm(_param, w);
  _param.printParams();
  log_debug(_param.verbose, "Running FFM predict for float.");
  ffm.predict(dataset, predictions);
}

void ffm_predict_double(int *features, int* fields, double* values, double *scales, int* rowPositions, double *predictions, double *w, Params &_param){
  log_debug(_param.verbose, "Converting %d double rows into a dataset for predictions.", _param.numRows);
  Dataset<double> dataset(_param.numFields, _param.numFeatures, _param.numRows, _param.numNodes, features, fields, values, nullptr, scales, rowPositions);
  FFM<double> ffm(_param, w);
  _param.printParams();
  log_debug(_param.verbose, "Running FFM predict for double.");
  ffm.predict(dataset, predictions);
}

} // namespace ffm