#ifndef DFA_HPP
#define DFA_HPP

#include <iostream>
#include <vector>
#include <map>

#include <gecode/int.hh>

#include "dictionary.hpp"

const int DFA_MIN_SYMBOL = 'a';
const int DFA_MAX_SYMBOL = 'z'+1;

struct DictionaryTransition
{
    DictionaryTransition(int n, int s):
        nodeFrom(n),
        symbol(s)
    {
    }

    int nodeFrom;
    int symbol;
};

struct Graph
{
public:
    Graph():
        latestState(0)
    {
    }

    Gecode::DFA *ToGecodeAlloc() const
    {
        Gecode::DFA::Transition *t = new Gecode::DFA::Transition[transitions.size() + 1];
        int *f = new int[finalStates.size() + 1];

        size_t i = 0;
        for(const auto it: transitions)
        {
            t[i] = Gecode::DFA::Transition(it.first.nodeFrom, it.first.symbol, it.second);
            ++i;
        }
        t[i] = {-1, 0, 0};

        i = 0;
        for(int state : finalStates)
        {
            f[i] = state;
            ++i;
        }
        f[i] = -1;

        Gecode::DFA *ret = new Gecode::DFA(0, t, f, false);

        delete [] t;
        delete [] f;

        return ret;
    }

    void MakeBorder(const Dictionary &dict, size_t length)
    {
        const auto &words = dict.GetCollection(length);
        int baseindex = dict.FirstIndexOfLength(length);
        size_t i = 0;
        for(const auto &word: words)
        {
            // Initial state is always 0
            int state = addWord(word, 0);
            int wordIndex = baseindex + (i++);
            int finalState = tryTransitionOrCreate(state, wordIndex);
            makeStateFinal(finalState);
        }
    }

    void MakeFirst(const Dictionary &dict, size_t maxlength)
    {
        // Pos phase (first => pos == 0 || pos == 2)
        int pos0state = tryTransitionOrCreate(0, 0);
        int pos2state = tryTransitionOrCreate(0, 2);

        // Transitions from pos2state to pos1state for all symbols except black tile
        int pos1state = tryTransitionOrCreate(pos2state, DFA_MIN_SYMBOL);
        for(int c = DFA_MIN_SYMBOL+1; c < DFA_MAX_SYMBOL; ++c)
            transitions.insert(std::make_pair(DictionaryTransition{pos2state, c}, pos1state));
        // Transition: pos1state -- black tile --> pos0state
        transitions.insert(std::make_pair(DictionaryTransition{pos1state, DFA_MAX_SYMBOL}, pos0state));

        // Letter phase
        for(size_t length = 2; length <= maxlength; ++length)
        {
            //if(length == maxlength-1)
            //    continue;

            const auto &words = dict.GetCollection(length);
            int baseindex = dict.FirstIndexOfLength(length);
            size_t i = 0;
            for(const auto &word: words)
            {
                int state = addWord(word, pos0state); // Start word at pos 0
                int wordIndex = baseindex + (i++);

                int indexState = tryTransitionOrCreate(state, wordIndex);
                makeStateFinal(tryTransitionOrCreate(indexState, length));

                if(length < maxlength)
                {
                    state = tryTransitionOrCreate(state, DFA_MAX_SYMBOL);
                    for(int c = DFA_MIN_SYMBOL; c <= DFA_MAX_SYMBOL; ++c)
                        transitions.insert(std::make_pair(DictionaryTransition{state, c}, state));
                    transitions.insert(std::make_pair(DictionaryTransition{state, wordIndex}, indexState));
                }
            }
        }
    }

    void MakeSecond(const Dictionary &dict, size_t maxlength)
    {
        // Pos phase (pos in [3, maxlength-2])
        for(int pos = 3; pos <= (int)maxlength-2; ++pos)
        {
            tryTransitionOrCreate(0, pos);
            if(pos > 4)
            {
                for(int c = DFA_MIN_SYMBOL; c <= DFA_MAX_SYMBOL; ++c)
                    transitions.insert(std::make_pair(DictionaryTransition{pos-2, c}, pos-3));
            }
        }

        transitions.insert(std::make_pair(DictionaryTransition{2, DFA_MAX_SYMBOL}, 1));

        // Add bypass when there is no second word
        int state = tryTransitionOrCreate(0, maxlength);
        transitions.insert(std::make_pair(DictionaryTransition{0, (int)maxlength-1}, state));
        transitions.insert(std::make_pair(DictionaryTransition{0, (int)maxlength+1}, state));
        for(int c = DFA_MIN_SYMBOL; c <= DFA_MAX_SYMBOL; ++c)
            transitions.insert(std::make_pair(DictionaryTransition{state, c}, state));
        makeStateFinal(tryTransitionOrCreate(state, MIN_INDEX)); // MIN_INDEX = no word

        int startState = 1;
        
        // Letter phase (length in [2, maxlength-3])
        for(size_t length = 2; length <= maxlength-3; ++length)
        {
            const auto &words = dict.GetCollection(length);
            int baseindex = dict.FirstIndexOfLength(length);
            size_t i = 0;
            for(const auto &word: words)
            {
                int state = addWord(word, startState); // Start word after startState
                int wordIndex = baseindex + (i++);

                int finalState = tryTransitionOrCreate(state, wordIndex);
                makeStateFinal(finalState);

                state = tryTransitionOrCreate(state, DFA_MAX_SYMBOL);
                int lastLetter = createState();
                for(int c = DFA_MIN_SYMBOL; c < DFA_MAX_SYMBOL; ++c)
                    transitions.insert(std::make_pair(DictionaryTransition{state, c}, lastLetter));
                transitions.insert(std::make_pair(DictionaryTransition{lastLetter, wordIndex}, finalState));
            }
        }
    }

