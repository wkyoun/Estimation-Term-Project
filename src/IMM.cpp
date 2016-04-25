//
// Created by clancy on 4/24/16.
//

#include "../include/IMM.h"

IMM::IMM(KalmanFilter f1, KalmanFilter f2){
  _filters.push_back(f1);
  _filters.push_back(f2);
  _p<<.9,.1,
      .1,.9;
  _muMode<<.5,.5;
}

pair<StateVector,StateCovarianceMatrix> IMM::Update(MeasurementVector z) {
  CalculateMixingProbabilities();
  Mix();
  GetLikelihoods(z);
  UpdateModeProbabilities();
  Estimate();
  return make_pair(_x,_P);
};

void IMM::CalculateNormalizingConstants() {
  _c<<0,0;
  for(int j = 0;j<NUM_FILTERS;j++) {
    for(int i = 0;i<NUM_FILTERS;i++) {
      _c(j) += _p(i,j)*_muMode(i);
    }
  }
}

void IMM::CalculateMixingProbabilities() {
  CalculateNormalizingConstants();
  for(int i = 0;i<NUM_FILTERS;i++) {
    for(int j = 0;j<NUM_FILTERS;j++) {
      _muMix(i,j) = _p(i,j)*_muMode(i)/_c(j);
    }
  }
}

void IMM::Mix() {
  for(int j = 0;j<NUM_FILTERS;j++) {
    for(int i = 0;i<NUM_FILTERS;i++) {
      StateVector xi = _filters[i].GetEstimate().first;
      _mixed[j].first += xi*_muMix(i,j);
      StateCovarianceMatrix Pi = _filters[i].GetEstimate().second;
      StateVector temp = xi - _mixed[j].first;
      _mixed[j].second += _muMix(i,j)*(Pi+temp*temp.transpose());
      _filters[j].Reinitialize(_mixed[j]);
    }
  }
}

void IMM::GetLikelihoods(MeasurementVector z) {
  for(auto x:_filters)
    x.Update(z);
  for(int i = 0;i<NUM_FILTERS;i++){
    _Lambda(i) = _filters[i].GetLikelihood();
  }
}

void IMM::UpdateModeProbabilities() {
  double c;
  for(int j = 0;j<NUM_FILTERS;j++) {
    c += _Lambda(j)*_c(j);
  }
  for(int i = 0;i<NUM_FILTERS;i++) {
    _muMode(i) = _Lambda(i)*_c(i)/c;
  }
}

void IMM::Estimate() {
  _x<<0,0,0,0,0;
  _P<<0,0,0,0,0,
  0,0,0,0,0,
  0,0,0,0,0,
  0,0,0,0,0,
  0,0,0,0,0;
  for(int i = 0;i<NUM_FILTERS;i++) {
    StateVector xi = _filters[i].GetEstimate().first;
    _x += xi*_muMode(i);
  }
  for(int i = 0;i<NUM_FILTERS;i++) {
    StateVector xi = _filters[i].GetEstimate().first;
    StateVector temp = xi - _x;
    StateCovarianceMatrix Pi = _filters[i].GetEstimate().second;
    _P += _muMode(i)*(Pi+temp*temp.transpose());
  }
}