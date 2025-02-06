#include "WordsTrie.hpp"

#include <string>
#include <fstream>
#include <array>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/regex.hpp>


namespace org::voidland::concurrent::turning_grille
{

class TrieNode
{
private:
    TrieNode* parent;
    char c;

    std::array<std::unique_ptr<TrieNode>, ('Z' - 'A' + 1)> children;
    bool wordEnd;

public:
    TrieNode(char c, TrieNode* parent);

    TrieNode& getOrCreateChild(char c);
    const TrieNode* getChild(char c) const;

    bool isWordEnd() const;
    void setWordEnd();

    std::string getWord() const;
};


WordsTrie::WordsTrie(const std::string& wordsFilePath) :
    root(new TrieNode('\0', nullptr))
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

        const static boost::regex nonLettersRe("[^A-Z]");
        boost::erase_all_regex(word, nonLettersRe);

        if (word.length() >= 3) // Short words significantly increase the count of false positives.
        {
            this->addWord(*this->root, word.c_str());
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
    this->addWord(child, c+1);
}

unsigned WordsTrie::countWords(const std::string& text)
{
    thread_local std::vector<const TrieNode*> iterators;
    iterators.assign(text.length() + 1, nullptr);
    iterators.front() = this->root.get();
    return this->countWords(text.c_str(), iterators);
}

unsigned WordsTrie::countWords(const char* c, std::vector<const TrieNode*>& iterators)
{
    if (*c == '\0')
    {
        return 0;
    }

    unsigned words = 0;

    std::vector<const TrieNode*>::iterator i = iterators.begin();
    std::vector<const TrieNode*>::iterator j = iterators.begin();

    while (*i != nullptr)
    {
        const TrieNode* nextIterator = (*i)->getChild(*c);
        *i = nullptr;
        i++;

        if (!nextIterator)
        {
            continue;
        }

        if (nextIterator->isWordEnd())
        {
            words++;
        }

        *j = nextIterator;
        j++;
    }
    *j = this->root.get();

    return words + this->countWords(c+1, iterators);
}


TrieNode::TrieNode(char c, TrieNode* parent) :
    parent(parent),
    c(c),
    wordEnd(false)
{
}

TrieNode& TrieNode::getOrCreateChild(char c)
{
    int i = c - 'A';
    std::unique_ptr<TrieNode>& charTrieNode = this->children.at(i);

    if (!charTrieNode)
    {
        charTrieNode.reset(new TrieNode(c, this));
    }

    return *charTrieNode;
}

const TrieNode* TrieNode::getChild(char c) const
{
    int i = c - 'A';
    return this->children[i].get();
}

bool TrieNode::isWordEnd() const
{
    return this->wordEnd;
}

void TrieNode::setWordEnd()
{
    this->wordEnd = true;
}

std::string TrieNode::getWord() const
{
    std::string sb;
    const TrieNode* n = this;
    while (n->c != '\0')
    {
        sb.insert(sb.begin(), n->c);
        n = n->parent;
    }
    return sb;
}

}
