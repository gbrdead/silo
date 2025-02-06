package org.voidland.concurrent.queue;


public interface UnboundedNonBlockingQueue<E>
{
	default void setSizeParameters(int producerCount, int desiredMaxSize)
	{
	}
	
    boolean tryEnqueue(E portion);
    E tryDequeue();
}
