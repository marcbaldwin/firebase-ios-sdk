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

#ifndef FIRESTORE_CORE_SRC_FIREBASE_FIRESTORE_LOCAL_LEVELDB_OPENER_H_
#define FIRESTORE_CORE_SRC_FIREBASE_FIRESTORE_LOCAL_LEVELDB_OPENER_H_

#include <memory>
#include <string>
#include <utility>

#include "Firestore/core/src/firebase/firestore/core/database_info.h"
#include "Firestore/core/src/firebase/firestore/util/status.h"
#include "Firestore/core/src/firebase/firestore/util/statusor.h"
#include "leveldb/db.h"

namespace firebase {
namespace firestore {
namespace local {

/**
 * Opens a leveldb database, creating directories required to to contain the
 * data files, and performing required filesystem annotations (for example to
 * exclude the files from iCloud backup on Apple platforms).
 *
 * Testing implementations may also clear data directories before use.
 */
class LevelDbOpener {
 public:
  static std::unique_ptr<LevelDbOpener> Create(
      const core::DatabaseInfo& database_info);

  explicit LevelDbOpener(std::string directory)
      : directory_{std::move(directory)} {
  }

  virtual ~LevelDbOpener() {
  }

  virtual util::StatusOr<std::unique_ptr<leveldb::DB>> Open();

  const std::string& directory() const {
    return directory_;
  }

 protected:
  util::Status EnsureDirectory(const std::string& dir);
  util::StatusOr<std::unique_ptr<leveldb::DB>> Open(
      const std::string& directory, leveldb::Options options);

 private:
  std::string directory_;
};

}  // namespace local
}  // namespace firestore
}  // namespace firebase

#endif  // FIRESTORE_CORE_SRC_FIREBASE_FIRESTORE_LOCAL_LEVELDB_OPENER_H_
