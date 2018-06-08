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

#import "Firestore/Source/Local/FSTLevelDB.h"

#include <memory>
#include <utility>

#import "FIRFirestoreErrors.h"
#import "Firestore/Source/Local/FSTLevelDBMigrations.h"
#import "Firestore/Source/Local/FSTLevelDBMutationQueue.h"
#import "Firestore/Source/Local/FSTLevelDBQueryCache.h"
#import "Firestore/Source/Local/FSTLevelDBRemoteDocumentCache.h"
#import "Firestore/Source/Remote/FSTSerializerBeta.h"

#include "Firestore/core/src/firebase/firestore/auth/user.h"
#include "Firestore/core/src/firebase/firestore/core/database_info.h"
#include "Firestore/core/src/firebase/firestore/local/leveldb_transaction.h"
#include "Firestore/core/src/firebase/firestore/model/database_id.h"
#include "Firestore/core/src/firebase/firestore/util/hard_assert.h"
#include "Firestore/core/src/firebase/firestore/util/string_apple.h"
#include "absl/memory/memory.h"
#include "leveldb/db.h"

namespace util = firebase::firestore::util;
using firebase::firestore::auth::User;
using firebase::firestore::core::DatabaseInfo;
using firebase::firestore::model::DatabaseId;
using firebase::firestore::local::LevelDbOpener;
using firebase::firestore::local::LevelDbTransaction;
using leveldb::DB;
using leveldb::Options;
using leveldb::ReadOptions;
using leveldb::Status;
using leveldb::WriteOptions;

NS_ASSUME_NONNULL_BEGIN

static NSString *const kReservedPathComponent = @"firestore";

@interface FSTLevelDB ()

@property(nonatomic, assign, getter=isStarted) BOOL started;
@property(nonatomic, strong, readonly) FSTLocalSerializer *serializer;

@end

@implementation FSTLevelDB {
  std::unique_ptr<LevelDbOpener> _opener;
  std::unique_ptr<LevelDbTransaction> _transaction;
  std::unique_ptr<leveldb::DB> _ptr;
  FSTTransactionRunner _transactionRunner;
}

/**
 * For now this is paranoid, but perhaps disable that in production builds.
 */
+ (const ReadOptions)standardReadOptions {
  ReadOptions options;
  options.verify_checksums = true;
  return options;
}

- (instancetype)initWithOpener:(std::unique_ptr<LevelDbOpener>)opener
                    serializer:(FSTLocalSerializer *)serializer {
  if (self = [super init]) {
    _opener = std::move(opener);
    _serializer = serializer;
    _transactionRunner.SetBackingPersistence(self);
  }
  return self;
}

- (leveldb::DB *)ptr {
  return _ptr.get();
}

- (const FSTTransactionRunner &)run {
  return _transactionRunner;
}

#pragma mark - Startup

- (firebase::firestore::util::Status)start {
  HARD_ASSERT(!self.isStarted, "FSTLevelDB double-started!");
  self.started = YES;

  util::StatusOr<std::unique_ptr<leveldb::DB>> result = _opener->Open();
  if (!result.ok()) {
    return result.status();
  }

  _ptr = std::move(result).ValueOrDie();

  LevelDbTransaction transaction(_ptr.get(), "Start LevelDB");
  [FSTLevelDBMigrations runMigrationsWithTransaction:&transaction];
  transaction.Commit();

  return util::Status::OK();
}

- (LevelDbTransaction *)currentTransaction {
  HARD_ASSERT(_transaction != nullptr, "Attempting to access transaction before one has started");
  return _transaction.get();
}

#pragma mark - Persistence Factory methods

- (id<FSTMutationQueue>)mutationQueueForUser:(const User &)user {
  return [FSTLevelDBMutationQueue mutationQueueWithUser:user db:self serializer:self.serializer];
}

- (id<FSTQueryCache>)queryCache {
  return [[FSTLevelDBQueryCache alloc] initWithDB:self serializer:self.serializer];
}

- (id<FSTRemoteDocumentCache>)remoteDocumentCache {
  return [[FSTLevelDBRemoteDocumentCache alloc] initWithDB:self serializer:self.serializer];
}

- (void)startTransaction:(absl::string_view)label {
  HARD_ASSERT(_transaction == nullptr, "Starting a transaction while one is already outstanding");
  _transaction = absl::make_unique<LevelDbTransaction>(_ptr.get(), label);
}

- (void)commitTransaction {
  HARD_ASSERT(_transaction != nullptr, "Committing a transaction before one is started");
  _transaction->Commit();
  _transaction.reset();
}

- (void)shutdown {
  HARD_ASSERT(self.isStarted, "FSTLevelDB shutdown without start!");
  self.started = NO;
  _ptr.reset();
}

- (_Nullable id<FSTReferenceDelegate>)referenceDelegate {
  return nil;
}

#pragma mark - Error and Status

+ (nullable NSError *)errorWithStatus:(Status)status description:(NSString *)description, ... {
  if (status.ok()) {
    return nil;
  }

  va_list args;
  va_start(args, description);

  NSString *message = [[NSString alloc] initWithFormat:description arguments:args];
  NSString *reason = [self descriptionOfStatus:status];
  NSError *result = [NSError errorWithDomain:FIRFirestoreErrorDomain
                                        code:FIRFirestoreErrorCodeInternal
                                    userInfo:@{
                                      NSLocalizedDescriptionKey : message,
                                      NSLocalizedFailureReasonErrorKey : reason
                                    }];

  va_end(args);

  return result;
}

+ (NSString *)descriptionOfStatus:(Status)status {
  return [NSString stringWithCString:status.ToString().c_str() encoding:NSUTF8StringEncoding];
}

@end

NS_ASSUME_NONNULL_END
