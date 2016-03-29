//
//  Database.hpp
//  Wikipedia Search Engine
//
//  Created by 甘流卓 on 16/2/28.
//  Copyright © 2016年 甘流卓. All rights reserved.
//

#ifndef Database_hpp
#define Database_hpp

#include <stdio.h>
#include <sqlite3.h>
#include <iostream>
#include <string>
using namespace std;

class Database{
    
private:
    sqlite3 *db;
    sqlite3_stmt *insert_document_st;
    sqlite3_stmt *get_document_id_st;
    sqlite3_stmt *get_token_id_st;
    sqlite3_stmt *update_document_st;
    sqlite3_stmt *insert_token_st;
    sqlite3_stmt *get_postings_st;
    sqlite3_stmt *update_postings_st;
    sqlite3_stmt *begin_st;
    sqlite3_stmt *commit_st;
    sqlite3_stmt *get_document_count_st;
    sqlite3_stmt *get_document_title_st;
    
public:
    Database(string db_path);
    int insert_document(string title,string text);
    int get_document_id(string title);
    int get_token_id(string token,bool insert);
    int get_postings(const int &token_id,void **postings,int &docs_count,int &postings_size);
    int update_postings(const int &token_id,void *postings,const int &postings_len,const int &docs_count);
    int get_documents_count();
    void get_document_title(int &doc_id,string &title);
    void begin();
    void commit();
    void finish();
};

#endif /* Database_hpp */
