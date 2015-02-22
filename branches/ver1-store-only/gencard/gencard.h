#pragma once

#define MAX_MEDIA_COUNT 0xFFFF
#define MODE_FLAG_POS	300
#define BATCH_INIT_FLAG	1

struct lock_key_st { unsigned short key[4]; long lockNumber; };
typedef struct lock_key_st LOCK_KEY;
LOCK_KEY security;
