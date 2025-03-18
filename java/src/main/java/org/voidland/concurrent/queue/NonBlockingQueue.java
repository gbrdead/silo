package org.voidland.concurrent.queue;


public interface NonBlockingQueue<E>
{
	default void setSizeParameters(int producerCount, int maxSize)
	{
	}
	
    boolean tryEnqueue(E portion);
    E tryDequeue();
}
