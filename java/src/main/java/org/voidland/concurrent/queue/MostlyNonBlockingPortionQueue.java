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
    
    private Object notFullCondition = new Object();
    private Object notEmptyCondition = new Object();
    private Object emptyCondition = new Object();
    
    
    public MostlyNonBlockingPortionQueue(int initialConsumerCount, int producerCount, NonBlockingQueue<E> nonBlockingQueue)
    {
    	this.nonBlockingQueue = nonBlockingQueue;
        this.maxSize = initialConsumerCount * producerCount * 1000;
        this.size = new AtomicInteger(0);
        this.workDone = false;
        
        this.nonBlockingQueue.setSizeParameters(producerCount, this.maxSize + producerCount);
    }
    
    @Override
    public void addPortion(E portion)
    {
        try
        {
            int newSize = this.size.incrementAndGet();
            
            do
            {
                if (this.size.get() > this.maxSize)
                {
                    synchronized (this.notFullCondition)
                    {
                        if (this.size.get() > this.maxSize)
                        {
                            this.notFullCondition.wait();
                        }
                    }
                }
            }
            while (!this.nonBlockingQueue.tryEnqueue(portion));
    
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
            E portion;
    
            if ((portion = this.nonBlockingQueue.tryDequeue()) == null)
            {
                synchronized (this.notEmptyCondition)
                {
                    while ((portion = this.nonBlockingQueue.tryDequeue()) == null)
                    {
                        if (this.workDone)
                        {
                            return null;
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
    public void stopConsumers(Collection<Thread> consumerThreads)
    {
        this.workDone = true;
        synchronized (this.notEmptyCondition)
        {
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
