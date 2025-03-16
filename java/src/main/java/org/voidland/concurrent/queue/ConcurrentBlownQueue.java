package org.voidland.concurrent.queue;

import java.lang.reflect.InvocationTargetException;


public class ConcurrentBlownQueue<E>
	extends MostlyNonBlockingPortionQueue<E>
{
	public ConcurrentBlownQueue(int initialConsumerCount, int producerCount)
    		throws InstantiationException, IllegalAccessException, IllegalArgumentException, InvocationTargetException, NoSuchMethodException, SecurityException
	{
		super(initialConsumerCount, producerCount, ConcurrentNonBlockingQueue.class);
	}
}
