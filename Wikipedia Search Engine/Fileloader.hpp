//
//  Fileloader.hpp
//  Wikipedia Search Engine
//
//  Created by 甘流卓 on 16/2/28.
//  Copyright © 2016年 甘流卓. All rights reserved.
//

#ifndef Fileloader_hpp
#define Fileloader_hpp

#include <stdio.h>
#include <string>
#include <expat.h>
#include <unordered_map>
#include <iostream>
#include "Posting.hpp"
#include "Util.hpp"
#include "Database.hpp"
using namespace std;

class Fileloader{
    
public:
    
    typedef enum {
        IN_DOCUMENT,
        IN_PAGE,
        IN_PAGE_TITLE,
        IN_PAGE_REVISION,
        IN_PAGE_REVISION_TEXT
    } tag_status;               //用于标记当前解析的标签范围
    
    typedef struct {
        tag_status status;
        string title;
        string text;
        int doc_count;
        int max_count;
        Fileloader *fl;
        Database *db;
    } parser_data;
    
    Fileloader(const string &XML_file_path,const string &doc_encoding,const int &max_count,Database &db);
    
    int load_file();
    
private:
    
    parser_data pd;
    XML_Parser xp;
    FILE *fp;
    int max_count = -1;
    static const long BUFFER_SIZE = 0x40000;
    
    //缓冲区小倒排索引
    unordered_map<int, vector<Posting>> inverted_index_buffer;
    
    //记录缓冲区文档数
    int buffer_doc_count = 0;
    
    //缓冲区文档上限
    const int buffer_update_threshold = 16384;
    
    //标签开始处理函数
    static void XMLCALL start(void *data_st,const XML_Char *tag,const XML_Char *attr[]);
    
    //标签结束处理函数
    static void XMLCALL end(void *data_st,const XML_Char *tag);
    
    //处理标签内元素
    static void XMLCALL element_data(void *data_st,const XML_Char *data,int data_size);
    
    static void handle_document(const string &title,const string &text,Fileloader &fl,Database &db);
};

#endif /* Fileloader_hpp */
