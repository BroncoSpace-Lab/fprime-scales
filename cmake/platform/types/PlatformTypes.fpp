# PlatformTypes.fpp
#
# Platform type definitions for Linux/Darwin-style targets.
#
# This file defines the logical platform types used by F Prime.
# It is intended to replace the C/C++ typedef-style PlatformTypes.h
# for FPP-based type configuration.

module Fw {

  # ----------------------------------------------------------------------
  # Logical platform integer types
  # ----------------------------------------------------------------------

  @ Platform signed integer type
  type PlatformIntType = I32

  @ Platform unsigned integer type
  type PlatformUIntType = U32

  @ Platform index type
  type PlatformIndexType = PlatformIntType

  @ Platform signed size type
  type PlatformSignedSizeType = I64

  @ Platform unsigned size type
  type PlatformSizeType = U64

  @ Platform assert argument type
  type PlatformAssertArgType = PlatformIntType

  @ Platform task priority type
  type PlatformTaskPriorityType = PlatformIntType

  @ Platform queue priority type
  type PlatformQueuePriorityType = PlatformIntType

  # ----------------------------------------------------------------------
  # Pointer cast type
  # ----------------------------------------------------------------------
  #
  # Original C header selected this based on __SIZEOF_POINTER__.
  # For normal 64-bit Linux/Darwin targets, use U64.
  #
  # For a 32-bit target, change this to:
  #
  #   type PlatformPointerCastType = U32
  #
  # For a 16-bit target:
  #
  #   type PlatformPointerCastType = U16
  #
  # For an 8-bit target:
  #
  #   type PlatformPointerCastType = U8

  @ Platform pointer cast type
  type PlatformPointerCastType = U64

}