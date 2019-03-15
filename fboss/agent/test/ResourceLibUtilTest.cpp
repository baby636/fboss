// Copyright 2004-present Facebook. All Rights Reserved.

#include <gtest/gtest.h>

#include "fboss/agent/test/ResourceLibUtil.h"

namespace facebook {
namespace fboss {

TEST(ResourceLibUtilTest, IPv4Generator) {
  auto generator = utility::IPAddressGenerator<folly::IPAddressV4>();
  std::array<folly::IPAddressV4, 5> ips = {
      folly::IPAddressV4("0.0.0.1"),
      folly::IPAddressV4("0.0.0.2"),
      folly::IPAddressV4("0.0.0.3"),
      folly::IPAddressV4("0.0.0.4"),
      folly::IPAddressV4("0.0.0.5"),
  };

  for (int i = 0; i < 5; i++) {
    auto generatedIp = generator.getNext();
    ASSERT_EQ(generatedIp, ips[i]);
  }
}

TEST(ResourceLibUtilTest, HostPrefixV4Generator) {
  using IpT = folly::IPAddressV4;
  using RoutePrefixT = RoutePrefix<folly::IPAddressV4>;

  auto generator = utility::PrefixGenerator<IpT, 32>();
  std::array<RoutePrefixT, 5> hostPrefixes = {
      RoutePrefixT{IpT("0.0.0.1"), 32},
      RoutePrefixT{IpT("0.0.0.2"), 32},
      RoutePrefixT{IpT("0.0.0.3"), 32},
      RoutePrefixT{IpT("0.0.0.4"), 32},
      RoutePrefixT{IpT("0.0.0.5"), 32},
  };

  for (int i = 0; i < 5; i++) {
    auto hostPrefix = generator.getNext();
    ASSERT_EQ(hostPrefix, hostPrefixes[i]);
  }
}

TEST(ResourceLibUtilTest, LpmPrefixV4Generator) {
  using IpT = folly::IPAddressV4;
  using RoutePrefixT = RoutePrefix<folly::IPAddressV4>;

  auto generator = utility::PrefixGenerator<IpT, 24>();
  std::array<RoutePrefixT, 5> prefixes = {
      RoutePrefixT{IpT("0.0.1.0"), 24},
      RoutePrefixT{IpT("0.0.2.0"), 24},
      RoutePrefixT{IpT("0.0.3.0"), 24},
      RoutePrefixT{IpT("0.0.4.0"), 24},
      RoutePrefixT{IpT("0.0.5.0"), 24},
  };

  for (int i = 0; i < 5; i++) {
    auto prefix = generator.getNext();
    ASSERT_EQ(prefix, prefixes[i]);
  }
}

TEST(ResourceLibUtilTest, GenerateNv4Prefix) {
  using IpT = folly::IPAddressV4;
  using RoutePrefixT = RoutePrefix<folly::IPAddressV4>;

  auto generator = utility::PrefixGenerator<IpT, 24>();
  std::vector<RoutePrefixT> generatedPrefixes = generator.getNextN(5);

  std::array<RoutePrefixT, 5> prefixes = {
      RoutePrefixT{IpT("0.0.1.0"), 24},
      RoutePrefixT{IpT("0.0.2.0"), 24},
      RoutePrefixT{IpT("0.0.3.0"), 24},
      RoutePrefixT{IpT("0.0.4.0"), 24},
      RoutePrefixT{IpT("0.0.5.0"), 24},
  };

  for (int i = 0; i < 5; i++) {
    ASSERT_EQ(generatedPrefixes[i], prefixes[i]);
  }
}

TEST(ResourceLibUtilTest, GenerateResetGenerateV4) {
  using IpT = folly::IPAddressV4;
  using RoutePrefixT = RoutePrefix<folly::IPAddressV4>;

  auto generator = utility::PrefixGenerator<IpT, 24>();
  generator.getNextN(5);

  EXPECT_EQ(generator.getCursorPosition(), 5);
  generator.startOver(1);
  EXPECT_EQ(generator.getCursorPosition(), 1);
  auto expectedPrefix = RoutePrefixT{IpT("0.0.2.0"), 24};
  EXPECT_EQ(generator.getNext(), expectedPrefix);
  EXPECT_EQ(generator.getCursorPosition(), 2);
}

} // namespace fboss
} // namespace facebook
