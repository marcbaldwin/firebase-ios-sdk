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

#include "Firestore/core/test/firebase/firestore/local/leveldb_testing.h"

#include <utility>

#include "Firestore/core/src/firebase/firestore/util/filesystem.h"
#include "Firestore/core/src/firebase/firestore/util/path.h"
#include "absl/memory/memory.h"

using firebase::firestore::util::Dir;
using firebase::firestore::util::Path;
using firebase::firestore::util::Status;

namespace firebase {
namespace firestore {
namespace local {

namespace {

std::string DocumentsDirectory() {
  return Path::Join(Dir::TempDir(), "firestore-leveldb-testing");
}

Status ClearDataInternal(const std::string& dir) {
  if (Dir::Exists(dir)) {
    // Delete the directory first to ensure isolation between runs.
    Status status = Dir::RecursivelyDelete(dir);
    if (!status.ok()) {
      return status.Annotate(
          util::StringFormat("failed to clean up leveldb path %s", dir));
    }
  }
  return Status::OK();
}

}  // namespace

TestingLevelDbOpener::~TestingLevelDbOpener() {
}

std::unique_ptr<LevelDbOpener> TestingLevelDbOpener::Create() {
  return absl::make_unique<TestingLevelDbOpener>(DocumentsDirectory());
}

std::unique_ptr<leveldb::DB> TestingLevelDbOpener::OpenForTesting() {
  std::unique_ptr<LevelDbOpener> opener = Create();
  return opener->Open().ValueOrDie();
}

void TestingLevelDbOpener::ClearData() {
  Status status = ClearDataInternal(DocumentsDirectory());
  STATUS_CHECK_OK(status);
}

TestingLevelDbOpener::TestingLevelDbOpener(std::string dir)
    : LevelDbOpener(std::move(dir)) {
}

util::StatusOr<std::unique_ptr<leveldb::DB>> TestingLevelDbOpener::Open() {
  const std::string& dir = directory();
  Status status = ClearDataInternal(dir);
  STATUS_CHECK_OK(status);

  leveldb::Options options;
  options.create_if_missing = true;
  options.error_if_exists = true;
  auto result = LevelDbOpener::Open(dir, options);
  STATUS_CHECK_OK(result.status());

  return result;
}

}  // namespace local
}  // namespace firestore
}  // namespace firebase
