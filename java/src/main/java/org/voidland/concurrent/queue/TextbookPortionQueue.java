package org.voidland.concurrent.queue;

import java.util.ArrayDeque;
import java.util.Collection;
import java.util.Queue;
import java.util.concurrent.locks.Condition;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;


public class TextbookPortionQueue<E> 
	implements MPMC_PortionQueue<E>
{
	private Queue<E> queue;
	private boolean workDone;
	
	private int maxSize;
	
	// Not using Semaphore in order to be 1:1 with the C++ version. Semaphore seems to be slower anyway.
	private Lock mutex;
	private Condition notEmptyCondition;
	private Condition notFullCondition;
	
	
	public TextbookPortionQueue(int initialConsumerCount, int producerCount)
	{
		this.maxSize = initialConsumerCount * producerCount * 1000;
		
		this.queue = new ArrayDeque<>(this.maxSize);
		this.workDone = false;
		
		this.mutex = new ReentrantLock(false);
		this.notEmptyCondition = this.mutex.newCondition();
		this.notFullCondition = this.mutex.newCondition();
	}
	
	@Override
    public void addPortion(E work)
	{
	    this.mutex.lock();
	    try
	    {
	        while (this.queue.size() >= this.maxSize)
	        {
	            this.notFullCondition.awaitUninterruptibly();
	        }

	        this.queue.add(work);

	        this.notEmptyCondition.signal();
	    }
	    finally
	    {
	        this.mutex.unlock();
	    }
	}
	
	@Override
    public E retrievePortion()
	{
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

            E portion = this.queue.poll();

            this.notFullCondition.signal();
            
            return portion;
        }
        finally
        {
            this.mutex.unlock();
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
}
