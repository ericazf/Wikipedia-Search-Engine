//
//  Posting.cpp
//  Wikipedia Search Engine
//
//  Created by 甘流卓 on 16/2/28.
//  Copyright © 2016年 甘流卓. All rights reserved.
//

#include "Posting.hpp"

/*
 * @param document_id 文档id
 * @param u32text utf32编码的文档内容
 * @param buffer_inverted_index 缓冲区倒排索引
 */
void Posting::text_to_inverted_index(const int &document_id,const u32string &u32text,unordered_map<int,vector<Posting>> &buffer_inverted_index,Database &db){
    
    auto iter = u32text.begin();
    int position = 1;
    
    while (iter < u32text.end()) {
        while (iter < u32text.end() && is_ignored_char(*iter)) {
            iter++;
        }
        int n = 0;
        auto p = iter;
        u32string temp_u32str = U"";
        while (true) {
            if (!is_ignored_char(*p) && p < u32text.end()) {
                temp_u32str += *p;
                n++;
            }else{
                //当遇到该忽略的字符或者到达字符串末尾的时候跳出该循环
                break;
            }
            if (n >= N) {
                break;
            }
            p++;
        }
        if (!temp_u32str.empty() && n >= N) {
            string u8token = Util::utf32toutf8(temp_u32str);
            token_to_postings_vec(document_id, u8token, position, buffer_inverted_index,db);
            position++;
        }
        iter++;
    }
}

/*
 * @param document_id 文档id
 * @param token 词语
 * @param positon 词语在文档中的位置
 * @param buffer_inverted_index 缓冲区倒排索引
 */
void Posting::token_to_postings_vec(const int &document_id,const string &token,const int &position,unordered_map<int,vector<Posting>> &buffer_inverted_index,Database &db){
    
    int token_id = db.get_token_id(token, true);
    if (token_id != 0) {
        Posting temp;
        auto search_iter = buffer_inverted_index.find(token_id);
        if (search_iter == buffer_inverted_index.end()) {
            temp.document_id = document_id;
            temp.position_vec.push_back(position);
            vector<Posting> postings_vec;
            postings_vec.push_back(temp);
            buffer_inverted_index.insert(unordered_map<int,vector<Posting>>::value_type(token_id,postings_vec));
        }else{
            //如果缓冲区倒排索引里已经存在该词语，取列表最后一项判断是否已经有该文档的记录
            vector<Posting> &postings_vec = search_iter->second;
            Posting &posting_temp = postings_vec.back();
            if (posting_temp.document_id == document_id) {
                posting_temp.position_vec.push_back(position);
            }else{
                Posting temp;
                temp.document_id = document_id;
                temp.position_vec.push_back(position);
                postings_vec.push_back(temp);
            }
        }
    }
}

/*
 * @ret 返回指向一个倒排列表字节序列缓冲区的指针
 */
postings_buffer_bin* Posting::create_buffer(){
    postings_buffer_bin *pbb = new postings_buffer_bin;
    pbb->head = new char[POSTINGS_BUFFER_INIT_SIZE];
    pbb->cur = pbb->head;
    pbb->tail = pbb->head + POSTINGS_BUFFER_INIT_SIZE;
    pbb->bit = 0;
    
    return pbb;
}

/*
 * @param pbb 指向需要重新分配的缓冲区的指针
 */
void Posting::enlarge_buffer(postings_buffer_bin *pbb){
    //每次重新分配的大小都是之前的2倍
    size_t new_size = (pbb->tail - pbb->head) * 2;
    char *new_head = new char[new_size];
    char *new_cur = new_head;
    
    //把原来的内容复制到新的空间里
    for (char *i = pbb->head; i < pbb->cur; ++i,++new_cur) {
        *new_cur = *i;
    }
    
    //删除原来new分配的空间
    pbb->cur = new_cur;
    pbb->tail = new_head + (new_size);
    delete [](pbb->head);
    pbb->head = new_head;
    
//    cout<<"enlarge buffer"<<endl;
}

/*
 * 把指定字节的数据加入到缓冲区中
 * @param pbb 指向缓冲区的指针
 * @param data 指向要加入的数据的指针
 * @param data_size 数据大小
 */
