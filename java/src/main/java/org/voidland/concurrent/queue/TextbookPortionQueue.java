package org.voidland.concurrent.queue;

import java.util.ArrayDeque;
import java.util.Collection;
import java.util.Queue;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.locks.Condition;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;


public class TextbookPortionQueue<E> 
	implements MPMC_PortionQueue<E>
{
	private Queue<E> queue;
	private boolean workDone;
	
	private int maxSize;
	
	// Not using Semaphore in order to be 1:1 with the C++ version.
	private Lock mutex;
	private Condition notEmptyCondition;
	private Condition notFullCondition;
	
    private AtomicInteger blockedProducers;
    private AtomicInteger blockedConsumers;
	
	
	public TextbookPortionQueue(int initialConsumerCount, int producerCount)
	{
		this.maxSize = initialConsumerCount * producerCount * 1000;
		
		this.queue = new ArrayDeque<>(this.maxSize);
		this.workDone = false;
		
		this.mutex = new ReentrantLock(false);
		this.notEmptyCondition = this.mutex.newCondition();
		this.notFullCondition = this.mutex.newCondition();
		
        this.blockedProducers = new AtomicInteger(0);
        this.blockedConsumers = new AtomicInteger(0);
	}
	
	@Override
    public void addPortion(E work)
	{
		this.blockedProducers.getAndIncrement();
	    this.mutex.lock();
	    try
	    {
	        while (this.queue.size() >= this.maxSize)
	        {
	            this.notFullCondition.awaitUninterruptibly();
	        }
	        this.blockedProducers.getAndDecrement();

	        this.queue.add(work);

	        this.blockedProducers.getAndIncrement();
	        this.notEmptyCondition.signal();
	    }
	    finally
	    {
	        this.mutex.unlock();
	        this.blockedProducers.getAndDecrement();
	    }
	}
	
	@Override
    public E retrievePortion()
	{
		this.blockedConsumers.getAndIncrement();
        this.mutex.lock();
        try
        {
            while (this.queue.isEmpty())
            {
                if (this.workDone)
                {
                    return null;
                }

                this.notEmptyCondition.awaitUninterruptibly();
            }
            this.blockedConsumers.getAndDecrement();

            E portion = this.queue.poll();

            this.blockedConsumers.getAndIncrement();    
            this.notFullCondition.signal();
            
            return portion;
        }
        finally
        {
            this.mutex.unlock();
            this.blockedConsumers.getAndDecrement();
        }
	}
	
	@Override
	public void ensureAllPortionsAreRetrieved()
	{
        this.mutex.lock();
        try
        {
            while (!this.queue.isEmpty())
            {
                this.notFullCondition.awaitUninterruptibly();
            }
        }
        finally
        {
            this.mutex.unlock();
        }
	}

	@Override
	public void stopConsumers(Collection<Thread> finalConsumerThreads)
	{
        this.mutex.lock();
        try
        {
            this.workDone = true;

            this.notEmptyCondition.signalAll();
        }
        finally
        {
            this.mutex.unlock();
        }
    }
	
	@Override
    public int getSize()
    {
        this.mutex.lock();
        try
        {
            return this.queue.size();
        }
        finally
        {
            this.mutex.unlock();
        }
    }
    
	@Override
    public int getMaxSize()
    {
	    return this.maxSize;
    }
    
    @Override
    public int getBlockedProducers()
    {
    	return this.blockedProducers.get();
    }
    
    @Override
    public int getBlockedConsumers()
    {
    	return this.blockedConsumers.get();
    }
}
