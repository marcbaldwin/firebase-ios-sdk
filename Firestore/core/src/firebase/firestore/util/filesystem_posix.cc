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

#if !defined(_WIN32)

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

Status Dir::Create(const Path& path) {
  if (::mkdir(path.c_str(), 0777) == 0 || errno == EEXIST) {
    // Successfully created the directory or it already existed.
    return Status::OK();
  }

  return Status::FromErrno("Could not create directory %s", path.ToString());
}

Status Dir::Delete(const Path& path) {
  if (!::rmdir(path.c_str())) {
    if (errno != ENOENT && errno != ENOTDIR) {
      return Status::FromErrno("Could not delete directory %s", path.ToString());
    }
  }

  return Status::OK();
}

bool Dir::Exists(const Path& path) {
  return File::IsDirectory(path, nullptr);
}

//#if !defined(__APPLE__)
Path Dir::TempDir() {
  const char* env_tmpdir = getenv("TMPDIR");
  if (env_tmpdir != nullptr) {
    return Path::FromUtf8(env_tmpdir);
  }

#if defined(__ANDROID__)
  // The /tmp directory doesn't exist as a fallback; each application is
  // supposed to keep its own temporary files. Previously /data/local/tmp may
  // have been reasonable, but current lore points to this being unreliable for
  // writing at higher API levels or certain phone models because default
  // permissions on this directory no longer permit writing.
  //
  // TODO(wilhuff): Validate on recent Android.
#error "Not yet sure about temporary file locations on Android."
  return Path::FromUtf8("/data/local/tmp");

#else
  return Path::FromUtf8("/tmp");
#endif  // defined(__ANDROID__)
}

/**
 * Recursively deletes a path known to be a directory.
 */
static Status RecursivelyDeleteDir(const Path& parent) {
  DIR* dir = ::opendir(parent.c_str());
  if (!dir) {
    return Status::FromErrno("Could not read directory %s", parent);
  }

  Status result;
  for (;;) {
    errno = 0;
    struct dirent* entry = ::readdir(dir);
    if (!entry) {
      if (errno != 0) {
        result = Status::FromErrno("Could not read directory %s", parent);
      }
      break;
    }

    if (::strcmp(".", entry->d_name) == 0 ||
        ::strcmp("..", entry->d_name) == 0) {
      continue;
    }

    Path child = parent.Append(PathView(entry->d_name, entry->d_namlen));
    result = Dir::RecursivelyDelete(child);
    if (!result.ok()) {
      break;
    }
  }

  if (::closedir(dir)) {
    if (result.ok()) {
      return Status::FromErrno("Could not close directory %s", parent);
    }
  }
  if (!result.ok()) {
    return result;
  }

  if (::rmdir(parent.c_str())) {
    return Status::FromErrno("Could not delete directory %s", parent);
  }

  return Status::OK();
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
  } else {
    return File::Delete(path);
  }
}

//#endif  // !defined(__APPLE__)

Status File::Delete(const Path& path) {
  if (::unlink(path.c_str())) {
    return Status::FromErrno("Could not delete file %s", path);
  }
  return Status::OK();
}

bool File::Exists(const Path& path) {
  struct stat buffer;
  int rc = ::stat(path.c_str(), &buffer);
  return rc == 0;
}

bool File::IsDirectory(const Path& path, Status* error) {
  struct stat buffer;
  if (::stat(path.c_str(), &buffer)) {
    if (error) {
      *error = Status::FromErrno("Could not stat file %s", path);
    }
    return false;
  }
  return S_ISDIR(buffer.st_mode);
}

#endif  // !defined(_WIN32)

}  // namespace util
}  // namespace firestore
}  // namespace firebase
