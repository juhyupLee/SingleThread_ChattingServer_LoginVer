#include "RingBuffer.h"
#include <iostream>
#include <Windows.h>
#include "Log.h"
#include "MemoryLog.h"


enum
{
    BEFRE_REAR,
    BEFORE_FRONT,
    AFTER_REAR,
    AFTER_FRONT
};

RingQ::RingQ()
    :
    m_Buffer{ 0, },
    m_Front(0),
    m_Rear(0)
{
    int a = 10;
}

RingQ::~RingQ()
{
}

int RingQ::Enqueue(char* buffer, int32_t size, Session* curSession)
{
    int32_t copyFront = m_Front;
    int32_t copyRear = m_Rear;

    int32_t copyFreeSize = GetFreeSize(copyFront,copyRear);

    //-----------------------------------
    // �������� ���� ����� ���� ������� �۴ٸ� size�� ����������ιٲ�
    //-----------------------------------
    if (copyFreeSize < size)
    {
        size = copyFreeSize;
        if (size <= 0)
        {
            return 0;
        }
    }
    int32_t diff = 0;
    int32_t directSize = GetDirectEnqueueSize(copyFront, copyRear);

    //-----------------------------------
    // Enqueue ������ m_Rear�� �ܻ� �����ϰ�, Enqeue���Ѵ�.
    //-----------------------------------
    copyRear = (copyRear + 1) % RING_BUFFER_SIZE;

    //-----------------------------------
    // ���ڷ� ���� �����  ���ӵ� ������ ���� �� ũ�ٸ�, memcpy�� �������ؾ��Ѵ�
    //-----------------------------------
    if (directSize < size)
    {
        memcpy(m_Buffer + copyRear, buffer, directSize);
        copyRear = (copyRear +directSize-1) % RING_BUFFER_SIZE;
        diff = size - directSize;

        copyRear = (copyRear + 1) % RING_BUFFER_SIZE;
        memcpy(m_Buffer + copyRear, buffer + directSize, diff);
        copyRear = (copyRear + diff-1) % RING_BUFFER_SIZE;
    }
    else
    {
        memcpy(m_Buffer + copyRear, buffer, size);
        copyRear = (copyRear +size-1) % RING_BUFFER_SIZE;
    }

    m_Rear = copyRear;

    return size;
}

int RingQ::Enqueue(char* buffer, int32_t size)
{

    int32_t copyFront = m_Front;
    int32_t copyRear = m_Rear;

    int32_t copyFreeSize = GetFreeSize(copyFront, copyRear);

    //-----------------------------------
    // �������� ���� ����� ���� ������� �۴ٸ� size�� ����������ιٲ�
    //-----------------------------------
    if (copyFreeSize < size)
    {
        size = copyFreeSize;
        if (size <= 0)
        {
            return 0;
        }
    }
    int32_t diff = 0;
    int32_t directSize = GetDirectEnqueueSize(copyFront, copyRear);

    //-----------------------------------
    // Enqueue ������ m_Rear�� �ܻ� �����ϰ�, Enqeue���Ѵ�.
    //-----------------------------------
    copyRear = (copyRear + 1) % RING_BUFFER_SIZE;

    //-----------------------------------
    // ���ڷ� ���� �����  ���ӵ� ������ ���� �� ũ�ٸ�, memcpy�� �������ؾ��Ѵ�
    //-----------------------------------
    if (directSize < size)
    {
        memcpy(m_Buffer + copyRear, buffer, directSize);
        copyRear = (copyRear + directSize - 1) % RING_BUFFER_SIZE;
        diff = size - directSize;

        copyRear = (copyRear + 1) % RING_BUFFER_SIZE;
        memcpy(m_Buffer + copyRear, buffer + directSize, diff);
        copyRear = (copyRear + diff - 1) % RING_BUFFER_SIZE;
    }
    else
    {
        memcpy(m_Buffer + copyRear, buffer, size);
        copyRear = (copyRear + size - 1) % RING_BUFFER_SIZE;
    }

    m_Rear = copyRear;

    return size;
}

