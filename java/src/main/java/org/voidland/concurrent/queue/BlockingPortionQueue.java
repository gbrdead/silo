package org.voidland.concurrent.queue;

import java.util.Collection;
import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.atomic.AtomicInteger;


public class BlockingPortionQueue<E> 
	implements MPMC_PortionQueue<E>
{
    private int maxSize;
	private BlockingQueue<E> queue;
	
    private AtomicInteger blockedProducers;
    private AtomicInteger blockedConsumers;
	
	
	public BlockingPortionQueue(int initialConsumerCount, int producerCount)
	{
	    this.maxSize = initialConsumerCount * producerCount * 1000;
		this.queue = new ArrayBlockingQueue<>(this.maxSize);
		
        this.blockedProducers = new AtomicInteger(0);
        this.blockedConsumers = new AtomicInteger(0);
	}
	
	@Override
	public void addPortion(E portion)
	{
		this.blockedProducers.getAndIncrement();
	    try
	    {
	        this.queue.put(portion);
	    }
	    catch (InterruptedException e)
	    {
	        throw new RuntimeException(e);
	    }
	    finally
	    {
	    	this.blockedProducers.getAndDecrement();	
	    }
	}
	
	@Override
	public E retrievePortion()
	{
		this.blockedConsumers.getAndIncrement();
	    try
	    {
	    	
	        return this.queue.take();
	    }
	    catch (InterruptedException e)
	    {
	        return null;
	    }
	    finally
	    {
	    	this.blockedConsumers.getAndDecrement();	
	    }
	}
	
    @Override
    public void ensureAllPortionsAreRetrieved()
    {
        while (!this.queue.isEmpty())
        {
            try
            {
                Thread.sleep(1);
            }
            catch (InterruptedException e)
            {
            }
        }
    }
	
	@Override
    public void stopConsumers(Collection<Thread> finalConsumerThreads)
	{
	    for (Thread consumerThread : finalConsumerThreads)
	    {
	        consumerThread.interrupt();
	    }
	}
	
    @Override
    public int getSize()
    {
        return this.queue.size();
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
