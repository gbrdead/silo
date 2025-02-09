package org.voidland.concurrent;

import java.io.BufferedReader;
import java.io.FileReader;
import java.time.Duration;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;

import org.voidland.concurrent.queue.BlockingPortionQueue;
import org.voidland.concurrent.queue.ConcurrentNonBlockingQueue;
import org.voidland.concurrent.queue.MostlyNonBlockingPortionQueue;
import org.voidland.concurrent.queue.TextbookPortionQueue;
import org.voidland.concurrent.turning_grille.Grille;
import org.voidland.concurrent.turning_grille.TurningGrilleCracker;
import org.voidland.concurrent.turning_grille.TurningGrilleCrackerProducerConsumer;
import org.voidland.concurrent.turning_grille.TurningGrilleCrackerSyncless;


public class Silo
{
    public static boolean VERBOSE = "true".equalsIgnoreCase(System.getenv("VERBOSE"));
    
    
    private static final String CIPHER_TEXT_PATH = "encrypted_msg.txt";
    
    
    private static volatile boolean stop;
    
    private static void heatCpu()
    {
        try
        {
            int cpuCount = Runtime.getRuntime().availableProcessors();
            List<Thread> workerThreads = new ArrayList<>(cpuCount);
    
            Silo.stop = false;
    
            for (int i = 0; i < cpuCount; i++)
            {
                Thread workerThread = new Thread(() ->
                {
                    while (!Silo.stop);
                });
                workerThread.start();
                workerThreads.add(workerThread);
            }
    
            Thread.sleep(Duration.ofMinutes(1).toMillis());
            Silo.stop = true;
    
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

    public static void main(String[] args)
    {
        try
        {
            String cipherText;
            try (BufferedReader cipherTextReader = new BufferedReader(new FileReader(Silo.CIPHER_TEXT_PATH)))
            {
                cipherText = cipherTextReader.readLine();
            }
            
            String arg;
            if (args.length >= 1)
            {
                arg = args[0];
                arg = arg.toLowerCase(Locale.ENGLISH);
            }
            else
            {
                arg = "syncless";
            }

            TurningGrilleCracker cracker;
            
            int cpuCount = Runtime.getRuntime().availableProcessors();
            switch (arg)
            {
                case "concurrent":
                {
                    int initialConsumerCount = cpuCount * 3;
                    int producerCount = cpuCount;
                    
                    cracker = new TurningGrilleCrackerProducerConsumer(cipherText, initialConsumerCount, producerCount,
                    		new MostlyNonBlockingPortionQueue<Grille>(initialConsumerCount, producerCount, new ConcurrentNonBlockingQueue<Grille>()));
                    break;
                }
                case "blocking":
                {
                    int initialConsumerCount = cpuCount * 3;
                    int producerCount = cpuCount;
                    
                    cracker = new TurningGrilleCrackerProducerConsumer(cipherText, initialConsumerCount, producerCount,
                    		new BlockingPortionQueue<Grille>(initialConsumerCount, producerCount));
                    break;
                }
                case "textbook":
                {
                    int initialConsumerCount = cpuCount * 3;
                    int producerCount = cpuCount;
                    
                    cracker = new TurningGrilleCrackerProducerConsumer(cipherText, initialConsumerCount, producerCount,
                    		new TextbookPortionQueue<Grille>(initialConsumerCount, producerCount));
                    break;
                }
                case "syncless":
                {
                    cracker = new TurningGrilleCrackerSyncless(cipherText);
                    break;
                }
                default:
                {
                    throw new IllegalArgumentException("Unexpected argument: " + arg);
                }
            }
            
            Silo.heatCpu();
            cracker.bruteForce();
        }
        catch (Exception e)
        {
            e.printStackTrace();
        }
    }
}
