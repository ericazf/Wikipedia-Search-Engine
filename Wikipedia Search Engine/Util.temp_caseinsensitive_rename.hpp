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

class Util{
    
};
std::u32string utf8toutf32(const std::string &utf8str);

std::string utf32toutf8(const std::u32string &utf32str);

#endif /* util_hpp */
