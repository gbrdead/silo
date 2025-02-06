package org.voidland.concurrent.queue;

import java.util.Collection;


public interface MPMC_PortionQueue<E>
{
	void addPortion(E portion);
	E retrievePortion();

    // The following two methods must be called in succession after the producer threads are done adding portions.
    // After that, the queue cannot be reused.
	void ensureAllPortionsAreRetrieved();
	void stopConsumers(Collection<Thread> cosumerThreads);
	
    int getSize();
    int getMaxSize();
}
