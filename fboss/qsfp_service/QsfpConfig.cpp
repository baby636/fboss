// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include "fboss/qsfp_service/QsfpConfig.h"

#include "fboss/agent/FbossError.h"

#include <folly/FileUtil.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>

DEFINE_string(
    qsfp_config,
    "/etc/coop/qsfp/current",
    "The path to the local JSON configuration file");

namespace facebook::fboss {

std::unique_ptr<QsfpConfig> QsfpConfig::fromDefaultFile() {
  return fromFile(FLAGS_qsfp_config);
}

std::unique_ptr<QsfpConfig> QsfpConfig::fromFile(folly::StringPiece path) {
  std::string configStr;
  if (!folly::readFile(path.data(), configStr)) {
    throw FbossError("unable to read ", path);
  }
  return fromRawConfig(std::move(configStr));
}

std::unique_ptr<QsfpConfig> QsfpConfig::fromRawConfig(
    const std::string& configStr) {
  cfg::QsfpServiceConfig qsfpConfig;

  apache::thrift::SimpleJSONSerializer::deserialize<cfg::QsfpServiceConfig>(
      configStr, qsfpConfig);

  return std::make_unique<QsfpConfig>(std::move(qsfpConfig), configStr);
}
} // namespace facebook::fboss
