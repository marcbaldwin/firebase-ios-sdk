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

#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <cerrno>
#include <deque>
#include <string>

#include "Firestore/core/src/firebase/firestore/util/defer.h"
#include "Firestore/core/src/firebase/firestore/util/hard_assert.h"
#include "Firestore/core/src/firebase/firestore/util/path.h"
#include "Firestore/core/src/firebase/firestore/util/string_util.h"

using firebase::firestore::util::Path;

namespace firebase {
namespace firestore {
namespace util {

Status Dir::RecursivelyCreate(const Path& path) {
  Status result = Create(path);
  if (result.ok() || result.code() != FirestoreErrorCode::NotFound) {
    // Successfully created the directory, it already existed, or some other
    // unrecoverable error.
    return result;
  }

  // Missing parent
  Path parent = path.Dirname();
  result = RecursivelyCreate(parent);
  if (!result.ok()) {
    return result;
  }

  // Successfully created the parent so try again.
  return Create(path);
}

}  // namespace util
}  // namespace firestore
}  // namespace firebase
