#ifndef PROP_REGEX_HPP
#define PROP_REGEX_HPP

#include <iostream>
#include <vector>
#include <set>
#include <string>
#include <sstream>
#include <regex>

#include <gecode/int.hh>

#include "dictionary.hpp"
#include "helper.hpp"

typedef std::vector<Gecode::Int::IntView> VIntView;

class PropRegex : public Gecode::Propagator
{
    public:
        PropRegex(Gecode::Space &home, const Dictionary &dict, size_t w, size_t h, const VIntView &avletters, const VIntView &avh, const VIntView &avv, const VIntView &avborders):
            Gecode::Propagator(home),
            dictionary(dict),
            width(w),
            height(h),
            vletters(avletters),
            vh(avh),
            vv(avv),
            vborders(avborders)
        {
            for(auto &vl : vletters)
                vl.subscribe(home, *this, Gecode::Int::PC_INT_VAL);
        }

        PropRegex(Gecode::Space &home, PropRegex &p):
            Gecode::Propagator(home, p),
            dictionary(p.dictionary),
            width(p.width),
            height(p.height),
            vletters(p.vletters.size()),
            vh(p.vh.size()),
            vv(p.vv.size()),
            vborders(p.vborders.size())
        {
            for(size_t i = 0; i < p.vletters.size(); ++i)
                vletters[i].update(home, p.vletters[i]);

            for(size_t i = 0; i < p.vh.size(); ++i)
                vh[i].update(home, p.vh[i]);

            for(size_t i = 0; i < p.vv.size(); ++i)
                vv[i].update(home, p.vv[i]);

            for(size_t i = 0; i < p.vborders.size(); ++i)
                vborders[i].update(home, p.vborders[i]);
        }

        virtual Gecode::ExecStatus propagate(Gecode::Space &home, const Gecode::ModEventDelta &)
        {
            // Subsumed iff all letters are assigned, i.e., never subsumed before a solution is found

            // Horizontal words
            for(size_t y = 1; y < height-1; ++y)
            {
                VIntView::const_iterator itBegin = vletters.cbegin() + (y*width);
                VIntView::const_iterator itEnd   = itBegin + width;

                if(!at_least_one_assigned(itBegin, itEnd, 1))
                    continue;

                std::string pattern = regex_first(itBegin, itEnd, 1);
                if(pattern.size())
                {
                    std::vector<size_t> indicesToKeep;
                    dictionary.MatchingIndices(vh[y-1], pattern, indicesToKeep);

                    // TODO: Add index 0 if second word and 0 belongs to domain

                    FakeView fv(indicesToKeep);
                    vh[y-1].narrow_v(home, fv);

                    std::cout << '[' << y << "] " << indicesToKeep.size() << std::endl;

                    /*
                    for(int i : indicesToRemove)
                    {
                        GECODE_ME_CHECK(vh[y-1].nq(home, i));
                    }
                    */
                }
            }

            // Vertical words
            
            // Borders

            (void) home;
            // TODO
            return Gecode::ES_FIX;
        }

        virtual size_t dispose(Gecode::Space &home)
        {
            for(auto &vl : vletters)
                vl.cancel(home, *this, Gecode::Int::PC_INT_VAL);
            (void) Gecode::Propagator::dispose(home);
            return sizeof(*this);
        }

        virtual Gecode::Propagator *copy(Gecode::Space &home)
        {
            return new (home) PropRegex(home, *this);
        }

        virtual Gecode::PropCost cost(const Gecode::Space &, const Gecode::ModEventDelta &) const
        {
            // TODO: dictionary.size()
            return Gecode::PropCost::linear(Gecode::PropCost::LO, 200000);
        }

        virtual void reschedule(Gecode::Space &home)
        {
            for(auto &vl : vletters)
                vl.reschedule(home, *this, Gecode::Int::PC_INT_VAL);
        }

