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

#include "Firestore/core/src/firebase/firestore/util/filesystem.h"

#include <sys/stat.h>
#include <sys/types.h>

#include <fstream>

#include "Firestore/core/src/firebase/firestore/util/autoid.h"
#include "Firestore/core/src/firebase/firestore/util/defer.h"
#include "Firestore/core/src/firebase/firestore/util/log.h"
#include "Firestore/core/src/firebase/firestore/util/path.h"
#include "Firestore/core/test/firebase/firestore/util/status_test_util.h"
#include "absl/strings/match.h"
#include "absl/types/optional.h"
#include "gtest/gtest.h"

namespace firebase {
namespace firestore {
namespace util {

static void Touch(const Path& pathname) {
  std::ofstream out{pathname.native_value()};
  ASSERT_TRUE(out.good());
  out.close();
}

static Path TestFilename() {
  return Path::FromUtf8("firestore-testing-" + CreateAutoId());
}

TEST(DirTest, Exists) {
  EXPECT_TRUE(Dir::Exists(Path::FromUtf8("/")));

  Path file = Path::Join("/", TestFilename());
  EXPECT_FALSE(Dir::Exists(file));
}

TEST(DirTest, GetTempDir) {
  Path tmp = Dir::TempDir();
  ASSERT_TRUE(absl::StartsWith(tmp.ToString(), "/"));
}

TEST(Filesystem, GetTempDirNoTmpdir) {
  // Save aside old value of TMPDIR (if set) and force TMPDIR to unset.
  absl::optional<std::string> old_tmpdir;
  const char* tmpdir = getenv("TMPDIR");
  if (tmpdir != nullptr) {
    old_tmpdir = tmpdir;
    unsetenv("TMPDIR");
    ASSERT_EQ(nullptr, getenv("TMPDIR"));
  }

  Path tmp = Dir::TempDir();
  ASSERT_TRUE(absl::StartsWith(tmp.ToString(), "/"));

  // Save aside old value of TMPDIR, if set
  if (old_tmpdir) {
    int result = setenv("TMPDIR", old_tmpdir->c_str(), 1);
    ASSERT_EQ(0, result);
  }
}

TEST(Filesystem, RecursivelyCreateDir) {
  Path dir = Path::Join(Dir::TempDir(), TestFilename());

  std::cerr << "Created dir " << dir.ToString() << std::endl;
  Status status = Dir::RecursivelyCreate(dir);
  ASSERT_OK(status);
  ASSERT_TRUE(Dir::Exists(dir));

  struct stat old_stat;
  if (stat(dir.c_str(), &old_stat)) {
    GTEST_FAIL() << "Could not stat " << dir.ToString() << " (errno: " << errno << ")";
  }

  Path subdir = Path::Join(dir, TestFilename(), TestFilename());

  {
    // Make the parent directory inaccessible, creation and removal should fail
    chmod(dir.c_str(), 0);
    Defer restore{[&]() { chmod(dir.c_str(), old_stat.st_mode); }};

    status = Dir::RecursivelyCreate(subdir);
    EXPECT_EQ(FirestoreErrorCode::PermissionDenied, status.code());

    status = Dir::RecursivelyDelete(subdir);
    EXPECT_EQ(FirestoreErrorCode::PermissionDenied, status.code());
  }

  // Permissions restored.
  status = Dir::RecursivelyDelete(subdir);
  EXPECT_OK(status);

  status = Dir::RecursivelyCreate(subdir);
  EXPECT_OK(status);

  status = Dir::RecursivelyDelete(dir);
  ASSERT_OK(status);
  ASSERT_FALSE(Dir::Exists(dir));
}

TEST(Filesystem, RecursivelyDelete) {
  Path tmp_dir = Dir::TempDir();
  ASSERT_TRUE(Dir::Exists(tmp_dir));

  Path file = Path::Join(tmp_dir, TestFilename());
  Status status = Dir::RecursivelyDelete(file);
  EXPECT_OK(status);
  EXPECT_FALSE(Dir::Exists(file));

  Touch(file);
  EXPECT_TRUE(File::Exists(file));
  EXPECT_FALSE(Dir::Exists(file));

  status = Dir::RecursivelyDelete(file);
  EXPECT_OK(status);
  EXPECT_FALSE(File::Exists(file));
}

}  // namespace util
}  // namespace firestore
}  // namespace firebase
