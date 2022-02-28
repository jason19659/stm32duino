#pragma once

#include <Arduino.h>
class Cards {

public:
    int64_t array[100];
    int size = 0;

    int length() {
        return size;
    }

    void insert(const int64_t& data) {
        array[size] = data;
        size++;
    }

    void clear() {
        size=0;
    }

    int64_t get(int index){
        return array[index];
    }

    int64_t& operator[](int index){
        return array[index];
    }

    //insert()
};
/*
UidVec a;
a.insert(0xffffff);
for(int i=0;i<a.size();i++){
    Serial.println(a[i]);
}
a.clear();


 */
struct SearchData
{
    SearchData(int64_t mask_value, uint8_t position, uint8_t current_bit_value) : mask(mask_value), position(position), current_bit_value(current_bit_value) {}
    SearchData() : mask(0), position(0), current_bit_value(0) {}
    int64_t mask = 0;
    uint8_t position = 0;
    uint8_t current_bit_value = 0;
};

class MyStack
{
public:
    MyStack(int Size)
    {
        m_iSize = Size;
        m_pBuffer = new SearchData[Size];
        m_iTop = 0;
    }
    ~MyStack()
    {
        delete[] m_pBuffer;
        m_pBuffer = NULL;
    }
    bool empty()
    {
        if (0 == m_iTop)
        {
            return true;
        }
        return false;
    }
    bool isFull()
    {
        if (m_iTop == m_iSize)
        {
            return true;
        }
        return false;
    }
    void clear()
    {
        m_iTop = 0;
    }

    int size()
    {
        return m_iTop;
    }
    bool push(const SearchData& elem)
    {
        if (isFull())
        {
            return false;
        }
        if(m_iTop<0)return false;
        m_pBuffer[m_iTop].current_bit_value = elem.current_bit_value;
        m_pBuffer[m_iTop].mask = elem.mask;
        m_pBuffer[m_iTop].position = elem.position;
        m_iTop++;
        return true;
    }
    bool pop(SearchData &elem)
    {
        if (empty())
        {
            return false;
        }
        m_iTop--;
        elem.current_bit_value = m_pBuffer[m_iTop].current_bit_value;
        elem.mask = m_pBuffer[m_iTop].mask;
        elem.position = m_pBuffer[m_iTop].position;
        return true;
    }

private:
    SearchData *m_pBuffer; //栈空间指针
    int m_iSize;           //栈容量
    int m_iTop;            //栈顶，栈中元素个数
};

class UidVec
{

public:
    UidVec()
    {
        data_ = new int64_t[100];
        memset(data_, 0, sizeof(int64_t)*100);
        cnt_ = 0;
    }

    ~UidVec(){
        delete[] data_;
    }

    UidVec(const UidVec&) = delete;

    UidVec& operator=(const UidVec&) = delete;

    int8_t size() const
    {
        return cnt_;
    }

    bool contains(const int64_t &uid) const
    {
        for (int i = 0; i < cnt_; i++)
        {
            if (_cmp_(data_[i], uid) == 0)
            {
                return true;
            }
        }
        return false;
    }


    bool insert(const int64_t& uid)
    {
        if(cnt_>=99){
            return false;
        }
        if (contains(uid))
        {
            return false;
        }
        data_[cnt_++] = uid;
        return true;
    }

    void clear()
    {
        memset(data_, 0, sizeof(int64_t)*100);
        cnt_ = 0;
    }

    int64_t& operator[](const int& index)
    {
        return data_[index];
    }

    const int64_t& operator[](const int& index) const
    {
        return data_[index];
    }

    bool operator=(const UidVec& oth) const{
        if(cnt_!=oth.size()){
            return false;
        }
        for(int i=0;i<cnt_;i++){
            if(data_[i]!=oth[i]){
                return false;
            }
        }
        return true;
    }

private:
    int64_t _cmp_(const int64_t &a, const int64_t &b) const
    {
        return a - b;
    }

    //int64_t data_[100];
    int64_t* data_;
    int8_t cnt_;
};

class UidSet
{
    struct Node
    {
        Node() : next(nullptr)
        {
            memset(data_, 0, 8);
        }
        uint8_t data_[8];
        Node *next;
    };

public:
    UidSet()
    {
        root_ = new Node();

        cnt_ = 0;
    }

    int size()
    {
        return cnt_;
    }

    int contains(uint8_t *uid)
    {
        Node *p = root_->next;
        while (p)
        {
            if (uid_cmp(uid, p->data_) == 0)
            {
                return 1;
            }
            p = p->next;
        }
        return 0;
    }

    int insert(uint8_t *uid)
    {

        Node *p = root_;
        while (p->next)
        {
            if (uid_cmp(p->next->data_, uid) == 0)
            {
                return 0;
            }
            p = p->next;
        }
        p->next = new Node();
        uid_cpy(p->next->data_, uid);
        cnt_++;
        return 1;
    }

    int erase(uint8_t *uid)
    {

        Node *p = root_;
        while (p->next && (uid_cmp(p->next->data_, uid)) != 0)
        {
            p = p->next;
        }
        if (p->next)
        {
            Node *tmp = p->next;
            p->next = tmp->next;
            delete tmp;
            cnt_--;
            return 1;
        }
        else
        {
            return 0;
        }
    }

    void clear()
    {
        Node *p = root_->next;
        Node *tmp;
        while (p)
        {
            tmp = p;
            p = p->next;
            delete tmp;
        }
        root_->next = nullptr;
        cnt_ = 0;
    }

private:
    int uid_cmp(uint8_t *a, uint8_t *b)
    {
        for (int i = 0; i < 8; i++)
        {
            if (a[i] != b[i])
                return a[i] - b[i];
        }
        return 0;
    }

    uint8_t *uid_cpy(uint8_t *dst, uint8_t *src)
    {
        for (int i = 0; i < 8; i++)
        {
            dst[i] = src[i];
        }
        return dst;
    }

    int cnt_;
    Node *root_;
};

//sample
/**
 * 
 * int main(int argc, char* argv[])
{	
	uint8_t a[8];
	for (int i = 0; i < 8; i++) {
		a[i] = 0x30 + i;
	}
	uint8_t b[8];
	for (int i = 0; i < 8; i++) {
		b[i] = 0x31 + i;
	}
	UidSet set;
	set.addUid(a);
	set.addUid(b);
	set.delUid(a);
	set.delUid(a);
}
 */