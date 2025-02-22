package org.voidland.concurrent.queue;

import java.util.Collection;
import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.BlockingQueue;


public class BlockingPortionQueue<E> 
	implements MPMC_PortionQueue<E>
{
    private int maxSize;
	private BlockingQueue<E> queue;
	
	
	public BlockingPortionQueue(int initialConsumerCount, int producerCount)
	{
	    this.maxSize = initialConsumerCount * producerCount * 1000;
		this.queue = new ArrayBlockingQueue<>(this.maxSize);
	}
	
	@Override
	public void addPortion(E portion)
	{
	    try
	    {
	        this.queue.put(portion);
	    }
	    catch (InterruptedException e)
	    {
	        throw new RuntimeException(e);
	    }
	}
	
	@Override
	public E retrievePortion()
	{
	    try
	    {
	        return this.queue.take();
	    }
	    catch (InterruptedException e)
	    {
	        return null;
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
}
