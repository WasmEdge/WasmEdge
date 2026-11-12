// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "common/spdlog.h"

#include <atomic>
#include <gtest/gtest.h>
#include <string_view>

namespace {
using namespace std::literals;

// ---------------------------------------------------------------------------
// Group 1 — setLoggingLevelFromString: valid inputs
// ---------------------------------------------------------------------------

TEST(SpdlogTest, SetLevelFromString_Trace) {
  EXPECT_TRUE(WasmEdge::Log::setLoggingLevelFromString("trace"sv));
  EXPECT_EQ(spdlog::get_level(), spdlog::level::trace);
}

TEST(SpdlogTest, SetLevelFromString_Debug) {
  EXPECT_TRUE(WasmEdge::Log::setLoggingLevelFromString("debug"sv));
  EXPECT_EQ(spdlog::get_level(), spdlog::level::debug);
}

TEST(SpdlogTest, SetLevelFromString_Info) {
  EXPECT_TRUE(WasmEdge::Log::setLoggingLevelFromString("info"sv));
  EXPECT_EQ(spdlog::get_level(), spdlog::level::info);
}

TEST(SpdlogTest, SetLevelFromString_Warn) {
  EXPECT_TRUE(WasmEdge::Log::setLoggingLevelFromString("warn"sv));
  EXPECT_EQ(spdlog::get_level(), spdlog::level::warn);
}

TEST(SpdlogTest, SetLevelFromString_Warning) {
  EXPECT_TRUE(WasmEdge::Log::setLoggingLevelFromString("warning"sv));
  EXPECT_EQ(spdlog::get_level(), spdlog::level::warn);
}

TEST(SpdlogTest, SetLevelFromString_Error) {
  EXPECT_TRUE(WasmEdge::Log::setLoggingLevelFromString("error"sv));
  EXPECT_EQ(spdlog::get_level(), spdlog::level::err);
}

TEST(SpdlogTest, SetLevelFromString_Critical) {
  EXPECT_TRUE(WasmEdge::Log::setLoggingLevelFromString("critical"sv));
  EXPECT_EQ(spdlog::get_level(), spdlog::level::critical);
}

// "fatal" is also accepted as an alias for critical per the implementation.
TEST(SpdlogTest, SetLevelFromString_Fatal) {
  EXPECT_TRUE(WasmEdge::Log::setLoggingLevelFromString("fatal"sv));
  EXPECT_EQ(spdlog::get_level(), spdlog::level::critical);
}

TEST(SpdlogTest, SetLevelFromString_Off) {
  EXPECT_TRUE(WasmEdge::Log::setLoggingLevelFromString("off"sv));
  EXPECT_EQ(spdlog::get_level(), spdlog::level::off);
}

// ---------------------------------------------------------------------------
// Group 2 — setLoggingLevelFromString: invalid inputs
// ---------------------------------------------------------------------------

TEST(SpdlogTest, SetLevelFromString_EmptyString) {
  // Reset to a known level first so we can check it doesn't change.
  WasmEdge::Log::setLoggingLevelFromString("info"sv);
  EXPECT_FALSE(WasmEdge::Log::setLoggingLevelFromString(""sv));
  // Level must remain unchanged.
  EXPECT_EQ(spdlog::get_level(), spdlog::level::info);
}

TEST(SpdlogTest, SetLevelFromString_InvalidString) {
  WasmEdge::Log::setLoggingLevelFromString("info"sv);
  EXPECT_FALSE(WasmEdge::Log::setLoggingLevelFromString("invalid"sv));
  EXPECT_EQ(spdlog::get_level(), spdlog::level::info);
}

// The comparison uses == on string_view so it is case-sensitive.
TEST(SpdlogTest, SetLevelFromString_UppercaseDebug) {
  WasmEdge::Log::setLoggingLevelFromString("info"sv);
  EXPECT_FALSE(WasmEdge::Log::setLoggingLevelFromString("DEBUG"sv));
  EXPECT_EQ(spdlog::get_level(), spdlog::level::info);
}

TEST(SpdlogTest, SetLevelFromString_UppercaseInfo) {
  WasmEdge::Log::setLoggingLevelFromString("warn"sv);
  EXPECT_FALSE(WasmEdge::Log::setLoggingLevelFromString("INFO"sv));
  EXPECT_EQ(spdlog::get_level(), spdlog::level::warn);
}

TEST(SpdlogTest, SetLevelFromString_MixedCase) {
  WasmEdge::Log::setLoggingLevelFromString("info"sv);
  EXPECT_FALSE(WasmEdge::Log::setLoggingLevelFromString("Error"sv));
  EXPECT_EQ(spdlog::get_level(), spdlog::level::info);
}

TEST(SpdlogTest, SetLevelFromString_WhitespaceString) {
  WasmEdge::Log::setLoggingLevelFromString("info"sv);
  EXPECT_FALSE(WasmEdge::Log::setLoggingLevelFromString(" "sv));
  EXPECT_EQ(spdlog::get_level(), spdlog::level::info);
}

TEST(SpdlogTest, SetLevelFromString_StringWithSpaces) {
  WasmEdge::Log::setLoggingLevelFromString("info"sv);
  EXPECT_FALSE(WasmEdge::Log::setLoggingLevelFromString(" debug "sv));
  EXPECT_EQ(spdlog::get_level(), spdlog::level::info);
}

// ---------------------------------------------------------------------------
// Group 3 — Level setter functions
// ---------------------------------------------------------------------------

TEST(SpdlogTest, LevelSetter_SetLogOff) {
  // Must not throw or crash.
  EXPECT_NO_THROW(WasmEdge::Log::setLogOff());
  EXPECT_EQ(spdlog::get_level(), spdlog::level::off);
}

TEST(SpdlogTest, LevelSetter_SetTrace) {
  EXPECT_NO_THROW(WasmEdge::Log::setTraceLoggingLevel());
  EXPECT_EQ(spdlog::get_level(), spdlog::level::trace);
}

TEST(SpdlogTest, LevelSetter_SetDebug) {
  EXPECT_NO_THROW(WasmEdge::Log::setDebugLoggingLevel());
  EXPECT_EQ(spdlog::get_level(), spdlog::level::debug);
}

TEST(SpdlogTest, LevelSetter_SetInfo) {
  EXPECT_NO_THROW(WasmEdge::Log::setInfoLoggingLevel());
  EXPECT_EQ(spdlog::get_level(), spdlog::level::info);
}

TEST(SpdlogTest, LevelSetter_SetWarn) {
  EXPECT_NO_THROW(WasmEdge::Log::setWarnLoggingLevel());
  EXPECT_EQ(spdlog::get_level(), spdlog::level::warn);
}

TEST(SpdlogTest, LevelSetter_SetError) {
  EXPECT_NO_THROW(WasmEdge::Log::setErrorLoggingLevel());
  EXPECT_EQ(spdlog::get_level(), spdlog::level::err);
}

TEST(SpdlogTest, LevelSetter_SetCritical) {
  EXPECT_NO_THROW(WasmEdge::Log::setCriticalLoggingLevel());
  EXPECT_EQ(spdlog::get_level(), spdlog::level::critical);
}

// Repeated calls to the same setter must be idempotent.
TEST(SpdlogTest, LevelSetter_Idempotent) {
  WasmEdge::Log::setDebugLoggingLevel();
  WasmEdge::Log::setDebugLoggingLevel();
  EXPECT_EQ(spdlog::get_level(), spdlog::level::debug);
}

// ---------------------------------------------------------------------------
// Group 4 — setLoggingCallback
// ---------------------------------------------------------------------------

TEST(SpdlogTest, SetLoggingCallback_NullptrDoesNotCrash) {
  // Passing nullptr must reset to the default color logger without crashing.
  EXPECT_NO_THROW(WasmEdge::Log::setLoggingCallback(nullptr));
}

TEST(SpdlogTest, SetLoggingCallback_ValidCallbackDoesNotCrash) {
  auto Cb = [](const spdlog::details::log_msg &) {};
  EXPECT_NO_THROW(WasmEdge::Log::setLoggingCallback(Cb));
  // Reset to default afterwards.
  WasmEdge::Log::setLoggingCallback(nullptr);
}

TEST(SpdlogTest, SetLoggingCallback_CallbackIsInvoked) {
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

TEST(SpdlogTest, SetLoggingCallback_CallbackReceivesCorrectLevel) {
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

TEST(SpdlogTest, SetLoggingCallback_NullptrAfterCallbackResetsLogger) {
  // Install a callback, then remove it — must not crash and must restore a
  // functioning default logger.
  auto Cb = [](const spdlog::details::log_msg &) {};
  WasmEdge::Log::setLoggingCallback(Cb);
  EXPECT_NO_THROW(WasmEdge::Log::setLoggingCallback(nullptr));
  // After reset the global level API should still work normally.
  WasmEdge::Log::setInfoLoggingLevel();
  EXPECT_EQ(spdlog::get_level(), spdlog::level::info);
}

TEST(SpdlogTest, SetLoggingCallback_ReplaceCallback) {
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

// ---------------------------------------------------------------------------
// Group 5 — Edge cases
// ---------------------------------------------------------------------------

TEST(SpdlogTest, EdgeCase_SetLevelFromStringMultipleTimes) {
  EXPECT_TRUE(WasmEdge::Log::setLoggingLevelFromString("trace"sv));
  EXPECT_TRUE(WasmEdge::Log::setLoggingLevelFromString("debug"sv));
  EXPECT_TRUE(WasmEdge::Log::setLoggingLevelFromString("info"sv));
  EXPECT_TRUE(WasmEdge::Log::setLoggingLevelFromString("warn"sv));
  EXPECT_TRUE(WasmEdge::Log::setLoggingLevelFromString("error"sv));
  EXPECT_TRUE(WasmEdge::Log::setLoggingLevelFromString("critical"sv));
  EXPECT_TRUE(WasmEdge::Log::setLoggingLevelFromString("off"sv));
  EXPECT_EQ(spdlog::get_level(), spdlog::level::off);
}

TEST(SpdlogTest, EdgeCase_CallbackSetResetSet) {
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

TEST(SpdlogTest, EdgeCase_LevelSetterAfterStringParser) {
  // Mix setLoggingLevelFromString and direct setters to verify both
  // target the same global spdlog level.
  EXPECT_TRUE(WasmEdge::Log::setLoggingLevelFromString("trace"sv));
  WasmEdge::Log::setErrorLoggingLevel();
  EXPECT_EQ(spdlog::get_level(), spdlog::level::err);

  WasmEdge::Log::setDebugLoggingLevel();
  EXPECT_TRUE(WasmEdge::Log::setLoggingLevelFromString("critical"sv));
  EXPECT_EQ(spdlog::get_level(), spdlog::level::critical);
}

} // namespace
