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

#import <Foundation/Foundation.h>

namespace firebase {
namespace firestore {
namespace util {

namespace {

/**
 * Returns true if the given NSError object matches the given domain and code.
 */
bool Matches(NSError* error, NSErrorDomain domain, NSInteger code) {
  return [error.domain isEqual:domain] && error.code == code;
}

NSString* ToNSString(const Path& pathname) {
  return WrapNSString(pathname.native_value());
}

Path ToPath(NSString* pathname) {
  return Path::FromUtf8(MakeStringView(pathname));
}

}  // namespace

Path Dir::TempDir() {
  const char* env_tmpdir = getenv("TMPDIR");
  if (env_tmpdir) {
    return Path::FromUtf8(env_tmpdir);
  }

  NSString* ns_tmpdir = NSTemporaryDirectory();
  if (ns_tmpdir) {
    return ToPath(ns_tmpdir);
  }

  return Path::FromUtf8("/tmp");
}

Status Dir::RecursivelyCreate(const Path& path) {
  NSString* ns_path = ToNSString(path);

  NSError* error = nil;
  if (![[NSFileManager defaultManager] createDirectoryAtPath:ns_path
                                 withIntermediateDirectories:YES
                                                  attributes:nil
                                                       error:&error]) {
    return Status::FromNSError(error);
  }
  return Status::OK();
}

Status Dir::RecursivelyDelete(const Path& path) {
  NSString* ns_path = ToNSString(path);
  NSError* error = nil;
  if (![[NSFileManager defaultManager] removeItemAtPath:ns_path error:&error]) {
    if (Matches(error, NSCocoaErrorDomain, NSFileNoSuchFileError)) {
      // Successful by definition
      return Status::OK();
    }

    return Status::FromNSError(error);
  }
  return Status::OK();
}

}  // namespace util
}  // namespace firestore
}  // namespace firebase