    void MakeNoIndex(const Dictionary &dict, size_t maxlength)
    {
        int bridge = createState();
        for(int c = DFA_MIN_SYMBOL; c < DFA_MAX_SYMBOL; ++c)
        {
            int state = tryTransitionOrCreate(0, c);
            makeStateFinal(state);
            transitions.insert(std::make_pair(DictionaryTransition{bridge, c}, state));
        }

        for(size_t length = 2; length <= maxlength; ++length)
        {
            const auto &words = dict.GetCollection(length);
            for(const auto &word: words)
            {
                int state = addWord(word, 0);
                makeStateFinal(state);
                transitions.insert(std::make_pair(DictionaryTransition{state, DFA_MAX_SYMBOL}, bridge));
            }
        }
    }

    void MakeSingleWord(const std::string &word)
    {
        int begdontcare = createState();
        int begblacktile = tryTransitionOrCreate(begdontcare, DFA_MAX_SYMBOL);
        for(int c = DFA_MIN_SYMBOL; c < DFA_MAX_SYMBOL; ++c)
            if(c != word[0])
            {
                transitions.insert(std::make_pair(DictionaryTransition{0, c}, begdontcare));
                transitions.insert(std::make_pair(DictionaryTransition{begdontcare, c}, begdontcare));
                transitions.insert(std::make_pair(DictionaryTransition{begblacktile, c}, begdontcare));
            }

        int tmp = tryTransitionOrCreate(0, word[0]);
        transitions.insert(std::make_pair(DictionaryTransition{begblacktile, word[0]}, tmp));
        for(size_t i = 1; i < word.size(); ++i)
        {
            for(int c = DFA_MIN_SYMBOL; c < DFA_MAX_SYMBOL; ++c)
                if(c != word[i])
                    transitions.insert(std::make_pair(DictionaryTransition{tmp, c}, begdontcare));

            tmp = tryTransitionOrCreate(tmp, word[i]);
        }

        for(int c = DFA_MIN_SYMBOL; c < DFA_MAX_SYMBOL; ++c)
            transitions.insert(std::make_pair(DictionaryTransition{tmp, c}, begdontcare));

        tmp = tryTransitionOrCreate(tmp, DFA_MAX_SYMBOL);
        makeStateFinal(tmp);

        for(int c = DFA_MIN_SYMBOL; c <= DFA_MAX_SYMBOL; ++c)
            transitions.insert(std::make_pair(DictionaryTransition{tmp, c}, tmp));
    }

private:
    int createState()
    {
        // Don't change this!
        return ++latestState;
    }

    // make_state_final
    // Must be called after having called createDontCareLoop
    void makeStateFinal(int state)
    {
        finalStates.push_back(state);
    }

    int tryTransitionOrCreate(int stateFrom, int symbol)
    {
        DictionaryTransition transition{stateFrom, symbol};
        const auto it = transitions.find(transition);
        if(it != transitions.cend()) // Transition exists
        {
            return it->second;
        }

        // else
        int newState = createState();
        transitions.insert(std::make_pair(transition, newState));
        return newState;
    }

    int addWord(const std::string &word, int initialState)
    {
        int currentState = initialState;

        // Explore DFA with the sequence of characters in word
        // Create the missing states and transitions when required
        for(char c: word)
            currentState = tryTransitionOrCreate(currentState, c);

        return currentState;
    }

    std::vector<int> finalStates;
    std::map<DictionaryTransition, int> transitions;
    int latestState;
};

bool operator<(const DictionaryTransition &a, const DictionaryTransition &b)
{
    return a.nodeFrom < b.nodeFrom
        || (a.nodeFrom == b.nodeFrom && a.symbol < b.symbol);
}

class DictionaryDFA
{
    public:
        DictionaryDFA(const Dictionary &dict, size_t width, size_t height):
            dictionary(dict)
        {
            std::cout << "DFA initialization..." << std::endl;

            // Border H
            borderH.MakeBorder(dict, width);

            // Border V
            borderV.MakeBorder(dict, height);

            // First H/V
            firstH.MakeFirst(dict, width);
            firstV.MakeFirst(dict, height);
            
            // Second H/V
            secondH.MakeSecond(dict, width);
            secondV.MakeSecond(dict, height);

            noIndex.MakeNoIndex(dict, width > height ? width : height);

            std::cout << "DFA initialized!" << std::endl;
        }

        Gecode::DFA *BorderH() const
        {
            return borderH.ToGecodeAlloc();
        }

        Gecode::DFA *BorderV() const
        {
            return borderV.ToGecodeAlloc();
        }

        Gecode::DFA *FirstH() const
        {
            return firstH.ToGecodeAlloc();
        }

        Gecode::DFA *FirstV() const
        {
            return firstV.ToGecodeAlloc();
        }

        Gecode::DFA *SecondH() const
        {
            return secondH.ToGecodeAlloc();
        }

        Gecode::DFA *SecondV() const
        {
            return secondV.ToGecodeAlloc();
        }

        Gecode::DFA *NoIndex() const
        {
            return noIndex.ToGecodeAlloc();
        }

        static Gecode::DFA *Mandatory(const std::string &word)
        {
            Graph graph;
            graph.MakeSingleWord(word);
            return graph.ToGecodeAlloc();
        }

    protected:

        const Dictionary &dictionary;

        Graph borderH;
        Graph borderV;
        Graph firstH;
        Graph firstV;
        Graph secondH;
        Graph secondV;

        Graph noIndex;
};

#endif

