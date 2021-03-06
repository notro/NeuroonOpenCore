#include <chrono>
#include <gtest/gtest.h>
#include <memory>
#include <sstream>
#include <stdio.h>
#include <thread>

#include "../../src/DataSink.h"
#include "../../src/DataSource.h"
#include "../../src/SignalSimulator.h"
#include "test_utils.h"

#include "../../src/DataSink.h"
#include "../../src/FrameStreamPipe.h"
#include "../../src/NeuroonSignals.h"
#include "../../src/SignalSimulator.h"

struct StreamingPipelineAndCsvSimulatorTests : public ::testing::Test {

  // ------------ CSV FRAMES SOURCES -------------------

  // this file should contain values from 0 to 249
  const std::string sample_csv1 = "../../../test/test_data/sample1.csv";

  // this file should contain values from 0 to 499 as column under header signal
  const std::string sample_csv2 = "../../../test/test_data/sample2.csv";

  std::shared_ptr<EegFramesSource> eeg_source_sample1;
  std::shared_ptr<EegFramesSource> eeg_source_sample2;
  std::shared_ptr<PatFramesSource> irled_source_sample2;

  template <class T>
  LambdaSignalFrameDataSink<T> accumulate_to_vector_sink(std::vector<T> &out) {
    return LambdaSignalFrameDataSink<T>(
        [&out](T data) { out.push_back(data); });
  }

  // ------------ GOOGLE TEST'S ------------------------

  virtual void SetUp() {
    eeg_source_sample1 =
        std::unique_ptr<EegFramesSource>(new EegFramesSource(sample_csv1, 0));
    eeg_source_sample2 = std::unique_ptr<EegFramesSource>(
        new EegFramesSource(sample_csv2, "signal"));
    irled_source_sample2 = std::unique_ptr<PatFramesSource>(new PatFramesSource(
        SignalSource<std::int32_t>::csv_column(sample_csv2, "signal")));
  }

  void TearDown() {}
};

TEST_F(StreamingPipelineAndCsvSimulatorTests, frame_from_bytes_tests) {

  const std::size_t L = 20;
  const unsigned char bytes[] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xAB,
                                 0x89, 0xCD, 0xEF, 0xEF, 0xCD, 0x01, 0x23,
                                 0x45, 0x67, 0x89, 0xAB, 0xAB, 0x89};

  // big endian
  auto ef_be = EegFrame::from_bytes_array((char *)bytes, L,
                                          NeuroonFrameBytes::ByteOrder::BE);
  EXPECT_EQ(19088743, ef_be.timestamp);
  std::int16_t expected_ef_be[] = {-30293, -21623, -12817, -4147,
                                   291,    17767,  -30293, -21623};

  for (std::size_t i = 0; i < EegFrame::Length; i++) {
    EXPECT_EQ(expected_ef_be[i], ef_be.signal[i]);
  }

  auto af_be = PatFrame::from_bytes_array((char *)bytes, L,
                                          NeuroonFrameBytes::ByteOrder::BE);
  EXPECT_EQ(19088743, af_be.timestamp);
  EXPECT_EQ(-1985238135, af_be.ir_led);
  EXPECT_EQ(291, af_be.accel_axes.x);
  EXPECT_EQ(17767, af_be.accel_axes.y);
  EXPECT_EQ(-30293, af_be.accel_axes.z);
  EXPECT_EQ(-85, af_be.temperature[0]);
  EXPECT_EQ(-119, af_be.temperature[1]);

  // little endian
  auto ef_le = EegFrame::from_bytes_array((char *)bytes, 20,
                                          NeuroonFrameBytes::ByteOrder::LE);
  EXPECT_EQ(1732584193, ef_le.timestamp);
  std::int16_t expected_ef_le[] = {-21623, -30293, -4147,  -12817,
                                   8961,   26437,  -21623, -30293};
  for (std::size_t i = 0; i < EegFrame::Length; i++) {
    EXPECT_EQ(expected_ef_le[i], ef_le.signal[i]);
  }

  auto af_le = PatFrame::from_bytes_array((char *)bytes, 20,
                                          NeuroonFrameBytes::ByteOrder::LE);
  EXPECT_EQ(1732584193, af_le.timestamp);
  EXPECT_EQ(-1985238135, af_le.ir_led);
  EXPECT_EQ(8961, af_le.accel_axes.x);
  EXPECT_EQ(26437, af_le.accel_axes.y);
  EXPECT_EQ(-21623, af_le.accel_axes.z);
  EXPECT_EQ(-85, af_le.temperature[0]);
  EXPECT_EQ(-119, af_le.temperature[1]);

  // to bytes conversion
  auto af_def = PatFrame::from_bytes_array((char *)bytes, L);
  auto ef_def = EegFrame::from_bytes_array((char *)bytes, L);

  unsigned char back[L];
  ef_def.to_bytes((char *)back);
  for (std::size_t i = 0; i < L; i++) {
    EXPECT_EQ(bytes[i], back[i]);
  }
  af_def.to_bytes((char *)back);
  for (std::size_t i = 0; i < L; i++) {
    EXPECT_EQ(bytes[i], back[i]);
  }
}

