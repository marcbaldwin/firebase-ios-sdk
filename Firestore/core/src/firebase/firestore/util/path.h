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

#ifndef FIRESTORE_CORE_SRC_FIREBASE_FIRESTORE_UTIL_PATH_H_
#define FIRESTORE_CORE_SRC_FIREBASE_FIRESTORE_UTIL_PATH_H_

#include <string>
#include <utility>

#include "absl/strings/string_view.h"

namespace firebase {
namespace firestore {
namespace util {

class Path;

/**
 * A view over a native pathname string.
 */
class PathView {
 public:
#if defined(_WIN32)
  using char_type = wchar_t;
  using string_type = std::wstring;
#else
  using char_type = char;
  using string_type = std::string;
#endif  // defined(_WIN32)

  static constexpr size_t npos = static_cast<size_t>(-1);

  PathView(const char_type* pathname, size_t size)
      : pathname_{pathname}, size_{size} {
  }

  explicit PathView(const string_type& pathname)
      : pathname_{pathname.data()}, size_{pathname.size()} {
  }

  PathView(const Path& path);

  bool empty() const {
    return size_ == 0;
  }

  /**
   * Returns the unqualified trailing part of the pathname, e.g. "c" for
   * "/a/b/c".
   */
  PathView Basename() const;

  /**
   * Returns the parent directory name, e.g. "/a/b" for "/a/b/c".
   *
   * Note:
   *   * Trailing slashes are treated as a separator between an empty path
   *     segment and the dirname, so Dirname("/a/b/c/") is "/a/b/c".
   *   * Runs of more than one slash are treated as a single separator, so
   *     Dirname("/a/b//c") is "/a/b".
   *   * Paths are not canonicalized, so Dirname("/a//b//c") is "/a//b".
   *   * Presently only UNIX style paths are supported (but compilation
   *     intentionally fails on Windows to prompt implementation there).
   */
  PathView Dirname() const;

  /**
   * Returns true if this path view is an absolute path.
   */
  bool IsAbsolute() const;

 private:
  const char_type* pathname_;
  size_t size_;

  size_t LastSeparator(size_t pos = npos) const;
  size_t LastNonSeparator(size_t pos = npos) const;

  PathView StripDriveLetter() const;

  friend class ::firebase::firestore::util::Path;
};

class Path {
 public:
  using char_type = PathView::char_type;
  using string_type = PathView::string_type;

  static Path FromUtf8(absl::string_view utf8_pathname);

#if defined(_WIN32)
  static Path FromUtf16(wchar_t* begin, size_t size);
#endif

  Path() {
  }

  explicit Path(PathView path) : pathname_{path.pathname_, path.size_} {
  }

  const string_type& native_value() const {
    return pathname_;
  }

  const char_type* c_str() const {
    return pathname_.c_str();
  }

#if defined(_WIN32)
  std::string ToString() const;

#else
  const std::string& ToString() const {
    return pathname_;
  }
#endif  // defined(_WIN32)

  /**
   * Returns the unqualified trailing part of the pathname, e.g. "c" for
   * "/a/b/c".
   */
  Path Basename() const;

  /**
   * Returns the parent directory name, e.g. "/a/b" for "/a/b/c".
   *
   * Note:
   *   * Trailing slashes are treated as a separator between an empty path
   *     segment and the dirname, so Dirname("/a/b/c/") is "/a/b/c".
   *   * Runs of more than one slash are treated as a single separator, so
   *     Dirname("/a/b//c") is "/a/b".
   *   * Paths are not canonicalized, so Dirname("/a//b//c") is "/a//b".
   *   * Presently only UNIX style paths are supported (but compilation
   *     intentionally fails on Windows to prompt implementation there).
   */
  Path Dirname() const;

  /**
   * Returns true if the given `pathname` is an absolute path.
   */
  bool IsAbsolute() const {
    return PathView(pathname_).IsAbsolute();
  }

  /**
   * Returns the paths separated by path separators.
   *
   * @param base If base is of type std::string&& the result is moved from this
   *     value. Otherwise the first argument is copied.
   * @param paths The rest of the path segments.
   */
  template <typename... P>
  Path Append(const P&... paths) const {
    Path result{*this};
    result.JoinAppend(paths...);
    return result;
  }

  /**
   * Returns the paths separated by path separators.
   *
   * @param base If base is of type std::string&& the result is moved from this
   *     value. Otherwise the first argument is copied.
   * @param paths The rest of the path segments.
   */
  template <typename P1, typename... PA>
  static Path Join(P1&& base, const PA&... paths) {
    Path result{std::forward<P1>(base)};
    result.JoinAppend(paths...);
    return result;
  }

  friend bool operator==(const Path& lhs, const Path& rhs) {
    return lhs.pathname_ == rhs.pathname_;
  }
  friend bool operator!=(const Path& lhs, const Path& rhs) {
    return !(lhs == rhs);
  }

 private:
  explicit Path(string_type&& native_pathname)
      : pathname_{std::move(native_pathname)} {
  }

  /**
   * Joins the given base path with a UTF-8 encoded suffix. If `path` is
   * relative, appends it to the given base path. If `path` is absolute,
   * replaces `base`.
   */
  void JoinAppend(const PathView& path);

  template <typename... P>
  void JoinAppend(const PathView& path, const P&... rest) {
    JoinAppend(path);
    JoinAppend(rest...);
  }

  static void JoinAppend() {
    // Recursive base case; nothing to do.
  }

  string_type pathname_;
};

inline PathView::PathView(const Path& path) : PathView{path.native_value()} {
}

}  // namespace util
}  // namespace firestore
}  // namespace firebase

#endif  // FIRESTORE_CORE_SRC_FIREBASE_FIRESTORE_UTIL_PATH_H_
