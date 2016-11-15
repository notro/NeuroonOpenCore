/**
 * This file contains the implementation of the public API for the
 * signal streamining part of the library. The implementation uses C++ constructions
 * please make sure this constructions do not 'leak' to the header
 * file containing the API, as the API is supposed to be pure C.
 *
 * @author Tomasz Grel, t.grel@inteliclinic.com
 * @date October 2016
 */

#include <memory>
#include <sstream>

#include "NeuroonSignalStreamApi.h"
#include "AlgCoreDaemon.h"
#include "OnlineStagingAlgorithm.h"
#include "OnlinePresentationAlgorithm.h"
#include "logger.h"

struct NeuroonSignalProcessingState {
	AlgCoreDaemon _daemon;
	OnlinePresentationAlgorithm* _online_presentation;
};

struct logging_sink : public OnlineStagingAlgorithm::sink_t {
	SleepStagingResult m_last_staging;

	virtual ~logging_sink() {}

	void consume(SleepStagingResult& res) {
		m_last_staging = res;

		std::stringstream ss;
		for (int i = 0; i != res.m_stages.size(); ++i) {
			ss << res.m_stages[i].stage << " ";
		}
		LOG(INFO) << "online staging: " << ss.str();
	}
};

struct callback_staging_sink : public OnlineStagingAlgorithm::sink_t {
	virtual ~callback_staging_sink() {}

	staging_callback_t _callback;
	callback_staging_sink(staging_callback_t callback) {
		_callback = callback;
	}

	void consume(SleepStagingResult& res) {
		(*_callback)(res.m_stages.data(), res.m_stages.size());
	}
};

struct callback_presentation_sink : public OnlinePresentationAlgorithm::sink_t {
	virtual ~callback_presentation_sink() {}

	presentation_callback_t _callback;
	callback_presentation_sink(presentation_callback_t callback) {
		_callback = callback;
	}

	void consume(OnlinePresentationResult& res) {
		(*_callback)(res.brain_waves, res.bw_size,
				     res.heart_rate, res.pulse_data, res.pd_size);
	}
};

NeuroonSignalProcessingState* initialize_neuroon_alg_core(staging_callback_t staging_callback,
                                                          signal_quality_callback_t sq_callback,
                                                          presentation_callback_t presentation_callback) {
	LOG(INFO) << "API CALL";
	NeuroonSignalProcessingState* data = new NeuroonSignalProcessingState();
	data->_online_presentation = nullptr;

	logging_sink* ls = new logging_sink();
	callback_staging_sink* css = new callback_staging_sink(staging_callback);
	auto online_alg = std::unique_ptr<IStreamingAlgorithm>(new OnlineStagingAlgorithm({ls, css}));
	data->_daemon.add_streaming_algorithms(online_alg);

	if (reinterpret_cast<long> (presentation_callback) != 0) {
		callback_presentation_sink* ps = new callback_presentation_sink(presentation_callback);
		auto presentation_alg = new OnlinePresentationAlgorithm({ps});
		auto alg_ptr = std::unique_ptr<IStreamingAlgorithm>(presentation_alg);
		data->_daemon.add_streaming_algorithms(alg_ptr);
		data->_online_presentation = presentation_alg;
	}

	LOG(INFO) << "API CALL END";
	return data;
}

bool destroy_neuroon_alg_core(NeuroonSignalProcessingState* data) {
	LOG(INFO) << "API CALL";
	delete data;
	LOG(INFO) << "API CALL END";
    return true;
}

bool start_sleep(NeuroonSignalProcessingState* data) {
	LOG(INFO) << "API CALL";
	data->_daemon.start_processing();
	LOG(INFO) << "API CALL END";
    return true;
}

bool stop_sleep(NeuroonSignalProcessingState* data) {
	LOG(INFO) << "API CALL";
	data->_daemon.end_processing();
	LOG(INFO) << "API CALL END";
    return true;
}

bool feed_data_stream0(NeuroonSignalProcessingState* data, char* bytes, int size) {
	LOG(DEBUG) << "API CALL";

	NeuroonFrameBytes frame;
	frame.bytes = bytes;
	frame.size = size;
	frame.source_stream = NeuroonFrameBytes::SourceStream::EEG;
	data->_daemon.consume(frame);

	LOG(DEBUG) << "API CALL END";
    return true;
}

bool feed_data_stream1(NeuroonSignalProcessingState* data, char* bytes, int size) {
	LOG(DEBUG) << "API CALL";

	NeuroonFrameBytes frame;
	frame.bytes = bytes;
	frame.size = size;
	frame.source_stream = NeuroonFrameBytes::SourceStream::ALT;
	data->_daemon.consume(frame);

	LOG(DEBUG) << "API CALL END";
    return true;
}

bool feed_data_stream2(NeuroonSignalProcessingState* data, char* bytes, int size) {
	LOG(DEBUG) << "API CALL -- NOT USED CURRENTLY";
    return true;
}

bool install_log_callback(NeuroonSignalProcessingState* data, logger_callback_t callback) {
	LOG(INFO) << "API CALL";
	configure_logger(callback);
	LOG(INFO) << "API CALL END";
    return true;
}

bool start_presentation(NeuroonSignalProcessingState* data) {
	LOG(INFO) << "API CALL";

	if (!data->_online_presentation) {
		return false;
	}

	data->_online_presentation->activate();
	LOG(INFO) << "API CALL END";
    return true;
}

bool stop_presentation(NeuroonSignalProcessingState* data) {
	LOG(INFO) << "API CALL";

	if (!data->_online_presentation) {
		return false;
	}

	data->_online_presentation->deactivate();
	LOG(INFO) << "API CALL END";
    return true;
}