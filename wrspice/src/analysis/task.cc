
/*========================================================================*
 *                                                                        *
 *  Distributed by Whiteley Research Inc., Sunnyvale, California, USA     *
 *                       http://wrcad.com                                 *
 *  Copyright (C) 2017 Whiteley Research Inc., all rights reserved.       *
 *  Author: Stephen R. Whiteley, except as indicated.                     *
 *                                                                        *
 *  As fully as possible recognizing licensing terms and conditions       *
 *  imposed by earlier work from which this work was derived, if any,     *
 *  this work is released under the Apache License, Version 2.0 (the      *
 *  "License").  You may not use this file except in compliance with      *
 *  the License, and compliance with inherited licenses which are         *
 *  specified in a sub-header below this one if applicable.  A copy       *
 *  of the License is provided with this distribution, or you may         *
 *  obtain a copy of the License at                                       *
 *                                                                        *
 *        http://www.apache.org/licenses/LICENSE-2.0                      *
 *                                                                        *
 *  See the License for the specific language governing permissions       *
 *  and limitations under the License.                                    *
 *                                                                        *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,      *
 *   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES      *
 *   OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-        *
 *   INFRINGEMENT.  IN NO EVENT SHALL WHITELEY RESEARCH INCORPORATED      *
 *   OR STEPHEN R. WHITELEY BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER     *
 *   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,      *
 *   ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE       *
 *   USE OR OTHER DEALINGS IN THE SOFTWARE.                               *
 *                                                                        *
 *========================================================================*
 *               XicTools Integrated Circuit Design System                *
 *                                                                        *
 * WRspice Circuit Simulation and Analysis Tool                           *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#include "circuit.h"
#include "output.h"
#include "miscutil/lstring.h"


sTASK::~sTASK()
{
    delete TSKshellOpts;
    while (TSKjobs) {
        sJOB *jx = TSKjobs;
        TSKjobs = TSKjobs->JOBnextJob;
        delete jx;
    }
}

 
// Create a new job, linked into task if task not null.
//
// Static function.
int
sTASK::newAnal(sTASK *tsk, int type, sJOB **analPtr)
{
    IFanalysis *an = IFanalysis::analysis(type);
    if (an) {
        sJOB *job = an->newAnal();
        if (job) {
            job->JOBname = an->name;
            job->JOBtype = type;
            if (tsk) {
                // Link at end to maintain input file order.
                if (!tsk->TSKjobs)
                    tsk->TSKjobs = job;
                else {
                    sJOB *j = tsk->TSKjobs;
                    while (j->JOBnextJob)
                        j = j->JOBnextJob;
                    j->JOBnextJob = job;
                }
            }
            if (analPtr)
                *analPtr = job;
            return (OK);
        }
    }
    return (E_NOANAL);
}


// Find the given Analysis given its name and return the Analysis
// pointer.  If name is null, count the number of jobs.
//
int
sTASK::findAnal(int *numjobs, sJOB **anal, IFuid name)
{
    for (sJOB *job = TSKjobs; job; job = job->JOBnextJob) {
        if (!job->JOBname)
            continue;  // "can't happen"

        if (!name) {
            if (numjobs) {
                int n = 0;
                for ( ; job; job = job->JOBnextJob)
                    n++;
                *numjobs = n;
            }
            return (OK);
        }
        if (strcmp((const char*)job->JOBname, (const char*)name) == 0) {
            if (anal)
                *anal = job;
            return (OK);
        }
    }
    return (E_NOTFOUND);
}


// Set the task struct to represent the options set in optp, and if
// domerge is set, merge in the options set in sOPTIONS::shellOpts(). 
// Save a pointer to a copy of the sOPTIONS::shellOpts().
//
void
sTASK::setOptions(sOPTIONS *optp, bool domerge)
{
    if (!optp)
        return;

    TSKopts.setup(optp, OMRG_GLOBAL);
    OMRG_TYPE mt = OMRG_NOSHELL;
    if (domerge) {
        if (optp->OPToptmerge_given)
            mt = (OMRG_TYPE)optp->OPToptmerge;
        else
            mt = OMRG_GLOBAL;
    }

    if (mt == OMRG_GLOBAL || mt == OMRG_LOCAL) {
        TSKopts.setup(sOPTIONS::shellOpts(), mt);
        delete TSKshellOpts;
        TSKshellOpts = sOPTIONS::shellOpts()->copy();
    }
}
// End of sTASK functions.


sJOB::~sJOB()
{
    OP.endPlot(JOBrun, true);   
    delete JOBoutdata;
}


// Set an analysis parameter given the parameter id.
//
int
sJOB::setParam(int parm, IFdata *data)
{
    IFanalysis *an = IFanalysis::analysis(JOBtype);
    if (!an)
        return (E_NOANAL);
    return (an->setParm(this, parm, data));
}


// Set an analysis parameter given the parameter name.
//
int
sJOB::setParam(const char *pname, IFdata *data)
{
    if (pname) {
        IFanalysis *an = IFanalysis::analysis(JOBtype);
        if (an) {
            for (int i = 0; i < an->numParms; i++) {
                if (lstring::cieq(pname, an->analysisParms[i].keyword)) {
                    return (an->setParm(this,
                        an->analysisParms[i].id, data));
                }
            }
            return (E_BADPARM);
        }
    }
    return (E_NOANAL);
}

