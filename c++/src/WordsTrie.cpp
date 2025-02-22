#include "WordsTrie.hpp"
#include "TurningGrilleCracker.hpp"

#include <fstream>
#include <boost/algorithm/string.hpp>


namespace org::voidland::concurrent::turning_grille
{


WordsTrie::WordsTrie(const std::string& wordsFilePath) :
    root(new TrieNode())
{
    this->loadWords(wordsFilePath);
}

WordsTrie::~WordsTrie()
{
}

void WordsTrie::loadWords(const std::string& wordsFilePath)
{
    std::ifstream wordsStream(wordsFilePath);
    if (wordsStream.fail())
    {
        throw std::ios::failure("Cannot open file: " + wordsFilePath);
    }

    for (std::string word; std::getline(wordsStream, word); )
    {
        boost::to_upper(word);
        boost::erase_all_regex(word, NOT_ENGLISH_LETTERS_RE);

        if (word.length() >= 3) // Short words significantly increase the count of false positives.
        {
        	WordsTrie::addWord(*this->root, word.c_str());
        }
    }
}

void WordsTrie::addWord(TrieNode& parent, const char* c)
{
    if (*c == '\0')
    {
        parent.setWordEnd();
        return;
    }
    TrieNode& child = parent.getOrCreateChild(*c);
    WordsTrie::addWord(child, c + 1);
}


}
