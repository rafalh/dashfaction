#pragma once

#include "../rf/bmpman.h"

void bm_set_dynamic(int bm_handle, bool dynamic);
bool bm_is_dynamic(int bm_handle);
void bm_change_format(int bm_handle, rf::BmFormat format);
void bm_apply_patch();
bool bm_is_compressed_format(rf::BmFormat format);