int RingQ::Dequeue(char* buffer, int32_t size)
{
    int copyRear = m_Rear;
    int copyFront = m_Front;
    int copyUsedSize = GetUsedSize(copyFront, copyRear);

    //-------------------------------------------
    //�����ۿ� ������� ����� ���ڷ� ���� ������� ������ ����
    //-------------------------------------------
    if (copyUsedSize < size)
    {
        size = copyUsedSize;
        if (size <= 0)
        {
            return 0;
        }
    }

    int directSize = GetDirectDequeueSize(copyFront,copyRear);

    //-----------------------------------
    // Enqueue ������ m_Rear�� �ܻ� �����ϰ�, Enqeue���Ѵ�.
    //-----------------------------------
    copyFront = (copyFront + 1) % RING_BUFFER_SIZE;
    //-------------------------------------------
    // �����Ҵ��Ҽ��ִ� ����� ���� ������� �۴ٸ� �ι������� ��ť�� �ؾ���.
    //-------------------------------------------
    if (directSize < size)
    {
        memcpy(buffer, m_Buffer + copyFront, directSize);
        copyFront = (copyFront + directSize -1) % RING_BUFFER_SIZE;;

        int diff = size - directSize;
        copyFront = (copyFront + 1) % RING_BUFFER_SIZE;
        memcpy(buffer + directSize, m_Buffer + copyFront, diff);
        copyFront = (copyFront +diff -1 ) % RING_BUFFER_SIZE;
    }
    else
    {
        memcpy(buffer, m_Buffer + copyFront, size);
        copyFront = (copyFront +size -1 ) % RING_BUFFER_SIZE;
    }

    m_Front = copyFront;
 
    return size;
}

int32_t RingQ::GetFreeSize() const
{
    int32_t copyFront = m_Front;
    int32_t copyRear = m_Rear;

    int freeSize = 0;

    if ((copyRear + 1) % RING_BUFFER_SIZE == copyFront)
    {
        return 0;
    }
    if (copyFront > copyRear)
    {
        freeSize = copyFront - copyRear -1;
    }
    else
    {
        freeSize = copyFront + (RING_BUFFER_SIZE - 1) - copyRear;
    }
    return freeSize;
}

int32_t RingQ::GetUsedSize() const
{
    int32_t copyRear = m_Rear;
    int32_t copyFront = m_Front;

    int32_t usedSize = 0;

    if (copyRear == copyFront)
    {
        return 0;
    }
    if (copyFront < copyRear)
    {
        usedSize = copyRear - copyFront;
    }
    else
    {
        usedSize = copyRear + (RING_BUFFER_SIZE - 1) - copyFront + 1;
    }
    return usedSize;
}

int32_t RingQ::GetFreeSize(int32_t front, int32_t rear) const
{
    int freeSize = 0;
    if ((rear + 1) % RING_BUFFER_SIZE == front)
    {
        return 0;
    }
    if (front > rear)
    {
        freeSize = front - rear-1;
    }
    else
    {
        freeSize = front + (RING_BUFFER_SIZE - 1) - rear;
    }

    return freeSize;
}

int32_t RingQ::GetUsedSize(int32_t front, int32_t rear) const
{
    int32_t usedSize = 0;

    if (rear == front)
    {
        return 0;
    }
    if (front < rear)
    {
        usedSize = rear - front ;
    }
    else
    {
        usedSize = rear+ (RING_BUFFER_SIZE - 1) - front + 1;
    }

    return usedSize;
}

int32_t RingQ::GetDirectEnqueueSize() const
{
     //-----------------------------------
    // �������� ����� ��á�ٸ�, Direct Size�� ���Ҽ� ����. 0��ȯ
    //-----------------------------------
    int copyFront = m_Front;
    int copyRear = m_Rear;

    if ((copyRear + 1) % RING_BUFFER_SIZE == copyFront)
    {
        return 0;
    }

    int directSize = 0;
    int rearNext = (copyRear + 1) % RING_BUFFER_SIZE;

    //-----------------------------------
    // mRear < mFront < �ε��� ��
    // �̷���  ���ӵ� �ε����� m_Front ��������.
    //-----------------------------------

    if (rearNext < copyFront)
    {
        directSize = copyFront - rearNext;
    }
    //-----------------------------------
    // �װ��� �ƴ϶��, m_Rear ~ �ε����������� ���ӵ� �޸� �Ҵ������.
    //-----------------------------------
    else
    {
        directSize = (RING_BUFFER_SIZE - 1) - rearNext + 1;
    }

    return directSize;
}

