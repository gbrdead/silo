package org.voidland.concurrent;

import java.io.BufferedReader;
import java.io.FileReader;
import java.time.Duration;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;
import java.util.NoSuchElementException;
import java.util.SortedSet;
import java.util.concurrent.atomic.AtomicBoolean;

import org.voidland.concurrent.queue.BlockingPortionQueue;
import org.voidland.concurrent.queue.MPMC_PortionQueue;
import org.voidland.concurrent.queue.MostlyNonBlockingPortionQueue;
import org.voidland.concurrent.queue.TextbookPortionQueue;
import org.voidland.concurrent.turning_grille.Grille;
import org.voidland.concurrent.turning_grille.TurningGrilleCracker;
import org.voidland.concurrent.turning_grille.TurningGrilleCrackerImplDetails;
import org.voidland.concurrent.turning_grille.TurningGrilleCrackerProducerConsumer;
import org.voidland.concurrent.turning_grille.TurningGrilleCrackerSerial;
import org.voidland.concurrent.turning_grille.TurningGrilleCrackerSyncless;


public class Silo
{
    public static boolean VERBOSE = "true".equalsIgnoreCase(System.getenv("VERBOSE"));
    
    
    private static final String CIPHER_TEXT_PATH = "encrypted_msg.txt";
    private static final String CLEAR_TEXT_PATH = "decrypted_msg.txt";
    
    
    private static void heatCpu()
    {
        try
        {
            int cpuCount = Runtime.getRuntime().availableProcessors();
            List<Thread> workerThreads = new ArrayList<>(cpuCount);
    
            AtomicBoolean stop = new AtomicBoolean(false);
    
            for (int i = 0; i < cpuCount; i++)
            {
                Thread workerThread = new Thread(() ->
                {
                    while (!stop.get());
                });
                workerThread.start();
                workerThreads.add(workerThread);
            }
    
            Thread.sleep(Duration.ofMinutes(1).toMillis());
            stop.set(true);
    
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
            String clearText;
            try (BufferedReader clearTextReader = new BufferedReader(new FileReader(Silo.CLEAR_TEXT_PATH)))
            {
                clearText = clearTextReader.readLine();
            }
            clearText = clearText.toUpperCase(Locale.ENGLISH);
            clearText = TurningGrilleCracker.NOT_CAPITAL_ENGLISH_LETTERS_RE.matcher(clearText).replaceAll("");
            
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
            
            TurningGrilleCrackerImplDetails crackerImplDetails;
            
            int cpuCount = Runtime.getRuntime().availableProcessors();
            switch (arg)
            {
                case "concurrent":
                {
                    int initialConsumerCount = cpuCount * 3;
                    int producerCount = cpuCount;
                    int maxQueueSize = initialConsumerCount * producerCount * 1000;
                    
                    MPMC_PortionQueue<Grille> portionQueue = MostlyNonBlockingPortionQueue.<Grille>createConcurrentBlownQueue(maxQueueSize);
                    crackerImplDetails = new TurningGrilleCrackerProducerConsumer(initialConsumerCount, producerCount, portionQueue);
                    break;
                }
                case "blocking":
                {
                    int initialConsumerCount = cpuCount * 3;
                    int producerCount = cpuCount;
                    int maxQueueSize = initialConsumerCount * producerCount * 1000;
                    
                    MPMC_PortionQueue<Grille> portionQueue = new BlockingPortionQueue<Grille>(maxQueueSize);
                    crackerImplDetails = new TurningGrilleCrackerProducerConsumer(initialConsumerCount, producerCount, portionQueue);
                    break;
                }
                case "textbook":
                {
                    int initialConsumerCount = cpuCount * 3;
                    int producerCount = cpuCount;
                    int maxQueueSize = initialConsumerCount * producerCount * 1000;
                    
                    MPMC_PortionQueue<Grille> portionQueue = new TextbookPortionQueue<Grille>(maxQueueSize);
                    crackerImplDetails = new TurningGrilleCrackerProducerConsumer(initialConsumerCount, producerCount, portionQueue);
                    break;
                }
                case "syncless":
                {
                	crackerImplDetails = new TurningGrilleCrackerSyncless();
                    break;
                }
                case "serial":
                {
                	crackerImplDetails = new TurningGrilleCrackerSerial();
                    break;
                }
                default:
                {
                    throw new IllegalArgumentException("Unexpected argument: " + arg);
                }
            }

            TurningGrilleCracker cracker = new TurningGrilleCracker(cipherText, crackerImplDetails);
            if (!Silo.VERBOSE)
            {
            	Silo.heatCpu();
            }
            SortedSet<String> candidates = cracker.bruteForce();
            
            if (!candidates.contains(clearText))
            {
            	throw new NoSuchElementException("The correct clear text was not found among the decrypted candidates.");
            }
        }
        catch (Exception e)
        {
            e.printStackTrace();
            System.exit(1);
        }
    }
}
