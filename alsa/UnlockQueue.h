/**
  ******************************************************************************
  * @file           : UnlockQueue.h
  * @brief          : https://blog.csdn.net/chen19870707/article/details/39994303
  *                 https://github.com/liigo/kfifo
  * @attention      : None
  * @version        : 1.0
  * @revision       : none
  * @date           : 2023/11/12
  * @author         : yangkun
  * @email          : xyyangkun@163.com
  * @company        : osee-dig.com.cn
  ******************************************************************************
  */

#ifndef RV11XX_PRJ_UNLOCKQUEUE_H
#define RV11XX_PRJ_UNLOCKQUEUE_H


class UnlockQueue {

public:
   UnlockQueue(int nSize);
   virtual ~UnlockQueue();

    bool Initialize();
    // full  len > mask; mask = size - 1;
    bool IsFull(){return m_nIn - m_nOut > m_nSize-1;};
    bool IsEmpty(){return m_nIn == m_nOut;};
    // 已经使用的空间 >= 总空间 - 要输入的空间 时才可以
    bool IsEnough(unsigned int input_size){return m_nIn - m_nOut <= m_nSize - input_size;};

    unsigned int Put(const void *pBuffer, unsigned int nLen);
    unsigned int Get(void *pBuffer, unsigned int nLen);

    inline void Clean() { m_nIn = m_nOut = 0; }
    inline unsigned int GetDataLen() const { return  m_nIn - m_nOut; }
    inline unsigned int GetQueueSize() const { return  m_nSize; }
private:
    inline bool is_power_of_2(unsigned long n) { return (n != 0 && ((n & (n - 1)) == 0)); };
    inline unsigned long roundup_power_of_two(unsigned long val);

private:
    unsigned char *m_pBuffer;    /* the buffer holding the data */
    unsigned int   m_nSize;      /* the size of the allocated buffer */
    unsigned int   m_nIn;        /* data is added at offset (in % size) */
    unsigned int   m_nOut;       /* data is extracted from off. (out % size) */
};


#endif //RV11XX_PRJ_UNLOCKQUEUE_H