void Posting::append_buffer(postings_buffer_bin *pbb, const void *data, size_t &data_size){
    //如果当前加入的数据未满一个字节，跳过该字节
    if (pbb->bit) {
        cout<<"pbb->bit = "<<pbb->bit<<endl;
        pbb->cur++;
        pbb->bit = 0;
    }
    if (pbb->cur + data_size > pbb->tail) {
        enlarge_buffer(pbb);
    }
    if (data && data_size > 0) {
        memcpy(pbb->cur, data, data_size);
        pbb->cur += data_size;
    }
}

/*
 * 添加单个位到缓冲区中
 * @param pbb 执行缓冲区的指针
 * @param bit 插入的位的值
 */
void Posting::append_buffer_bit(postings_buffer_bin *pbb, int bit){
//    cout<<bit;
    if(pbb->cur >= pbb->tail){
        enlarge_buffer(pbb);
    }
    
    //对新的一个char字节进行初始化
    if (pbb->bit == 0) {
        *pbb->cur = 0;
    }
    if (bit == 1) {
        *pbb->cur |= 1 << (7 - pbb->bit);
    }
    pbb->bit++;
    if (pbb->bit == 8) {
        pbb->bit = 0;
        pbb->cur++;
    }
}

/*
 * 编码格式为document_id,position_count,x,x,x,...
 * @param postings_vec 待编码倒排列表
 * @param postings_bin 用于存放编码后的倒排列表
 */
void Posting::encode_postings(vector<Posting> &postings_vec, postings_buffer_bin *postings_bin){
    
    size_t size_of_int = sizeof(int);
    for (auto iter = postings_vec.begin(); iter != postings_vec.end(); ++iter) {
        append_buffer(postings_bin, static_cast<void*>(&iter->document_id), size_of_int);
        int vec_length = static_cast<int>((iter->position_vec).size());
        append_buffer(postings_bin, static_cast<void*>(&vec_length), size_of_int);
        for (auto vec_iter = (iter->position_vec).begin(); vec_iter != (iter->position_vec).end(); ++vec_iter) {
            int position = *vec_iter;
            append_buffer(postings_bin, static_cast<void*>(&position), size_of_int);
        }
    }
}

void Posting::encode_int_golomb(int num,int m,int b,int t,postings_buffer_bin *postings_bin){
    
    int r = num % m;
    int q = (num - r) / m;
    for (int i = 0; i < q; ++i) {
        append_buffer_bit(postings_bin, 1);
    }
    append_buffer_bit(postings_bin, 0);
    if (r < t) {
        for (int i = 1 << (b - 2); i > 0; i >>= 1) {
            if (i & r) {
                append_buffer_bit(postings_bin, 1);
            }else{
                append_buffer_bit(postings_bin, 0);
            }
        }
    }else{
        for (int i = 1 << (b - 1); i > 0; i >>= 1) {
            if (i & (r + t)) {
                append_buffer_bit(postings_bin, 1);
            }else{
                append_buffer_bit(postings_bin, 0);
            }
        }
    }
}

/*
 * 使用golomb进行编码
 * @param postings_vec 等待编码的倒排列表
 * @param postings_bin 指向缓冲区的指针
 */
