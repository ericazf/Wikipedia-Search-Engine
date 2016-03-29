//
//  main.cpp
//  Wikipedia Search Engine
//
//  Created by 甘流卓 on 16/2/28.
//  Copyright © 2016年 甘流卓. All rights reserved.
//

#include <iostream>
#include "Engine.hpp"
#include "Database.hpp"
#include "Fileloader.hpp"
#include "Util.hpp"
#include <type_traits>
#include <iomanip>
#include <numeric>
#include <unistd.h>

int main(int argc, const char * argv[]) {
    
    /*
     *  db_path 数据库路径
     *  xml_path 维基百科词条xml文件路径
     *  max_doc_count 读取词条数
     *  doc_encoding 文件编码格式
     */
    std::string db_path = "";
    std::string xml_path = "";
    int max_doc_count = 1000;
    std::string doc_encoding = "UTF-8";
    Engine engine(xml_path,doc_encoding,max_doc_count,db_path);
    engine.create_inverted_index();
    
//    Engine engine(db_path);
//    string query = "数学";
//    engine.search(query);
    return 0;
}
