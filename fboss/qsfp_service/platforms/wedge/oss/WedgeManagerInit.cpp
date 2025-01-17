/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/qsfp_service/platforms/wedge/WedgeManagerInit.h"

#include "fboss/qsfp_service/TransceiverManager.h"

namespace facebook {
namespace fboss {
std::unique_ptr<TransceiverManager> createFBTransceiverManager(
    std::unique_ptr<PlatformProductInfo> /*productInfo*/) {
  return std::unique_ptr<TransceiverManager>{};
}

std::unique_ptr<TransceiverManager> createYampTransceiverManager() {
  return std::unique_ptr<TransceiverManager>{};
}

std::unique_ptr<TransceiverManager> createElbertTransceiverManager() {
  return std::unique_ptr<TransceiverManager>{};
}

std::shared_ptr<FbossMacsecHandler> createFbossMacsecHandler(
    TransceiverManager* /* wedgeMgr */) {
  return nullptr;
}

} // namespace fboss
} // namespace facebook
