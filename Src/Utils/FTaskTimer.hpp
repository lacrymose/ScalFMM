#ifndef FTASKTIMER_HPP
#define FTASKTIMER_HPP

#include "FGlobal.hpp"
#include "FTic.hpp"
#include "FAssert.hpp"

#include "../Containers/FVector.hpp"

#include <unordered_set>
#include <omp.h>


#ifdef SCALFMM_TIME_OMPTASKS
#define FTIME_TASKS(X) X;
#else
#define FTIME_TASKS(X)
#endif


class FTaskTimer{
protected:
    static const int MaxTextLength = 16;

    struct EventDescriptor{
        char text[MaxTextLength];
        double duration;
        double start;
        long long int eventId;
    };

    struct ThreadData{
        FVector<EventDescriptor> events;
    };

    const int nbThreads;
    // We create array of ptr to avoid the modifcation of clause
    // memory by different threads
    ThreadData** threadEvents;
    double startingTime;
    double duration;

public:

    explicit FTaskTimer(const int inNbThreads = -1)
            : nbThreads(inNbThreads>0?inNbThreads:omp_get_max_threads()), threadEvents(nullptr),
                startingTime(0) {
        FLOG( FLog::Controller << "\tFTaskTimer is used\n" );

        threadEvents = new ThreadData*[nbThreads];
        #pragma omp parallel num_threads(nbThreads)
        {
            threadEvents[omp_get_thread_num()] = new ThreadData;
        }
    }

    ~FTaskTimer(){
        for(int idxThread = 0 ; idxThread < nbThreads ; ++idxThread){
            delete threadEvents[idxThread];
        }
        delete[] threadEvents;
    }

    void start(){
        FLOG( FLog::Controller << "\tFTaskTimer starts\n" );
        #pragma omp parallel num_threads(nbThreads)
        {
            threadEvents[omp_get_thread_num()]->events.clear();
        }
        startingTime = FTic::GetTime();
    }

    void end(){
        FLOG( FLog::Controller << "\tFTaskTimer ends\n" );
        duration = FTic::GetTime() - startingTime;
    }

    void saveToDisk(const char inFilename[]) const {
        FLOG( FLog::Controller << "\tFTaskTimer saved to " << inFilename << "\n" );
        FILE* foutput = fopen(inFilename, "w");
        FAssert(foutput);

        fprintf(foutput, "ScalFMM Task Records\n");

        FSize totalEvents = 0;
        for(int idxThread = 0 ; idxThread < nbThreads ; ++idxThread){
            totalEvents += threadEvents[idxThread]->events.getSize();
        }
        fprintf(foutput, "global{@duration=%e;@max threads=%d;@nb events=%lld}\n",
                duration, nbThreads, totalEvents);

        std::unordered_set<long long int> ensureUniqueness;
        ensureUniqueness.reserve(totalEvents);

        for(int idxThread = 0 ; idxThread < nbThreads ; ++idxThread){
            for(int idxEvent = 0 ; idxEvent < threadEvents[idxThread]->events.getSize() ; ++idxEvent){
                const EventDescriptor& event = threadEvents[idxThread]->events[idxEvent];
                fprintf(foutput, "event{@id=%lld;@duration=%e;@start=%e;@text=%s}\n",
                        event.eventId, event.duration, event.start, event.text);
                FAssertLF(ensureUniqueness.find(event.eventId) == ensureUniqueness.end());
                ensureUniqueness.insert(event.eventId);
            }
        }

        fclose(foutput);
    }

    class ScopeEvent{
    protected:
        const double eventStartingTime;
        const double measureStartingTime;
        ThreadData*const  myEvents;

        const long long int taskId;
        char taskText[MaxTextLength];

    public:
        ScopeEvent(FTaskTimer* eventsManager, const long long int inTaskId, const char inText[MaxTextLength])
            : eventStartingTime(FTic::GetTime()), measureStartingTime(eventsManager->startingTime),
              myEvents(eventsManager->threadEvents[omp_get_thread_num()]),
              taskId(inTaskId){
            taskText[0] = '\0';
            strncpy(taskText, inText, MaxTextLength);
        }

        template <class FirstParameters, class ... Parameters>
        ScopeEvent(FTaskTimer* eventsManager, const long long int inTaskId, const char inTextFormat, FirstParameters fparam, Parameters ... params)
            : eventStartingTime(FTic::GetTime()), measureStartingTime(eventsManager->startingTime),
              myEvents(eventsManager->threadEvents[omp_get_thread_num()]),
              taskId(inTaskId){
            snprintf(taskText, MaxTextLength, inTextFormat, fparam, params...);
        }

        ~ScopeEvent(){
            EventDescriptor event;
            event.duration = FTic::GetTime()-eventStartingTime;
            event.eventId = taskId;
            event.start = eventStartingTime-measureStartingTime;
            strncpy(event.text, taskText, MaxTextLength);
            myEvents->events.push(event);
        }
    };
};



#endif // FTASKTIMER_HPP