TEST_F(StreamingPipelineAndCsvSimulatorTests, SimpleEegFrameSource1) {

  eeg_source_sample1->reset();
  auto frames = eeg_source_sample1->getValues();
  auto frame_length = EegFrame::Length;

  EXPECT_TRUE(frames.size() > 0);

  for (uint16_t i = 0; i < 250; i += frame_length) {
    auto &f = frames[i / frame_length];
    if (250 - i >= frame_length)
      for (uint16_t j = 0; j < frame_length; j++) {
        EXPECT_EQ(i + j, f->signal[j]);
      }
  }
}

TEST_F(StreamingPipelineAndCsvSimulatorTests, SimpleAccelLedTempFrameSource1) {

  for (auto i = 0; i < 100; i++) {
    irled_source_sample2 = std::unique_ptr<PatFramesSource>(new PatFramesSource(
        SignalSource<std::int32_t>::csv_column(sample_csv2, "signal")));
  }

  auto frames = irled_source_sample2->getValues();

  EXPECT_TRUE(frames.size() == 500);

  for (uint32_t i = 0; i < 500; i++) {
    EXPECT_EQ(i, frames[i]->ir_led);
  }
}

TEST_F(StreamingPipelineAndCsvSimulatorTests, SignalSource) {
  auto zeros_int16 = SignalSource<std::uint16_t>::zeros(5);

  auto v = zeros_int16.getValues();
  std::vector<std::uint16_t> expected = {0, 0, 0, 0, 0};
  EXPECT_EQ_VECTORS(expected, v);

  auto zeros_strings = SignalSource<std::string>::generation_from_index(
      [](std::size_t) { return "0"; }, 3);
  std::vector<std::string> expected_str = {"0", "0", "0"};
  auto v2 = zeros_strings.getValues();
  EXPECT_EQ_VECTORS(expected_str, v2);

  auto csv_header = SignalSource<int>::csv_column(sample_csv2, "signal");
  auto v3 = csv_header.getValues();
  std::vector<int> expected_csv = {};
  for (int i = 0; i < 500; i++) {
    expected_csv.push_back(i);
  }
  EXPECT_EQ_VECTORS(expected_csv, v3);
}

TEST_F(StreamingPipelineAndCsvSimulatorTests, TrivialSinkTest) {

  std::vector<int> v = {};
  auto sink = accumulate_to_vector_sink(v);
  for (int i = 0; i < 5; i++) {
    sink.consume(i);
  }
  std::vector<int> expected = {0, 1, 2, 3, 4};

  EXPECT_EQ_VECTORS(expected, v);
}

TEST_F(StreamingPipelineAndCsvSimulatorTests, SimpleFrameStreamPipe) {

  auto frames = eeg_source_sample1->getValues();
  auto frame_length = EegFrame::Length;

  std::vector<std::shared_ptr<EegFrame>> v = {};
  auto sink_sp =
      std::make_shared<LambdaSignalFrameDataSink<std::shared_ptr<EegFrame>>>(
          accumulate_to_vector_sink(v));

  eeg_source_sample1->reset();
  auto pipe = FrameStreamPipe<EegFrame>(eeg_source_sample1, sink_sp);

  for (uint16_t i = 0; i < 250 / frame_length; i++) {
    // only full frames are created
    if (250 - i * frame_length >= frame_length) {
      pipe.passNextFrameWithTimestamp(i);
      EXPECT_EQ(v.size(), i + 1);
      auto &passed = v[i];
      EXPECT_EQ(i, passed->timestamp);
      for (uint16_t j = 0; j < frame_length; j++) {
        EXPECT_EQ(frames[i]->signal[j], passed->signal[j]);
      }
    }
  }

  EXPECT_TRUE(pipe.isDepleted());

  sink_sp = nullptr;
  EXPECT_TRUE(pipe.isBroken());
}

