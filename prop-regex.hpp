#ifndef PROP_REGEX_H
#define PROP_REGEX_H

#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <regex>

#include <gecode/int.hh>

typedef std::vector<Gecode::Int::IntView> VIntView;

class PropRegex : public Gecode::Propagator
{
    public:
        PropRegex(Gecode::Space &home, size_t w, size_t h, const VIntView &avletters, const VIntView &avh, const VIntView &avv, const VIntView &avborders):
            Gecode::Propagator(home),
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

                std::vector<std::string> patterns;
                regex_first(itBegin, itEnd, 1, patterns);

                for(auto i : patterns)
                    std::cout << i << std::endl;
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

        static Gecode::ExecStatus post(Gecode::Space &home, size_t width, size_t height, const Gecode::IntVarArgs &letters, const Gecode::IntVarArgs &horizontal, const Gecode::IntVarArgs &vertical, const Gecode::IntVarArgs &borders)
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

            (void) new (home) PropRegex(home, width, height, vletters, vh, vv, vborders);
            return Gecode::ES_OK;
        }

        static void propregex(Gecode::Space &home, size_t width, size_t height, const Gecode::IntVarArgs &letters, const Gecode::IntVarArgs &horizontal, const Gecode::IntVarArgs &vertical, const Gecode::IntVarArgs &borders)
        {
            if(PropRegex::post(home, width, height, letters, horizontal, vertical, borders) != Gecode::ES_OK)
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

        static void black_tiles(const VIntView::const_iterator &b, const VIntView::const_iterator &e, size_t increment, std::vector<size_t> &positions)
        {
            auto c = b + 2;
            positions.clear();

            for(auto it = c; it != e; it += increment)
                if(it->assigned() && it->val() == '{')
                    positions.push_back(it - b);
        }

        static void regex_first(const VIntView::const_iterator &b, const VIntView::const_iterator &e, size_t increment, std::vector<std::string> &ret)
        {
            std::vector<size_t> blackPositions;
            auto stop = e;
            auto secondLetter = b+increment;

            ret.clear();

            black_tiles(b, e, increment, blackPositions);

            if(blackPositions.size())
                stop = b + blackPositions[0];

            if(secondLetter->assigned())
            {
                auto start = b + (secondLetter->val() == '{' ? (2*increment) : 0);
                std::string r = letters2regex(start, stop, increment);
                ret.push_back(process_regex_first(r));
            }
            else
            {
                for(size_t i = 0; i < 2; ++i)
                {
                    auto start = b + (2*i*increment);
                    std::string r = letters2regex(start, stop, increment);
                    ret.push_back(process_regex_first(r));
                }
            }
        }

        size_t width;
        size_t height;

        VIntView vletters;
        VIntView vh; // Word indices, horizontal
        VIntView vv; // Word indices, vertical
        VIntView vborders; // Word indices on the borders, in the following order : top, bottom, left, right
};

#endif

