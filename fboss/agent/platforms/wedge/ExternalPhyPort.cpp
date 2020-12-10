/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/platforms/wedge/ExternalPhyPort.h"
#include <folly/logging/xlog.h>
#include <thrift/lib/cpp/util/EnumUtils.h>
#include "fboss/agent/platforms/wedge/facebook/ExternalPhyPlatform.h"
#include "fboss/agent/platforms/wedge/facebook/ExternalPhyPortStatsUtils.h"
#include "fboss/agent/platforms/wedge/facebook/NullPortStats.h"
#include "fboss/agent/platforms/wedge/facebook/elbert/ElbertPlatform.h"
#include "fboss/agent/platforms/wedge/facebook/fuji/FujiPlatform.h"
#include "fboss/agent/platforms/wedge/facebook/minipack/MinipackPlatform.h"
#include "fboss/agent/platforms/wedge/facebook/minipack/MinipackPortStats.h"
#include "fboss/agent/platforms/wedge/facebook/yamp/YampPlatform.h"
#include "fboss/agent/platforms/wedge/facebook/yamp/YampPortStats.h"
#include "fboss/lib/config/PlatformConfigUtils.h"

namespace facebook::fboss {
template <typename PlatformT, typename PortStatsT>
void ExternalPhyPort<PlatformT, PortStatsT>::portChanged(
    std::shared_ptr<Port> oldPort,
    std::shared_ptr<Port> newPort,
    WedgePort* platPort) {
  if (!newPort->isEnabled()) {
    // No need to mess with disabled ports
    XLOG(DBG1) << "Skip reprogramming platform port on disabled port: "
               << newPort->getName();
    return;
  }

  auto profileID = newPort->getProfileID();
  if (profileID == cfg::PortProfileID::PROFILE_DEFAULT) {
    throw FbossError("Found default profile for port ", newPort->getName());
  }

  auto enabling = (!oldPort || !oldPort->isEnabled());
  auto changingSpeed = (!oldPort || profileID != oldPort->getProfileID());
  auto changingPrbsState = oldPort &&
      (oldPort->getGbSystemPrbs() != newPort->getGbSystemPrbs() ||
       oldPort->getGbLinePrbs() != newPort->getGbLinePrbs());

  if (!enabling && !changingSpeed && !changingPrbsState) {
    XLOG(DBG1) << "No need to reprogram " << newPort->getName();
    return;
  }

  auto platformPortEntry = platPort->getPlatformPortEntry();
  if (!platformPortEntry.has_value()) {
    throw FbossError("No PlatformPortEntry found for ", newPort->getName());
  }

  auto platformPortConfig =
      platformPortEntry.value().supportedProfiles_ref()->find(profileID);
  if (platformPortConfig ==
      platformPortEntry.value().supportedProfiles_ref()->end()) {
    throw FbossError(
        "No speed profile with id ",
        apache::thrift::util::enumNameSafe(profileID),
        " found in PlatformPortEntry for ",
        newPort->getName());
  }

  folly::EventBase evb;
  auto platform = dynamic_cast<PlatformT*>(platPort->getPlatform());
  std::optional<ExtendedSpecComplianceCode> transceiverSpecComplianceCode =
      platform->needExtendedSpecComplianceCode()
      ? platPort->getTransceiverExtendedSpecCompliance(&evb).getVia(&evb)
      : std::nullopt;
  auto portProfileConfig =
      platform->getPortProfileConfig(profileID, transceiverSpecComplianceCode);
  if (!portProfileConfig.has_value()) {
    throw FbossError(
        "No port profile with id ",
        apache::thrift::util::enumNameSafe(profileID),
        " found in PlatformConfig for ",
        newPort->getName());
  }

  const auto& chips = platform->getDataPlanePhyChips();
  if (chips.empty()) {
    throw FbossError("No DataPlanePhyChips found");
  }

  phy::PhyPortConfig phyPortConfig;
  phyPortConfig.config = phy::ExternalPhyConfig::fromConfigeratorTypes(
      *platformPortConfig->second.pins_ref(),
      utility::getXphyLinePolaritySwapMap(
          *platformPortEntry->mapping_ref()->pins_ref(), chips));
  phyPortConfig.profile =
      phy::ExternalPhyProfileConfig::fromPortProfileConfig(*portProfileConfig);

  auto xphy = platform->xphy(phyID_);
  xphy->programOnePort(phyPortConfig);

  if (changingPrbsState) {
    XLOG(INFO) << "Trying to setPortPrbs for port " << newPort->getID();

    auto setupPortPrbsAndCollection =
        [&](phy::Side side, bool enable, int32_t polynominal) {
          xphy->setPortPrbs(phyPortConfig, side, enable, polynominal);
          auto laneSpeed = xphy->getLaneSpeed(phyPortConfig, side);
          auto lockedStats = portStats_.wlock();
          if (!lockedStats->has_value()) {
            lockedStats->emplace(newPort->getName());
          }
          (*lockedStats)->setupPrbsCollection(phyPortConfig, side, laneSpeed);
        };

    if (oldPort->getGbSystemPrbs() != newPort->getGbSystemPrbs()) {
      auto newGbSystemPrbsState = newPort->getGbSystemPrbs();
      setupPortPrbsAndCollection(
          phy::Side::SYSTEM,
          *newGbSystemPrbsState.enabled_ref(),
          *newGbSystemPrbsState.polynominal_ref());
    }

    if (oldPort->getGbLinePrbs() != newPort->getGbLinePrbs()) {
      auto newGbLinePrbsState = newPort->getGbLinePrbs();
      setupPortPrbsAndCollection(
          phy::Side::LINE,
          *newGbLinePrbsState.enabled_ref(),
          *newGbLinePrbsState.polynominal_ref());
    }
  }

  xphyConfig_ = phyPortConfig;
}

template class ExternalPhyPort<MinipackPlatform, MinipackPortStats>;
template class ExternalPhyPort<YampPlatform, YampPortStats>;
template class ExternalPhyPort<FujiPlatform, NullPortStats>;
template class ExternalPhyPort<ElbertPlatform, NullPortStats>;
} // namespace facebook::fboss