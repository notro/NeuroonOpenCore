/**
 * @file    MicroScenarioSequenceLibrary.h
 * @Author  Wojciech Weclewski <w.weclewski@inteliclinic.com>
 * @date    May, 2017
 * @brief   Brief description
 *
 * Description
 */

#ifndef MICROSCENARIOSEQUENCELIBRARY_H
#define MICROSCENARIOSEQUENCELIBRARY_H

#include <vector>
#include "NeuroonMaskScenariosApi.h"

typedef enum{
  WU_ALARM_SOFT = 0x01,
  WU_ALARM_MEDIUM,
  WU_ALARM_HARD,
  WU_ALARM_OFF
}ncWakeUpSequenceIntensity;

namespace sequence{

  std::vector<ncAtomicInstruction> descendSequence(unsigned int);
  std::vector<ncAtomicInstruction> parametrizedSinusLikeSequence(unsigned int, unsigned int,
      unsigned int, unsigned int, unsigned int);
  std::vector<ncAtomicInstruction> artificialDawnSequence(unsigned int);
  std::vector<ncAtomicInstruction> wakeUpSequence(ncWakeUpSequenceIntensity);
  std::vector<ncAtomicInstruction> lucidDreamSequence();
  std::vector<ncAtomicInstruction> circadianShiftSequence();
  std::vector<ncAtomicInstruction> noContactSequence();
  std::vector<ncAtomicInstruction> killSequence();
}

#endif /* !MICROSCENARIOSEQUENCELIBRARY_H */