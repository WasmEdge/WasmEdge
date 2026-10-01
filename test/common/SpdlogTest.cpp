// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "common/spdlog.h"

#include <atomic>
#include <gtest/gtest.h>
#include <string_view>

namespace {
using namespace std::literals;

TEST(SpdlogTest, SetLevelFromString) {
  // Test setting the log level from a string.
  //
  //   1.  Trace.
  //   2.  Debug.
  //   3.  Info.
  //   4.  Warn.
  //   5.  Warning.
  //   6.  Error.
  //   7.  Critical.
  //   8.  Fatal.
  //   9.  Off.
  //   10. Empty string.
  //   11. Invalid string.
  //   12. Uppercase debug.
  //   13. Uppercase info.
  //   14. Mixed case.
  //   15. Whitespace string.
  //   16. With spaces.
  //   17. Repeated.

  EXPECT_TRUE(WasmEdge::Log::setLoggingLevelFromString("trace"sv));
  EXPECT_EQ(spdlog::get_level(), spdlog::level::trace);

  EXPECT_TRUE(WasmEdge::Log::setLoggingLevelFromString("debug"sv));
  EXPECT_EQ(spdlog::get_level(), spdlog::level::debug);

  EXPECT_TRUE(WasmEdge::Log::setLoggingLevelFromString("info"sv));
  EXPECT_EQ(spdlog::get_level(), spdlog::level::info);

  EXPECT_TRUE(WasmEdge::Log::setLoggingLevelFromString("warn"sv));
  EXPECT_EQ(spdlog::get_level(), spdlog::level::warn);

  EXPECT_TRUE(WasmEdge::Log::setLoggingLevelFromString("warning"sv));
  EXPECT_EQ(spdlog::get_level(), spdlog::level::warn);

  EXPECT_TRUE(WasmEdge::Log::setLoggingLevelFromString("error"sv));
  EXPECT_EQ(spdlog::get_level(), spdlog::level::err);

  EXPECT_TRUE(WasmEdge::Log::setLoggingLevelFromString("critical"sv));
  EXPECT_EQ(spdlog::get_level(), spdlog::level::critical);

  // "fatal" is also accepted as an alias for critical per the implementation.
  EXPECT_TRUE(WasmEdge::Log::setLoggingLevelFromString("fatal"sv));
  EXPECT_EQ(spdlog::get_level(), spdlog::level::critical);

  EXPECT_TRUE(WasmEdge::Log::setLoggingLevelFromString("off"sv));
  EXPECT_EQ(spdlog::get_level(), spdlog::level::off);

  // Reset to a known level first so we can check it doesn't change.
  WasmEdge::Log::setLoggingLevelFromString("info"sv);
  EXPECT_FALSE(WasmEdge::Log::setLoggingLevelFromString(""sv));
  // Level must remain unchanged.
  EXPECT_EQ(spdlog::get_level(), spdlog::level::info);

  WasmEdge::Log::setLoggingLevelFromString("info"sv);
  EXPECT_FALSE(WasmEdge::Log::setLoggingLevelFromString("invalid"sv));
  EXPECT_EQ(spdlog::get_level(), spdlog::level::info);

  // The comparison uses == on string_view so it is case-sensitive.
  WasmEdge::Log::setLoggingLevelFromString("info"sv);
  EXPECT_FALSE(WasmEdge::Log::setLoggingLevelFromString("DEBUG"sv));
  EXPECT_EQ(spdlog::get_level(), spdlog::level::info);

  WasmEdge::Log::setLoggingLevelFromString("warn"sv);
  EXPECT_FALSE(WasmEdge::Log::setLoggingLevelFromString("INFO"sv));
  EXPECT_EQ(spdlog::get_level(), spdlog::level::warn);

  WasmEdge::Log::setLoggingLevelFromString("info"sv);
  EXPECT_FALSE(WasmEdge::Log::setLoggingLevelFromString("Error"sv));
  EXPECT_EQ(spdlog::get_level(), spdlog::level::info);

  WasmEdge::Log::setLoggingLevelFromString("info"sv);
  EXPECT_FALSE(WasmEdge::Log::setLoggingLevelFromString(" "sv));
  EXPECT_EQ(spdlog::get_level(), spdlog::level::info);

  WasmEdge::Log::setLoggingLevelFromString("info"sv);
  EXPECT_FALSE(WasmEdge::Log::setLoggingLevelFromString(" debug "sv));
  EXPECT_EQ(spdlog::get_level(), spdlog::level::info);

  EXPECT_TRUE(WasmEdge::Log::setLoggingLevelFromString("trace"sv));
  EXPECT_TRUE(WasmEdge::Log::setLoggingLevelFromString("debug"sv));
  EXPECT_TRUE(WasmEdge::Log::setLoggingLevelFromString("info"sv));
  EXPECT_TRUE(WasmEdge::Log::setLoggingLevelFromString("warn"sv));
  EXPECT_TRUE(WasmEdge::Log::setLoggingLevelFromString("error"sv));
  EXPECT_TRUE(WasmEdge::Log::setLoggingLevelFromString("critical"sv));
  EXPECT_TRUE(WasmEdge::Log::setLoggingLevelFromString("off"sv));
  EXPECT_EQ(spdlog::get_level(), spdlog::level::off);
}

TEST(SpdlogTest, LevelSetter) {
  // Test the log level setters.
  //
  //   1.  Set log off.
  //   2.  Set trace.
  //   3.  Set debug.
  //   4.  Set info.
  //   5.  Set warn.
  //   6.  Set error.
  //   7.  Set critical.
  //   8.  Idempotent.
  //   9.  After string parser.

  // Must not throw or crash.
  EXPECT_NO_THROW(WasmEdge::Log::setLogOff());
  EXPECT_EQ(spdlog::get_level(), spdlog::level::off);

  EXPECT_NO_THROW(WasmEdge::Log::setTraceLoggingLevel());
  EXPECT_EQ(spdlog::get_level(), spdlog::level::trace);

  EXPECT_NO_THROW(WasmEdge::Log::setDebugLoggingLevel());
  EXPECT_EQ(spdlog::get_level(), spdlog::level::debug);

  EXPECT_NO_THROW(WasmEdge::Log::setInfoLoggingLevel());
  EXPECT_EQ(spdlog::get_level(), spdlog::level::info);

  EXPECT_NO_THROW(WasmEdge::Log::setWarnLoggingLevel());
  EXPECT_EQ(spdlog::get_level(), spdlog::level::warn);

  EXPECT_NO_THROW(WasmEdge::Log::setErrorLoggingLevel());
  EXPECT_EQ(spdlog::get_level(), spdlog::level::err);

  EXPECT_NO_THROW(WasmEdge::Log::setCriticalLoggingLevel());
  EXPECT_EQ(spdlog::get_level(), spdlog::level::critical);

  // Repeated calls to the same setter must be idempotent.
  WasmEdge::Log::setDebugLoggingLevel();
  WasmEdge::Log::setDebugLoggingLevel();
  EXPECT_EQ(spdlog::get_level(), spdlog::level::debug);

  // Mix setLoggingLevelFromString and direct setters to verify both
  // target the same global spdlog level.
  EXPECT_TRUE(WasmEdge::Log::setLoggingLevelFromString("trace"sv));
  WasmEdge::Log::setErrorLoggingLevel();
  EXPECT_EQ(spdlog::get_level(), spdlog::level::err);

  WasmEdge::Log::setDebugLoggingLevel();
  EXPECT_TRUE(WasmEdge::Log::setLoggingLevelFromString("critical"sv));
  EXPECT_EQ(spdlog::get_level(), spdlog::level::critical);
}

TEST(SpdlogTest, LoggingCallback) {
  // Test the logging callback.
  //
  //   1.  Accepts nullptr.
  //   2.  Accepts callback.
  //   3.  Is invoked.
  //   4.  Receives level.
  //   5.  Nullptr resets logger.
  //   6.  Replaced.
  //   7.  Set, reset, and set again.

  // Passing nullptr must reset to the default color logger without crashing.
  EXPECT_NO_THROW(WasmEdge::Log::setLoggingCallback(nullptr));

  // Accepts callback.
  {
    auto Cb = [](const spdlog::details::log_msg &) {};
    EXPECT_NO_THROW(WasmEdge::Log::setLoggingCallback(Cb));
    // Reset to default afterwards.
    WasmEdge::Log::setLoggingCallback(nullptr);
  }

  // Is invoked.
  {
    std::atomic<int> CallCount{0};

    // Install the callback and enable the level so the message is delivered.
    WasmEdge::Log::setLoggingCallback(
        [&CallCount](const spdlog::details::log_msg &) { ++CallCount; });
    WasmEdge::Log::setErrorLoggingLevel();

    // Emit a log message at a level that will be delivered.
    spdlog::error("spdlogTest callback probe"sv);

    EXPECT_GT(CallCount.load(), 0);

    // Reset to default logger.
    WasmEdge::Log::setLoggingCallback(nullptr);
  }

  // Receives level.
  {
    spdlog::level::level_enum Captured = spdlog::level::off;

    WasmEdge::Log::setLoggingCallback(
        [&Captured](const spdlog::details::log_msg &Msg) {
          Captured = Msg.level;
        });
    WasmEdge::Log::setWarnLoggingLevel();

    spdlog::warn("level probe"sv);

    EXPECT_EQ(Captured, spdlog::level::warn);

    WasmEdge::Log::setLoggingCallback(nullptr);
  }

  {
    // Install a callback, then remove it — must not crash and must restore a
    // functioning default logger.
    auto Cb = [](const spdlog::details::log_msg &) {};
    WasmEdge::Log::setLoggingCallback(Cb);
    EXPECT_NO_THROW(WasmEdge::Log::setLoggingCallback(nullptr));
    // After reset the global level API should still work normally.
    WasmEdge::Log::setInfoLoggingLevel();
    EXPECT_EQ(spdlog::get_level(), spdlog::level::info);
  }

  // Replaced.
  {
    std::atomic<int> FirstCount{0};
    std::atomic<int> SecondCount{0};

    WasmEdge::Log::setLoggingCallback(
        [&FirstCount](const spdlog::details::log_msg &) { ++FirstCount; });
    WasmEdge::Log::setErrorLoggingLevel();
    spdlog::error("first callback"sv);

    // Replace with a second callback.
    WasmEdge::Log::setLoggingCallback(
        [&SecondCount](const spdlog::details::log_msg &) { ++SecondCount; });
    spdlog::error("second callback"sv);

    EXPECT_GT(FirstCount.load(), 0);
    EXPECT_GT(SecondCount.load(), 0);

    WasmEdge::Log::setLoggingCallback(nullptr);
  }

  // Set, reset, and set again.
  {
    std::atomic<int> Count{0};
    auto Cb = [&Count](const spdlog::details::log_msg &) { ++Count; };

    // Set → reset → set again.
    WasmEdge::Log::setLoggingCallback(Cb);
    WasmEdge::Log::setLoggingCallback(nullptr);
    WasmEdge::Log::setLoggingCallback(Cb);

    WasmEdge::Log::setErrorLoggingLevel();
    spdlog::error("edge case probe"sv);

    EXPECT_GT(Count.load(), 0);

    WasmEdge::Log::setLoggingCallback(nullptr);
  }
}

} // namespace
