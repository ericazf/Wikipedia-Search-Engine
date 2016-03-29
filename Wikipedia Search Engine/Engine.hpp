//
//  Engine.hpp
//  Wikipedia Search Engine
//
//  Created by 甘流卓 on 16/2/28.
//  Copyright © 2016年 甘流卓. All rights reserved.
//

#ifndef Engine_hpp
#define Engine_hpp

#include <stdio.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include "Database.hpp"
#include "Fileloader.hpp"
#include "Posting.hpp"
using namespace std;

typedef struct {
    string token;
    int docs_count;
    int position;   //用于记录在需查询的字符串里该词语的位置
    int cur_position;   //记录当前历遍的倒排项的位置
    vector<Posting> postings_vec;
} token_info;

typedef struct {
    int document_id;
    string document_title;
    double rank;
} doc_result;

class Engine{
    
private:
    
    string xml_path;
    
    string doc_encoding;
    
    string db_path;
    
    int max_doc_count;                      //读取文档数，为-1时表示全部读取
    
    void search_docs(vector<token_info> &token_info_vec,Database &db);
    
    vector<doc_result> ret_result;
    
    bool search_parse(const vector<token_info> &token_info_vec);
    
    void add_search_result(const vector<token_info> &token_info_vec,const double &rank,Database &db);
    
    double calc_rank(const vector<token_info> &token_info_vec);
    
    int database_doc_count;
    
    void display_result();
    
public:
    
    Engine(string &xml_path,string &doc_encoding,int &max_doc_count,string &db_path);
    
    Engine(string &db_path);
    
    void create_inverted_index();
    
    void search(string &query);
};

#endif /* Engine_hpp */
