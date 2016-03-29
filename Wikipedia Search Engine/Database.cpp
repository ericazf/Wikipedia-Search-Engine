//
//  Database.cpp
//  Wikipedia Search Engine
//
//  Created by 甘流卓 on 16/2/28.
//  Copyright © 2016年 甘流卓. All rights reserved.
//

#include "Database.hpp"

/*
 * @param db_path 数据库路径
 */
Database::Database(string db_path){
    if (sqlite3_open(db_path.c_str(), &db) != SQLITE_OK) {
        cout<<"Cannot open database"<<db_path<<endl;
        exit(-1);
    }
    
    //建表
    sqlite3_exec(db, "CREATE TABLE documents(id INTEGER PRIMARY KEY,title TEXT NOT NULL,body TEXT NOT NULL);", nullptr, nullptr, nullptr);
    sqlite3_exec(db, "CREATE TABLE tokens(id INTEGER PRIMARY KEY,token TEXT NOT NULL,docs_count INT NOT NULL,postings BLOB NOT NULL);", nullptr, nullptr, nullptr);
    sqlite3_exec(db, "CREATE UNIQUE INDEX token_index on tokens(token);", nullptr, nullptr, nullptr);
    sqlite3_exec(db, "CREATE UNIQUE INDEX title_index on documents(title);", nullptr, nullptr, nullptr);
    
    sqlite3_prepare(db, "SELECT id FROM documents WHERE title= ?;", -1, &get_document_id_st, nullptr);
    sqlite3_prepare(db, "SELECT id FROM tokens WHERE token= ?;", -1, &get_token_id_st, nullptr);
    sqlite3_prepare(db, "INSERT INTO documents(title,body) values(?,?)", -1, &insert_document_st, nullptr);
    sqlite3_prepare(db, "UPDATE documents set body = ? WHERE id = ?;", -1, &update_document_st, nullptr);
    sqlite3_prepare(db, "INSERT INTO tokens(token,docs_count,postings) values(?,0,?)", -1, &insert_token_st, nullptr);
    sqlite3_prepare(db, "SELECT postings,docs_count FROM tokens where id = ?", -1, &get_postings_st, nullptr);
    sqlite3_prepare(db, "UPDATE tokens SET docs_count = ?, postings = ? WHERE id = ?", -1, &update_postings_st, nullptr);
    sqlite3_prepare(db, "SELECT COUNT(*) FROM documents;", -1, &get_document_count_st, nullptr);
    sqlite3_prepare(db, "SELECT title FROM documents where id = ?;", -1, &get_document_title_st, nullptr);
    
    sqlite3_prepare(db, "BEGIN;", -1, &begin_st, nullptr);
    sqlite3_prepare(db, "COMMIT;", -1, &commit_st, nullptr);
}

/*
 * @param token_id 词语id
 * @param postings 用于指向获得的倒排列表
 * @param docs_count 用于存放总文档数
 * @param postings_size 用于存放字节序列的长度
 */
int Database::get_postings(const int &token_id, void **postings,int &docs_count,int &postings_size){
    sqlite3_reset(get_postings_st);
    sqlite3_bind_int(get_postings_st, 1, token_id);
    int rc = sqlite3_step(get_postings_st);
    if (rc == SQLITE_ROW) {
        docs_count = sqlite3_column_int(get_postings_st, 1);
        if (docs_count != 0) {
            *postings = (void*)sqlite3_column_blob(get_postings_st, 0);
            postings_size = static_cast<int>(sqlite3_column_bytes(get_postings_st, 0));
        }else{
            postings = nullptr;
            postings_size = 0;
        }
        return 1;
    }
    return 0;
}

/*
 * @param token 词元
 * @param insert 词元不存在的时候是否插入
 * 不存在的时候会把词语加到数据库里
 */
int Database::get_token_id(string token,bool insert){
    sqlite3_reset(get_token_id_st);
    sqlite3_bind_text(get_token_id_st, 1, token.c_str(), static_cast<int>(token.length()), SQLITE_STATIC);
    int rc = sqlite3_step(get_token_id_st);
    if (rc == SQLITE_ROW) {
        return sqlite3_column_int(get_token_id_st, 0);
    }else{
        if (insert) {
            sqlite3_reset(insert_token_st);
            sqlite3_bind_text(insert_token_st, 1, token.c_str(), static_cast<int>(token.length()), SQLITE_STATIC);
            sqlite3_bind_text(insert_token_st, 2, "", 0, SQLITE_STATIC);
            sqlite3_step(insert_token_st);
            sqlite3_reset(get_token_id_st);
            sqlite3_bind_text(get_token_id_st, 1, token.c_str(), static_cast<int>(token.length()), SQLITE_STATIC);
            rc = sqlite3_step(get_token_id_st);
            if (rc == SQLITE_ROW) {
                return sqlite3_column_int(get_token_id_st, 0);
            }else{
                return 0;
            }
        }
        return 0;
    }
}

