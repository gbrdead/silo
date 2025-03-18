package org.voidland.concurrent.queue;

import java.util.concurrent.ConcurrentLinkedQueue;


public class ConcurrentNonBlockingQueue<E> 
	implements NonBlockingQueue<E>
{
	private ConcurrentLinkedQueue<E> queue;
	
	
	public ConcurrentNonBlockingQueue()
	{
		this.queue = new ConcurrentLinkedQueue<>();
	}
	
    @Override
    public boolean tryEnqueue(E portion)
    {
        return this.queue.offer(portion);
    }
    
    @Override
    public E tryDequeue()
    {
        return this.queue.poll();
    }
}
