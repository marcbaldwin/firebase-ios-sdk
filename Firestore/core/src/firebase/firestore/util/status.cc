/*
 * Copyright 2015, 2018 Google
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

#include "Firestore/core/src/firebase/firestore/util/status.h"

#include <cerrno>

#include "Firestore/core/src/firebase/firestore/util/strerror.h"
#include "Firestore/core/src/firebase/firestore/util/string_format.h"
#include "Firestore/core/src/firebase/firestore/util/string_win.h"
#include "absl/memory/memory.h"

namespace firebase {
namespace firestore {
namespace util {

Status::Status(FirestoreErrorCode code, absl::string_view msg) {
  HARD_ASSERT(code != FirestoreErrorCode::Ok);
  state_ = absl::make_unique<State>();
  state_->code = code;
  state_->msg = static_cast<std::string>(msg);
}

/// Returns the Canonical error code for the given errno value.
static FirestoreErrorCode CodeForErrno(int errno_code) {
  switch (errno_code) {
    case 0:
      return FirestoreErrorCode::Ok;

      // Internal canonical mappings call these failed preconditions, but for
      // our purposes these must indicate an internal error in file handling.
    case EBADF:  // Invalid file descriptor
#if defined(EBADFD)
    case EBADFD:  // File descriptor in bad state
#endif
      return FirestoreErrorCode::Internal;

    case EINVAL:        // Invalid argument
    case ENAMETOOLONG:  // Filename too long
    case E2BIG:         // Argument list too long
    case EDESTADDRREQ:  // Destination address required
    case EDOM:          // Mathematics argument out of domain of function
    case EFAULT:        // Bad address
    case EILSEQ:        // Illegal byte sequence
    case ENOPROTOOPT:   // Protocol not available
    case ENOSTR:        // Not a STREAM
    case ENOTSOCK:      // Not a socket
    case ENOTTY:        // Inappropriate I/O control operation
    case EPROTOTYPE:    // Protocol wrong type for socket
    case ESPIPE:        // Invalid seek
      return FirestoreErrorCode::InvalidArgument;

    case ETIMEDOUT:  // Connection timed out
    case ETIME:      // Timer expired
      return FirestoreErrorCode::DeadlineExceeded;

    case ENODEV:  // No such device
    case ENOENT:  // No such file or directory
#if defined(ENOMEDIUM)
    case ENOMEDIUM:  // No medium found
#endif
    case ENXIO:  // No such device or address
    case ESRCH:  // No such process
      return FirestoreErrorCode::NotFound;

    case EEXIST:         // File exists
    case EADDRNOTAVAIL:  // Address not available
    case EALREADY:       // Connection already in progress
#if defined(ENOTUNIQ)
    case ENOTUNIQ:  // Name not unique on network
#endif
      return FirestoreErrorCode::AlreadyExists;

    case EPERM:   // Operation not permitted
    case EACCES:  // Permission denied
#if defined(ENOKEY)
    case ENOKEY:  // Required key not available
#endif
    case EROFS:  // Read only file system
      return FirestoreErrorCode::PermissionDenied;

    case ENOTEMPTY:   // Directory not empty
    case EISDIR:      // Is a directory
    case ENOTDIR:     // Not a directory
    case EADDRINUSE:  // Address already in use
    case EBUSY:       // Device or resource busy
    case ECHILD:      // No child processes
    case EISCONN:     // Socket is connected
#if defined(EISNAM)
    case EISNAM:  // Is a named type file
#endif
#if defined(ENOTBLK)
    case ENOTBLK:  // Block device required
#endif
    case ENOTCONN:  // The socket is not connected
    case EPIPE:     // Broken pipe
#if defined(ESHUTDOWN)
    case ESHUTDOWN:  // Cannot send after transport endpoint shutdown
#endif
    case ETXTBSY:  // Text file busy
#if defined(EUNATCH)
    case EUNATCH:  // Protocol driver not attached
#endif
      return FirestoreErrorCode::FailedPrecondition;

    case ENOSPC:  // No space left on device
#if defined(EDQUOT)
    case EDQUOT:  // Disk quota exceeded
#endif
    case EMFILE:   // Too many open files
    case EMLINK:   // Too many links
    case ENFILE:   // Too many open files in system
    case ENOBUFS:  // No buffer space available
    case ENODATA:  // No message is available on the STREAM read queue
    case ENOMEM:   // Not enough space
    case ENOSR:    // No STREAM resources
#if defined(EUSERS)
    case EUSERS:  // Too many users
#endif
      return FirestoreErrorCode::ResourceExhausted;

#if defined(ECHRNG)
    case ECHRNG:  // Channel number out of range
#endif
    case EFBIG:      // File too large
    case EOVERFLOW:  // Value too large to be stored in data type
    case ERANGE:     // Result too large
      return FirestoreErrorCode::OutOfRange;

#if defined(ENOPKG)
    case ENOPKG:  // Package not installed
#endif
    case ENOSYS:        // Function not implemented
    case ENOTSUP:       // Operation not supported
    case EAFNOSUPPORT:  // Address family not supported
#if defined(EPFNOSUPPORT)
    case EPFNOSUPPORT:  // Protocol family not supported
#endif
    case EPROTONOSUPPORT:  // Protocol not supported
#if defined(ESOCKTNOSUPPORT)
    case ESOCKTNOSUPPORT:  // Socket type not supported
#endif
    case EXDEV:  // Improper link
      return FirestoreErrorCode::Unimplemented;

    case EAGAIN:  // Resource temporarily unavailable
#if defined(ECOMM)
    case ECOMM:  // Communication error on send
#endif
    case ECONNREFUSED:  // Connection refused
    case ECONNABORTED:  // Connection aborted
    case ECONNRESET:    // Connection reset
    case EINTR:         // Interrupted function call
#if defined(EHOSTDOWN)
    case EHOSTDOWN:  // Host is down
#endif
    case EHOSTUNREACH:  // Host is unreachable
    case ENETDOWN:      // Network is down
    case ENETRESET:     // Connection aborted by network
    case ENETUNREACH:   // Network unreachable
    case ENOLCK:        // No locks available
    case ENOLINK:       // Link has been severed
#if defined(ENONET)
    case ENONET:  // Machine is not on the network
#endif
      return FirestoreErrorCode::Unavailable;

    case EDEADLK:  // Resource deadlock avoided
#if defined(ESTALE)
    case ESTALE:  // Stale file handle
#endif
      return FirestoreErrorCode::Aborted;

    case ECANCELED:  // Operation cancelled
      return FirestoreErrorCode::Cancelled;

    default:
      return FirestoreErrorCode::Unknown;
  }
}

Status Status::FromErrno(absl::string_view msg) {
  return FromErrno(errno, msg);
}

Status Status::FromErrno(int errno_code, absl::string_view msg) {
  FirestoreErrorCode canonical_code = CodeForErrno(errno_code);
  return Status{canonical_code,
                util::StringFormat("%s (errno %s: %s)", msg, errno_code,
                                   StrError(errno_code))};
}

#if defined(_WIN32)
/**
 * Returns the Canonical error code for the given Windows API error code as
 * obtained from GetLastError().
 */
