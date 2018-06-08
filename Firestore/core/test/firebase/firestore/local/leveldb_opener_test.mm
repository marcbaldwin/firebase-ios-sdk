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

#include <memory>
#include <string>

#include "Firestore/core/src/firebase/firestore/util/autoid.h"
#include "Firestore/core/src/firebase/firestore/util/defer.h"
#include "Firestore/core/src/firebase/firestore/util/filesystem.h"
#include "Firestore/core/src/firebase/firestore/util/path.h"
#include "Firestore/core/test/firebase/firestore/util/status_test_util.h"
#include "gtest/gtest.h"

using Path = firebase::firestore::util::Path;

namespace firebase {
namespace firestore {
namespace local {

TEST(MigratingLevelDbOpenerTest, Avoids) {
  std::string tmp_dir = Path::Join(util::GetTempDir(), "migrating-opener");
  util::Defer defer{[&]() { (void)util::RecursivelyDelete(tmp_dir); }};

  std::string db_dir = Path::Join(tmp_dir, util::CreateAutoId());
  std::string legacy_dir = Path::Join(tmp_dir, util::CreateAutoId());

  ASSERT_FALSE(util::Exists(db_dir));
  ASSERT_FALSE(util::Exists(legacy_dir));

  MigratingLevelDbOpener opener(db_dir, legacy_dir);
  util::StatusOr<std::unique_ptr<leveldb::DB>> result = opener.Open();
  EXPECT_OK(result.status());
}

}  // namespace local
}  // namespace firestore
}  // namespace firebase
