use std::string::String;
use std::fs::read_to_string;
use regex::Regex;
use core::str::Chars;


const NUMBER_OF_LETTERS: usize = ('Z' as usize) - ('A' as usize) + 1;

struct TrieNode
{
    children: [Option<Box<Self>>; NUMBER_OF_LETTERS],
    wordEnd: bool
}

impl TrieNode
{
    #[inline]
    pub fn new() -> Self
    {
        Self
        {
            children: [const { None }; NUMBER_OF_LETTERS],
            wordEnd: false
        }
    }
    
    #[inline]
    pub fn getOrCreateChild<'a>(&'a mut self, c: char) -> &'a mut Self
    {
        let i: usize = (c as usize) - ('A' as usize);
        self.children[i].get_or_insert_with(|| Box::new(TrieNode::new()))
    }
    
    #[inline]
    pub fn getChild<'a>(&'a self, c: char) -> Option<&'a Self>
    {
        let i: usize = (c as usize) - ('A' as usize);
        let child: &Option<Box<Self>> = &self.children[i];
        match child
        {
            Some(realChild) => Some(realChild.as_ref()),
            None => None
        }
    }
    
    #[inline]
    pub fn isWordEnd(&self) -> bool
    {
        self.wordEnd
    }
    
    #[inline]
    pub fn setWordEnd(&mut self)
    {
        self.wordEnd = true;
    }
}


pub struct WordsTrie
{
	root: TrieNode
}

impl WordsTrie
{
    pub fn new(wordsFilePath: &str) -> Self
	{
		let mut ret: Self = Self
		{
			root: TrieNode::new()
		};
		ret.loadWords(wordsFilePath);
		ret
	}
	
	fn loadWords(&mut self, wordsFilePath: &str)
	{
		for line in read_to_string(wordsFilePath).unwrap().lines()
		{
			let word: String = line.to_uppercase();
			let nonLettersRe = Regex::new(r"[^A-Z]").unwrap();
			let word = nonLettersRe.replace_all(&word, "");
			if word.chars().count() >= 3
			{
				Self::addWord(&mut self.root, word.chars());
			}
		}
	}
	
	fn addWord(parent: &mut TrieNode, mut ci: Chars)
	{
		let c: Option<char> = ci.next();
		if c.is_none() 
		{
			parent.setWordEnd();
			return;
		}
		let child: &mut TrieNode = parent.getOrCreateChild(c.unwrap());
		Self::addWord(child, ci);
	}
	
    #[inline]
	pub fn countWords(&self, text: &str) -> usize
	{
		let mut iterators: Vec<Option<&TrieNode>> = vec![None; text.len() + 1];
		iterators[0] = Some(&self.root);
        
        let mut words: usize = 0;
        for c in text.chars()
        {
    		let mut i: usize = 0;
    		let mut j: usize = 0;
            
            loop
            {
                let nextIterator: Option<&TrieNode>;
                match iterators[i]
                {
                    None =>
                    {
                        break;
                    }
                    Some(currentNode) =>
                    {
                       nextIterator = currentNode.getChild(c);
                    }
                }
                iterators[i] = None;
                i += 1;

                match nextIterator
                {
                    None =>
                    {
                        continue;
                    }
                    Some(nextNode) =>
                    {
                        if nextNode.isWordEnd()
                        {
                            words += 1;
                        }
                    }
                }
    			iterators[j] = nextIterator;
    			j += 1;
    		}
    		iterators[j] = Some(&self.root);
        }
		
		words
	}
}
