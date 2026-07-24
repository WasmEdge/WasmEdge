// SPDX-License-Identifier: Apache-2.0

// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "common/configure.h"

#include "common/statistics.h"

#include <gtest/gtest.h>
#include <string>

using namespace WasmEdge;

static std::string captureStdout(const Statistics::Statistics &Stats,
                                 const Configure &Conf) {
  testing::internal::CaptureStdout();
  Stats.dumpToLog(Conf);
  return testing::internal::GetCapturedStdout();
}

TEST(StatsJsonTest, JsonOnlyEmitsTimingWhenOnlyTimeMeasured) {

  Configure Conf;

  Conf.getStatisticsConfigure().setTimeMeasuring(true);

  Conf.getStatisticsConfigure().setStatsOutputFormat(Configure::StatisticsConfigure::StatsOutputFormat::JSON);

  Statistics::Statistics Stats;

  std::string Out = captureStdout(Stats, Conf);

  EXPECT_NE(Out.find("total_execution_time_ns"), std::string::npos);

  EXPECT_NE(Out.find("wasm_instruction_time_ns"), std::string::npos);

  EXPECT_NE(Out.find("host_function_time_ns"), std::string::npos);

  EXPECT_EQ(Out.find("instruction_count"), std::string::npos);

  EXPECT_EQ(Out.find("gas_used"), std::string::npos);

  EXPECT_EQ(Out.find("instructions_per_second"), std::string::npos);

}

TEST(StatsJsonTest, JsonOnlyEmitsCountWhenOnlyCountMeasured) {

  Configure Conf;

  Conf.getStatisticsConfigure().setInstructionCounting(true);

  Conf.getStatisticsConfigure().setStatsOutputFormat(Configure::StatisticsConfigure::StatsOutputFormat::JSON);

  Statistics::Statistics Stats;

  std::string Out = captureStdout(Stats, Conf);

  EXPECT_NE(Out.find("instruction_count"), std::string::npos);

  EXPECT_EQ(Out.find("total_execution_time_ns"), std::string::npos);

  EXPECT_EQ(Out.find("gas_used"), std::string::npos);

  EXPECT_EQ(Out.find("instructions_per_second"), std::string::npos);

}

TEST(StatsJsonTest, IPSOnlyAppearsWhenBothTimeAndCountEnabled) {

  Configure Conf;

  Conf.getStatisticsConfigure().setTimeMeasuring(true);

  Conf.getStatisticsConfigure().setInstructionCounting(true);

  Conf.getStatisticsConfigure().setStatsOutputFormat(Configure::StatisticsConfigure::StatsOutputFormat::JSON);

  Statistics::Statistics Stats;

  std::string Out = captureStdout(Stats, Conf);

  EXPECT_NE(Out.find("instructions_per_second"), std::string::npos);

}

TEST(StatsJsonTest, AllStatsFlagsProduceAllFields) {

  Configure Conf;

  Conf.getStatisticsConfigure().setTimeMeasuring(true);

  Conf.getStatisticsConfigure().setInstructionCounting(true);

  Conf.getStatisticsConfigure().setCostMeasuring(true);

  Conf.getStatisticsConfigure().setStatsOutputFormat(Configure::StatisticsConfigure::StatsOutputFormat::JSON);

  Statistics::Statistics Stats;

  std::string Out = captureStdout(Stats, Conf);

  EXPECT_NE(Out.find("total_execution_time_ns"), std::string::npos);

  EXPECT_NE(Out.find("instruction_count"), std::string::npos);

  EXPECT_NE(Out.find("gas_used"), std::string::npos);

  EXPECT_NE(Out.find("instructions_per_second"), std::string::npos);

}

TEST(StatsJsonTest, NoJsonOutputWhenNoStatFlagEnabled) {
  Configure Conf;
  Conf.getStatisticsConfigure().setStatsOutputFormat(
      Configure::StatisticsConfigure::StatsOutputFormat::JSON);
  Statistics::Statistics Stats;
  testing::internal::CaptureStderr();
  std::string Out = captureStdout(Stats, Conf);
  std::string Err = testing::internal::GetCapturedStderr();
  EXPECT_EQ(Out, "");
  EXPECT_NE(Err.find("--stats-format=json has no effect"), std::string::npos);
}

TEST(StatsJsonTest, ValidJsonStructure) {

  Configure Conf;

  Conf.getStatisticsConfigure().setTimeMeasuring(true);

  Conf.getStatisticsConfigure().setInstructionCounting(true);

  Conf.getStatisticsConfigure().setCostMeasuring(true);

  Conf.getStatisticsConfigure().setStatsOutputFormat(Configure::StatisticsConfigure::StatsOutputFormat::JSON);

  Statistics::Statistics Stats;

  std::string Out = captureStdout(Stats, Conf);

  EXPECT_FALSE(Out.empty());

  EXPECT_EQ(Out.front(), '{');

  EXPECT_NE(Out.rfind('}'), std::string::npos);

  const auto CloseBrace = Out.rfind('}');
  ASSERT_NE(CloseBrace, std::string::npos);
  const auto BeforeClose = Out.find_last_not_of(" \t\r\n", CloseBrace - 1);
  ASSERT_NE(BeforeClose, std::string::npos);
  EXPECT_NE(Out[BeforeClose], ',');

}
