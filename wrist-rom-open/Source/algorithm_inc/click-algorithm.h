#ifndef _HEALTH_ALGORITHM_INNER_H_
#define _HEALTH_ALGORITHM_INNER_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct click_event {
    long long click_time;
#ifdef CLICK_TEST_MODE
    char is_valid;
#endif // CLICK_TEST_MODE
} click_event_t;


typedef void(*click_algorithm_cb)(click_event_t *event, void* user_data);

int init_click_algorithm(int sample_rate);
int click_algorithm_finalize(void);
int register_click_algorithm_callback(click_algorithm_cb cb, void *user_data);
int click_algorithm_accelerate_data_in(short int x, short int y, short int z, int compose, long long timestamp);
const char * click_algorithm_error_desc(void);
int generate_click_event(click_event_t *event);

#ifdef __cplusplus
}
#endif
  
#endif
