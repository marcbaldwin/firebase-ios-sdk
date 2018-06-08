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

#ifndef FIRESTORE_CORE_TEST_FIREBASE_FIRESTORE_LOCAL_LEVELDB_TESTING_H_
#define FIRESTORE_CORE_TEST_FIREBASE_FIRESTORE_LOCAL_LEVELDB_TESTING_H_

#include <memory>
#include <string>

#include "Firestore/core/src/firebase/firestore/local/leveldb_opener.h"

namespace firebase {
namespace firestore {
namespace local {

class TestingLevelDbOpener : public LevelDbOpener {
 public:
  static std::unique_ptr<LevelDbOpener> Create();
  static std::unique_ptr<leveldb::DB> OpenForTesting();
  static void ClearData();

  explicit TestingLevelDbOpener(std::string dir);
  ~TestingLevelDbOpener();

  util::StatusOr<std::unique_ptr<leveldb::DB>> Open() override;
};

}  // namespace local
}  // namespace firestore
}  // namespace firebase

#endif  // FIRESTORE_CORE_TEST_FIREBASE_FIRESTORE_LOCAL_LEVELDB_TESTING_H_