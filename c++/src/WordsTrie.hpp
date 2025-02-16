#ifndef __VOIDLAND_WORDS_TRIE_HPP__
#define __VOIDLAND_WORDS_TRIE_HPP__

#include <memory>
#include <string>
#include <vector>
#include <array>



namespace org::voidland::concurrent::turning_grille
{


class TrieNode
{
private:
	std::array<std::unique_ptr<TrieNode>, ('Z' - 'A' + 1)> children;
	bool wordEnd;

public:
	TrieNode();

	TrieNode& getOrCreateChild(char c);
	const TrieNode* getChild(char c) const;

	bool isWordEnd() const;
	void setWordEnd();
};

inline TrieNode::TrieNode() :
    wordEnd(false)
{
}

inline TrieNode& TrieNode::getOrCreateChild(char c)
{
    int i = c - 'A';
    std::unique_ptr<TrieNode>& charTrieNode = this->children.at(i);

    if (!charTrieNode)
    {
        charTrieNode = std::make_unique<TrieNode>();
    }

    return *charTrieNode;
}

inline const TrieNode* TrieNode::getChild(char c) const
{
    int i = c - 'A';
    return this->children[i].get();
}

inline bool TrieNode::isWordEnd() const
{
    return this->wordEnd;
}

inline void TrieNode::setWordEnd()
{
    this->wordEnd = true;
}


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
    static void addWord(TrieNode& parent, const char* c);

    unsigned countWords(const char* c, std::vector<const TrieNode*>& iterators);
};

inline unsigned WordsTrie::countWords(const std::string& text)
{
    std::vector<const TrieNode*> iterators(text.length() + 1, nullptr);
    iterators.front() = this->root.get();
    return this->countWords(text.c_str(), iterators);
}

inline unsigned WordsTrie::countWords(const char* c, std::vector<const TrieNode*>& iterators)
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


}

#endif
