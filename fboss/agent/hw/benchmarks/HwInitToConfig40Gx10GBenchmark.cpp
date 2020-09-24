/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/benchmarks/HwInitToConfigBenchmarkHelper.h"

namespace facebook::fboss {

INIT_TO_CONFIG_BENCHMARK_HELPER(
    HwInitToConfig40Gx10GBenchmark,
    cfg::PortSpeed::FORTYG,
    cfg::PortSpeed::XG);

} // namespace facebook::fboss