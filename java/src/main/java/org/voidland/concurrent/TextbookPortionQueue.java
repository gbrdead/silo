package org.voidland.concurrent;

import java.util.ArrayDeque;
import java.util.Collection;
import java.util.Queue;
import java.util.concurrent.locks.Condition;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;


public class TextbookPortionQueue<E> 
	implements MPMC_PortionQueue<E>
{
	private int maxSize;
	private Queue<E> queue;
	
	// Not using Semaphore in order to be 1:1 with the C++ version.
	private Lock mutex;
	private Condition fullCondition;
	private Condition emptyCondition;
	private boolean workDone;
	
	
	public TextbookPortionQueue(int initialConsumerCount, int producerCount)
	{
		this.maxSize = initialConsumerCount * producerCount * 1000;
		this.queue = new ArrayDeque<>(this.maxSize);
		
		this.mutex = new ReentrantLock(false);
		this.fullCondition = this.mutex.newCondition();
		this.emptyCondition = this.mutex.newCondition();
		this.workDone = false;
	}
	
	@Override
    public void addPortion(E work)
	{
	    this.mutex.lock();
	    try
	    {
	        while (this.queue.size() >= this.maxSize)
	        {
	            this.emptyCondition.awaitUninterruptibly();
	        }

	        this.queue.add(work);

	        this.fullCondition.signal();
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

                this.fullCondition.awaitUninterruptibly();
            }

            E portion = this.queue.poll();

            this.emptyCondition.signal();
            
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
                this.emptyCondition.awaitUninterruptibly();
            }
        }
        finally
        {
            this.mutex.unlock();
        }
	}

	@Override
	public void stopConsumers(Collection<Thread> consumerThreads)
	{
        this.mutex.lock();
        try
        {
            this.workDone = true;

            this.fullCondition.signalAll();
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
