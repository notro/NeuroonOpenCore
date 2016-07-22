#include "RollingAlgorithms.h"
#include <numeric>
#include <vector>

using namespace std;

void RollingApply::init(size_t n, const RollWindow &window ) {}

double RollingApply::step(const vector<double> &v, StepType type){
  return _apply_fun(v);
}


void RollingPriority::init(size_t n, const RollWindow &window ) {
  _windows_length = window.length();
}


double RollingPriority::step(const vector<double> &v, StepType type){
  switch(type){
  case StepType::START:
    _set.insert(v.begin(),v.end());
    _prev_last = *(v.begin());
    break;
  case StepType::END:
  case StepType::STEP:
    if(_set.size()==_windows_length){
      _set.erase(_prev_last);
      _prev_last = *(v.begin());
    }
    _set.insert(*(v.end()-1));
    break;
  }
  return *_set.begin();
}

void RollingSumOrMean::init(size_t n, const RollWindow &window ) {}

double RollingSumOrMean::step(const vector<double> &v, StepType type){
  switch(type){
  case StepType::START:
    _sum = accumulate(v.begin(), v.end(), 0);
    _prev_last = *(v.begin());
    break;
  case StepType::END:
  case StepType::STEP:
    // window increased
    if((int)v.size() - (int)_prev_win_length >= 0){
      _sum+=*(v.end()-1);
    }
    if((int)v.size() - (int)_prev_win_length <= 0){
      _sum-=_prev_last;
    }
    _prev_last = *(v.begin());
  }
  _prev_win_length = v.size();

  return _type == Type::MEAN ? _sum / v.size() : _sum;
}