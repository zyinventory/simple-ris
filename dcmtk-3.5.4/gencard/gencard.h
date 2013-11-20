#pragma once

struct lock_key_st { unsigned short key[4]; long lockNumber; };
typedef struct lock_key_st LOCK_KEY;
LOCK_KEY security;
