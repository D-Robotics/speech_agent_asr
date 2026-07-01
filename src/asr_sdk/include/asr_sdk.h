#ifndef ASR_SDK_H
#define ASR_SDK_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Error codes
#define SA_OK                    0
#define SA_ERR_INVALID_PARAM    -1
#define SA_ERR_NOT_INITIALIZED  -2
#define SA_ERR_AUTH_FAILED      -3
#define SA_ERR_ENGINE_BUSY      -4
#define SA_ERR_ENGINE_INIT      -5
#define SA_ERR_ENGINE_FAILED    -6
#define SA_ERR_TEXT_EMPTY       -7  // Reserved for TTS compatibility
#define SA_ERR_CONFIG_LOAD      -8
#define SA_ERR_THREAD_CREATE    -9

const char* sa_error_string(int error_code);

// Configuration
typedef struct {
    const char* log_file;
    const char* log_level;
    const char* asr_engine_path;
    const char* data_path;
    const char* auth_appid;
    const char* auth_sn;
    const char* auth_lib_path;
    const char* license_file;
} sa_config_t;

// SDK lifecycle
int  sa_init(const sa_config_t* config);
void sa_destroy(void);

// ASR session
typedef void (*sa_asr_callback_t)(const char* text, int is_final, void* user_data);
typedef struct sa_asr_handle_t* sa_asr_handle;

int  sa_asr_create(sa_asr_handle* handle, sa_asr_callback_t callback, void* user_data);
int  sa_asr_write(sa_asr_handle handle, const void* pcm_data, size_t pcm_len);
int  sa_asr_finish(sa_asr_handle handle);
void sa_asr_destroy(sa_asr_handle handle);

#ifdef __cplusplus
}
#endif
#endif
