#ifndef _HEALTH_ALGORITHM_H_
#define _HEALTH_ALGORITHM_H_

#ifdef __cplusplus
extern "C" {
#endif

#pragma anon_unions

typedef enum step_mode {
    STEP_MODE_WALK_SLOW = 0,
    STEP_MODE_WALK_QUICK,
    STEP_MODE_RUN,
} step_mode_t;

typedef enum sleep_mode {
    NONE_SLEEP = 3,
    LIGHT_SLEEP = 2,
    DEEP_SLEEP = 1,
} sleep_mode_t;

typedef enum algorithm_event_type {
    SLEEP_EVENT = 1,
    DISTANCE_EVENT = 2,
} algorithm_event_type_t;

typedef struct algorithm_event_common {
    algorithm_event_type_t type;
} algorithm_event_common_t;

typedef struct algorithm_event_sleep {
    sleep_mode_t mode;
    int starting_time_stamp;
} algorithm_event_sleep_t;

typedef struct algorithm_event_distance {
    int new_steps;
    int new_distances;  // 10,000 times of the true distances
    step_mode_t mode;   
    int new_calory;     // 10,000 times of the true calory
} algorithm_event_distance_t;

typedef struct algorithm_event {
    algorithm_event_common_t event_common;
    union {
        algorithm_event_sleep_t sleep;
        algorithm_event_distance_t distance;
    } ;
} algorithm_event_t;

typedef void(*health_algorithm_cb)(algorithm_event_t *event, void* user_data);

/*
before call any other function ,you need to call init_health_algorithm() first.
param:
    sample_rate : sensor data sample rate in HZ
    hight_cm: user hight in cm
    weight_kg: user weight in kg
    age: user age
    isfemale: 1 for female, 0 for male
return: 0 or -1
        0 means every thing ok
        -1 means something wrong happen, for example the sample rate is not allowed by system
*/
int init_health_algorithm(int sample_rate, int hight_cm, int weight_kg, int age, char isfemale);

int health_algorithm_finalize(void);

/*
 *after init the algorithm, you should call register_health_algorithm_callback() before you pass any sensor data
 *into algorithm. then when you pass sensor data into algorithm, if something happen, for example new step
 * detected, algorithm will call you registered callback
 * param: cb:
 *       NOTE, when you call pass sensor data in A thread ,if algorithm detected new step, this callback will be called in A thread
 *       ,so you should take care of time problem when call pass_data().
 *       user_data: you can pass you data in when register, this data will
 *       passed to you when call you callback function
 *return: 0 or -1
 *        0 means every thing ok
 *        -1 means something wrong happen in algorithm,you should check it
 *
 */
int register_health_algorithm_callback(health_algorithm_cb cb, void *user_data);

/*
 *you call health_algorithm_data_in_accelerate() pass the accelerate sensor data into
 *algorithm.
 *param:
 *      x,y,z : the 3D accelerate data
 *      compose: compose = sqrt(X*x + y*y + Z*Z)
 *return: 0 or -1
 *        0 means every thing ok
 *        -1 means something wrong happen in algorithm,you should check it
 *
 */
int  health_algorithm_data_in_accelerate(short int x, short int y, short int z, int compose, long long timestamp);

const char * health_algorithm_get_error_desc(void);
  
#ifdef __cplusplus
}
#endif
#endif
