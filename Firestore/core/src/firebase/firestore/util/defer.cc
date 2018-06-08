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

#include "Firestore/core/src/firebase/firestore/util/defer.h"

#include "absl/base/config.h"

namespace firebase {
namespace firestore {
namespace util {

Defer::~Defer() {
#if ABSL_HAVE_EXCEPTPIONS
  try {
    action_();
  } catch (...) {
    // Destructors must not throw
  }

#else
  action_();
#endif
}

}  // namespace util
}  // namespace firestore
}  // namespace firebase