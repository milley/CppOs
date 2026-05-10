/* string.h - 字符串工具函数 */

#ifndef STRING_H
#define STRING_H

#include <stdint.h>
#include <stddef.h>

/* 字符串长度 */
size_t strlen(const char *str);

/* 字符串复制 */
char *strcpy(char *dest, const char *src);

/* 字符串比较 */
int strcmp(const char *s1, const char *s2);

/* 内存复制 */
void *memcpy(void *dest, const void *src, size_t n);

/* 内存设置 */
void *memset(void *s, int c, size_t n);

/* 内存比较 */
int memcmp(const void *s1, const void *s2, size_t n);

#endif
