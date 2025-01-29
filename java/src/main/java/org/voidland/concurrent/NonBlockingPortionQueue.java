package org.voidland.concurrent;

import java.util.Collection;
import java.util.concurrent.atomic.AtomicInteger;


abstract public class NonBlockingPortionQueue<E>
        implements MPMC_PortionQueue<E>
{
    protected int maxSize;
    private AtomicInteger size;
    private boolean workDone;
    
    private Object fullCondition = new Object();
    private Object emptyCondition = new Object();
    private Object notEmptyCondition = new Object();
    
    
    public NonBlockingPortionQueue(int initialConsumerCount, int producerCount)
    {
        this.maxSize = initialConsumerCount * producerCount * 1000;
        this.size = new AtomicInteger(0);
        this.workDone = false;
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
                    synchronized (this.fullCondition)
                    {
                        if (this.size.get() > this.maxSize)
                        {
                            this.fullCondition.wait();
                        }
                    }
                }
            }
            while (!this.tryEnqueue(portion));
    
            if (newSize == this.maxSize * 1 / 4)
            {
                synchronized (this.emptyCondition)
                {
                    this.emptyCondition.notifyAll();
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
    
            if ((portion = this.tryDequeue()) == null)
            {
                synchronized (this.emptyCondition)
                {
                    while ((portion = this.tryDequeue()) == null)
                    {
                        if (this.workDone)
                        {
                            return null;
                        }
                        this.emptyCondition.wait();
                    }
                }
            }
            
            int newSize = this.size.decrementAndGet();
            if (newSize == this.maxSize * 3 / 4)
            {
                synchronized (this.fullCondition)
                {
                    this.fullCondition.notifyAll();
                }
            }
            if (newSize == 0)
            {
                synchronized (this.notEmptyCondition)
                {
                    this.notEmptyCondition.notify();
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
            synchronized (this.emptyCondition)
            {
                this.emptyCondition.notifyAll();
            }
    
            synchronized (this.notEmptyCondition)
            {
                while (this.size.get() > 0)
                {
                    this.notEmptyCondition.wait();
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
        synchronized (this.emptyCondition)
        {
            this.emptyCondition.notifyAll();
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
    
    abstract protected boolean tryEnqueue(E work);
    abstract protected E tryDequeue();
}
