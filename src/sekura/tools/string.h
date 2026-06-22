#pragma once

#include <stddef.h>

void *memcpy(void *dest, const void *src, size_t n);
void *memcpy_sse(void *dest, const void *src, size_t n);
size_t strlen(const char *s);
int strcmp(char* s1, char* s2);
char *strrchr(const char *s, int c);
void* memcpy_rep(void* dest, const void* src, size_t n);