        static Gecode::ExecStatus post(Gecode::Space &home, const Dictionary &dictionary, size_t width, size_t height, const Gecode::IntVarArgs &letters, const Gecode::IntVarArgs &horizontal, const Gecode::IntVarArgs &vertical, const Gecode::IntVarArgs &borders)
        {
            /* For now, assume that letters are not "same"
            if(letters.same())
                return Gecode::ES_FAILED;
            */

            VIntView vletters;
            for(auto &l : letters)
                vletters.push_back(l);

            VIntView vh;
            for(auto &h : horizontal)
                vh.push_back(h);

            VIntView vv;
            for(auto &v : vertical)
                vv.push_back(v);

            VIntView vborders;
            for(auto &b : borders)
                vborders.push_back(b);

            (void) new (home) PropRegex(home, dictionary, width, height, vletters, vh, vv, vborders);
            return Gecode::ES_OK;
        }

        static void propregex(Gecode::Space &home, const Dictionary &dictionary, size_t width, size_t height, const Gecode::IntVarArgs &letters, const Gecode::IntVarArgs &horizontal, const Gecode::IntVarArgs &vertical, const Gecode::IntVarArgs &borders)
        {
            if(PropRegex::post(home, dictionary, width, height, letters, horizontal, vertical, borders) != Gecode::ES_OK)
                home.fail();
        }

    protected:
        static bool at_least_one_assigned(const VIntView::const_iterator &b, const VIntView::const_iterator &e, size_t increment)
        {
            for(auto it = b; it != e; it += increment)
                if(it->assigned())
                    return true;
            return false;
        }

        static std::string letters2regex(const VIntView::const_iterator &b, const VIntView::const_iterator &e, size_t increment)
        {
            std::string ret;

            for(auto it = b; it != e; it += increment)
            {
                if(it->assigned())
                    ret += it->val();
                else
                    ret += '.';
            }
            return ret;
        }

        static std::string process_regex_first(const std::string &r)
        {
            std::regex re("\\.+$");
            return std::regex_replace(r, re, ".*");
        }

        static std::string join_regex(const std::set<std::string> &re)
        {
            std::string ret = "";
            size_t i = 0;
            for(const auto &r : re)
            {
                ret += r;
                if(i+1 != re.size())
                    ret += '|';
                ++i;
            }
            return ret;
        }

        static void black_tiles(const VIntView::const_iterator &b, const VIntView::const_iterator &e, size_t increment, std::vector<size_t> &positions)
        {
            auto c = b + (2*increment);
            positions.clear();

            for(auto it = c; it != e; it += increment)
                if(it->assigned() && it->val() == '{')
                    positions.push_back(it - b);
        }

        static void unassigned(const VIntView::const_iterator &b, const VIntView::const_iterator &e, size_t increment, std::vector<size_t> &positions)
        {
            auto c = b + (2*increment);
            positions.clear();

            for(auto it = c; it != e; it += increment)
                if(!it->assigned())
                    positions.push_back(it - b);
        }

        static std::string regex_first(const VIntView::const_iterator &b, const VIntView::const_iterator &e, size_t increment)
        {
            std::set<std::string> patterns;
            std::vector<size_t> blackPositions;
            std::vector<size_t> stopPositions(1);
            size_t stopIndex = e - b;
            auto secondLetter = b+increment;

            black_tiles(b, e, increment, blackPositions);

            if(blackPositions.size())
                stopIndex = blackPositions[0];

            stopPositions[0] = stopIndex;
            unassigned(b, b + (stopIndex * increment), increment, stopPositions);

            for(size_t i = 0; i < 2; ++i)
            {
                size_t start = i*2;
                if(secondLetter->assigned())
                {
                    if((secondLetter->val() == '{' && start != 2)
                    || start != 0)
                        continue;
                }

                for(size_t stop : stopPositions)
                {
                    std::string r = letters2regex(b + start, b + stop, increment);
                    if(r.size())
                        patterns.insert(process_regex_first(r));
                }
            }
            
            return join_regex(patterns);
        }

        const Dictionary &dictionary;

        size_t width;
        size_t height;

        VIntView vletters;
        VIntView vh; // Word indices, horizontal
        VIntView vv; // Word indices, vertical
        VIntView vborders; // Word indices on the borders, in the following order : top, bottom, left, right
};

#endif

