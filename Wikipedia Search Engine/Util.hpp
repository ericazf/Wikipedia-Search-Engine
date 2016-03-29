//
//  util.hpp
//  Wikipedia Search Engine
//
//  Created by 甘流卓 on 16/2/28.
//  Copyright © 2016年 甘流卓. All rights reserved.
//

#ifndef util_hpp
#define util_hpp

#include <stdio.h>
#include <string>
#include <codecvt>
#include <locale>
#include <sys/time.h>
#include <stdlib.h>
#include <time.h>
using namespace std;

class Util{
    
private:
    static const int TIMEVAL_STR_BUFFER = 40;
    
public:
    
    static u32string utf8toutf32(const string &utf8str);
    
    static string utf32toutf8(const u32string &utf32str);
    
    static void print_time_diff();
    
    static void timeval_to_str(struct timeval *clock, char *const buffer);
    
    static double timeval_to_double(struct timeval *tv);
};

#endif /* util_hpp */
