//
//  Posting.hpp
//  Wikipedia Search Engine
//
//  Created by 甘流卓 on 16/2/28.
//  Copyright © 2016年 甘流卓. All rights reserved.
//

#ifndef Posting_hpp
#define Posting_hpp

#include <stdio.h>
#include <vector>
#include <string>
#include <unordered_map>
#include "Util.hpp"
#include "Database.hpp"
using namespace std;

typedef uint32_t UTF32Char;
const int N = 2;    //决定N-gram的分词位数

//倒排项字节序列
typedef struct {
    char *head;     //头指针
    char *cur;      //当前指向
    char *tail;     //尾指针
    int bit;        //记录当前比特位置(0-7)
} postings_buffer_bin;

/*
 *  倒排项
 */
class Posting{
    
public:
    int document_id;
    vector<int> position_vec;
    static const int POSTINGS_BUFFER_INIT_SIZE = 4096;
    
    static void text_to_inverted_index(const int &document_id,const u32string &u32text,unordered_map<int,vector<Posting>> &buffer_inverted_index,Database &db);
    
    static void token_to_postings_vec(const int &document_id,const string &token,const int &position,unordered_map<int,vector<Posting>> &buffer_inverted_index,Database &db);
    
    static void update_postings_vec(const int &token_id,vector<Posting> &postings_vec,Database &db);
    
    static bool is_ignored_char(const UTF32Char &u32char);
    
    static void merge_postings_vec(vector<Posting> &postings_vec1,const vector<Posting> &postings_vec2);
    
    //倒排项列表编码成字节序列
    static void encode_postings(vector<Posting> &postings_vec,postings_buffer_bin *postings_bin);
    
    //字节序列解码成倒排列表
    static void decode_postings(const char *postings_bin,const int &postings_bin_size,vector<Posting> &postings_vec);
    
    //使用golomb编码进行编码解码
    static void encode_postings_golomb(vector<Posting> &postings_vec,postings_buffer_bin *postings_bin);
    
    static void decode_postings_golomb(vector<Posting> &postings_vec,char *postings_bin,const int &postings_bin_size);
    
    //使用golomb对单个数字进行编码
    static void encode_int_golomb(int num,int m,int b,int t,postings_buffer_bin *postings_bin);
    
    //使用golomb对单个数字进行解码
    static void decode_int_golomb(int m,int b,int t,char *pstart,char *pend,int &bit,int &num);
    
    static int read_bit(char *pstart,char *pend,int &bit);
    
    //对倒排项字节序列缓冲区进行管理
    static postings_buffer_bin* create_buffer();
    
    static void append_buffer(postings_buffer_bin *pbb,const void *data,size_t &data_size);
    
    static void enlarge_buffer(postings_buffer_bin *pbb);
    
    static void free_buffer(postings_buffer_bin *pbb);
    
    static void append_buffer_bit(postings_buffer_bin *pbb,int bit);
    
    
    //用于调试
    static void dump_buffer(const int &token_id,vector<Posting> &postings_vec,postings_buffer_bin *pbb);
    
};

#endif /* Posting_hpp */
