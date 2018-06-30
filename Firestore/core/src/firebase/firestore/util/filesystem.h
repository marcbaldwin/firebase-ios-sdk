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

#ifndef FIRESTORE_CORE_SRC_FIREBASE_FIRESTORE_UTIL_FILESYSTEM_H_
#define FIRESTORE_CORE_SRC_FIREBASE_FIRESTORE_UTIL_FILESYSTEM_H_

#include <string>

#include "Firestore/core/src/firebase/firestore/util/path.h"
#include "Firestore/core/src/firebase/firestore/util/status.h"
#include "Firestore/core/src/firebase/firestore/util/statusor.h"
#include "absl/strings/string_view.h"

namespace firebase {
namespace firestore {
namespace util {

class Dir {
 public:
  static Status Create(const Path& path);
  static Status Delete(const Path& path);

  /**
   * Returns true if the given file exists and is a directory.
   */
  static bool Exists(const Path& path);

  /**
   * Returns the best directory in which to create temporary files.
   */
  static Path TempDir();

  /**
   * Recursively creates all the directories in the path name if they don't
   * exist.
   *
   * @return Ok if the directory was created or already existed.
   */
  static Status RecursivelyCreate(const Path& path);

  /**
   * Recursively deletes the contents of the given pathname. If the pathname is
   * a file, deletes just that file. The the pathname is a directory, deletes
   * everything within the directory.
   *
   * @return Ok if the directory was deleted or did not exist.
   */
  static Status RecursivelyDelete(const Path& path);
};

class File {
 public:
  static Status Delete(const Path& path);

  /**
   * Returns true if the given file exists.
   */
  static bool Exists(const Path& path);

  static bool IsDirectory(const Path& path, Status* error);
};

}  // namespace util
}  // namespace firestore
}  // namespace firebase

#endif  // FIRESTORE_CORE_SRC_FIREBASE_FIRESTORE_UTIL_FILESYSTEM_H_