static FirestoreErrorCode CodeFromLastError(DWORD error) {
  switch (error) {
    case ERROR_SUCCESS:
      return FirestoreErrorCode::Ok;

    case ERROR_INVALID_ACCESS:
      return FirestoreErrorCode::Internal;

    case ERROR_INVALID_FUNCTION:
    case ERROR_INVALID_HANDLE:
    case ERROR_INVALID_NAME:
      return FirestoreErrorCode::InvalidArgument;

      // return FirestoreErrorCode::DeadlineExceeded;

    case ERROR_FILE_NOT_FOUND:
    case ERROR_PATH_NOT_FOUND:
    case ERROR_INVALID_DRIVE:
    case ERROR_BAD_NETPATH:
    case ERROR_DEV_NOT_EXIST:
      return FirestoreErrorCode::NotFound;

    case ERROR_FILE_EXISTS:
    case ERROR_ALREADY_EXISTS:
      return FirestoreErrorCode::AlreadyExists;

    case ERROR_ACCESS_DENIED:
    case ERROR_SHARING_VIOLATION:
    case ERROR_WRITE_PROTECT:
    case ERROR_LOCK_VIOLATION:
      return FirestoreErrorCode::PermissionDenied;

      // return FirestoreErrorCode::FailedPrecondition;

    case ERROR_TOO_MANY_OPEN_FILES:
    case ERROR_NOT_ENOUGH_MEMORY:
    case ERROR_OUTOFMEMORY:
    case ERROR_NO_MORE_FILES:
    case ERROR_DISK_FULL:
    case ERROR_HANDLE_DISK_FULL:
      return FirestoreErrorCode::ResourceExhausted;

      // return FirestoreErrorCode::OutOfRange;

    case ERROR_CALL_NOT_IMPLEMENTED:
      return FirestoreErrorCode::Unimplemented;

    case ERROR_NOT_READY:
      return FirestoreErrorCode::Unavailable;

      // return FirestoreErrorCode::Aborted;

      // return FirestoreErrorCode::Cancelled;

    default:
      return FirestoreErrorCode::Unknown;
  }
}

