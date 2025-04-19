#ifndef __VOIDLAND_NON_BLOCKING_QUEUE_HPP__
#define __VOIDLAND_NON_BLOCKING_QUEUE_HPP__

#include <cstddef>
#include <memory>
#include <cmath>

#include <boost/lockfree/queue.hpp>
#include <oneapi/tbb/concurrent_queue.h>
#include "concurrentqueue/concurrentqueue.h"
#include "atomic_queue/atomic_queue.h"

#include "xenium/reclamation/generic_epoch_based.hpp"
#include "xenium/michael_scott_queue.hpp"
#include "xenium/ramalhete_queue.hpp"
#include "xenium/vyukov_bounded_queue.hpp"
#include "xenium/kirsch_kfifo_queue.hpp"
#include "xenium/kirsch_bounded_kfifo_queue.hpp"
#include "xenium/nikolaev_queue.hpp"
#include "xenium/nikolaev_bounded_queue.hpp"



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
	queue(maxSize)
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
    this->queue.push(std::move(portion));
    return true;
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



template <typename E>
class MichaelScottPortionQueue :
    public NonBlockingQueue<E>
{
private:
    xenium::michael_scott_queue<E, xenium::policy::reclaimer<xenium::reclamation::epoch_based<>>> queue;

public:
    MichaelScottPortionQueue(std::size_t maxSize);

    bool tryEnqueue(E&& portion);
    bool tryDequeue(E& portion);
};

template <typename E>
MichaelScottPortionQueue<E>::MichaelScottPortionQueue(std::size_t maxSize) :
	queue()
{
}

template <typename E>
bool MichaelScottPortionQueue<E>::tryEnqueue(E&& portion)
{
    this->queue.push(std::move(portion));
    return true;
}

template <typename E>
bool MichaelScottPortionQueue<E>::tryDequeue(E& portion)
{
    return this->queue.try_pop(portion);
}



template <typename E>
class RamalhetePortionQueue :
    public NonBlockingQueue<E>
{
private:
	xenium::ramalhete_queue<E*, xenium::policy::reclaimer<xenium::reclamation::epoch_based<>>> queue;

public:
    RamalhetePortionQueue(std::size_t maxSize);
    ~RamalhetePortionQueue();

    bool tryEnqueue(E&& portion);
    bool tryDequeue(E& portion);
};

template <typename E>
RamalhetePortionQueue<E>::RamalhetePortionQueue(std::size_t maxSize) :
	queue()
{
}

template <typename E>
RamalhetePortionQueue<E>::~RamalhetePortionQueue()
{
	E* portionCopy;
	while (this->queue.try_pop(portionCopy))
	{
	    delete portionCopy;
	}
}

template <typename E>
bool RamalhetePortionQueue<E>::tryEnqueue(E&& portion)
{
    E* portionCopy = new E(std::move(portion));
    this->queue.push(portionCopy);
    return true;
}

template <typename E>
bool RamalhetePortionQueue<E>::tryDequeue(E& portion)
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
class VyukovPortionQueue :
    public NonBlockingQueue<E>
{
private:
    xenium::vyukov_bounded_queue<E> queue;

public:
    VyukovPortionQueue(std::size_t maxSize);

    bool tryEnqueue(E&& portion);
    bool tryDequeue(E& portion);
};

template <typename E>
VyukovPortionQueue<E>::VyukovPortionQueue(std::size_t maxSize) :
	queue(1 << (unsigned)std::ceill(std::log2l(maxSize)))
{
}

template <typename E>
bool VyukovPortionQueue<E>::tryEnqueue(E&& portion)
{
    return this->queue.try_push(std::move(portion));
}

template <typename E>
bool VyukovPortionQueue<E>::tryDequeue(E& portion)
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
    return this->queue.try_push(portion);	// std::move must not be used here.
}

template <typename E>
bool NikolaevBoundedPortionQueue<E>::tryDequeue(E& portion)
{
    return this->queue.try_pop(portion);
}


}

#endif
