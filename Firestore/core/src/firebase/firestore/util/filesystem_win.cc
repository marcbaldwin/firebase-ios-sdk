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

#if defined(_WIN32)

#include <windows.h>

#include <cerrno>
#include <string>

#include "Firestore/core/src/firebase/firestore/util/defer.h"
#include "Firestore/core/src/firebase/firestore/util/hard_assert.h"
#include "Firestore/core/src/firebase/firestore/util/path.h"
#include "Firestore/core/src/firebase/firestore/util/string_util.h"

using firebase::firestore::util::Path;

namespace firebase {
namespace firestore {
namespace util {

Status Dir::Create(const Path& path) {
  if (CreateDirectoryW(path.c_str(), nullptr)) {
    return Status::OK();
  }

  DWORD error = GetLastError();
  if (error == ERROR_ALREADY_EXISTS) {
    return Status::Ok();
  }

  return Status::FromLastError(error, "Could not create directory %s", path);
}

bool Dir::Exists(const std::string& path) {
  DWORD attrs = GetFileAttributesW(path.c_str());
  return attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY);
}

Path Dir::TempDir() {
  // Returns a null-terminated string with a trailing backslash.
  wchar_t buffer[MAX_PATH + 2];
  DWORD count = GetTempPath(sizeof(buffer), buffer);
  HARD_ASSERT(count > 0, "Failed to determine temporary directory %s",
              GetLastError());

  return Path::FromUtf16(buffer, count);
}

/**
 * Recursively deletes a path known to be a directory.
 */
static Status RecursivelyDeleteDir(const std::string& parent) {
  // TODO(wilhuff): FindFirstFile, FindNextFile
}

Status Dir::RecursivelyDelete(const Path& path) {
  Status result;
  bool is_dir = File::IsDirectory(path, &result);
  if (!result.ok()) {
    if (result.code() == FirestoreErrorCode::NotFound) {
      return Status::OK();
    }
    return result;
  }

  if (is_dir) {
    return RecursivelyDeleteDir(path);
  }

  // A single file: short circuit
  if (unlink(path.c_str())) {
    return Status::FromErrno("Could not delete file %s", pathname);
  }
  return Status::OK();
}

// #endif  // !defined(__APPLE__)

bool File::Exists(const std::string& pathname) {
  DWORD attrs = GetFileAttributesW(Utf8ToWide(pathname).c_str());
  return attrs != INVALID_FILE_ATTRIBUTES;
}

Status File::Delete(const Path& path) {
  if (DeleteFileW(path.c_str())) {
    return Status::OK();
  }

  DWORD error = GetLastError();
  if (error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND) {
    return Status::Ok();
  }

  return Status::FromLastError(error, "Could not delete file %s", path);
}

}  // namespace util
}  // namespace firestore
}  // namespace firebase

#endif  // defined(_WIN32)