void Posting::encode_postings_golomb(vector<Posting> &postings_vec, postings_buffer_bin *postings_bin){
    
    if (postings_vec.size() <= 0) {
        return;
    }
    int sum = (postings_vec.begin())->document_id - 1;
    int preDocId = (postings_vec.begin())->document_id;
    (postings_vec.begin())->document_id--;
    for (auto it = postings_vec.begin() + 1; it != postings_vec.end(); ++it) {
        sum += it->document_id - preDocId - 1;
        int temp = it->document_id;
        it->document_id = it->document_id - preDocId - 1;
        preDocId = temp;
    }
    int m = sum / postings_vec.size();
    
    //m可能会比总文档数小，此时改为unary编码
    if (m <= 0) {
        m = 1;
    }
    int b = ceil(log2(m));
    int t = pow(2.0, b) - m;
    
    size_t size_of_int = sizeof(int);
    int doc_length = static_cast<int>(postings_vec.size());
    
    //记录文档数和m，两个数均不压缩存储
    append_buffer(postings_bin, &doc_length, size_of_int);
    append_buffer(postings_bin, &m, size_of_int);
    //把词条的文档号先添加到缓冲区中
    for (auto it = postings_vec.begin(); it != postings_vec.end(); ++it) {
        encode_int_golomb(it->document_id, m, b, t, postings_bin);
        cout<<endl;
    }
    
    //处理每个文档里词条位置的信息
    for (int i = 0; i < postings_vec.size(); ++i) {
        if (postings_vec[i].position_vec.size() <= 0) {
            cout<<"Postings vec size is zero"<<endl;
            exit(-1);
        }

        sum = *postings_vec[i].position_vec.begin() - 1;
        int prePosition = *postings_vec[i].position_vec.begin();
        (*postings_vec[i].position_vec.begin())--;
        for (auto it = postings_vec[i].position_vec.begin() + 1; it != postings_vec[i].position_vec.end(); ++it) {
            sum += *it - prePosition - 1;
            int temp = *it;
            *it = *it - prePosition - 1;
            prePosition = temp;
        }
        m = sum / postings_vec[i].position_vec.size();
        if (m <= 0) {
            m = 1;
        }
        b = ceil(log2(m));
        t = pow(2.0, b) - m;
        
        int position_vec_size = static_cast<int>(postings_vec[i].position_vec.size());
        append_buffer(postings_bin, &position_vec_size, size_of_int);
        append_buffer(postings_bin, &m, size_of_int);
        for (auto it = postings_vec[i].position_vec.begin(); it != postings_vec[i].position_vec.end(); ++it) {
            encode_int_golomb(*it, m, b, t, postings_bin);
        }
    }
}

/*
 * 使用golomb进行解码
 * @param postings_vec 用于存储解码后的倒排列表
 * @param postings_bin 倒排列表字节序列
 */
void Posting::decode_postings_golomb(vector<Posting> &postings_vec, char *postings_bin,const int &postings_bin_size){
    
    char *pstart = postings_bin,*pend = postings_bin + postings_bin_size;
    
    postings_vec.clear();
    int docs_count = *((int*)pstart);
    pstart += sizeof(int);
    int m = *((int*)pstart);
    pstart += sizeof(int);
    int b = ceil(log2(m));
    int t = pow(2.0, b) - m;
    
    int bit = 0;
    int preDocId = 0;
    //提取doc
    for (int i = 0; i < docs_count; ++i) {
        Posting posting;
        decode_int_golomb(m,b,t,pstart,pend,bit,posting.document_id);
        posting.document_id += preDocId + 1;
        preDocId = posting.document_id;
        postings_vec.push_back(posting);
    }
    
    cout<<"-------------Deocde document id success------------"<<endl;
    
    if (bit != 0) {
        pstart++;
        bit = 0;
    }
    
    cout<<"-------------start decode position-----------------"<<endl;
    //提取在各个文档里词条的位置信息
    for (int i = 0; i < docs_count; ++i) {
        int position_count = *((int*)pstart);
        pstart += sizeof(int);
        m = *((int*)pstart);
        pstart += sizeof(int);
        b = ceil(log2(m));
        t = pow(2.0, b) - m;
        
        int position = 0;
        int prePosition = 0;
        for (int j = 0; j < position_count; ++j) {
            decode_int_golomb(m, b, t, pstart,pend, bit,position);
            position += prePosition + 1;
            prePosition = position;
            postings_vec[i].position_vec.push_back(position);
        }
        if (bit != 0) {
            cout<<"Bit : "<<bit<<endl;
            pstart++;
            bit = 0;
        }
    }
}

int Posting::read_bit(char *pstart,char *pend,int &bit){
    if (pstart > pend) {
        return -1;
    }
    int ibit = 1 << (7 - bit);
    int ret = ((*pstart) & ibit) ? 1 : 0;
    bit++;
    if (bit >= 8) {
        bit = 0;
        pstart++;
    }
    return ret;
}

void Posting::decode_int_golomb(int m, int b, int t, char *pstart,char *pend,int &bit,int &num){
    num = 0;
    
    //unary
    while (read_bit(pstart,pend,bit) == 1) {
        num += m;
    }
    
    //m=1时为golomb编码的特殊形式,unary编码
    if (m > 1) {
        int r = 0;
        //读取后面b-1位的数字，获得编码时的余数
        for (int i = 0; i < b - 1; ++i) {
            int temp = read_bit(pstart,pend,bit);
            r = (r << 1) | temp;
        }
        
        //r >= t时还需要读多一位，因为此时用了b位来保存r+t
        if (r >= t) {
            int temp = read_bit(pstart,pend,bit);
            r = (r << 1) | temp;
            r -= t;
        }
        num += r;
    }
    cout<<"Num : "<<num<<endl;
}

