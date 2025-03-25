package org.voidland.concurrent.queue;

import java.util.ArrayDeque;
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
	
	
	public TextbookPortionQueue(int maxSize)
	{
		this.maxSize = maxSize;
		
		this.queue = new ArrayDeque<>(this.maxSize);
		this.workDone = false;
		
		this.mutex = new ReentrantLock(false);
		this.notEmptyCondition = this.mutex.newCondition();
		this.notFullCondition = this.mutex.newCondition();
	}
	
	@Override
    public void addPortion(E portion)
    	throws InterruptedException
	{
		if (portion == null)
		{
			throw new NullPointerException();
		}
		
	    this.mutex.lock();
	    try
	    {
	        while (this.queue.size() >= this.maxSize)
	        {
	            this.notFullCondition.await();
	        }

	        this.queue.add(portion);

	        this.notEmptyCondition.signal();
	    }
	    finally
	    {
	        this.mutex.unlock();
	    }
	}
	
	@Override
    public E retrievePortion()
    	throws InterruptedException
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

                this.notEmptyCondition.await();
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
		throws InterruptedException
	{
        this.mutex.lock();
        try
        {
            while (!this.queue.isEmpty())
            {
                this.notFullCondition.await();
            }
        }
        finally
        {
            this.mutex.unlock();
        }
	}

	@Override
	public void stopConsumers(int finalConsumerThreadsCount)
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
