#ifndef UNIX_SECS_H
#define UNIX_SECS_H

#include <chrono>
using namespace std::chrono;


//for mocking time in test
extern int time_offset;

inline seconds::rep time_secs() {
    return duration_cast<seconds>(
            system_clock::now().time_since_epoch()
        ).count() + time_offset;
}

#endif
