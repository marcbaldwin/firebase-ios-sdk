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

#include "Firestore/core/src/firebase/firestore/util/string_win.h"
#include "absl/strings/ascii.h"
#include "absl/strings/string_view.h"

namespace firebase {
namespace firestore {
namespace util {

namespace {

#if defined(_WIN32)
constexpr wchar_t kPreferredSeparator = L'\\';
#else
constexpr char kPreferredSeparator = '/';
#endif

static constexpr absl::string_view::size_type npos = absl::string_view::npos;

/** Returns true if the given character is a pathname separator. */
inline bool IsSeparator(char c) {
#if defined(_WIN32)
  return c == '/' || c == '\\';
#else
  return c == '/';
#endif  // defined(_WIN32)
}

}  // namespace

PathView PathView::Basename() const {
  size_t slash = LastSeparator();
  if (slash == npos) {
    // No path separator found => the whole string.
    return *this;
  }

  // Otherwise everything after the slash is the basename (even if empty string)
  size_t start = slash + 1;
  return PathView{pathname_ + start, size_ - start};
}

PathView PathView::Dirname() const {
  size_t last_slash = LastSeparator();
  if (last_slash == npos) {
    // No path separator found => empty string. Conformance with POSIX would
    // have us return "." here.
    return PathView{pathname_, 0};
  }

  // Collapse runs of slashes.
  size_t non_slash = LastNonSeparator(last_slash);
  if (non_slash == npos) {
    // All characters preceding the last path separator are slashes
    return PathView{pathname_, 1};
  }

  last_slash = non_slash + 1;

  // Otherwise everything up to the slash is the parent directory
  return PathView{pathname_, last_slash};
}

bool PathView::IsAbsolute() const {
  PathView path = StripDriveLetter();
  return !path.empty() && IsSeparator(path.pathname_[0]);
}

size_t PathView::LastNonSeparator(size_t pos) const {
  if (empty()) return npos;

  size_t i = std::min(pos, size_);
  for (; i-- > 0;) {
    if (!IsSeparator(pathname_[i])) {
      return i;
    }
  }
  return npos;
}

size_t PathView::LastSeparator(size_t pos) const {
  if (empty()) return npos;

  size_t i = std::min(pos, size_);
  for (; i-- > 0;) {
    if (IsSeparator(pathname_[i])) {
      return i;
    }
  }
  return npos;
}

/** Returns the given path with its leading drive letter removed. */
PathView PathView::StripDriveLetter() const {
#if defined(_WIN32)
  if (size_ >= 2 && pathname_[1] == L':' && absl::ascii_isalpha(pathname_[0])) {
    return PathView{pathname_ + 2, size_ - 2};
  }
  return *this;

#else
  return *this;
#endif  // defined(_WIN32)
}


Path Path::FromUtf8(absl::string_view utf8_pathname) {
  return Path{Utf8ToNative(utf8_pathname)};
}

#if defined(_WIN32)
std::string ToString() const {
  return NativeToUtf8(pathname_);
}
#endif

Path Path::Basename() const {
  return Path{PathView{*this}.Basename()};
}

Path Path::Dirname() const {
  return Path{PathView{*this}.Dirname()};
}

void Path::JoinAppend(const PathView& path) {
  if (path.IsAbsolute()) {
    pathname_.assign(path.pathname_);

  } else {
    size_t non_slash = PathView(pathname_).LastNonSeparator();
    if (non_slash != npos) {
      pathname_.resize(non_slash + 1);
      pathname_.push_back(kPreferredSeparator);
    }

    // If path started with a slash we'd treat it as absolute above
    pathname_.append(path.pathname_);
  }
}

}  // namespace util
}  // namespace firestore
}  // namespace firebase