/*
 * @param token_id 词语id
 * @param postings_vec 词语的倒排项列表
 * @param db 数据库句柄
 */
void Posting::update_postings_vec(const int &token_id,vector<Posting> &postings_vec,Database &db){
    
    char *postings_bin = nullptr;
    int docs_count = 0,postings_size = 0;
    db.get_postings(token_id, (void**)&postings_bin, docs_count, postings_size);
    postings_buffer_bin *pbb = create_buffer();
    docs_count += postings_vec.size();
    
    if (postings_bin != nullptr) {
        vector<Posting> database_postings_vec;
        decode_postings(postings_bin, postings_size, database_postings_vec);
        merge_postings_vec(database_postings_vec, postings_vec);
        encode_postings(database_postings_vec, pbb);
//        encode_postings_golomb(database_postings_vec, pbb);
    }else{
        encode_postings(postings_vec, pbb);
//        encode_postings_golomb(postings_vec, pbb);
    }
    
    db.update_postings(token_id, static_cast<void*>(pbb->head), static_cast<int>((pbb->cur - pbb->head)), static_cast<int>(docs_count));
    free_buffer(pbb);
}

/*
 * @param postings_bin 倒排列表字节序列
 * @param postings_bin_size 倒排列表字节序列长度
 * @param postings_vec 用于存放解码后的倒排列表
 */
void Posting::decode_postings(const char *postings_bin, const int &postings_bin_size,vector<Posting> &postings_vec){
    postings_vec.clear();
    const int *p = (const int *)(postings_bin),*pend = (const int *)(postings_bin + postings_bin_size);
    while (p < pend) {
        int document_id = *(p++);
        int positions_count = *(p++);
        Posting pos;
        pos.document_id = document_id;
        for (int i = 0; i < positions_count; i++) {
            pos.position_vec.push_back(*p);
            p++;
        }
        postings_vec.push_back(pos);
    }
}

/*
 * @param pbb 指向字节序列缓冲区的指针
 */
void Posting::free_buffer(postings_buffer_bin *pbb){
    delete [](pbb->head);
    delete pbb;
}

/*
 * @param postings_vec1 将要合并的倒排项列表，用于返回
 * @param postings_vec2 将要合并的倒排项列表，不可修改，用于历遍
 */
void Posting::merge_postings_vec(vector<Posting> &postings_vec1, const vector<Posting> &postings_vec2){
    for (auto iter = postings_vec2.cbegin(); iter != postings_vec2.cend(); iter++) {
        postings_vec1.push_back(*iter);
    }
}

/*
 * @param u32char utf32编码的字符
 */
bool Posting::is_ignored_char(const UTF32Char &u32char){
    switch (u32char) {
        case ' ':
        case '\f':
        case '\n':
        case '\r':
        case '\t':
        case '\v':
        case '!':
        case '"':
        case '#':
        case '$':
        case '%':
        case '&':
        case '\'':
        case '(':
        case ')':
        case '*':
        case '+':
        case ',':
        case '-':
        case '.':
        case '/':
        case ':':
        case ';':
        case '<':
        case '=':
        case '>':
        case '?':
        case '@':
        case '[':
        case '\\':
        case ']':
        case '^':
        case '_':
        case '`':
        case '{':
        case '|':
        case '}':
        case '~':
        case 0x2018:
        case 0x2019:
        case 0x201C: /* 全角前双印 */
        case 0x201D: /* 全角后双印 */
        case 0x3000: /* 全角空格 */
        case 0x3001: /* 、 */
        case 0x3002: /* 。 */
        case 0xFF08: /* （ */
        case 0xFF09: /* ） */
        case 0xFF01: /* ！ */
        case 0xFF0C: /* ， */
        case 0xFF1A: /* ： */
        case 0xFF1B: /* ； */
        case 0xFF1F: /* ? */
            return true;
        default:
            return false;
    }
}