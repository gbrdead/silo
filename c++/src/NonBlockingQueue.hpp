#ifndef __VOIDLAND_SILO_NON_BLOCKING_QUEUE_HPP__
#define __VOIDLAND_SILO_NON_BLOCKING_QUEUE_HPP__

#include <voidland/blown_queue/NonBlockingQueue.hpp>

#include <cstddef>
#include <memory>
#include <cmath>

#include <oneapi/tbb/concurrent_queue.h>
#include <xenium/reclamation/generic_epoch_based.hpp>
#include <xenium/kirsch_kfifo_queue.hpp>
#include <xenium/kirsch_bounded_kfifo_queue.hpp>
#include <xenium/nikolaev_queue.hpp>
#include <xenium/nikolaev_bounded_queue.hpp>



namespace voidland::concurrent::queue
{


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



template <typename E>
class Kirsch1FifoPortionQueue :
    public NonBlockingQueue<E>
{
private:
    xenium::kirsch_kfifo_queue<E*, xenium::policy::reclaimer<xenium::reclamation::epoch_based<>>> queue;

public:
    Kirsch1FifoPortionQueue(std::size_t maxSize);
    ~Kirsch1FifoPortionQueue();

    bool tryEnqueue(E&& portion);
    bool tryDequeue(E& portion);
};

template <typename E>
Kirsch1FifoPortionQueue<E>::Kirsch1FifoPortionQueue(std::size_t maxSize) :
	queue(1)
{
}

template <typename E>
Kirsch1FifoPortionQueue<E>::~Kirsch1FifoPortionQueue()
{
	E* portionCopy;
	while (this->queue.try_pop(portionCopy))
	{
	    delete portionCopy;
	}
}

template <typename E>
bool Kirsch1FifoPortionQueue<E>::tryEnqueue(E&& portion)
{
    E* portionCopy = new E(std::move(portion));
    this->queue.push(portionCopy);
    return true;
}

template <typename E>
bool Kirsch1FifoPortionQueue<E>::tryDequeue(E& portion)
{
    E* portionCopy;
    bool success = this->queue.try_pop(portionCopy);
    if (success)
    {
        portion = std::move(*portionCopy);
        delete portionCopy;
    }
    return success;
}



template <typename E>
class KirschBounded1FifoPortionQueue :
    public NonBlockingQueue<E>
{
private:
    xenium::kirsch_bounded_kfifo_queue<E*, xenium::policy::reclaimer<xenium::reclamation::epoch_based<>>> queue;

public:
    KirschBounded1FifoPortionQueue(std::size_t maxSize);
    ~KirschBounded1FifoPortionQueue();

    bool tryEnqueue(E&& portion);
    bool tryDequeue(E& portion);
};

template <typename E>
KirschBounded1FifoPortionQueue<E>::KirschBounded1FifoPortionQueue(std::size_t maxSize) :
	queue(1, maxSize)
{
}

template <typename E>
KirschBounded1FifoPortionQueue<E>::~KirschBounded1FifoPortionQueue()
{
	E* portionCopy;
	while (this->queue.try_pop(portionCopy))
	{
	    delete portionCopy;
	}
}

template <typename E>
bool KirschBounded1FifoPortionQueue<E>::tryEnqueue(E&& portion)
{
    E* portionCopy = new E(std::move(portion));
    bool success = this->queue.try_push(portionCopy);
    if (!success)
    {
        portion = std::move(*portionCopy);
        delete portionCopy;
    }
    return success;
}

template <typename E>
bool KirschBounded1FifoPortionQueue<E>::tryDequeue(E& portion)
{
    E* portionCopy;
    bool success = this->queue.try_pop(portionCopy);
    if (success)
    {
        portion = std::move(*portionCopy);
        delete portionCopy;
    }
    return success;
}



template <typename E>
class NikolaevPortionQueue :
    public NonBlockingQueue<E>
{
private:
    xenium::nikolaev_queue<E, xenium::policy::pop_retries<0>, xenium::policy::reclaimer<xenium::reclamation::epoch_based<>>> queue;

public:
    NikolaevPortionQueue(std::size_t maxSize);

    bool tryEnqueue(E&& portion);
    bool tryDequeue(E& portion);
};

template <typename E>
NikolaevPortionQueue<E>::NikolaevPortionQueue(std::size_t maxSize) :
	queue()
{
}

template <typename E>
bool NikolaevPortionQueue<E>::tryEnqueue(E&& portion)
{
    this->queue.push(std::move(portion));
    return true;
}

template <typename E>
bool NikolaevPortionQueue<E>::tryDequeue(E& portion)
{
    return this->queue.try_pop(portion);
}



template <typename E>
class NikolaevBoundedPortionQueue :
    public NonBlockingQueue<E>
{
private:
    xenium::nikolaev_bounded_queue<E, xenium::policy::pop_retries<0>> queue;

public:
    NikolaevBoundedPortionQueue(std::size_t maxSize);

    bool tryEnqueue(E&& portion);
    bool tryDequeue(E& portion);
};

template <typename E>
NikolaevBoundedPortionQueue<E>::NikolaevBoundedPortionQueue(std::size_t maxSize) :
	queue(1 << (unsigned)std::ceill(std::log2l(maxSize)))
{
}

template <typename E>
bool NikolaevBoundedPortionQueue<E>::tryEnqueue(E&& portion)
{
    return this->queue.try_push(portion);	// This queue has a bug, not using std::move here is a workaround.
}

template <typename E>
bool NikolaevBoundedPortionQueue<E>::tryDequeue(E& portion)
{
    return this->queue.try_pop(portion);
}


}

#endif
