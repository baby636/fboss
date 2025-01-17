/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/qsfp_service/test/hw_test/phy/HwTest.h"

#include "fboss/lib/fpga/MultiPimPlatformPimContainer.h"
#include "fboss/qsfp_service/test/hw_test/phy/HwPhyEnsemble.h"

namespace facebook::fboss {

TEST_F(HwTest, CheckPimPresent) {
  auto targetPimID = getHwPhyEnsemble()->getTargetPimID();
  EXPECT_TRUE(getPimContainer(targetPimID)->isPimPresent());
}
} // namespace facebook::fboss
