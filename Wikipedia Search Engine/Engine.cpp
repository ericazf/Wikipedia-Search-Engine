//
//  Engine.cpp
//  Wikipedia Search Engine
//
//  Created by 甘流卓 on 16/2/28.
//  Copyright © 2016年 甘流卓. All rights reserved.
//

#include "Engine.hpp"

/*
 * @param xml_path Wikipedia XML文件路径
 * @param doc_encoding XML文档编码，如utf-8
 * @param max_doc_count 最大文档读取数量
 * @param db_path 数据库路径
 */
Engine::Engine(string &xml_path,string &doc_encoding,int &max_doc_count,string &db_path){
    this->max_doc_count = max_doc_count;
    this->xml_path = xml_path;
    this->doc_encoding = doc_encoding;
    this->db_path = db_path;
}

Engine::Engine(string &db_path){
    this->db_path = db_path;
}

void Engine::create_inverted_index(){
    Util::print_time_diff();
    Database db(db_path);
    Fileloader floader(xml_path,doc_encoding,max_doc_count,db);
    db.begin();
    floader.load_file();
    db.commit();
    db.finish();
    Util::print_time_diff();
}

/*
 * @param query 搜索关键字
 */
void Engine::search(string &query){
    cout<<"Key : "<<query<<endl;
    Util::print_time_diff();
    Database db(db_path);
    database_doc_count = db.get_documents_count();
    if (database_doc_count < 0) {
        database_doc_count = 0;
    }
    
    //分词
    u32string u32query = Util::utf8toutf32(query);
    auto iter = u32query.begin();
    vector<token_info> token_info_vec;
    int position = 0;
    while (iter < u32query.end()) {
        while (iter < u32query.end() && Posting::is_ignored_char(*iter)) {
            iter++;
        }
        int n = 0;
        auto p = iter;
        u32string temp_u32str = U"";
        while (true) {
            if (!Posting::is_ignored_char(*p) && p < u32query.end()) {
                temp_u32str += *p;
                n++;
            }else{
                break;
            }
            if (n >= N) {
                break;
            }
            p++;
        }
        if (!temp_u32str.empty() && n >= N) {
            string u8token = Util::utf32toutf8(temp_u32str);
            token_info ti = {
                u8token,
                0,
                position,
                0
            };
            token_info_vec.push_back(ti);
            position++;
        }
        iter++;
    }
    search_docs(token_info_vec,db);
    if (ret_result.empty()) {
        cout<<"No document found"<<endl;
        exit(0);
    }else{
        sort(ret_result.begin(), ret_result.end(), [](const doc_result &lhs,const doc_result &rhs){
            return lhs.rank > rhs.rank;
        });
        display_result();
    }
    Util::print_time_diff();
}

void Engine::display_result(){
    cout<<"Result:"<<endl;
    for (auto it = ret_result.cbegin(); it != ret_result.cend(); ++it) {
        cout<<"DocId: "<<it->document_id<<"  title: "<<it->document_title<<"  rank: "<<it->rank<<endl;
    }
    cout<<ret_result.size()<<" document(s) found"<<endl;
}

/*
 * @param token_info_vec 分词存储结果
 * @param db 数据库句柄
 */
