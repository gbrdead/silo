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
    
    
    public MostlyNonBlockingPortionQueue(int initialConsumerCount, int producerCount, NonBlockingQueue<E> nonBlockingQueue)
    {
    	this.nonBlockingQueue = nonBlockingQueue;
        this.maxSize = initialConsumerCount * producerCount * 1000;
        this.size = new AtomicInteger(0);
        this.workDone = false;
        
        this.notFullCondition = new Object();
        this.notEmptyCondition = new Object();
        this.emptyCondition = new Object();

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
                    synchronized (this.notFullCondition)
                    {
                        while (this.size.get() >= this.maxSize)
                        {
                            this.notFullCondition.wait();
                        }
                    }
                }
                
                if (this.nonBlockingQueue.tryEnqueue(portion))
                {
                	break;
                }
            }
    
            if (newSize == this.maxSize * 1 / 4)
            {
                synchronized (this.notEmptyCondition)
                {
                    this.notEmptyCondition.notifyAll();
                }
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
                synchronized (this.notEmptyCondition)
                {
                	while (true)
                	{
                        if (this.workDone)
                        {
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
            }
            
            int newSize = this.size.decrementAndGet();
            if (newSize == this.maxSize * 3 / 4)
            {
                synchronized (this.notFullCondition)
                {
                    this.notFullCondition.notifyAll();
                }
            }
            if (newSize == 0)
            {
                synchronized (this.emptyCondition)
                {
                    this.emptyCondition.notify();
                }
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
}
