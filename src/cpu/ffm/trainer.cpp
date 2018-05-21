/*!
 * Copyright 2018 H2O.ai, Inc.
 * License   Apache License Version 2.0 (see LICENSE for details)
 */
#include "../../base/ffm/trainer.h"

namespace ffm {

template<typename T>
Trainer<T>::Trainer(Params &params) : params(params) {
  // TODO implement
}

template<typename T>
Trainer<T>::Trainer(const T* weights, Params &params) : params(params) {
  // TODO implement
}

template<typename T>
void Trainer<T>::setDataset(const Dataset<T> &dataset) {
  // TODO implement
}

template<typename T>
void Trainer<T>::predict(T *predictions) {
  // TODO implement
}

template<typename T>
T Trainer<T>::oneEpoch(bool update) {
  // TODO implement
}

template<typename T>
bool Trainer<T>::earlyStop() {
  // TODO implement
  return false;
}

template class Trainer<float>;
template class Trainer<double>;

}