void RingQ::GetDirectEnQData(DirectData* directSize) 
{
    int32_t  copyFront = m_Front;
    int32_t copyRear = m_Rear;


    (*directSize).bufferPtr1 = GetRearBufferPtr(copyFront, copyRear);

    int directEnQSize = GetDirectEnqueueSize(copyFront,copyRear);

    (*directSize)._Direct1 = directEnQSize;


    copyRear = (copyRear +directEnQSize) % RING_BUFFER_SIZE;

    (*directSize).bufferPtr2 = GetRearBufferPtr(copyFront, copyRear);
    (*directSize)._Direct2 = GetDirectEnqueueSize(copyFront, copyRear);
 

  
}

void RingQ::GetDirectDeQData(DirectData* directSize, Session* curSession)
{
    
    int32_t copyRear = m_Rear;
    int32_t  copyFront = m_Front;

    (*directSize).bufferPtr1 = GetFrontBufferPtr(copyFront, copyRear);

    int directDeQSize = GetDirectDequeueSize(copyFront, copyRear);

  
    (*directSize)._Direct1 = directDeQSize;


    copyFront = (copyFront + directDeQSize ) % RING_BUFFER_SIZE;

    (*directSize).bufferPtr2 = GetFrontBufferPtr(copyFront, copyRear);
    (*directSize)._Direct2 = GetDirectDequeueSize(copyFront, copyRear);

}

void RingQ::GetDirectDeQData(DirectData* directSize)
{
    int32_t copyRear = m_Rear;
    int32_t  copyFront = m_Front;

    (*directSize).bufferPtr1 = GetFrontBufferPtr(copyFront, copyRear);

    int directDeQSize = GetDirectDequeueSize(copyFront, copyRear);


    (*directSize)._Direct1 = directDeQSize;


    copyFront = (copyFront + directDeQSize) % RING_BUFFER_SIZE;

    (*directSize).bufferPtr2 = GetFrontBufferPtr(copyFront, copyRear);
    (*directSize)._Direct2 = GetDirectDequeueSize(copyFront, copyRear);
}

int32_t RingQ::GetDirectDequeueSize() const
{
    int32_t copyRear = m_Rear;
    int32_t copyFront = m_Front;
    
    //------------------------------------------
    // �����۰� ����ִٸ�, 0����ȯ
    //------------------------------------------
    if (copyFront == copyRear)
    {
        return 0;
    }

    int32_t nextFront = (copyFront + 1) % RING_BUFFER_SIZE;

    int32_t directSize = 0;
    //�Ϲ����� ���
    if (nextFront <= copyRear)
    {
        directSize = copyRear - nextFront+1;
    }
    else
    {
        directSize = (RING_BUFFER_SIZE - 1) - nextFront+1;
    }

    return directSize;
}

void RingQ::ClearBuffer()
{
    m_Front = 0;
    m_Rear = 0;
}

void RingQ::MoveRear(int size)
{
    if (size < 0)
    {
        return;
    }

    int copyRear = m_Rear;
    m_Rear = (copyRear + size) % RING_BUFFER_SIZE;
}

void RingQ::MoveFront(int size)
{
    if (size < 0)
    {
        return;
    }
    int copyFront = m_Front;
    m_Front = (copyFront + size ) % RING_BUFFER_SIZE;

}

void RingQ::MoveFrontTest(int size, Session* curSession)
{
    if (size < 0)
    {
        return;
    }
    int copyFront = m_Front;
    m_Front = (copyFront + size) % RING_BUFFER_SIZE;
}

char* RingQ::GetFrontBufferPtr(int32_t copyFront, int32_t copyRear)
{
    if (copyRear == copyFront)
    {
        return nullptr;
    }
    return m_Buffer + ((copyFront + 1) % RING_BUFFER_SIZE);
}

char* RingQ::GetFrontBufferPtr(void)
{
    //--------------------------------------
    // FrontBufferPtr�� �������ϴ°��� ����
    // dequeue���Ϸ��� ��Ȳ�ε�, ť�� ����ִٸ�
    // null����ȯ
    //--------------------------------------
    
    int32_t copyRear = m_Rear;
    int32_t copyFront = m_Front;

    if (copyRear == copyFront)
    {
        return nullptr;
    }
    return m_Buffer + ((copyFront+1)% RING_BUFFER_SIZE); 
}

