#include <gtest/gtest.h>

GTEST_API_ int main(int Argc, char **Argv) {
  testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
