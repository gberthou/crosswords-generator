#ifndef PROP_REGEX_H
#define PROP_REGEX_H

#include <vector>

#include <gecode/int.hh>

typedef std::vector<Gecode::Int::IntView> VIntView;

class PropRegex : public Gecode::Propagator
{
    public:
        PropRegex(Gecode::Space &home, const VIntView &avletters):
            Gecode::Propagator(home),
            vletters(avletters)
        {
            for(auto &vl : vletters)
                vl.subscribe(home, *this, Gecode::Int::PC_INT_VAL);
        }

        PropRegex(Gecode::Space &home, PropRegex &p):
            Gecode::Propagator(home, p),
            vletters(p.vletters.size())
        {
            for(size_t i = 0; i < p.vletters.size(); ++i)
                vletters[i].update(home, p.vletters[i]);
        }

        virtual Gecode::ExecStatus propagate(Gecode::Space &home, const Gecode::ModEventDelta &)
        {
            (void) home;
            // TODO
            return Gecode::ES_FIX;
        }

        virtual size_t dispose(Gecode::Space &home)
        {
            for(auto &vl : vletters)
            {
                vl.cancel(home, *this, Gecode::Int::PC_INT_VAL);
            }
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

        static Gecode::ExecStatus post(Gecode::Space &home, const Gecode::IntVarArray &letters)
        {
            /* For now, assume that letters are not "same"
            if(letters.same())
                return Gecode::ES_FAILED;
            */

            VIntView vletters;
            for(auto &l : letters)
                vletters.push_back(l);

            (void) new (home) PropRegex(home, vletters);
            return Gecode::ES_OK;
        }

        static void propregex(Gecode::Space &home, const Gecode::IntVarArray &letters)
        {
            if(PropRegex::post(home, letters) != Gecode::ES_OK)
                home.fail();
        }

    protected:
        VIntView vletters;
};

#endif