void Engine::search_docs(vector<token_info> &token_info_vec,Database &db){
    for (auto it = token_info_vec.begin(); it != token_info_vec.end(); it++) {
        int token_id = db.get_token_id(it->token, false);
        if (token_id == 0) {
            //建立倒排索引的时候不存在该词直接提示并退出
            cout<<it->token<<" not found on the database"<<endl;
            exit(0);
        }else{
            char *postings_bin = nullptr;
            int postings_size = 0;
            db.get_postings(token_id, (void**)(&postings_bin), it->docs_count, postings_size);
            if (postings_bin == nullptr) {
                //该词语没有成功建立倒排索引
                cout<<it->token<<" has no postings vec"<<endl;
                exit(0);
            }else{
                Posting::decode_postings(postings_bin, postings_size, it->postings_vec);
//                cout<<"Postings size : "<<postings_size<<endl;
//                Posting::decode_postings_golomb(it->postings_vec, postings_bin, postings_size);
            }
        }
    }
    //把词素按出现文档数从小到大排序，缩小后面操作的范围
    sort(token_info_vec.begin(), token_info_vec.end(), [](token_info &lhs,token_info &rhs){
        return lhs.docs_count < rhs.docs_count;
    });
    
    auto first_token_it = token_info_vec.begin();
    auto tokens_it = first_token_it;
    ++tokens_it;
    auto &first_token_postings = first_token_it->postings_vec;
    bool isOver = false;
    bool gotoBegin = false;
    
    //遍历词语A的倒排项
    while (first_token_it->cur_position < first_token_postings.size()) {
        int docId = (first_token_it->postings_vec[first_token_it->cur_position]).document_id;
        if (token_info_vec.size() >= 2) {
            
            //历遍其他词
            for (auto other_words_it = tokens_it; other_words_it != token_info_vec.end(); ++other_words_it) {
                
                //历遍词的倒排项
                for (int j = other_words_it->cur_position; j < other_words_it->postings_vec.size(); ++j) {
                    other_words_it->cur_position = j;
                    
                    if (docId == other_words_it->postings_vec[other_words_it->cur_position].document_id) {
                        break;
                    }else if(other_words_it->postings_vec[other_words_it->cur_position].document_id > docId){
                        
                        while (first_token_it->cur_position < first_token_postings.size() && (first_token_it->postings_vec[first_token_it->cur_position]).document_id < other_words_it->postings_vec[other_words_it->cur_position].document_id) {
                            ++first_token_it->cur_position;
                        }
                        
                        //重置other_words_it
                        if (first_token_it->cur_position < first_token_postings.size()) {
                            gotoBegin = true;
                        }
                        break;
                    }
                }
                
                if (gotoBegin) {
                    gotoBegin = false;
                    break;
                }
                
                if (other_words_it->cur_position >= other_words_it->postings_vec.size() || first_token_it->cur_position >= first_token_postings.size()) {
                    isOver = true;
                }
                if (isOver) {
                    break;
                }
                if ((other_words_it == token_info_vec.end() - 1) && first_token_postings[first_token_it->cur_position].document_id == other_words_it->postings_vec[other_words_it->cur_position].document_id) {
                    bool is_parse = search_parse(token_info_vec);
                    if (is_parse) {
                        double rank = calc_rank(token_info_vec);
                        add_search_result(token_info_vec, rank, db);
                    }
                    ++first_token_it->cur_position;
                }
            }
        }else{
            bool is_parse = search_parse(token_info_vec);
            if (is_parse) {
                double rank = calc_rank(token_info_vec);
                add_search_result(token_info_vec,rank,db);
            }
            ++first_token_it->cur_position;
        }
    }
}

bool Engine::search_parse(const vector<token_info> &token_info_vec){
    
    if (token_info_vec.size() == 1) {
        return true;
    }
    
    auto &first_token_position_vec = token_info_vec[0].postings_vec[token_info_vec[0].cur_position].position_vec;
    int is_over = false;
    int *position = new int[token_info_vec.size()]();   //记录当前词元位置下标
    bool gotoBegin = false;
    
    //遍历词元A的位置信息
    while (position[0] < first_token_position_vec.size()) {
        int first_word_position = first_token_position_vec[position[0]] - token_info_vec[0].position;
        
        //历遍其他词
        for (int i = 1; i < token_info_vec.size(); ++i) {
            auto &other_word_position_vec = token_info_vec[i].postings_vec[token_info_vec[i].cur_position].position_vec;
            int token_relate_position = token_info_vec[i].position;
            
            //历遍词元在某个文档里的位置信息
            for (int j = position[i]; j < other_word_position_vec.size(); ++j) {
                position[i] = j;
                if (other_word_position_vec[j] - token_relate_position == first_word_position) {
                    break;
                }else if(other_word_position_vec[j] - token_relate_position > first_word_position){
                    while (position[0] < first_token_position_vec.size() && first_word_position < other_word_position_vec[j] - token_relate_position) {
                        ++position[0];
                    }
                    if (position[0] < first_token_position_vec.size()) {
                        gotoBegin = true;
                    }
                    break;
                }
            }
            
            if (gotoBegin) {
                gotoBegin = false;
                break;
            }
            
            if (position[i] >= other_word_position_vec.size() || position[0] >= first_token_position_vec.size()) {
                is_over = true;
            }
            if (is_over) {
                break;
            }
            if (i == token_info_vec.size() - 1 && other_word_position_vec[position[i]] - token_relate_position == first_word_position) {
                delete []position;
                return true;
            }else if(i == token_info_vec.size() - 1 && other_word_position_vec[position[i]] - token_relate_position != first_word_position){
                ++position[0];
            }
        }
    }
    delete []position;
    return false;
}

double Engine::calc_rank(const vector<token_info> &token_info_vec){
    double score = 0.0;
    for (auto it = token_info_vec.begin(); it != token_info_vec.end(); it++) {
        double idf = log2(static_cast<double>(database_doc_count) / it->docs_count);
        double tf = (it->postings_vec[it->cur_position]).position_vec.size();
        score += tf * idf;
    }
    return score;
}

void Engine::add_search_result(const vector<token_info> &token_info_vec, const double &rank,Database &db){
    int document_id = (token_info_vec[0].postings_vec)[token_info_vec[0].cur_position].document_id;
    string title;
    db.get_document_title(document_id, title);
    doc_result result = {
        document_id,
        title,
        rank
    };
    ret_result.push_back(result);
}

