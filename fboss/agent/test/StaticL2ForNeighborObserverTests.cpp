/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <gtest/gtest.h>

#include "fboss/agent/GtestDefs.h"
#include "fboss/agent/L2Entry.h"
#include "fboss/agent/NeighborUpdater.h"
#include "fboss/agent/StaticL2ForNeighborObserver.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/Vlan.h"
#include "fboss/agent/test/HwTestHandle.h"
#include "fboss/agent/test/TestUtils.h"

#include <folly/IPAddressV4.h>
#include <folly/IPAddressV6.h>
#include <folly/MacAddress.h>

using folly::IPAddress;
using folly::IPAddressV4;
using folly::IPAddressV6;
using folly::MacAddress;

namespace facebook::fboss {

template <typename AddrT>
class StaticL2ForNeighorObserverTest : public ::testing::Test {
 public:
  using Func = folly::Function<void()>;
  using StateUpdateFn = SwSwitch::StateUpdateFn;

  void SetUp() override {
    handle_ = createTestHandle(testStateA());
    sw_ = handle_->getSw();
  }

  void TearDown() override {
    schedulePendingTestStateUpdates();
  }

  void updateState(folly::StringPiece name, StateUpdateFn func) {
    sw_->updateStateBlocking(name, func);
  }

  VlanID kVlan() const {
    return VlanID(1);
  }

  PortID kPortID() const {
    return PortID(1);
  }

  PortID kPortID2() const {
    return PortID(2);
  }

  IPAddressV4 kIp4Addr() const {
    return IPAddressV4("10.0.0.2");
  }

  IPAddressV6 kIp6Addr() const {
    return IPAddressV6("2401:db00:2110:3001::0002");
  }
  MacAddress kMacAddress() const {
    return MacAddress("01:02:03:04:05:06");
  }

  void resolveNeighbor(IPAddress ipAddress, MacAddress macAddress) {
    /*
     * Cause a neighbor entry to resolve by receiving appropriate ARP/NDP, and
     * assert if valid CLASSID is associated with the newly resolved neighbor.
     */
    if constexpr (std::is_same<AddrT, folly::IPAddressV4>::value) {
      sw_->getNeighborUpdater()->receivedArpMine(
          kVlan(),
          ipAddress.asV4(),
          macAddress,
          PortDescriptor(kPortID()),
          ArpOpCode::ARP_OP_REPLY);
    } else {
      sw_->getNeighborUpdater()->receivedNdpMine(
          kVlan(),
          ipAddress.asV6(),
          macAddress,
          PortDescriptor(kPortID()),
          ICMPv6Type::ICMPV6_TYPE_NDP_NEIGHBOR_ADVERTISEMENT,
          0);
    }

    sw_->getNeighborUpdater()->waitForPendingUpdates();
    waitForBackgroundThread(sw_);
    waitForStateUpdates(sw_);
    sw_->getNeighborUpdater()->waitForPendingUpdates();
    waitForStateUpdates(sw_);
  }

  void unresolveNeighbor(IPAddress ipAddress) {
    sw_->getNeighborUpdater()->flushEntry(kVlan(), ipAddress);

    sw_->getNeighborUpdater()->waitForPendingUpdates();
    waitForBackgroundThread(sw_);
    waitForStateUpdates(sw_);
  }

  void resolve(IPAddress ipAddress, MacAddress macAddress) {
    if constexpr (std::is_same_v<AddrT, folly::MacAddress>) {
      resolveMac(macAddress);
    } else {
      this->resolveNeighbor(ipAddress, macAddress);
    }
  }

  void resolveMac(MacAddress macAddress) {
    auto l2Entry = L2Entry(
        macAddress,
        this->kVlan(),
        PortDescriptor(this->kPortID()),
        L2Entry::L2EntryType::L2_ENTRY_TYPE_PENDING);

    this->sw_->l2LearningUpdateReceived(
        l2Entry, L2EntryUpdateType::L2_ENTRY_UPDATE_TYPE_ADD);

    this->sw_->getNeighborUpdater()->waitForPendingUpdates();
    waitForBackgroundThread(this->sw_);
    waitForStateUpdates(this->sw_);
  }

