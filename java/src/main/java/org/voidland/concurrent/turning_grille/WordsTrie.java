package org.voidland.concurrent.turning_grille;

import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;
import java.util.Locale;


public class WordsTrie
{
    private static class TrieNode
	{
		private TrieNode[] children;
		private boolean wordEnd;
		
		
		public TrieNode()
		{
			this.children = new TrieNode['Z' - 'A' + 1];
			this.wordEnd = false;
		}
		
		public TrieNode getOrCreateChild(char c)
		{
			int i = c - 'A';
			if (this.children[i] == null)
			{
				this.children[i] = new TrieNode();
			}
			return this.children[i];
		}
		
		public TrieNode getChild(char c)
		{
			int i = c - 'A';
			return this.children[i];
		}

		public boolean isWordEnd()
		{
			return this.wordEnd;
		}
		
		public void setWordEnd()
		{
			this.wordEnd = true;
		}
	}

    
    private TrieNode root;

	
	public WordsTrie(String wordsFilePath)
		throws IOException
	{
		this.root = new TrieNode();
		this.loadWords(wordsFilePath);
	}
	
    public int countWords(CharSequence text)
    {
        TrieNode[] iterators = new TrieNode[text.length() + 1];
        iterators[0] = this.root;
        
        int words = 0;
        for (int charIndex = 0; charIndex < text.length(); charIndex++)
        {
        	char c = text.charAt(charIndex);
            int i = 0;
            int j = 0;
            
            TrieNode currentNode;
      		while ((currentNode = iterators[i]) != null)
        	{
        		TrieNode nextIterator = currentNode.getChild(c);
        		iterators[i] = null;
        		++i;
        		
        		if (nextIterator == null)
        		{
        			continue;
        		}
        		
        		if (nextIterator.isWordEnd())
        		{
        			words++;
        		}
        		
        		iterators[j] = nextIterator;
        		++j;
        	}
    	    iterators[j] = this.root;
        }
    	return words;
    }

    private void loadWords(String wordsFilePath)
            throws IOException
    {
        try (BufferedReader wordsReader = new BufferedReader(new FileReader(wordsFilePath)))
        {
            String word;
            while ((word = wordsReader.readLine()) != null)
            {
                word = word.toUpperCase(Locale.ENGLISH);
                word = TurningGrilleCracker.NOT_ENGLISH_LETTERS_RE.matcher(word).replaceAll("");
                
                if (word.length() >= 3)	// Short words significantly increase the count of false positives.
                {
                	WordsTrie.addWord(this.root, word, 0);
                }
            }
        }
    }
    
    private static void addWord(TrieNode parent, CharSequence word, int charIndex)
    {
    	if (charIndex >= word.length())
    	{
    		parent.setWordEnd();
    		return;
    	}
    	TrieNode child = parent.getOrCreateChild(word.charAt(charIndex));
    	WordsTrie.addWord(child, word, charIndex + 1);
    }
}
