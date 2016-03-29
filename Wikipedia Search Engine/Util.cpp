//
//  util.cpp
//  Wikipedia Search Engine
//
//  Created by 甘流卓 on 16/2/28.
//  Copyright © 2016年 甘流卓. All rights reserved.
//

#include "Util.hpp"

u32string Util::utf8toutf32(const string &utf8str){
    wstring_convert<codecvt_utf8<char32_t>,char32_t> utf8_ucs4_cvt;
    return utf8_ucs4_cvt.from_bytes(utf8str);
}


string Util::utf32toutf8(const u32string &utf32str){
    wstring_convert<codecvt_utf8<char32_t>,char32_t> utf8_ucs4_cvt;
    return utf8_ucs4_cvt.to_bytes(utf32str);
}

void Util::timeval_to_str(struct timeval *clock, char *const buffer) {
    struct tm result;
    localtime_r(&clock->tv_sec, &result);
    sprintf(buffer, "%04d/%02d/%02d %02d:%02d:%02d.%06d",
            result.tm_year + 1900,
            result.tm_mon + 1,
            result.tm_mday,
            result.tm_hour,
            result.tm_min,
            result.tm_sec,
            (int) (clock->tv_usec / 100000)
            );
}

double Util::timeval_to_double(struct timeval *tv) {
    return ((double) (tv->tv_sec) + (double) (tv->tv_usec) * 0.000001);
}

void Util::print_time_diff() {
    char datetime_buf[TIMEVAL_STR_BUFFER];
    static double pre_time = 0.0;
    struct timeval tv;
    gettimeofday(&tv, NULL);
    timeval_to_str(&tv, datetime_buf);
    double current_time = timeval_to_double(&tv);
    
    if (pre_time) {
        double time_diff = current_time - pre_time;
        printf("[time] %s (diff %10.6lf)\n", datetime_buf, time_diff);
    } else {
        printf("[time] %s\n", datetime_buf);
    }
    pre_time = current_time;
}