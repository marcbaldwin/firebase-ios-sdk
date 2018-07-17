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

#import <Foundation/Foundation.h>

#import "FSTFuzzTestTargets.h"

#include "Firestore/core/src/firebase/firestore/model/database_id.h"
#include "Firestore/core/src/firebase/firestore/remote/serializer.h"

using firebase::firestore::model::DatabaseId;
using firebase::firestore::remote::Serializer;

void firebase::firestore::fuzzing::FuzzTestDeserialization(const uint8_t *data, size_t size) {
  Serializer serializer{DatabaseId{"project", DatabaseId::kDefault}};

  @autoreleasepool {
    @try {
      serializer.DecodeFieldValue(data, size);
    } @catch (...) {
      // Caught exceptions are ignored because the input might be malformed and
      // the deserialization might throw an error as intended. Fuzzing focuses on
      // runtime errors that are detected by the sanitizers.
    }
  }
}

NSString *firebase::firestore::fuzzing::GetSerializerDictionaryLocation(NSString *resources_location) {
  return [resources_location stringByAppendingPathComponent:@"Serializer/serializer.dictionary"];
}

NSString *firebase::firestore::fuzzing::GetSerializerCorpusLocation(NSString *resources_location) {
  return @"FuzzTestsCorpus";
}