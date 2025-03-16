#ifndef __VOIDLAND_NON_BLOCKING_QUEUE_HPP__
#define __VOIDLAND_NON_BLOCKING_QUEUE_HPP__

#include <cstddef>
#include <memory>
#include <boost/lockfree/queue.hpp>
#include <oneapi/tbb/concurrent_queue.h>
#include "concurrentqueue/concurrentqueue.h"
#include "atomic_queue/atomic_queue.h"



namespace org::voidland::concurrent::queue
{


template <typename E>
class NonBlockingQueue
{
public:
	virtual ~NonBlockingQueue();

	virtual bool tryEnqueue(E&& portion) = 0;
	virtual bool tryDequeue(E& portion) = 0;
};

template <typename E>
NonBlockingQueue<E>::~NonBlockingQueue()
{
}



template <typename E>
class ConcurrentPortionQueue :
    public NonBlockingQueue<E>
{
private:
    moodycamel::ConcurrentQueue<E> queue;

public:
    ConcurrentPortionQueue(std::size_t maxSize);

    bool tryEnqueue(E&& portion);
    bool tryDequeue(E& portion);
};

template <typename E>
ConcurrentPortionQueue<E>::ConcurrentPortionQueue(std::size_t maxSize) :
	queue()
{
}

template <typename E>
bool ConcurrentPortionQueue<E>::tryEnqueue(E&& portion)
{
    return this->queue.enqueue(std::move(portion));
}

template <typename E>
bool ConcurrentPortionQueue<E>::tryDequeue(E& portion)
{
    return this->queue.try_dequeue(portion);
}



template <typename E>
class AtomicPortionQueue :
    public NonBlockingQueue<E>
{
private:
    atomic_queue::AtomicQueueB2<E> queue;

public:
    AtomicPortionQueue(std::size_t maxSize);

    bool tryEnqueue(E&& portion);
    bool tryDequeue(E& portion);
};

template <typename E>
AtomicPortionQueue<E>::AtomicPortionQueue(std::size_t maxSize) :
	queue(maxSize)
{
}

template <typename E>
bool AtomicPortionQueue<E>::tryEnqueue(E&& portion)
{
    return this->queue.try_push(std::move(portion));
}

template <typename E>
bool AtomicPortionQueue<E>::tryDequeue(E& portion)
{
    return this->queue.try_pop(portion);
}



template <typename E>
class LockfreePortionQueue :
    public NonBlockingQueue<E>
{
private:
    boost::lockfree::queue<E*> queue;

public:
    LockfreePortionQueue(std::size_t maxSize);
    ~LockfreePortionQueue();

    bool tryEnqueue(E&& portion);
    bool tryDequeue(E& portion);
};

template <typename E>
LockfreePortionQueue<E>::LockfreePortionQueue(std::size_t maxSize) :
	queue(maxSize)
{
}

template <typename E>
LockfreePortionQueue<E>::~LockfreePortionQueue()
{
    this->queue.consume_all(
        [](E* portionCopy)
        {
            delete portionCopy;
        });
}

template <typename E>
bool LockfreePortionQueue<E>::tryEnqueue(E&& portion)
{
    E* portionCopy = new E(std::move(portion));
    bool success = this->queue.bounded_push(portionCopy);
    if (!success)
    {
        portion = std::move(*portionCopy);
        delete portionCopy;
    }
    return success;
}

template <typename E>
bool LockfreePortionQueue<E>::tryDequeue(E& portion)
{
    E* portionCopy;
    bool success = this->queue.pop(portionCopy);
    if (success)
    {
        portion = std::move(*portionCopy);
        delete portionCopy;
    }
    return success;
}



template <typename E>
class OneTBB_PortionQueue :
    public NonBlockingQueue<E>
{
private:
    oneapi::tbb::concurrent_queue<E> queue;

public:
    OneTBB_PortionQueue(std::size_t maxSize);

    bool tryEnqueue(E&& portion);
    bool tryDequeue(E& portion);
};

template <typename E>
OneTBB_PortionQueue<E>::OneTBB_PortionQueue(std::size_t maxSize) :
	queue()
{
}

template <typename E>
bool OneTBB_PortionQueue<E>::tryEnqueue(E&& portion)
{
    this->queue.push(std::move(portion));
    return true;
}

template <typename E>
bool OneTBB_PortionQueue<E>::tryDequeue(E& portion)
{
    return this->queue.try_pop(portion);
}



}

#endif
