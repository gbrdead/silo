package org.voidland.concurrent;

import java.util.ArrayDeque;
import java.util.Collection;
import java.util.Queue;
import java.util.concurrent.Semaphore;


public class TextbookPortionQueue<E> 
	implements MPMC_PortionQueue<E>
{
	private int maxSize;
	private Queue<E> queue;
	
	private Semaphore fullSemaphore;
	private Semaphore emptySemaphore;
	
	
	public TextbookPortionQueue(int initialConsumerCount, int producerCount)
	{
		this.maxSize = initialConsumerCount * producerCount * 1000;
		this.queue = new ArrayDeque<>(this.maxSize);
		
		this.fullSemaphore = new Semaphore(0, true);
		this.emptySemaphore = new Semaphore(this.maxSize, true);
	}
	
	@Override
    public void addPortion(E work)
	{
		this.emptySemaphore.acquireUninterruptibly();
		synchronized (this.queue)
		{
		    this.queue.add(work);
		}
		this.fullSemaphore.release();
	}
	
	@Override
    public E retrievePortion()
	{
	    this.fullSemaphore.acquireUninterruptibly();
	    
	    E work;
	    synchronized (this.queue)
	    {
	        if (this.queue.isEmpty())
	        {
	            return null;
	        }
	        work = this.queue.poll();
	    }
	    
	    this.emptySemaphore.release();
	    
	    return work;
	}
	
	@Override
	public void ensureAllPortionsAreRetrieved()
	{
	    this.emptySemaphore.acquireUninterruptibly(this.maxSize);
	}

	@Override
	public void stopConsumers(Collection<Thread> consumerThreads)
	{
	    this.fullSemaphore.release(consumerThreads.size());
    }
	
	@Override
    public int getSize()
    {
	    synchronized (this.queue)
	    {
	        return this.queue.size();
	    }
    }
    
	@Override
    public int getMaxSize()
    {
	    return this.maxSize;
    }
}
