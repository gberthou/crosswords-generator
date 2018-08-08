#ifndef DICTIONARY_HPP
#define DICTIONARY_HPP

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <set>
#include <regex>

const size_t MIN_INDEX = 256;

class Dictionary
{
    public:
        Dictionary(const std::string &filename, size_t maxlen)
        {
            std::ifstream file(filename);
            std::string line;

            collections.resize(maxlen-1);

            while(std::getline(file, line))
            {
                if(line.size() >= 2 && line.size() <= maxlen)
                    collections[line.size()-2].insert(line);
            }
        }

        void AddMandatoryWords(const std::string &filename, size_t maxlen, std::set<int> &indices)
        {
            std::vector<std::string> tosearch;

            std::ifstream file(filename);
            std::string line;

            while(std::getline(file, line))
            {
                if(line.size() >= 2 && line.size() <= maxlen)
                {
                    collections[line.size()-2].insert(line);
                    tosearch.push_back(line);
                }
            }

            indices.clear();
            for(const auto &s : tosearch)
                indices.insert(IndexOfWord(s));
        }

        int FirstIndexOfLength(size_t length) const
        {
            size_t index = MIN_INDEX+1;
            length -= 2;
            for(size_t i = 0; i < length; ++i)
                index += collections[i].size();
            return index;
        }
        
        int LastIndexOfLength(size_t length) const
        {
            return FirstIndexOfLength(length) + collections[length-2].size()-1;
        }

        const std::set<std::string> &GetCollection(size_t length) const
        {
            return collections[length-2];
        }

        // Assumes that word does exist
        int IndexOfWord(const std::string &word) const
        {
            int baseIndex = FirstIndexOfLength(word.size());
            const auto &collection = collections[word.size()-2];
            size_t i = 0;

            for(std::set<std::string>::const_iterator it = collection.cbegin(); it != collection.cend() && *it != word; ++it, ++i);

            return baseIndex + i;
        }

        void NonMatchingIndices(const std::string &s, std::vector<size_t> &ret) const
        {
            size_t index = MIN_INDEX;
            std::regex re(s, std::regex::extended);

            ret.clear();

            for(const auto &collection : collections)
                for(const auto &word : collection)
                {
                    ++index;

                    std::smatch m;
                    if(!std::regex_match(word, m, re))
                        ret.push_back(index);
                }
        }

        void TestRegex() const
        {
            try
            {
                std::regex re(".*a", std::regex::extended);
                for(const auto &col : collections)
                {
                    for(const auto &str : col)
                    {
                        std::smatch m;
                        if(std::regex_match(str, m, re))
                            std::cout << m[0] << std::endl;
                    }
                }
            }
            catch(const std::regex_error &e)
            {
                std::cout << e.what() << std::endl;
            }
        }

    protected:
        std::vector<std::set<std::string> > collections;
};

#endif

