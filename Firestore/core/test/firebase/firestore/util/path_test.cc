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

#include "Firestore/core/src/firebase/firestore/util/path.h"

#include "gtest/gtest.h"

namespace firebase {
namespace firestore {
namespace util {

// There are several potential sources of inspiration for what is correct
// behavior for these functions.
//
// Python: test with
//
//   python -c 'import os.path; print(os.path.basename("a/b/c//"))'
//
// POSIX shell: test with
//
//   dirname "a/b/c//"
//
// libc++: std::filesystem does not yet ship with Xcode (as of 9.4). Test with a
// new (non-default installed) llvm, e.g. llvm-6.0:
//
//   brew install llvm
//   llvm=$(brew --prefix)/opt/llvm
//   $llvm/bin/clang++ -I$llvm/include -I$llvm/include/c++/v1 -L$llvm/lib
//       -Wl,-rpath,$llvm/lib test.cc -lc++experimental && ./a.out
//
//   test.cc contains something like:
//     #include <experimental/filesystem>
//     #include <iostream>
//     namespace fs = std::experimental::filesystem;
//     int main() {
//       std::cout << fs::path("/a/b/c//").parent_path() << std::endl;
//     }
//
// cppreference: look up example output in functions declared here:
//   https://en.cppreference.com/w/cpp/filesystem/path
//
// This implementation mostly follows python's example:
//
//   * It's pretty simple to implement
//   * POSIX is more complicated than we need
//   * std::filesystem is still too experimental (as of 2018-06-05)

#define EXPECT_PATH_EQ(expected, actual) \
  do {                                                        \
    EXPECT_EQ(Path::FromUtf8(expected), actual); \
  } while (0)

#define EXPECT_BASENAME_EQ(expected, source)                  \
  do {                                                        \
    EXPECT_PATH_EQ(expected, Path::FromUtf8(source).Basename()); \
  } while (0)

TEST(Path, Basename_NoSeparator) {
  // POSIX would require all of these to be ".".
  // python and libc++ agree this is "".
  EXPECT_BASENAME_EQ("", "");
  EXPECT_BASENAME_EQ("a", "a");
  EXPECT_BASENAME_EQ("foo", "foo");
  EXPECT_BASENAME_EQ(".", ".");
  EXPECT_BASENAME_EQ("..", "..");
}

TEST(Path, Basename_LeadingSlash) {
  EXPECT_BASENAME_EQ("", "/");
  EXPECT_BASENAME_EQ("", "///");
  EXPECT_BASENAME_EQ("a", "/a");
  EXPECT_BASENAME_EQ("a", "//a");

  EXPECT_BASENAME_EQ(".", "/.");
  EXPECT_BASENAME_EQ("..", "/..");
  EXPECT_BASENAME_EQ("..", "//..");
}

TEST(Path, Basename_IntermediateSlash) {
  EXPECT_BASENAME_EQ("b", "/a/b");
  EXPECT_BASENAME_EQ("b", "/a//b");
  EXPECT_BASENAME_EQ("b", "//a/b");
  EXPECT_BASENAME_EQ("b", "//a//b");

  EXPECT_BASENAME_EQ("b", "//..//b");
  EXPECT_BASENAME_EQ("b", "//a/./b");
  EXPECT_BASENAME_EQ("b", "//a/.//b");
}

TEST(Path, Basename_TrailingSlash) {
  // python: "a/b//" => ""
  // POSIX: "a/b//" => "b"
  // libc++ path::filename(): "a/b//" => "." (cppreference suggests "")
  EXPECT_BASENAME_EQ("", "/a/");
  EXPECT_BASENAME_EQ("", "/a///");

  EXPECT_BASENAME_EQ("", "/a/b/");
  EXPECT_BASENAME_EQ("", "/a/b//");
  EXPECT_BASENAME_EQ("", "/a//b//");
  EXPECT_BASENAME_EQ("", "//a//b//");
}

TEST(Path, Basename_RelativePath) {
  EXPECT_BASENAME_EQ("b", "a/b");
  EXPECT_BASENAME_EQ("b", "a//b");

  EXPECT_BASENAME_EQ("b", "..//b");
  EXPECT_BASENAME_EQ("b", "a/./b");
  EXPECT_BASENAME_EQ("b", "a/.//b");
  EXPECT_BASENAME_EQ("b", "a//.//b");
}

#define EXPECT_DIRNAME_EQ(expected, source)                  \
  do {                                                       \
    EXPECT_EQ(Path::FromUtf8(expected), Path::FromUtf8(source).Dirname()); \
  } while (0)

TEST(Path, Dirname_NoSeparator) {
  // POSIX would require all of these to be ".".
  // python and libc++ agree this is "".
  EXPECT_DIRNAME_EQ("", "");
  EXPECT_DIRNAME_EQ("", "a");
  EXPECT_DIRNAME_EQ("", "foo");
  EXPECT_DIRNAME_EQ("", ".");
  EXPECT_DIRNAME_EQ("", "..");
}

TEST(Path, Dirname_LeadingSlash) {
  // POSIX says all "/".
  // python starts with "/" but does not strip trailing slashes.
  // libc++ path::parent_path() considers all of these be "", though
  // cppreference.com indicates this should be "/" in example output so this is
  // likely a bug.
  EXPECT_DIRNAME_EQ("/", "/");
  EXPECT_DIRNAME_EQ("/", "///");
  EXPECT_DIRNAME_EQ("/", "/a");
  EXPECT_DIRNAME_EQ("/", "//a");

  EXPECT_DIRNAME_EQ("/", "/.");
  EXPECT_DIRNAME_EQ("/", "/..");
  EXPECT_DIRNAME_EQ("/", "//..");
}

TEST(Path, Dirname_IntermediateSlash) {
  EXPECT_DIRNAME_EQ("/a", "/a/b");
  EXPECT_DIRNAME_EQ("/a", "/a//b");
  EXPECT_DIRNAME_EQ("//a", "//a/b");
  EXPECT_DIRNAME_EQ("//a", "//a//b");

  EXPECT_DIRNAME_EQ("//..", "//..//b");
  EXPECT_DIRNAME_EQ("//a/.", "//a/./b");
  EXPECT_DIRNAME_EQ("//a/.", "//a/.//b");
}

TEST(Path, Dirname_TrailingSlash) {
  // POSIX demands stripping trailing slashes before computing dirname, while
  // python and libc++ effectively seem to consider the path to contain an empty
  // path segment there.
  EXPECT_DIRNAME_EQ("/a", "/a/");
  EXPECT_DIRNAME_EQ("/a", "/a///");

  EXPECT_DIRNAME_EQ("/a/b", "/a/b/");
  EXPECT_DIRNAME_EQ("/a/b", "/a/b//");
  EXPECT_DIRNAME_EQ("/a//b", "/a//b//");
  EXPECT_DIRNAME_EQ("//a//b", "//a//b//");
}

TEST(Path, Dirname_RelativePath) {
  EXPECT_DIRNAME_EQ("a", "a/b");
  EXPECT_DIRNAME_EQ("a", "a//b");

  EXPECT_DIRNAME_EQ("..", "..//b");
  EXPECT_DIRNAME_EQ("a/.", "a/./b");
  EXPECT_DIRNAME_EQ("a/.", "a/.//b");
  EXPECT_DIRNAME_EQ("a//.", "a//.//b");
}

TEST(Path, IsAbsolute) {
  EXPECT_FALSE(Path::FromUtf8("").IsAbsolute());
  EXPECT_TRUE(Path::FromUtf8("/").IsAbsolute());
  EXPECT_TRUE(Path::FromUtf8("//").IsAbsolute());
  EXPECT_TRUE(Path::FromUtf8("/foo").IsAbsolute());
  EXPECT_FALSE(Path::FromUtf8("foo").IsAbsolute());
  EXPECT_FALSE(Path::FromUtf8("foo/bar").IsAbsolute());
}

#define EXPECT_JOIN_EQ(expected, ...)                  \
  do {                                                       \
    EXPECT_EQ(Path::FromUtf8(expected), Path::Join(__VA_ARGS__).Dirname()); \
  } while (0)

template <typename... P>
Path JoinUtf8(const P&... paths) {
  return Path::Join(Path::FromUtf8(paths)...);
}

TEST(Path, Join_Absolute) {
  EXPECT_PATH_EQ("/", JoinUtf8("/"));

  EXPECT_PATH_EQ("/", JoinUtf8("", "/"));
  EXPECT_PATH_EQ("/", JoinUtf8("a", "/"));
  EXPECT_PATH_EQ("/b", JoinUtf8("a", "/b"));

  // Alternate root names should be preserved.
  EXPECT_PATH_EQ("//", JoinUtf8("a", "//"));
  EXPECT_PATH_EQ("//b", JoinUtf8("a", "//b"));
  EXPECT_PATH_EQ("///b///", JoinUtf8("a", "///b///"));

  EXPECT_PATH_EQ("/", JoinUtf8("/", "/"));
  EXPECT_PATH_EQ("/b", JoinUtf8("/", "/b"));
  EXPECT_PATH_EQ("//b", JoinUtf8("//host/a", "//b"));
  EXPECT_PATH_EQ("//b", JoinUtf8("//host/a/", "//b"));

  EXPECT_PATH_EQ("/", JoinUtf8("/", ""));
  EXPECT_PATH_EQ("/a", JoinUtf8("/", "a"));
  EXPECT_PATH_EQ("/a/b/c", JoinUtf8("/", "a", "b", "c"));
  EXPECT_PATH_EQ("/a/", JoinUtf8("/", "a/"));
  EXPECT_PATH_EQ("/.", JoinUtf8("/", "."));
  EXPECT_PATH_EQ("/..", JoinUtf8("/", ".."));
}

TEST(Path, Join_Relative) {
  EXPECT_PATH_EQ("", JoinUtf8(""));

  EXPECT_PATH_EQ("", JoinUtf8("", "", "", ""));
  EXPECT_PATH_EQ("a/b/c", JoinUtf8("a/b", "c"));
  EXPECT_PATH_EQ("/c/d", JoinUtf8("a/b", "/c", "d"));
  EXPECT_PATH_EQ("/c/d", JoinUtf8("a/b/", "/c", "d"));
}

}  // namespace util
}  // namespace firestore
}  // namespace firebase
