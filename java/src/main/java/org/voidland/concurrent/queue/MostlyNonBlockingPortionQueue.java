package org.voidland.concurrent.queue;

import java.util.Collection;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicInteger;


public class MostlyNonBlockingPortionQueue<E>
        implements MPMC_PortionQueue<E>
{
    private AtomicInteger size;
    private int maxSize;
    
    private Object notFullCondition;
    private Object notEmptyCondition;
    private Object emptyCondition;
    
    private AtomicBoolean aProducerIsWaiting;
    private AtomicBoolean aConsumerIsWaiting;
    
    private NonBlockingQueue<E> nonBlockingQueue;
    private boolean workDone;
    
    
    public static <E> MostlyNonBlockingPortionQueue<E> createConcurrentBlownQueue(int maxSize)
    {
    	NonBlockingQueue<E> nonBlockingQueue = new ConcurrentNonBlockingQueue<E>(maxSize);
        return new MostlyNonBlockingPortionQueue<E>(maxSize, nonBlockingQueue);
    }
    
    
    public MostlyNonBlockingPortionQueue(int maxSize, NonBlockingQueue<E> nonBlockingQueue)
    {
    	this.size = new AtomicInteger(0);
        this.maxSize = maxSize;
        
        this.notFullCondition = new Object();
        this.notEmptyCondition = new Object();
        this.emptyCondition = new Object();

        this.aProducerIsWaiting = new AtomicBoolean(false);
        this.aConsumerIsWaiting = new AtomicBoolean(false);
        
        this.nonBlockingQueue = nonBlockingQueue;
        this.workDone = false;
    }
    
    @Override
    public void addPortion(E portion)
    {
        try
        {
            while (true)
            {
                if (this.size.get() >= this.maxSize)
                {
                    synchronized (this.notFullCondition)
                    {
                        while (this.size.get() >= this.maxSize)
                        {
                        	this.aProducerIsWaiting.set(true);
                            this.notFullCondition.wait();
                        }
                    }
                }
                
                if (this.nonBlockingQueue.tryEnqueue(portion))
                {
                	break;
                }
            }
            this.size.getAndIncrement();
    
            if (this.aConsumerIsWaiting.compareAndSet(true, false))
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
                        
                        this.aConsumerIsWaiting.set(true);
                        this.notEmptyCondition.wait();
                	}
                }
            }
            
            int newSize = this.size.decrementAndGet();
            
            if (this.aProducerIsWaiting.compareAndSet(true, false))
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
