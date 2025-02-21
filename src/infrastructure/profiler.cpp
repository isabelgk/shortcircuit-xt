/*
** Shortcircuit XT is Free and Open Source Software
**
** Shortcircuit is made available under the Gnu General Public License, v3.0
** https://www.gnu.org/licenses/gpl-3.0.en.html; The authors of the code
** reserve the right to re-license their contributions under the MIT license in the
** future at the discretion of the project maintainers.
**
** Copyright 2004-2021 by various individuals as described by the git transaction log
**
** All source at: https://github.com/surge-synthesizer/surge.git
**
** Shortcircuit was a commercial product from 2004-2018, with copyright and ownership
** in that period held by Claes Johanson at Vember Audio. Claes made Shortcircuit
** open source in December 2020.
*/

#include "profiler.h"

#include <map>
#include <list>
#include "logfile.h"
#include "ticks.h"
#include <string.h>

#define MAX_ID_LEN 255

#if WINDOWS
#include <windows.h>
static DWORD_PTR gThreadAffinityOriginal = 0;
static int gTASet = 0;
#endif

namespace scxt::Perf
{

struct ProfID
{
    char mID[MAX_ID_LEN];
    ProfID(const char *id)
    {
        strncpy(mID, id, MAX_ID_LEN);
        mID[MAX_ID_LEN - 1] = 0;
    }
};

struct ProfIDComp
{
    bool operator()(const ProfID &a, const ProfID &b) const { return strcmp(a.mID, b.mID) < 0; }
};

struct Metrics
{
    scxt::Time::Timestamp mStamp;
    unsigned mNumCalls;
};

struct Prof
{
    scxt::Time::Timestamp start;
    scxt::Time::Timestamp subtract;
};

struct InternalRep
{
    explicit InternalRep(scxt::log::LoggingCallback *cb) : mLogger(cb) {}
    scxt::log::StreamLogger mLogger;
    std::list<Prof> mProfList;
    std::map<ProfID, Metrics, ProfIDComp> mProfTimes;
    bool logOutput{true};
};

Profiler::Profiler(scxt::log::LoggingCallback *logger, const char *msg) : mData(0)
{
    mData = (void *)new InternalRep(logger);
    InternalRep *rt = (InternalRep *)mData;

#if WINDOWS
    // set thread affinity for this thread so that our timers are always from core 1
    gTASet++; // should be in tls, but for our purposes its fine
    if (gTASet == 1)
    {
        gThreadAffinityOriginal = SetThreadAffinityMask(GetCurrentThread(), 1);
    }

#endif

    if (msg)
        reset(msg);
}

Profiler::~Profiler()
{
#if WINDOWS
    gTASet--;
    if (!gTASet)
    {
        SetThreadAffinityMask(GetCurrentThread(), gThreadAffinityOriginal);
    }
#endif
    delete (InternalRep *)mData;
}

void Profiler::setLogOutput(bool b)
{
    InternalRep *rt = (InternalRep *)mData;
    rt->logOutput = b;
}

void Profiler::reset(const char *msg)
{
    InternalRep *rt = (InternalRep *)mData;

    if (rt->logOutput)
        LOGDEBUG(rt->mLogger) << "Profiler reset " << msg << std::flush;
    rt->mProfList.clear();
    rt->mProfTimes.clear();
}

void Profiler::dump(const char *msg)
{
    InternalRep *rt = (InternalRep *)mData;
    int64_t calc;
    if (rt->logOutput && rt->mLogger.setLevel(log::Level::Debug))
    {
        LOGDEBUG(rt->mLogger) << "Profiler dump '" << msg << "' (id/ms/calls)" << std::flush;
        std::map<ProfID, Metrics, ProfIDComp>::iterator it;
        for (it = rt->mProfTimes.begin(); it != rt->mProfTimes.end(); it++)
        {
            // cvt to milliseconds
            calc = it->second.mStamp / 1000;
            LOGDEBUG(rt->mLogger) << it->first.mID << "," << (int)calc << ","
                                  << it->second.mNumCalls << std::flush;
        }
    }
}

void Profiler::enter()
{
    InternalRep *rt = (InternalRep *)mData;
    Prof prof;
    scxt::Time::getCurrentTimestamp(&prof.start);
    prof.subtract = 0;
    rt->mProfList.push_back(prof);
}

static void addTime(scxt::Time::Timestamp *target, scxt::Time::Timestamp *src) { *target += *src; }

void Profiler::exit(const char *id)
{
    InternalRep *rt = (InternalRep *)mData;
    std::map<ProfID, Metrics, ProfIDComp>::iterator it;
    Prof prof;
    scxt::Time::Timestamp endtime;
    scxt::Time::Timestamp tot;
    Metrics tot2;

    scxt::Time::getCurrentTimestamp(&endtime);
    if (!rt->mProfList.size())
    {
        LOGERROR(rt->mLogger) << "Inconsistent profile entry/exit count for %s." << id
                              << std::flush;
        return;
    }
    // remove last item pushed
    prof = rt->mProfList.back();
    rt->mProfList.pop_back();
    // determine it's time
    scxt::Time::getTimestampDiff(&endtime, &prof.start, &tot);

    if (rt->mProfList.size())
    {
        // if there is a previous item, add the current items run time
        // to the previous items subtract time - we don't want to include it in
        // that item's time
        Prof &profprev = rt->mProfList.back();
        // this is added to the previous item's subtract time
        addTime(&profprev.subtract, &tot);
    }

    // for our current item, we will subtract any time that was registered
    // when sub items ran
    scxt::Time::getTimestampDiff(&tot, &prof.subtract, &tot2.mStamp);

    // and add that time to the big map
    ProfID h(id);

    it = rt->mProfTimes.find(h);
    if (it == rt->mProfTimes.end())
    {
        // not found
        tot2.mNumCalls = 1; // initial value
        // rt->mProfTimes.insert(std::make_pair<const ProfID, Metrics>(h, tot2));
        rt->mProfTimes.insert(std::make_pair(h, tot2));
    }
    else
    {
        // found an entry for this id, so accumulate time and # of calls
        addTime(&it->second.mStamp, &tot2.mStamp);
        it->second.mNumCalls++;
    }
}

} // namespace scxt::Perf