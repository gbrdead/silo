package org.voidland.concurrent;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.util.concurrent.atomic.AtomicInteger;


public class TurningGrilleCrackerWithPerfectParallelism
        extends TurningGrilleCracker
{
	private AtomicInteger workersCount;
	private List<Thread> workerThreads;
	private List<GrilleInterval> grilleIntervals;
	
	
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
        
        this.startWorkerThreads(cpuCount);
        
        try
        {
            for (Thread workerThread : this.workerThreads)
            {
                workerThread.join();
            }
        }
        catch (InterruptedException e)
        {
            throw new RuntimeException(e);
        }
    }

    private void startWorkerThreads(int workerCount)
    {
        this.workerThreads = new ArrayList<>(workerCount);
        this.grilleIntervals = new ArrayList<>(workerCount);
        
        long nextIntervalBegin = 0;
        long intervalLength = Math.round((double)this.grilleCount / workerCount);
        for (int i = 0; i < workerCount; i++)
        {
            GrilleInterval grilleInterval = new GrilleInterval(this.sideLength / 2, nextIntervalBegin,
                    (i < workerCount - 1) ? (nextIntervalBegin + intervalLength) : this.grilleCount);
            this.grilleIntervals.add(grilleInterval);
            
            Thread workerThread = new Thread(() ->
            {
            	this.workersCount.incrementAndGet();
                Grille grille;
                while ((grille = grilleInterval.getNext()) != null)
                {
                    applyGrille(grille);
                }
                this.workersCount.decrementAndGet();
            });
            this.workerThreads.add(workerThread);
            workerThread.start();
            
            nextIntervalBegin += intervalLength;
        }
    }
    
    @Override
    protected String milestone(long grillesPerSecond)
    {
        if (Silo.VERBOSE)
        {
            StringBuilder status = new StringBuilder();
            
            status.append("worker threads: ");
            status.append(this.workersCount.get());
            status.append("; completion per thread: ");
            
            Iterator<GrilleInterval> i = this.grilleIntervals.iterator();
            while (i.hasNext())
            {
            	GrilleInterval grilleInterval = i.next();
            	status.append(String.format("%.1f", grilleInterval.calculateCompletion()));
            	if (i.hasNext())
            	{
            		status.append("/");
            	}
            }
            status.append("% done");
            
            return status.toString();
        }
        return "";
    }
}