  IPAddress getIpAddress() {
    IPAddress ipAddress;
    if constexpr (std::is_same_v<AddrT, folly::IPAddressV4>) {
      ipAddress = IPAddress(this->kIp4Addr());
    } else {
      ipAddress = IPAddress(this->kIp6Addr());
    }

    return ipAddress;
  }

  void bringPortDown(PortID portID) {
    this->sw_->linkStateChanged(portID, false);

    waitForStateUpdates(this->sw_);
    this->sw_->getNeighborUpdater()->waitForPendingUpdates();
    waitForBackgroundThread(this->sw_);
    waitForStateUpdates(this->sw_);
  }

 protected:
  void verifyMacEntryExists(MacEntryType type) const {
    waitForBackgroundThread(sw_);
    waitForStateUpdates(sw_);
    auto macEntry = getMacEntry();
    ASSERT_NE(macEntry, nullptr);
    EXPECT_EQ(macEntry->getType(), type);
  }
  void verifyMacEntryDoesNotExist() const {
    waitForBackgroundThread(sw_);
    waitForStateUpdates(sw_);
    EXPECT_EQ(getMacEntry(), nullptr);
  }

 private:
  auto getMacEntry() const {
    auto vlan = sw_->getState()->getVlans()->getVlan(kVlan());
    return vlan->getMacTable()->getNodeIf(kMacAddress());
  }
  void runInUpdateEventBaseAndWait(Func func) {
    auto* evb = sw_->getUpdateEvb();
    evb->runInEventBaseThreadAndWait(std::move(func));
  }

  void runInUpdateEvbAndWaitAfterNeighborCachePropagation(Func func) {
    schedulePendingTestStateUpdates();
    this->sw_->getNeighborUpdater()->waitForPendingUpdates();
    runInUpdateEventBaseAndWait(std::move(func));
  }

  void schedulePendingTestStateUpdates() {
    runInUpdateEventBaseAndWait([]() {});
  }

  std::unique_ptr<HwTestHandle> handle_;
  SwSwitch* sw_;
};

using TestTypes =
    ::testing::Types<folly::IPAddressV4, folly::IPAddressV6, folly::MacAddress>;
using TestTypesNeighbor =
    ::testing::Types<folly::IPAddressV4, folly::IPAddressV6>;

/*
 * Tests that are valid for arp/ndp neighbors only and not for Mac addresses
 */
template <typename AddrT>
class StaticL2ForNeighorObserverNeighborTest
    : public StaticL2ForNeighorObserverTest<AddrT> {};

TYPED_TEST_SUITE(StaticL2ForNeighorObserverNeighborTest, TestTypesNeighbor);

TYPED_TEST(
    StaticL2ForNeighorObserverNeighborTest,
    noStaticL2EntriesForUnResolvedNeighbor) {
  this->verifyMacEntryDoesNotExist();
}

TYPED_TEST(
    StaticL2ForNeighorObserverNeighborTest,
    staticL2EntriesForResolvedNeighbor) {
  this->verifyMacEntryDoesNotExist();
  this->resolve(this->getIpAddress(), this->kMacAddress());
  this->verifyMacEntryExists(MacEntryType::STATIC_ENTRY);
}

TYPED_TEST(
    StaticL2ForNeighorObserverNeighborTest,
    staticL2EntriesForUnresolvedToResolvedNeighbor) {
  this->verifyMacEntryDoesNotExist();
  this->resolve(this->getIpAddress(), this->kMacAddress());
  this->verifyMacEntryExists(MacEntryType::STATIC_ENTRY);
  this->unresolveNeighbor(this->getIpAddress());
  this->verifyMacEntryDoesNotExist();
  this->resolve(this->getIpAddress(), this->kMacAddress());
  this->verifyMacEntryExists(MacEntryType::STATIC_ENTRY);
}

} // namespace facebook::fboss