/*
 * OnlineStagingAlgorithm.h
 *
 *  Created on: Aug 23, 2016
 *      Author: tomek
 */

#ifndef SRC_SLEEP_STAGING_ONLINESTAGINGALGORITHM_H_
#define SRC_SLEEP_STAGING_ONLINESTAGINGALGORITHM_H_

#include "StreamingAlgorithm.h"
#include "OnlineStagingClassifier.h"
#include "CommonTypes.h"
#include <vector>
#include <stdexcept>

#include "NeuroonAlgCoreApi.h"

//struct sleep_stage_t {
//	SLEEP_STAGE stage;
//	llong timestamp;
//};

class SleepStagingResult {
public:
	SleepStagingResult() {}

	SleepStagingResult(const std::vector<int> &stages, const std::vector<ullong> &timestamps) {

		m_stages.resize(stages.size());

		for (int i = 0; i != stages.size(); ++i) {
			m_stages[i].stage = static_cast<SLEEP_STAGE>(stages[i]);
			m_stages[i].timestamp = timestamps[i];
		}
	}

	std::vector<staging_element_t> m_stages;
};

class OnlineStagingAlgorithm : public SinkStreamingAlgorithm<SleepStagingResult> {

	const int EEG_WINDOW = 2048 * 5;
	const int IR_WINDOW = EEG_WINDOW / 5;

	const int EEG_INTERVAL = EEG_WINDOW / 4;
	const int IR_INTERVAL = IR_WINDOW / 4;
public:

	typedef IDataSink<SleepStagingResult> sink_t;

	OnlineStagingAlgorithm(const std::vector<sink_t*> & sinks);
	virtual ~OnlineStagingAlgorithm();

	virtual void reset_state() override;
	virtual void process_input(const INeuroonSignals & input) override;
	virtual void end_streaming(const INeuroonSignals & input) override;

private:
	OnlineStagingClassifier m_model;

	int m_last_eeg_index;
	int m_last_ir_index;

	ullong m_first_timestamp;
	std::vector<ullong> m_timestamps;
};

#endif /* SRC_SLEEP_STAGING_ONLINESTAGINGALGORITHM_H_ */
