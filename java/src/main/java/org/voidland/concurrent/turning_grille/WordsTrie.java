package org.voidland.concurrent.turning_grille;

import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;
import java.util.Locale;


public class WordsTrie
{
	private TrieNode root;

	
	public WordsTrie(String wordsFilePath)
		throws IOException
	{
		this.root = new TrieNode('\0', null);
		this.loadWords(wordsFilePath);
	}
	
    private void loadWords(String wordsFilePath)
            throws IOException
    {
        try (BufferedReader wordsReader = new BufferedReader(new FileReader(wordsFilePath)))
        {
            String word;
            while ((word = wordsReader.readLine()) != null)
            {
                word = word.toUpperCase(Locale.ENGLISH).replaceAll("[^A-Z]", "");
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
    
    public int countWords(CharSequence text)
    {
        TrieNode[] iterators = new TrieNode[text.length() + 1];
        iterators[0] = this.root;
        for (int i = 1; i < iterators.length; i++)
        {
            iterators[i] = null;
        }
        
    	return this.countWords(text, 0, iterators);
    }
    
    private int countWords(CharSequence text, int charIndex, TrieNode[] iterators)
    {
    	if (charIndex >= text.length())
    	{
    		return 0;
    	}
    	
    	int words = 0;
    	char c = text.charAt(charIndex);
    	
        int i = 0;
        int j = 0;
    	
  		while (iterators[i] != null)
    	{
    		TrieNode nextIterator = iterators[i].getChild(c);
    		iterators[i] = null;
    		i++;
    		
    		if (nextIterator == null)
    		{
    			continue;
    		}
    		
    		if (nextIterator.isWordEnd())
    		{
    			words++;
    		}
    		
    		iterators[j] = nextIterator;
    		j++;
    	}
	    iterators[j] = this.root;
    	
    	return words + this.countWords(text, charIndex + 1, iterators);
    }
	

    private static class TrieNode
	{
    	private TrieNode parent;
    	private char c;
    	
		private TrieNode[] children;
		private boolean wordEnd;
		
		public TrieNode(char c, TrieNode parent)
		{
			this.c = c;
			this.parent = parent;
			
			this.children = new TrieNode['Z' - 'A' + 1];
			for (int i = 0; i < this.children.length; i++)
			{
				this.children[i] = null;
			}
			
			this.wordEnd = false;
		}
		
		public TrieNode getOrCreateChild(char c)
		{
			int i = c - 'A';
			if (this.children[i] == null)
			{
				this.children[i] = new TrieNode(c, this);
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
		
		@SuppressWarnings("unused")
        public String getWord()
		{
			StringBuilder sb = new StringBuilder();
			TrieNode n = this;
			while (n.c != '\0')
			{
				sb.insert(0, n.c);
				n = n.parent;
			}
			return sb.toString();
		}
	}
}
