/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include <memory>

#include <gflags/gflags.h>

#include <map>
#include <tuple>

#include <folly/File.h>
#include <folly/String.h>
#include <folly/Synchronized.h>

extern "C" {
#include <sai.h>
}

DECLARE_bool(enable_replayer);

namespace facebook::fboss {

class SaiTracer {
 public:
  explicit SaiTracer();
  ~SaiTracer();

  static std::shared_ptr<SaiTracer> getInstance();

  void logCreateFn(
      const std::string& fn_name,
      sai_object_id_t* create_object_id,
      sai_object_id_t switch_id,
      uint32_t attr_count,
      const sai_attribute_t* attr_list,
      sai_object_type_t object_type);

  void logRemoveFn(
      const std::string& fn_name,
      sai_object_id_t remove_object_id,
      sai_object_type_t object_type);

  void logSetAttrFn(
      const std::string& fn_name,
      sai_object_id_t set_object_id,
      const sai_attribute_t* attr,
      sai_object_type_t object_type);

  std::string getVariable(
      sai_object_id_t object_id,
      sai_object_type_t object_type);

  uint32_t
  checkListCount(uint32_t list_count, uint32_t elem_size, uint32_t elem_count);

  sai_acl_api_t* aclApi_;

 private:
  void writeToFile(const std::vector<std::string>& strVec);

  // Helper methods for variables and attribute list
  std::tuple<std::string, std::string> declareVariable(
      sai_object_id_t* object_id,
      sai_object_type_t object_type);

  std::vector<std::string> setAttrList(
      const sai_attribute_t* attr_list,
      uint32_t attr_count,
      sai_object_type_t object_type);

  std::string createFnCall(
      const std::string& fn_name,
      const std::string& var1,
      const std::string& var2,
      uint32_t attr_count,
      sai_object_type_t object_type);

  void checkAttrCount(uint32_t attr_count);

  // Init functions
  void setupGlobals();
  void initVarCounts();

  uint32_t maxAttrCount_;
  uint32_t maxListCount_;
  folly::Synchronized<folly::File> saiLogFile_;

  // Variables mappings in generated C code
  std::map<sai_object_type_t, std::atomic<uint32_t>> varCounts_;

  std::map<sai_object_type_t, std::string> varNames_{
      {SAI_OBJECT_TYPE_ACL_ENTRY, "aclEntry_"},
      {SAI_OBJECT_TYPE_ACL_TABLE, "aclTable_"},
      {SAI_OBJECT_TYPE_ACL_TABLE_GROUP, "aclTableGroup_"},
      {SAI_OBJECT_TYPE_ACL_TABLE_GROUP_MEMBER, "aclTableGroupMember_"},
      {SAI_OBJECT_TYPE_SWITCH, "switch_"}};

  folly::Synchronized<std::map<sai_object_id_t, std::string>> aclEntryMap_;
  folly::Synchronized<std::map<sai_object_id_t, std::string>> aclTableMap_;
  folly::Synchronized<std::map<sai_object_id_t, std::string>> aclTableGroupMap_;
  folly::Synchronized<std::map<sai_object_id_t, std::string>>
      aclTableGroupMemberMap_;
  folly::Synchronized<std::map<sai_object_id_t, std::string>> switchMap_;

  std::map<
      sai_object_type_t,
      folly::Synchronized<std::map<sai_object_id_t, std::string>>>
      variables_{
          {SAI_OBJECT_TYPE_ACL_ENTRY, aclEntryMap_},
          {SAI_OBJECT_TYPE_ACL_TABLE, aclTableMap_},
          {SAI_OBJECT_TYPE_ACL_TABLE_GROUP, aclTableGroupMap_},
          {SAI_OBJECT_TYPE_ACL_TABLE_GROUP_MEMBER, aclTableGroupMemberMap_},
          {SAI_OBJECT_TYPE_SWITCH, switchMap_}};

  std::map<sai_object_type_t, std::string> fnPrefix_{
      {SAI_OBJECT_TYPE_ACL_ENTRY, "acl_api->"},
      {SAI_OBJECT_TYPE_ACL_TABLE, "acl_api->"},
      {SAI_OBJECT_TYPE_ACL_TABLE_GROUP, "acl_api->"},
      {SAI_OBJECT_TYPE_ACL_TABLE_GROUP_MEMBER, "acl_api->"},
      {SAI_OBJECT_TYPE_SWITCH, "switch_api->"}};
};

} // namespace facebook::fboss
