#include "avcodec/avcodec_func.h"
#include "avcodec/module.h"
#include "common/defines.h"
#include "common/types.h"
#include "runtime/instance/module.h"

#include "gmock/gmock.h"
#include <gtest/gtest.h>

GTEST_API_ int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  testing::InitGoogleMock(&argc, argv);
  return RUN_ALL_TESTS();
}
