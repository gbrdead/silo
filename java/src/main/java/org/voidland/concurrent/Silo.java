package org.voidland.concurrent;

import java.io.BufferedReader;
import java.io.FileReader;
import java.time.Duration;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;


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
                arg = "perfect";
            }

            TurningGrilleCracker cracker;
            
            int cpuCount = Runtime.getRuntime().availableProcessors();
            switch (arg)
            {
                case "concurrent":
                {
                    int initialConsumerCount = cpuCount * 2;
                    int producerCount = cpuCount;
                    
                    MPMC_PortionQueue<Grille> portionQueue = new ConcurrentPortionQueue<Grille>(initialConsumerCount, producerCount);
                    cracker = new TurningGrilleCrackerProducerConsumer(cipherText, initialConsumerCount, producerCount, portionQueue);
                    break;
                }
                case "blocking":
                {
                    int initialConsumerCount = cpuCount * 6;
                    int producerCount = cpuCount;
                    
                    MPMC_PortionQueue<Grille> portionQueue = new BlockingPortionQueue<Grille>(initialConsumerCount, producerCount);
                    cracker = new TurningGrilleCrackerProducerConsumer(cipherText, initialConsumerCount, producerCount, portionQueue);
                    break;
                }
                case "textbook":
                {
                    int initialConsumerCount = cpuCount * 5;
                    int producerCount = cpuCount;
                    
                    MPMC_PortionQueue<Grille> portionQueue = new TextbookPortionQueue<Grille>(initialConsumerCount, producerCount);
                    cracker = new TurningGrilleCrackerProducerConsumer(cipherText, initialConsumerCount, producerCount, portionQueue);
                    break;
                }
                case "perfect":
                {
                    cracker = new TurningGrilleCrackerWithPerfectParallelism(cipherText);
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
