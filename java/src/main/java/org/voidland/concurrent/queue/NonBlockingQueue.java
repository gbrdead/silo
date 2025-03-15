package org.voidland.concurrent.queue;


public interface NonBlockingQueue<E>
{
	default void setMaxSize(int maxSize)
	{
	}
	
    boolean tryEnqueue(E portion);
    E tryDequeue();
}