char* RingQ::GetRearBufferPtr(int32_t copyFront, int32_t copyRear)
{
    if ((copyRear + 1) % RING_BUFFER_SIZE == copyFront)
    {
        return nullptr;
    }
    return m_Buffer + ((copyRear + 1) % RING_BUFFER_SIZE);
}

char* RingQ::GetRearBufferPtr(void)
{
    //--------------------------------------
    // Enqueue���Ϸ��� ��Ȳ�ε�, ť�� �����ִٸ�,
    // null����ȯ
    //--------------------------------------
    int32_t copyFront = m_Front;
    int32_t copyRear = m_Rear;
    
    if ((copyRear +1)%RING_BUFFER_SIZE == copyFront)
    {
        return nullptr;
    }

    return m_Buffer + ((copyRear + 1) % RING_BUFFER_SIZE);
}

int RingQ::Peek(char* dest, int size)
{
    //-------------------------------------------
   //�����ۿ� ������� ����� ���ڷ� ���� ������� ������ ���� ������� ������ιٲ�.
   // Peek�̱⶧���� ����Ʈ �纻 ����.
   //-------------------------------------------
    int32_t copyRear = m_Rear;
    int32_t copyFront = m_Front;
    int tempUsedSize = GetUsedSize(copyFront,copyRear);

    if (tempUsedSize < size)
    {
        size = tempUsedSize;
        if (size <= 0)
        {
            return 0;
        }
    }

    int directSize = GetDirectDequeueSize();


    //-----------------------------------
    // Peek ������ �纻 front �� �ܻ� �����ϰ�, dequeue���Ѵ�.
    //-----------------------------------
    copyFront = (copyFront + 1) % RING_BUFFER_SIZE;

    //-------------------------------------------
    // �����Ҵ��Ҽ��ִ� ����� ���� ������� �۴ٸ� �ι������� ��ť�� �ؾ���.
    //-------------------------------------------
    if (directSize < size)
    {
        memcpy(dest, m_Buffer + copyFront, directSize);
        copyFront = (copyFront + directSize - 1) % RING_BUFFER_SIZE;;

        int diff = size - directSize;
        copyFront = (copyFront + 1) % RING_BUFFER_SIZE;
        memcpy(dest + directSize, m_Buffer + copyFront, diff);
    }
    else
    {
        memcpy(dest, m_Buffer + copyFront, size);
    }

    return size;
}

int32_t RingQ::GetFront()
{
    return m_Front;
}

int32_t RingQ::GetRear()
{
    return m_Rear;
}

int32_t RingQ::GetDirectEnqueueSize(int32_t copyFront, int32_t copyRear) const
{
    //-----------------------------------
    // �������� ����� ��á�ٸ�, Direct Size�� ���Ҽ� ����. 0��ȯ
    //-----------------------------------

    if ((copyRear + 1) % RING_BUFFER_SIZE == copyFront)
    {
        return 0;
    }

    int directSize = 0;
    int rearNext = (copyRear + 1) % RING_BUFFER_SIZE;


    if (rearNext < copyFront)
    {
        directSize = copyFront - rearNext;
    }
    //-----------------------------------
    // �װ��� �ƴ϶��, m_Rear ~ �ε����������� ���ӵ� �޸� �Ҵ������.
    //-----------------------------------
    else
    {
        directSize = (RING_BUFFER_SIZE - 1) - rearNext + 1;
    }

    return directSize;
}

int32_t RingQ::GetDirectDequeueSize(int32_t copyFront, int32_t copyRear) const
{

    //------------------------------------------
    // �����۰� ����ִٸ�, 0����ȯ
    //------------------------------------------
    if (copyFront == copyRear)
    {
        return 0;
    }

    int nextFront = (copyFront + 1) % RING_BUFFER_SIZE;

    int32_t directSize = 0;
    //�Ϲ����� ���
    if (nextFront <= copyRear)
    {
        directSize = copyRear - nextFront + 1;
    }
    else
    {
        directSize = (RING_BUFFER_SIZE - 1) - nextFront + 1;
    }

    return directSize;
}
