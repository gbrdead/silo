package org.voidland.concurrent.queue;

import java.util.Optional;
import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.BlockingQueue;


public class BlockingPortionQueue<E> 
	implements MPMC_PortionQueue<E>
{
    private int maxSize;
	private BlockingQueue<Optional<E>> queue;
	
	
	public BlockingPortionQueue(int maxSize)
	{
	    this.maxSize = maxSize;
		this.queue = new ArrayBlockingQueue<>(this.maxSize);
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
        return this.queue.take().orElse(null);
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
    public void stopConsumers(int finalConsumerThreadsCount)
    	throws InterruptedException
	{
	    for (int i = 0; i < finalConsumerThreadsCount; i++)
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
