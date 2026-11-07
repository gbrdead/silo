#ifndef __VOIDLAND_BLOCKING_WORK_QUEUE_HPP__
#define __VOIDLAND_BLOCKING_WORK_QUEUE_HPP__

#include <voidland/blown_queue/MPMC_PortionQueue.hpp>

#include <queue>
#include <mutex>
#include <condition_variable>
#include <utility>
#include <thread>
#include <oneapi/tbb/concurrent_queue.h>
#include <boost/thread/sync_bounded_queue.hpp>


namespace voidland::concurrent::queue
{


template <typename E>
class TextbookPortionQueue :
    public MPMC_PortionQueue<E>
{
private:
	std::queue<E> queue;
	bool workDone;

    std::size_t maxSize;

    // Not using counting_semaphore because of https://gcc.gnu.org/bugzilla/show_bug.cgi?id=104928 .
    std::mutex mutex;
    std::condition_variable notEmptyCondition;
    std::condition_variable notFullCondition;

public:
    TextbookPortionQueue(std::size_t maxSize);

    void addPortion(const E& portion);
    void addPortion(E&& portion);
    std::optional<E> retrievePortion();
    void stopConsumers();
    std::size_t getSize();
    std::size_t getMaxSize();
};


template <typename E>
TextbookPortionQueue<E>::TextbookPortionQueue(std::size_t maxSize) :
	queue(),
    maxSize(maxSize),
    workDone(false),
	mutex(),
	notEmptyCondition(),
	notFullCondition()
{
}

template <typename E>
void TextbookPortionQueue<E>::addPortion(const E& portion)
{
    E portionCopy(portion);
    this->addPortion(std::move(portionCopy));
}

template <typename E>
void TextbookPortionQueue<E>::addPortion(E&& portion)
{
    std::unique_lock lock(this->mutex);

    while (this->queue.size() >= this->maxSize)
    {
        this->notFullCondition.wait(lock);
    }

    this->queue.push(std::move(portion));

    this->notEmptyCondition.notify_one();
}

template <typename E>
std::optional<E> TextbookPortionQueue<E>::retrievePortion()
{
    std::unique_lock lock(this->mutex);

    while (this->queue.empty())
    {
        if (this->workDone)
        {
            return std::nullopt;
        }

        this->notEmptyCondition.wait(lock);
    }

    E portion = std::move(this->queue.front());
    this->queue.pop();

    this->notFullCondition.notify_one();

    return portion;
}

template <typename E>
void TextbookPortionQueue<E>::stopConsumers()
{
    std::unique_lock lock(this->mutex);

    this->workDone = true;

    this->notEmptyCondition.notify_all();
}

template <typename E>
std::size_t TextbookPortionQueue<E>::getSize()
{
    std::unique_lock lock(this->mutex);

    return this->queue.size();
}

template <typename E>
std::size_t TextbookPortionQueue<E>::getMaxSize()
{
    return this->maxSize;
}



template <typename E>
class BlockingPortionQueue :
    public MPMC_PortionQueue<E>
{
private:
    std::atomic<std::size_t> activeConsumerCount;
    std::atomic<bool> workDone;

protected:
    std::size_t maxSize;

public:
    BlockingPortionQueue(std::size_t maxSize);

    std::optional<E> retrievePortion();
    void stopConsumers();
    std::size_t getMaxSize();

protected:
    virtual void addEmptyPortion() = 0;
    virtual void retrievePortion(std::optional<E>& portion) = 0;
    virtual bool isEmpty() = 0;
};


template <typename E>
BlockingPortionQueue<E>::BlockingPortionQueue(std::size_t maxSize) :
    maxSize(maxSize),
	activeConsumerCount(0),
	workDone(false)
{
}

template <typename E>
std::optional<E> BlockingPortionQueue<E>::retrievePortion()
{
	this->activeConsumerCount.fetch_add(1, std::memory_order_release);
	std::optional<E> portion;
	if (!this->workDone.load(std::memory_order_acquire))
	{
		this->retrievePortion(portion);
	}
    this->activeConsumerCount.fetch_sub(1, std::memory_order_release);
    return portion;
}

template <typename E>
void BlockingPortionQueue<E>::stopConsumers()
{
	while (!this->isEmpty())
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	this->workDone.store(true, std::memory_order_release);
	std::size_t activeConsumerCount = this->activeConsumerCount.load(std::memory_order_acquire);
    for (std::size_t i = 0; i < activeConsumerCount; i++)
    {
        this->addEmptyPortion();
    }
}

template <typename E>
std::size_t BlockingPortionQueue<E>::getMaxSize()
{
    return this->maxSize;
}



template <typename E>
class OneTBB_BoundedPortionQueue :
    public BlockingPortionQueue<E>
{
private:
    oneapi::tbb::concurrent_bounded_queue<std::optional<E>> queue;

public:
    OneTBB_BoundedPortionQueue(std::size_t maxSize);

    void addPortion(const E& portion);
    void addPortion(E&& portion);
    std::size_t getSize();

protected:
    void addEmptyPortion();
    void retrievePortion(std::optional<E>& portion);
    bool isEmpty();
};


template <typename E>
OneTBB_BoundedPortionQueue<E>::OneTBB_BoundedPortionQueue(std::size_t maxSize) :
	BlockingPortionQueue<E>(maxSize),
	queue()
{
    this->queue.set_capacity(maxSize);
}

template <typename E>
void OneTBB_BoundedPortionQueue<E>::addPortion(const E& portion)
{
    this->queue.push(portion);
}

template <typename E>
void OneTBB_BoundedPortionQueue<E>::addPortion(E&& portion)
{
    this->queue.push(std::move(portion));
}

template <typename E>
void OneTBB_BoundedPortionQueue<E>::addEmptyPortion()
{
    this->queue.push(std::nullopt);
}

template <typename E>
void OneTBB_BoundedPortionQueue<E>::retrievePortion(std::optional<E>& portion)
{
    this->queue.pop(portion);
}

template <typename E>
std::size_t OneTBB_BoundedPortionQueue<E>::getSize()
{
    return this->queue.size();
}

template <typename E>
bool OneTBB_BoundedPortionQueue<E>::isEmpty()
{
    return this->queue.empty();
}



template <typename E>
class SyncBoundedPortionQueue :
    public BlockingPortionQueue<E>
{
private:
    boost::sync_bounded_queue<std::optional<E>> queue;

public:
    SyncBoundedPortionQueue(std::size_t maxSize);

    void addPortion(const E& portion);
    void addPortion(E&& portion);
    std::size_t getSize();

protected:
    void addEmptyPortion();
    void retrievePortion(std::optional<E>& portion);
    bool isEmpty();
};


template <typename E>
SyncBoundedPortionQueue<E>::SyncBoundedPortionQueue(std::size_t maxSize) :
	BlockingPortionQueue<E>(maxSize),
	queue(maxSize)
{
}

template <typename E>
void SyncBoundedPortionQueue<E>::addPortion(const E& portion)
{
    this->queue.push_back(portion);
}

template <typename E>
void SyncBoundedPortionQueue<E>::addPortion(E&& portion)
{
    this->queue.push_back(std::move(portion));
}

template <typename E>
void SyncBoundedPortionQueue<E>::addEmptyPortion()
{
	this->queue.push_back(std::nullopt);
}

template <typename E>
void SyncBoundedPortionQueue<E>::retrievePortion(std::optional<E>& portion)
{
	this->queue.pull_front(portion);
}

template <typename E>
std::size_t SyncBoundedPortionQueue<E>::getSize()
{
    return this->queue.size();
}

template <typename E>
bool SyncBoundedPortionQueue<E>::isEmpty()
{
    return this->queue.empty();
}


}

#endif
