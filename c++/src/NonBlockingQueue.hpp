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


template <class E>
class NonBlockingQueue
{
public:
	virtual ~NonBlockingQueue();

	virtual void setSizeParameters(std::size_t producerCount, std::size_t maxSize);

	virtual bool tryEnqueue(E&& portion) = 0;
	virtual bool tryDequeue(E& portion) = 0;
};

template <class E>
NonBlockingQueue<E>::~NonBlockingQueue()
{
}

template <class E>
void NonBlockingQueue<E>::setSizeParameters(std::size_t producerCount, std::size_t maxSize)
{
}



template <class E>
class ConcurrentPortionQueue :
    public NonBlockingQueue<E>
{
private:
    std::unique_ptr<moodycamel::ConcurrentQueue<E>> queue;

public:
    ConcurrentPortionQueue();

    void setSizeParameters(std::size_t producerCount, std::size_t maxSize);

    bool tryEnqueue(E&& portion);
    bool tryDequeue(E& portion);
};

template <class E>
ConcurrentPortionQueue<E>::ConcurrentPortionQueue()
{
}

template <class E>
void ConcurrentPortionQueue<E>::setSizeParameters(std::size_t producerCount, std::size_t maxSize)
{
	this->queue.reset(new moodycamel::ConcurrentQueue<E>(maxSize, 0, producerCount));
}

template <class E>
bool ConcurrentPortionQueue<E>::tryEnqueue(E&& portion)
{
    return this->queue->enqueue(std::move(portion));
}

template <class E>
bool ConcurrentPortionQueue<E>::tryDequeue(E& portion)
{
    return this->queue->try_dequeue(portion);
}



template <class E>
class AtomicPortionQueue :
    public NonBlockingQueue<E>
{
private:
    std::unique_ptr<atomic_queue::AtomicQueueB2<E>> queue;

public:
    AtomicPortionQueue();

    void setSizeParameters(std::size_t producerCount, std::size_t maxSize);

    bool tryEnqueue(E&& portion);
    bool tryDequeue(E& portion);
};

template <class E>
AtomicPortionQueue<E>::AtomicPortionQueue()
{
}

template <class E>
void AtomicPortionQueue<E>::setSizeParameters(std::size_t producerCount, std::size_t maxSize)
{
	this->queue.reset(new atomic_queue::AtomicQueueB2<E>(maxSize));
}

template <class E>
bool AtomicPortionQueue<E>::tryEnqueue(E&& portion)
{
    this->queue->push(std::move(portion));
    return true;
}

template <class E>
bool AtomicPortionQueue<E>::tryDequeue(E& portion)
{
    return this->queue->try_pop(portion);
}



template <class E>
class LockfreePortionQueue :
    public NonBlockingQueue<E>
{
private:
    std::unique_ptr<boost::lockfree::queue<E*>> queue;

public:
    LockfreePortionQueue();
    ~LockfreePortionQueue();

    void setSizeParameters(std::size_t producerCount, std::size_t maxSize);

    bool tryEnqueue(E&& portion);
    bool tryDequeue(E& portion);
};

template <class E>
LockfreePortionQueue<E>::LockfreePortionQueue()
{
}

template <class E>
LockfreePortionQueue<E>::~LockfreePortionQueue()
{
    this->queue->consume_all(
        [](E* portionCopy)
        {
            delete portionCopy;
        });
}

template <class E>
void LockfreePortionQueue<E>::setSizeParameters(std::size_t producerCount, std::size_t maxSize)
{
	this->queue.reset(new boost::lockfree::queue<E*>(maxSize));
}

template <class E>
bool LockfreePortionQueue<E>::tryEnqueue(E&& portion)
{
    E* portionCopy = new E(std::move(portion));
    bool success = this->queue->bounded_push(portionCopy);
    if (!success)
    {
        portion = std::move(*portionCopy);
        delete portionCopy;
    }
    return success;
}

template <class E>
bool LockfreePortionQueue<E>::tryDequeue(E& portion)
{
    E* portionCopy;
    bool success = this->queue->pop(portionCopy);
    if (success)
    {
        portion = std::move(*portionCopy);
        delete portionCopy;
    }
    return success;
}



template <class E>
class OneTBB_PortionQueue :
    public NonBlockingQueue<E>
{
private:
    oneapi::tbb::concurrent_queue<E> queue;

public:
    OneTBB_PortionQueue();

    bool tryEnqueue(E&& portion);
    bool tryDequeue(E& portion);
};

template <class E>
OneTBB_PortionQueue<E>::OneTBB_PortionQueue()
{
}

template <class E>
bool OneTBB_PortionQueue<E>::tryEnqueue(E&& portion)
{
    this->queue.push(std::move(portion));
    return true;
}

template <class E>
bool OneTBB_PortionQueue<E>::tryDequeue(E& portion)
{
    return this->queue.try_pop(portion);
}



}

#endif
