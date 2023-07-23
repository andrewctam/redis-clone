#ifndef UNIX_TIMES_H
#define UNIX_TIMES_H

#include <chrono>
using namespace std::chrono;


//for mocking time in test
extern int secs_offset;

inline seconds::rep time_secs() {
    return duration_cast<seconds>(
            system_clock::now().time_since_epoch()
        ).count() + secs_offset;
}

inline milliseconds::rep time_ms() {
    return duration_cast<milliseconds>(
            system_clock::now().time_since_epoch()
        ).count();
}

#endif