Status Status::FromLastError(DWORD error, absl::string_view msg) {
  FirestoreErrorCode canonical_code = CodeForLastError(error);

  wchar_t* error_text = nullptr;
  DWORD count = FormatMessageW(
      FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER |
          FORMAT_MESSAGE_IGNORE_INSERTS,
      nullptr, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
      (wchar_t*)&error_text, 0, nullptr);

  std::string formatted;
  if (count == 0 || error_text == nullptr) {
    formatted = util::StringFormat(
        "%s (error %s; unknown error %s while getting error text)", msg, error,
        result);
  } else {
    formatted = NativeToUtf8(error_text, count);
    LocalFree(error_text);
    error_text = nullptr;
  }

  return Status{canonical_code,
                util::StringFormat("%s (error %s: %s)", msg, error, formatted)};
}
#endif  // defined(_WIN32)

void Status::Update(const Status& new_status) {
  if (ok()) {
    *this = new_status;
  }
}

void Status::SlowCopyFrom(const State* src) {
  if (src == nullptr) {
    state_ = nullptr;
  } else {
    state_ = absl::make_unique<State>(*src);
  }
}

const std::string& Status::empty_string() {
  static std::string* empty = new std::string;
  return *empty;
}

std::string Status::ToString() const {
  if (state_ == nullptr) {
    return "OK";
  } else {
    std::string result;
    switch (code()) {
      case FirestoreErrorCode::Cancelled:
        result = "Cancelled";
        break;
      case FirestoreErrorCode::Unknown:
        result = "Unknown";
        break;
      case FirestoreErrorCode::InvalidArgument:
        result = "Invalid argument";
        break;
      case FirestoreErrorCode::DeadlineExceeded:
        result = "Deadline exceeded";
        break;
      case FirestoreErrorCode::NotFound:
        result = "Not found";
        break;
      case FirestoreErrorCode::AlreadyExists:
        result = "Already exists";
        break;
      case FirestoreErrorCode::PermissionDenied:
        result = "Permission denied";
        break;
      case FirestoreErrorCode::Unauthenticated:
        result = "Unauthenticated";
        break;
      case FirestoreErrorCode::ResourceExhausted:
        result = "Resource exhausted";
        break;
      case FirestoreErrorCode::FailedPrecondition:
        result = "Failed precondition";
        break;
      case FirestoreErrorCode::Aborted:
        result = "Aborted";
        break;
      case FirestoreErrorCode::OutOfRange:
        result = "Out of range";
        break;
      case FirestoreErrorCode::Unimplemented:
        result = "Unimplemented";
        break;
      case FirestoreErrorCode::Internal:
        result = "Internal";
        break;
      case FirestoreErrorCode::Unavailable:
        result = "Unavailable";
        break;
      case FirestoreErrorCode::DataLoss:
        result = "Data loss";
        break;
      default:
        result = StringFormat("Unknown code(%s)", code());
        break;
    }
    result += ": ";
    result += state_->msg;
    return result;
  }
}

void Status::IgnoreError() const {
  // no-op
}

Status Status::Annotate(absl::string_view msg) const {
  if (ok() || msg.empty()) return *this;

  absl::string_view new_msg = msg;
  std::string annotated;
  if (!error_message().empty()) {
    absl::StrAppend(&annotated, error_message(), "; ", msg);
    new_msg = annotated;
  }
  return Status{code(), new_msg};
}

std::string StatusCheckOpHelperOutOfLine(const Status& v, const char* msg) {
  HARD_ASSERT(!v.ok());
  std::string r("Non-OK-status: ");
  r += msg;
  r += " status: ";
  r += v.ToString();
  return r;
}

}  // namespace util
}  // namespace firestore
}  // namespace firebase
