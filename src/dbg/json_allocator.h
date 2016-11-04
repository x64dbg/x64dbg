#pragma once

void* json_malloc(size_t size);
void json_free(void* ptr);
void json_reserve_cheap(size_t size);
void json_free_cheap();