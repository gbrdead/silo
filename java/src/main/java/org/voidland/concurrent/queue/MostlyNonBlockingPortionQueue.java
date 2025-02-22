package org.voidland.concurrent.queue;

import java.util.Collection;
import java.util.concurrent.atomic.AtomicInteger;


public class MostlyNonBlockingPortionQueue<E>
        implements MPMC_PortionQueue<E>
{
	private NonBlockingQueue<E> nonBlockingQueue;
	
    private int maxSize;
    private AtomicInteger size;
    private boolean workDone;
    
    private Object notFullCondition;
    private Object notEmptyCondition;
    private Object emptyCondition;
    
    private AtomicInteger blockedProducers;
    private AtomicInteger blockedConsumers;
    
    
    public MostlyNonBlockingPortionQueue(int initialConsumerCount, int producerCount, NonBlockingQueue<E> nonBlockingQueue)
    {
    	this.nonBlockingQueue = nonBlockingQueue;
        this.maxSize = initialConsumerCount * producerCount * 1000;
        this.size = new AtomicInteger(0);
        this.workDone = false;
        
        this.notFullCondition = new Object();
        this.notEmptyCondition = new Object();
        this.emptyCondition = new Object();
        
        this.blockedProducers = new AtomicInteger(0);
        this.blockedConsumers = new AtomicInteger(0);

        this.nonBlockingQueue.setSizeParameters(producerCount, this.maxSize + producerCount);
    }
    
    @Override
    public void addPortion(E portion)
    {
        try
        {
            int newSize = this.size.incrementAndGet();
            
            while (true)
            {
                if (this.size.get() >= this.maxSize)
                {
                	this.blockedProducers.getAndIncrement();
                    synchronized (this.notFullCondition)
                    {
                        while (this.size.get() >= this.maxSize)
                        {
                            this.notFullCondition.wait();
                        }
                    }
                    this.blockedProducers.getAndDecrement();
                }
                
                if (this.nonBlockingQueue.tryEnqueue(portion))
                {
                	break;
                }
            }
    
            if (newSize == this.maxSize * 1 / 4)
            {
            	this.blockedProducers.getAndIncrement();
                synchronized (this.notEmptyCondition)
                {
                    this.notEmptyCondition.notifyAll();
                }
                this.blockedProducers.getAndDecrement();
            }
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
            E portion = this.nonBlockingQueue.tryDequeue();
    
            if (portion == null)
            {
            	this.blockedConsumers.getAndIncrement();
                synchronized (this.notEmptyCondition)
                {
                	while (true)
                	{
                        if (this.workDone)
                        {
                        	this.blockedConsumers.getAndDecrement();
                            return null;
                        }
                        portion = this.nonBlockingQueue.tryDequeue();
                        if (portion != null)
                        {
                        	break;
                        }
                        this.notEmptyCondition.wait();
                	}
                }
                this.blockedConsumers.getAndDecrement();
            }
            
            int newSize = this.size.decrementAndGet();
            if (newSize == this.maxSize * 3 / 4)
            {
            	this.blockedConsumers.getAndIncrement();
                synchronized (this.notFullCondition)
                {
                    this.notFullCondition.notifyAll();
                }
                this.blockedConsumers.getAndDecrement();
            }
            if (newSize == 0)
            {
            	this.blockedConsumers.getAndIncrement();
                synchronized (this.emptyCondition)
                {
                    this.emptyCondition.notify();
                }
                this.blockedConsumers.getAndDecrement();
            }
    
            return portion;
        }
        catch (InterruptedException e)
        {
            throw new RuntimeException(e);
        }
    }
    
    @Override
    public void ensureAllPortionsAreRetrieved()
    {
        try
        {
            synchronized (this.notEmptyCondition)
            {
                this.notEmptyCondition.notifyAll();
            }
    
            synchronized (this.emptyCondition)
            {
                while (this.size.get() > 0)
                {
                    this.emptyCondition.wait();
                }
            }
        }
        catch (InterruptedException e)
        {
            throw new RuntimeException(e);
        }
    }

    @Override
    public void stopConsumers(Collection<Thread> finalConsumerThreads)
    {
        synchronized (this.notEmptyCondition)
        {
        	this.workDone = true;
            this.notEmptyCondition.notifyAll();
        }
    }
    
    @Override
    public int getSize()
    {
        return this.size.get();
    }
    
    @Override
    public int getMaxSize()
    {
        return this.maxSize;
    }
    
    @Override
    public int getBlockedProducers()
    {
    	return this.blockedProducers.get();
    }
    
    @Override
    public int getBlockedConsumers()
    {
    	return this.blockedConsumers.get();
    }
}
