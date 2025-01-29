package org.voidland.concurrent;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;


public class TurningGrilleCrackerWithPerfectParallelism
        extends TurningGrilleCracker
{
    public TurningGrilleCrackerWithPerfectParallelism(String cipherText)
        throws IOException
    {
        super(cipherText);
    }

    @Override
    protected void doBruteForce()
    {
        int cpuCount = Runtime.getRuntime().availableProcessors();
        
        List<Thread> workerThreads = this.startEorkerThreads(cpuCount);
        
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

    private List<Thread> startEorkerThreads(int workerCount)
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
                Grille grille;
                while ((grille = grilleInterval.getNext()) != null)
                {
                    applyGrille(grille);
                }
            });
            workerThreads.add(producerThread);
            producerThread.start();
            
            nextIntervalBegin += intervalLength;
        }
        
        return workerThreads;
    }
}
