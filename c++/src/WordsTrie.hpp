#ifndef __VOIDLAND_WORDS_TRIE_HPP__
#define __VOIDLAND_WORDS_TRIE_HPP__

#include <memory>
#include <string>
#include <vector>


namespace org::voidland::concurrent
{

class TrieNode;

class WordsTrie
{
private:
    std::unique_ptr<TrieNode> root;

public:
    WordsTrie(const std::string& wordsFilePath);
    ~WordsTrie();

    unsigned countWords(const std::string& text);

private:
    void loadWords(const std::string& wordsFilePath);
    void addWord(TrieNode& parent, const char* c);

    unsigned countWords(const char* c, std::vector<const TrieNode*>& iterators);
};

}

#endif