TEST_F(StreamingPipelineAndCsvSimulatorTests,
       CsvSimSingleEegPipe_1020ms_normal_time) {

  auto frame_length = EegFrame::Length;

  std::vector<std::shared_ptr<EegFrame>> v = {};

  auto sink_sp =
      std::make_shared<LambdaSignalFrameDataSink<std::shared_ptr<EegFrame>>>(
          accumulate_to_vector_sink(v));

  eeg_source_sample1->reset();
  auto frames = eeg_source_sample1->getValues();
  eeg_source_sample1->reset();

  auto pipe_up = std::unique_ptr<IFrameStreamPipe>(
      new FrameStreamPipe<EegFrame>(eeg_source_sample1, sink_sp));

  auto sim = SignalSimulator();
  sim.add_streaming_pipe(std::move(pipe_up),
                         EegFrame::DefaultEmissionInterval_ms);

  auto start = std::chrono::high_resolution_clock::now();
  int ms_to_pass = 1020;
  sim.pass_time(ms_to_pass);

  // std::cout << "Time to pass: " << ms_to_pass << "ms.";
  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double, std::milli> elapsed = end - start;
  // std::cout << " Passed: " << elapsed.count() << "ms.\n";

  ASSERT_TRUE(elapsed.count() >= ms_to_pass);

  std::vector<int> expected = {};

  for (uint16_t i = 0; i < 120 / frame_length; i++) {
    // only full frames are created
    if (120 - i * frame_length >= frame_length) {
      auto &passed = frames[i];
      auto received = v[i];
      // EXPECT_EQ(i, passed.timestamp);
      for (uint16_t j = 0; j < frame_length; j++) {
        EXPECT_EQ(passed->signal[j], received->signal[j]);
      }
    }
  }

  // pass rest of frames instantly
  sim.pass_time(0, 0);
  for (uint16_t i = 120 / frame_length; i < 250 / frame_length; i++) {
    // only full frames are created
    if (250 - i * frame_length >= frame_length) {
      auto &passed = frames[i];
      auto &received = v[i];
      // EXPECT_EQ(i, passed.timestamp);
      for (uint16_t j = 0; j < frame_length; j++) {
        EXPECT_EQ(passed->signal[j], received->signal[j]);
      }
    }
  }

  // EXPECT_TRUE(pipe.isDepleted());

  // sink_sp = nullptr;
  // EXPECT_TRUE(pipe.is_broken());
}

TEST_F(StreamingPipelineAndCsvSimulatorTests,
       CsvSimTwoEegPipeSingleSink_instant) {

  std::vector<std::shared_ptr<EegFrame>> v = {};
  auto sink_sp =
      std::make_shared<LambdaSignalFrameDataSink<std::shared_ptr<EegFrame>>>(
          accumulate_to_vector_sink(v));

  auto pipe_up = std::unique_ptr<IFrameStreamPipe>(
      new FrameStreamPipe<EegFrame>(eeg_source_sample1, sink_sp));
  auto pipe2_up = std::unique_ptr<IFrameStreamPipe>(
      new FrameStreamPipe<EegFrame>(eeg_source_sample2, sink_sp));

  auto pipe_cheat_rp = pipe_up.get();
  auto pipe2_cheat_rp = pipe2_up.get();

  auto sim = SignalSimulator();
  sim.add_streaming_pipe(std::move(pipe_up),
                         EegFrame::DefaultEmissionInterval_ms);
  sim.add_streaming_pipe(std::move(pipe2_up),
                         EegFrame::DefaultEmissionInterval_ms);

  int ms_to_pass = 1985;

  sim.pass_time(ms_to_pass, 0);
  EXPECT_EQ(true, pipe_cheat_rp->isDepleted());
  EXPECT_EQ(false, pipe2_cheat_rp->isDepleted());

  EXPECT_EQ(62, v.size());
  sim.pass_time(0, 0);
  EXPECT_EQ(true, pipe2_cheat_rp->isDepleted());
  EXPECT_EQ(93, v.size());
}

TEST_F(StreamingPipelineAndCsvSimulatorTests,
       CsvSimTwoEegPipeSingleSource_instant) {

  std::size_t frame_count = 0;
  llong frame_sum = 0;
  auto counting_sink_sp =
      std::make_shared<LambdaSignalFrameDataSinkSp<EegFrame>>(
          LambdaSignalFrameDataSinkSp<EegFrame>(
              [&frame_count](std::shared_ptr<EegFrame>) {
                frame_count += 1;
              }));

  auto suming_sink_sp = std::make_shared<LambdaSignalFrameDataSinkSp<EegFrame>>(
      LambdaSignalFrameDataSinkSp<EegFrame>(
          [&frame_sum](std::shared_ptr<EegFrame> f) {
            for (auto v : f->signal) {
              frame_sum += v;
            }
          }));

  eeg_source_sample1->reset();
  auto pipe_up = std::unique_ptr<IFrameStreamPipe>(
      new FrameStreamPipe<EegFrame>(eeg_source_sample1, counting_sink_sp));
  auto pipe2_up = std::unique_ptr<IFrameStreamPipe>(
      new FrameStreamPipe<EegFrame>(eeg_source_sample1, suming_sink_sp));

  auto pipe_cheat_rp = pipe_up.get();
  auto pipe2_cheat_rp = pipe2_up.get();

  auto sim = SignalSimulator();
  sim.add_streaming_pipe(std::move(pipe_up),
                         EegFrame::DefaultEmissionInterval_ms / 2);
  sim.add_streaming_pipe(std::move(pipe2_up),
                         EegFrame::DefaultEmissionInterval_ms);

  sim.pass_time(0, 0);

  EXPECT_EQ(21, frame_count);
  EXPECT_EQ(10200, frame_sum);
  EXPECT_EQ(true, pipe_cheat_rp->isDepleted());
  EXPECT_EQ(true, pipe2_cheat_rp->isDepleted());
}
