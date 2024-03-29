/*
 * Copyright (c) 2012-2018 Zeex
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <stddef.h>
#include <windows.h>

#define SUBHOOK_CODE_PROTECT_FLAGS PAGE_EXECUTE_READWRITE

int subhook_unprotect(void *address, size_t size) {
  DWORD old_flags;
  BOOL result = VirtualProtect(address,
                               size,
                               SUBHOOK_CODE_PROTECT_FLAGS,
                               &old_flags);
  return !result;
}

static HANDLE get_code_heap()
{
  static HANDLE heap = NULL;
  if (!heap)
    heap = HeapCreate(HEAP_CREATE_ENABLE_EXECUTE, 0, 0);
  return heap;
}

void *subhook_alloc_code(size_t size) {
  // return VirtualAlloc(NULL,
  //                     size,
  //                     MEM_COMMIT | MEM_RESERVE,
  //                     SUBHOOK_CODE_PROTECT_FLAGS);
  return HeapAlloc(get_code_heap(), 0, size); // changed
}

int subhook_free_code(void *address, size_t size) {
  (void)size;

  if (address == NULL) {
    return 0;
  }
  // return !VirtualFree(address, 0, MEM_RELEASE);
  return !HeapFree(get_code_heap(), 0, address); // changed
}
