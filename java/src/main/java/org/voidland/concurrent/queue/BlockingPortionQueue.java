package org.voidland.concurrent.queue;

import java.util.Optional;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.atomic.AtomicInteger;


public class BlockingPortionQueue<E> 
	implements MPMC_PortionQueue<E>
{
	private AtomicInteger activeConsumerCount;
	private volatile boolean workDone;
    private int maxSize;
	private BlockingQueue<Optional<E>> queue;
	
	
	public BlockingPortionQueue(int maxSize, BlockingQueue<Optional<E>> blockingQueue)
	{
	    this.maxSize = maxSize;
	    this.activeConsumerCount = new AtomicInteger(0);
	    this.workDone = false;
		this.queue = blockingQueue;
	}
	
	@Override
	public void addPortion(E portion)
		throws InterruptedException
	{
		if (portion == null)
		{
			throw new NullPointerException();
		}
		
		this.queue.put(Optional.of(portion));
	}
	
	@Override
	public E retrievePortion()
		throws InterruptedException
	{
		this.activeConsumerCount.getAndIncrement();
		E portion = null;
		if (!this.workDone)
		{
			portion = this.queue.take().orElse(null);
		}
		this.activeConsumerCount.getAndDecrement();
		return portion;
	}
	
	@Override
    public void stopConsumers()
    	throws InterruptedException
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
		this.workDone = true;
		int activeConsumerCount = this.activeConsumerCount.get();
	    for (int i = 0; i < activeConsumerCount; i++)
	    {
	    	this.queue.put(Optional.empty());
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
