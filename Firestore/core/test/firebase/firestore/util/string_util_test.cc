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

#include "Firestore/core/src/firebase/firestore/util/string_util.h"

#include "gtest/gtest.h"

namespace firebase {
namespace firestore {
namespace util {

TEST(StringUtilTest, PrefixSuccessor) {
  EXPECT_EQ(PrefixSuccessor("a"), "b");
  EXPECT_EQ(PrefixSuccessor("aaAA"), "aaAB");
  EXPECT_EQ(PrefixSuccessor("aaa\xff"), "aab");
  EXPECT_EQ(PrefixSuccessor(std::string("\x00", 1)), "\x01");
  EXPECT_EQ(PrefixSuccessor("az\xe0"), "az\xe1");
  EXPECT_EQ(PrefixSuccessor("\xff\xff\xff"), "");
  EXPECT_EQ(PrefixSuccessor(""), "");
}

TEST(StringUtilTest, ImmediateSuccessor) {
  EXPECT_EQ(ImmediateSuccessor("hello"), std::string("hello\0", 6));
  EXPECT_EQ(ImmediateSuccessor(""), std::string("\0", 1));
}

TEST(StringUtilTest, TrimTrailingNullsEmpty) {
  std::string str;
  TrimTrailingNulls(&str);
  EXPECT_EQ(std::string{}, str);

  str = std::string(1, '\0');
  TrimTrailingNulls(&str);
  EXPECT_EQ(std::string{}, str);

  str = std::string(10, '\0');
  TrimTrailingNulls(&str);
  EXPECT_EQ(std::string{}, str);
}

// A simulated API call that generates null embedded data.
void SimulatedCall(std::string* str, size_t len) {
  for (size_t i = 0; i < len; ++i) {
    (*str)[i] = i == 1 ? '\0' : 'a' + i;
  }
}

TEST(StringUtilTest, TrimTrailingNulls) {
  // Some APIs tell the length of the string
  size_t len = 3;
  std::string str(len + 1, '\0');
  SimulatedCall(&str, len);

  // We allocated enough space for the string and null, now resize
  ASSERT_EQ((std::string{'a', '\0', 'c', '\0'}), str);
  TrimTrailingNulls(&str);
  EXPECT_EQ((std::string{'a', '\0', 'c'}), str);

  // Some APIs tell the lenghth of the buffer and len + 1 allocates too much.
  len = 4;
  str = std::string(len + 1, '\0');
  SimulatedCall(&str, len - 1);

  // We allocated too much space.
  ASSERT_EQ((std::string{'a', '\0', 'c', '\0', '\0'}), str);
  TrimTrailingNulls(&str);
  EXPECT_EQ((std::string{'a', '\0', 'c'}), str);
}

}  // namespace util
}  // namespace firestore
}  // namespace firebase
