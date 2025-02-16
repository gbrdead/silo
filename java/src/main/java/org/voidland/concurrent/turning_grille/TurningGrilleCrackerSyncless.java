package org.voidland.concurrent.turning_grille;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.util.concurrent.atomic.AtomicInteger;

import org.voidland.concurrent.Silo;


public class TurningGrilleCrackerSyncless
        implements TurningGrilleCrackerImplDetails
{
	private AtomicInteger workersCount;
	private List<GrilleInterval> grilleIntervals;
	
	
    public TurningGrilleCrackerSyncless()
        throws IOException
    {
        this.workersCount = new AtomicInteger(0);
    }

    @Override
    public void bruteForce(TurningGrilleCracker cracker)
    {
        int cpuCount = Runtime.getRuntime().availableProcessors();
        
        List<Thread> workerThreads = this.startWorkerThreads(cracker, cpuCount);
        
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

    private List<Thread> startWorkerThreads(TurningGrilleCracker cracker, int workerCount)
    {
    	List<Thread> workerThreads = new ArrayList<>(workerCount);
        this.grilleIntervals = new ArrayList<>(workerCount);
        
        long nextIntervalBegin = 0;
        long intervalLength = Math.round((double)cracker.grilleCount / workerCount);
        for (int i = 0; i < workerCount; i++)
        {
        	this.workersCount.getAndIncrement();
        	
            GrilleInterval grilleInterval = new GrilleInterval(cracker.sideLength / 2, nextIntervalBegin,
                    (i < workerCount - 1) ? (nextIntervalBegin + intervalLength) : cracker.grilleCount);
            this.grilleIntervals.add(grilleInterval);
            Thread workerThread = new Thread(() ->
            {
            	
                Grille grille;
                while ((grille = grilleInterval.getNext()) != null)
                {
                    cracker.applyGrille(grille);
                }
                this.workersCount.getAndDecrement();
            });
            workerThreads.add(workerThread);
            workerThread.start();
            
            nextIntervalBegin += intervalLength;
        }
        
        return workerThreads;
    }
    
    @Override
    public String milestone(TurningGrilleCracker cracker, long grillesPerSecond)
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
