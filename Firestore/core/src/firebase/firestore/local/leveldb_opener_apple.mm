/*
 * Copyright 2018 Google
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "Firestore/core/src/firebase/firestore/local/leveldb_opener.h"

#import <Foundation/Foundation.h>

#include <memory>
#include <string>
#include <utility>

#include "Firestore/core/src/firebase/firestore/util/filesystem.h"
#include "Firestore/core/src/firebase/firestore/util/path.h"
#include "Firestore/core/src/firebase/firestore/util/status.h"
#include "Firestore/core/src/firebase/firestore/util/statusor.h"
#include "Firestore/core/src/firebase/firestore/util/string_apple.h"
#include "Firestore/core/src/firebase/firestore/util/string_format.h"
#include "absl/memory/memory.h"
#include "absl/strings/str_cat.h"

using firebase::firestore::util::Dir;
using firebase::firestore::util::Path;
using firebase::firestore::util::Status;
using firebase::firestore::util::StatusOr;

namespace firebase {
namespace firestore {
namespace local {

namespace {

const char* kReservedPathComponent = "firestore";

#if TARGET_OS_IPHONE || TARGET_OS_TV
/**
 * Searches for the directories that match the given `NSSearchPathDirectory`
 * and returns the first one.
 */
std::string ExpandDir(NSSearchPathDirectory directory) {
  NSArray<NSString*>* directories =
      NSSearchPathForDirectoriesInDomains(directory, NSUserDomainMask, YES);
  return util::MakeString(directories[0]);
}
#endif

std::string DocumentsDirectory() {
#if TARGET_OS_IPHONE
  return Path::Join(ExpandDir(NSApplicationSupportDirectory),
                    kReservedPathComponent);

#elif TARGET_OS_MAC
  std::string dotted = absl::StrCat(".", kReservedPathComponent);
  return Path::Join(util::MakeString(NSHomeDirectory()), dotted);

#elif TARGET_OS_TV
  // Have to use Library/Caches on tvOS:
  // https://developer.apple.com/library/content/documentation/General/Conceptual/AppleTV_PG/
  return Path::Join(ExpandDir(NSCachesDirectory), kReservedPathComponent);

#else
#error "Don't know how to find local storage on this target OS"

#endif
}

std::string StoragePathSuffix(const core::DatabaseInfo& database_info) {
  // Use two different path formats:
  //
  //   * persistence_key / project_id . database_id / name
  //   * persistence_key / project_id / name
  //
  // project_ids are DNS-compatible names and cannot contain dots so there's
  // no danger of collisions.
  std::string project_key = database_info.database_id().project_id();
  if (!database_info.database_id().IsDefaultDatabase()) {
    project_key.append(".").append(database_info.database_id().database_id());
  }

  // Reserve one additional path component to allow multiple physical databases
  return Path::Join(database_info.persistence_key(), project_key, "main");
}

}  // namespace

/**
 * Creates the directory at @a directory and marks it as excluded from iCloud
 * backup.
 */
Status LevelDbOpener::EnsureDirectory(const std::string& dir) {
  Status status = Dir::RecursivelyCreate(dir);
  if (!status.ok()) {
    return status.Annotate("Failed to create persistence directory");
  }

  NSError* error;
  NSURL* dir_url = [NSURL fileURLWithPath:util::WrapNSString(dir)];
  if (![dir_url setResourceValue:@YES
                          forKey:NSURLIsExcludedFromBackupKey
                           error:&error]) {
    return Status::FromNSError(error).Annotate(
        "Failed to mark persistence directory as excluded from backups");
  }

  return Status::OK();
}

util::StatusOr<std::unique_ptr<leveldb::DB>> LevelDbOpener::Open(
    const std::string& directory, leveldb::Options options) {
  leveldb::DB* database = nullptr;
  leveldb::Status status = leveldb::DB::Open(options, directory, &database);
  if (!status.ok()) {
    std::string message =
        util::StringFormat("Failed to create database at path %s: %s",
                           directory, status.ToString());
    return Status{FirestoreErrorCode::Internal, message};
  }

  return std::unique_ptr<leveldb::DB>{database};
}

std::unique_ptr<LevelDbOpener> LevelDbOpener::Create(
    const core::DatabaseInfo& database_info) {
  std::string suffix = StoragePathSuffix(database_info);
  std::string dir = Path::Join(DocumentsDirectory(), suffix);
  return absl::make_unique<LevelDbOpener>(dir);
}

util::StatusOr<std::unique_ptr<leveldb::DB>> LevelDbOpener::Open() {
  util::Status status = EnsureDirectory(directory_);
  if (!status.ok()) {
    return status;
  }

  leveldb::Options options;
  options.create_if_missing = true;
  return Open(directory_, options);
}

}  // namespace local
}  // namespace firestore
}  // namespace firebase
