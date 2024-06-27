#pragma once

#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>
#include <future>
#include <thread>
#include <mutex>
#include <condition_variable>

#include <SDL2/SDL.h>

using u8    = uint8_t;
using u16   = uint16_t;
using u32   = uint32_t;
using u64   = uint64_t;
using usize = size_t;

using i8    = int8_t;
using i16   = int16_t;
using i32   = int32_t;
using i64   = int64_t;
using isize = ssize_t;

#define BIT(n, b) ((n & (1 << (b))) ? 1 : 0)
#define SET_BIT(n, b, v) { if ((v) != 0) { n |= (1 << (b)); } else { n &= ~(1 << (b)); } }