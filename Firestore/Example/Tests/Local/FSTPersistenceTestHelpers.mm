/*
 * Copyright 2017 Google
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

#import "Firestore/Example/Tests/Local/FSTPersistenceTestHelpers.h"

#include <string>

#import "Firestore/Source/Local/FSTLevelDB.h"
#import "Firestore/Source/Local/FSTLocalSerializer.h"
#import "Firestore/Source/Local/FSTMemoryPersistence.h"
#import "Firestore/Source/Remote/FSTSerializerBeta.h"

#include "Firestore/core/src/firebase/firestore/model/database_id.h"
#include "Firestore/core/src/firebase/firestore/util/filesystem.h"
#include "Firestore/core/src/firebase/firestore/util/path.h"
#include "Firestore/core/src/firebase/firestore/util/status.h"
#include "Firestore/core/src/firebase/firestore/util/string_apple.h"
#include "Firestore/core/test/firebase/firestore/local/leveldb_testing.h"

namespace util = firebase::firestore::util;
using firebase::firestore::local::TestingLevelDbOpener;
using firebase::firestore::model::DatabaseId;
using firebase::firestore::util::Path;
using firebase::firestore::util::Status;

NS_ASSUME_NONNULL_BEGIN

@implementation FSTPersistenceTestHelpers

+ (FSTLevelDB *)levelDBPersistence {
  // This owns the DatabaseIds since we do not have FirestoreClient instance to own them.
  static DatabaseId database_id{"p", "d"};

  FSTSerializerBeta *remoteSerializer = [[FSTSerializerBeta alloc] initWithDatabaseID:&database_id];
  FSTLocalSerializer *serializer =
      [[FSTLocalSerializer alloc] initWithRemoteSerializer:remoteSerializer];

  FSTLevelDB *db =
      [[FSTLevelDB alloc] initWithOpener:TestingLevelDbOpener::Create() serializer:serializer];
  STATUS_CHECK_OK([db start]);
  return db;
}

+ (FSTMemoryPersistence *)memoryPersistence {
  FSTMemoryPersistence *persistence = [FSTMemoryPersistence persistence];
  STATUS_CHECK_OK([persistence start]);
  return persistence;
}

@end

NS_ASSUME_NONNULL_END
