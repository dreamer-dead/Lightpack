#include "TimeEvaluations.hpp"
#include <QtDebug>
#include <QtGlobal>

#if defined _MSC_VER
#include <winsock.h>    // timeval is defined in here
#include <sys/timeb.h>

static int gettimeofday(struct timeval* tv, struct timezone * /* tzp */)
{
    struct _timeb curSysTime;

    _ftime(&curSysTime);
    tv->tv_sec = (long)curSysTime.time;
    tv->tv_usec = curSysTime.millitm * 1000;
    return 0;
}

#else
#include <sys/time.h> // for timeval and gettimeofday()
#endif

void TimeEvaluations::howLongItStart()
{
    struct timeval tv;
    if(gettimeofday(&tv, 0) < 0){
        qWarning() << "howLongItStart(): Error gettimeofday()";
    }else{
        this->timevalue_ms = (tv.tv_sec % 1000) * 1000.0 + tv.tv_usec / 1000.0;
    }
}

double TimeEvaluations::howLongItEnd()
{
    struct timeval tv;
    if(gettimeofday(&tv, 0) < 0){
        qWarning() << "howLongItEnd(): Error gettimeofday()";
        return -1;
    }else{
        double now = ((tv.tv_sec % 1000) * 1000.0 + tv.tv_usec / 1000.0);
        double dt_ms = now - this->timevalue_ms;
        this->timevalue_ms = -1;
        return dt_ms;
    }
}
