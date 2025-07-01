#pragma once
#ifndef _PLATFORM_BASE64_CODE_H_
#define _PLATFORM_BASE64_CODE_H_
size_t base64_encode(char* target, const void* source, size_t bytes);
size_t base64_encode_url(char* target, const void* source, size_t bytes);
size_t base64_decode(void* target, const char* src, size_t bytes);
size_t base16_encode(char* target, const void* source, size_t bytes);
size_t base16_decode(void* target, const char* source, size_t bytes);
size_t base32_encode(char* target, const void* source, size_t bytes);
size_t base32_decode(void* target, const char* src, size_t bytes);
#endif