//
//  Fileloader.cpp
//  Wikipedia Search Engine
//
//  Created by 甘流卓 on 16/2/28.
//  Copyright © 2016年 甘流卓. All rights reserved.
//

#include "Fileloader.hpp"

Fileloader::Fileloader(const string &XML_file_path,const string &doc_encoding,const int &max_count,Database &db){
    this->max_count = max_count;
    xp = XML_ParserCreate(doc_encoding.c_str());
    if (!(fp = fopen(XML_file_path.c_str(), "rb"))) {
        cout<<"Cannot open xml file "<<XML_file_path<<endl;
        exit(-1);
    }
    pd = {
        IN_DOCUMENT,
        "",
        "",
        0,
        max_count,
        this,
        &db
    };
    
    //设置XML Parser的参数
    XML_SetElementHandler(xp, start, end);
    XML_SetCharacterDataHandler(xp, element_data);
    XML_SetUserData(xp, static_cast<void*>(&pd));
}

int Fileloader::load_file(){
    char *buffer = new char[BUFFER_SIZE];
    while (true) {
        int buffer_len,done;
        buffer_len = static_cast<int>(fread(buffer, 1, BUFFER_SIZE, fp));
        if (ferror(fp)) {
            cout<<"Read Wikipedia xml file error"<<endl;
            return 1;
        }
        done = feof(fp);
        
        int parser_code = XML_Parse(xp,buffer,buffer_len,done);
        if (parser_code == XML_STATUS_ERROR) {
            cout<<"Parse xml error, error code : "<<parser_code<<endl;
            return 2;
        }
        
        if (done || (max_count >= 0 && max_count <= pd.doc_count)) {
            break;
        }
    }
    
    //缓冲区里可能还存在倒排索引未与数据库进行合并
    if (!inverted_index_buffer.empty()) {
        Util::print_time_diff();
        for (auto iter = inverted_index_buffer.begin(); iter != inverted_index_buffer.end(); iter++) {
            Posting::update_postings_vec(iter->first,iter->second, *(pd.db));
        }
        inverted_index_buffer.clear();
        buffer_doc_count = 0;
        cout<<"Buffer flushed"<<endl;
        Util::print_time_diff();
    }
    
    if (fp) {
        fclose(fp);
    }
    XML_ParserFree(xp);
    delete []buffer;
    buffer = nullptr;
    return 0;
}

void XMLCALL Fileloader::start(void *user_data, const XML_Char *el, const XML_Char **attr){
    parser_data *p = static_cast<parser_data *>(user_data);
    switch (p->status) {
        case IN_DOCUMENT:
            //从page标签开始解析
            if (!strcmp(el, "page")) {
                p->status = IN_PAGE;
            }
            break;
        case IN_PAGE:
            if (!strcmp(el, "title")) {
                p->status = IN_PAGE_TITLE;
            }else if(!strcmp(el, "revision")){
                p->status = IN_PAGE_REVISION;
            }
            break;
        case IN_PAGE_TITLE:
            break;
        case IN_PAGE_REVISION:
            if (!strcmp(el, "text")) {
                p->status = IN_PAGE_REVISION_TEXT;
            }
            break;
        case IN_PAGE_REVISION_TEXT:
            break;
    }
}

void XMLCALL Fileloader::end(void *user_data, const XML_Char *el){
    parser_data *p = static_cast<parser_data*>(user_data);
    switch (p->status) {
        case IN_DOCUMENT:
            break;
        case IN_PAGE:
            if (!strcmp(el, "page")) {
                p->status = IN_DOCUMENT;
            }
            break;
        case IN_PAGE_TITLE:
            if (!strcmp(el, "title")) {
                p->status = IN_PAGE;
            }
            break;
        case IN_PAGE_REVISION:
            if (!strcmp(el, "revision")) {
                p->status = IN_PAGE;
            }
            break;
        case IN_PAGE_REVISION_TEXT:
            if (!strcmp(el, "text")) {
                p->status = IN_PAGE_REVISION;
                //对文档标题和文档内容进行处理
                if (p->max_count < 0 || p->doc_count < p->max_count) {
//                    cout<<"Title: "<<p->title<<endl;
//                    cout<<"Text: "<<p->text<<endl;
                    handle_document(p->title, p->text, *(p->fl), *(p->db));
                    p->doc_count++;
                    p->title = "";
                    p->text = "";
                }
            }
            break;
    }
}

void XMLCALL Fileloader::element_data(void *user_data, const XML_Char *data, int data_size){
    parser_data *p = static_cast<parser_data*>(user_data);
    switch (p->status) {
        case IN_PAGE_TITLE:
            for (int i = 0; i < data_size; i++) {
                p->title += data[i];
            }
            break;
        case IN_PAGE_REVISION_TEXT:
            for (int i = 0; i < data_size; i++) {
                p->text += data[i];
            }
            break;
        default:
            break;
    }
}

void Fileloader::handle_document(const string &title, const string &text, Fileloader &fl, Database &db){
    //先把文档存入数据库
    db.insert_document(title, text);
    int document_id = db.get_document_id(title);
    
    cout<<"Doc id:"<<document_id<<" title: "<<title<<endl;
    
    //转成utf32以方便分词
    u32string u32text = Util::utf8toutf32(text);
    
    //对该文档建立倒排索引并加到缓冲区里的倒排索引里
    Posting::text_to_inverted_index(document_id, u32text, fl.inverted_index_buffer,db);
    fl.buffer_doc_count++;
    
    //达到文档更新阈值的时候与数据库上的倒排索引进行合并
    if (!fl.inverted_index_buffer.empty() && (fl.buffer_doc_count > fl.buffer_update_threshold)) {
        Util::print_time_diff();
        for (auto iter = fl.inverted_index_buffer.begin(); iter != fl.inverted_index_buffer.end(); iter++) {
            Posting::update_postings_vec(iter->first, iter->second,db);
        }
        fl.inverted_index_buffer.clear();
        fl.buffer_doc_count = 0;
        Util::print_time_diff();
    }
}