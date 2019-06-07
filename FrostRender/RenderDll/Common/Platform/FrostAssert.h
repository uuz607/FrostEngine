#pragma once

#include<assert.h>
#define FROST_ASSERT_TRACE(condition,parenthese_message) assert(condition)
#define FROST_ASSERT_MESSAGE(condition,...)              assert(condition)
#define FROST_ASSERT(condition)                          assert(condition)