package org.voidland.concurrent;

import java.util.concurrent.ConcurrentLinkedQueue;


public class ConcurrentPortionQueue<E> 
	extends NonBlockingPortionQueue<E>
{
	private ConcurrentLinkedQueue<E> queue;
	
	
	public ConcurrentPortionQueue(int initialConsumerCount, int producerCount)
	{
	    super(initialConsumerCount, producerCount);
		this.queue = new ConcurrentLinkedQueue<>();
	}
	
    @Override
    protected boolean tryEnqueue(E work)
    {
        return this.queue.offer(work);
    }
    
    @Override
    protected E tryDequeue()
    {
        return this.queue.poll();
    }
}
