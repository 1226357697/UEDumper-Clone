#pragma once
#include <types.h>

#ifdef GetObject
#undef GetObject
#endif

#define USE_UE_DUMPER 1

enum class STATUS {
  SUCCESS,
  FAILED,
  WINDOW_NOT_FOUND,
  PROCESS_NOT_FOUND,
  READER_ERROR,
  CANNOT_GET_PROCNAME,
  MODULE_NOT_FOUND,
  ENGINE_NOT_FOUND,
  ENGINE_FAILED,
  CANNOT_READ,
  INVALID_IMAGE,
  FILE_NOT_OPEN,
  ZERO_PACKAGES
};