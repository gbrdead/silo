package org.voidland.concurrent;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.atomic.AtomicInteger;


public class TurningGrilleCrackerWithPerfectParallelism
        extends TurningGrilleCracker
{
	private AtomicInteger workersCount;
	
	
    public TurningGrilleCrackerWithPerfectParallelism(String cipherText)
        throws IOException
    {
        super(cipherText);
        this.workersCount = new AtomicInteger(0);
    }

    @Override
    protected void doBruteForce()
    {
        int cpuCount = Runtime.getRuntime().availableProcessors();
        
        List<Thread> workerThreads = this.startWorkerThreads(cpuCount);
        
        try
        {
            for (Thread workerThread : workerThreads)
            {
                workerThread.join();
            }
        }
        catch (InterruptedException e)
        {
            throw new RuntimeException(e);
        }
    }

    private List<Thread> startWorkerThreads(int workerCount)
    {
        List<Thread> workerThreads = new ArrayList<>(workerCount);
        
        long nextIntervalBegin = 0;
        long intervalLength = Math.round((double)this.grilleCount / workerCount);
        for (int i = 0; i < workerCount; i++)
        {
            GrilleInterval grilleInterval = new GrilleInterval(this.sideLength / 2, nextIntervalBegin,
                    (i < workerCount - 1) ? (nextIntervalBegin + intervalLength) : this.grilleCount);
            
            Thread producerThread = new Thread(() ->
            {
            	this.workersCount.incrementAndGet();
                Grille grille;
                while ((grille = grilleInterval.getNext()) != null)
                {
                    applyGrille(grille);
                }
                this.workersCount.decrementAndGet();
            });
            workerThreads.add(producerThread);
            producerThread.start();
            
            nextIntervalBegin += intervalLength;
        }
        
        return workerThreads;
    }
    
    @Override
    protected String milestone(long grillesPerSecond)
    {
        if (Silo.VERBOSE)
        {
            return "worker threads: " + this.workersCount.get();
        }
        return "";
    }
}