/*
 * @param title 文档标题
 * 不存在返回0
 */
int Database::get_document_id(string title){
    sqlite3_reset(get_document_id_st);
    sqlite3_bind_text(get_document_id_st, 1, title.c_str(), static_cast<int>(title.length()), SQLITE_STATIC);
    int rc = sqlite3_step(get_document_id_st);
    if (rc == SQLITE_ROW) {
        return sqlite3_column_int(get_document_id_st, 0);
    }else{
        return 0;
    }
}

/*
 * @param title 文档标题
 * @param text 文档内容
 *  数据插入前会先检测是否已经存在该文档，如果存在则直接覆盖
 */
int Database::insert_document(string title, string text){
    sqlite3_stmt *st;
    int document_id = get_document_id(title);
    if (document_id == 0) {
        st = insert_document_st;
        sqlite3_reset(st);
        sqlite3_bind_text(st, 1, title.c_str(), static_cast<int>(title.length()), SQLITE_STATIC);
        sqlite3_bind_text(st, 2, text.c_str(), static_cast<int>(text.length()),SQLITE_STATIC);
    }else{
        st = update_document_st;
        sqlite3_reset(st);
        sqlite3_bind_text(st, 1, text.c_str(), static_cast<int>(text.length()), SQLITE_STATIC);
        sqlite3_bind_int(st, 2, document_id);
    }
query:
    int rc = sqlite3_step(st);
    switch (rc) {
        case SQLITE_BUSY:
            goto query;
        case SQLITE_ERROR:
            cout<<"Insert error : "<<sqlite3_errmsg(db)<<endl;
            break;
        case SQLITE_MISUSE:
            cout<<"Insert misuse : "<<sqlite3_errmsg(db)<<endl;
            break;
    }
    return rc;
}

/*
 * @param token_id 词语id
 * @param postings 倒排列表字节序列
 * @param postings_len 倒排列表字节序列长度
 * @param docs_count 出现该词语的总文档数
 */
int Database::update_postings(const int &token_id, void *postings, const int &postings_len, const int &docs_count){
    sqlite3_reset(update_postings_st);
    sqlite3_bind_int(update_postings_st, 1, docs_count);
    sqlite3_bind_blob(update_postings_st, 2, postings, postings_len, SQLITE_STATIC);
    sqlite3_bind_int(update_postings_st, 3, token_id);
    
query:
    int rc = sqlite3_step(update_postings_st);
    switch (rc) {
        case SQLITE_BUSY:
            goto query;
        case SQLITE_ERROR:
            cout<<"Error : "<<sqlite3_errmsg(db)<<endl;
            break;
        case SQLITE_MISUSE:
            cout<<"Misuse : "<<sqlite3_errmsg(db)<<endl;
            break;
    }
    return rc;
}

/*
 * 获取总文档数
 * @ret 返回文档数，文档不存在的时候返回-1
 */
int Database::get_documents_count(){
    sqlite3_reset(get_document_count_st);
    int rc = sqlite3_step(get_document_count_st);
    if (rc == SQLITE_ROW) {
        return sqlite3_column_int(get_document_count_st, 0);
    }else{
        return -1;
    }
}

/*
 * @param doc_id 文档编号
 * @param title 文档标题（返回值）
 */
void Database::get_document_title(int &doc_id, string &title){
    sqlite3_reset(get_document_title_st);
    sqlite3_bind_int(get_document_title_st, 1, doc_id);
    int rc = sqlite3_step(get_document_title_st);
    if (rc == SQLITE_ROW) {
        const unsigned char *c = sqlite3_column_text(get_document_title_st, 0);
        int text_size = static_cast<int>(sqlite3_column_bytes(get_document_title_st, 0));
        for (int i = 0; i < text_size; ++i) {
            title += c[i];
        }
    }
}


void Database::begin(){
    sqlite3_step(begin_st);
}

void Database::commit(){
    sqlite3_step(commit_st);
}

void Database::finish(){
    sqlite3_finalize(insert_document_st);
    sqlite3_finalize(get_document_id_st);
    sqlite3_finalize(get_token_id_st);
    sqlite3_finalize(update_document_st);
    sqlite3_finalize(insert_token_st);
    sqlite3_finalize(get_postings_st);
    sqlite3_finalize(update_postings_st);
    sqlite3_finalize(begin_st);
    sqlite3_finalize(commit_st);
    sqlite3_finalize(get_document_count_st);
    sqlite3_finalize(get_document_title_st);
    sqlite3_close(db);
}