/*========================================================================*
 *                                                                        *
 *  Distributed by Whiteley Research Inc., Sunnyvale, California, USA     *
 *                       http://wrcad.com                                 *
 *  Copyright (C) 2017 Whiteley Research Inc., all rights reserved.       *
 *  Author: Stephen R. Whiteley, except as indicated.                     *
 *  Modified: Bernard H. Venter, November 2018                     
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
 * Xic Integrated Circuit Layout and Schematic Editor                     *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#include "main.h"              
#include "sced.h"              
#include "dsp_inlines.h"       
#include "events.h"             
#include "menu.h"               
#include "ebtn_menu.h"         
#include "promptline.h"         


#include "sced_spiceipc.h"      
#include "dsp_tkif.h"           
#include "errorlog.h"          
#include "select.h"             
#include "miscutil/pathlist.h"  
#ifdef WIN32
#include <winsock2.h>
#else
#include <arpa/inet.h>
#endif

// The pull-down menu entries.
// These keywords are the menu labels, and are also used as help system
// tags "run_spice:xxx".
// The order MUST match the RunType enum!
//


namespace {
    const char * run_spice[] = {
        "JoSIM",
        "WRSpice",
        0
    };
}


const char *const *
cSced::runList()
{
    return (run_spice);
}

// Electrical menu command prompt for a command to send to WRspice.
//
void
SpiceCMD(CmdDesc *cmd, const char *in )                     
{
    Deselector ds(cmd);
    if (!XM()->CheckCurMode(Electrical))
        return;
    if (!XM()->CheckCurCell(true, true, Electrical))
        return;

    EV()->InitCallback();

    if (cmd && Menu()->GetStatus(cmd->caller)) {            
        if (!in)
            return;

        char *s = lstring::copy(in);
        GCarray<char*> gc_s(s);

        char *retbuf;           // Message returned.
        char *outbuf;           // Stdout/stderr returned.
        char *errbuf;           // Error message.
        unsigned char *databuf; // Command data.
        if (!SCD()->spif()->DoCmd(s, &retbuf, &outbuf, &errbuf, &databuf)) {
            // No connection to simulator.
            if (retbuf) {
                PL()->ShowPrompt(retbuf);
                delete [] retbuf;
            }
            if (errbuf) {
                Log()->ErrorLog("spice ipc", errbuf);
                delete [] errbuf;
            }
            return;
        }
        if (retbuf) {
            PL()->ShowPrompt(retbuf);
            delete [] retbuf;
        }
        if (outbuf) {
            fputs(outbuf, stdout);
            delete [] outbuf;
        }
        if (errbuf) {
            Log()->ErrorLog("spice ipc", errbuf);
            delete [] errbuf;
        }
        delete [] databuf;
    }
}

//-----------------------------------------------------------------------------
// The RUN WRspice command.
//
// Submenu command for running WRspice.  In asynchronous mode, the run button
// is left active while simulation continues.

void runWRspiceExec(CmdDesc* cmd)
{
    Deselector ds( cmd);
    if (!XM()->CheckCurMode(Electrical))
        return;
    if (!XM()->CheckCurCell(false, false, Electrical))
        return;

    if (cmd && Menu()->GetStatus(cmd->caller)) {
        if (!SCD()->spif()->RunSpice(cmd))
            return;
        ds.clear();
    }
}

//-----------------------------------------------------------------------------
// The RUN JoSIM command.
//
// Submenu command for running JoSIM. The circuit file is dumped in the current 
// directory and sumilated using JoSIM. The location of JoSIM is selected via the
// open dialog.

void
runJoSIMExec(CmdDesc* cmd)                                       
{
    if (!XM()->CheckCurMode(Electrical))
        return;
    if (!XM()->CheckCurCell(false, false, Electrical))
        return;
    if (CurCell()->isEmpty()) {
        PL()->ShowPrompt("No electrical data found in current cell.");
        return;
    }
    if (DSP()->CurCellName() == DSP()->TopCellName()) {
        char *s = SCD()->getAnalysis(false);
        if (!s || strcmp(s, "run")) {
            char *in =
                PL()->EditPrompt("Enter optional analysis command: ", s);
            if (!in) {
                PL()->ErasePrompt();
                delete [] s;
                return;
            }
            while (isspace(*in))
                in++;
            if (*in)
                SCD()->setAnalysis(in);
        }
        delete [] s;
    }

    char tbuf[256];
    char jbuf[256];
    strcpy(tbuf, Tstring(DSP()->CurCellName()));
    char *s;
    if ((s = strrchr(tbuf, '.')) != 0) {
        if (!strcmp(s+1, "cir"))
            strcpy(s+1, "deck");
        else
            strcpy(s+1, "cir");
    }
    else
        strcat(tbuf, ".cir");

    char *in = XM()->SaveFileDlg("New SPICE file name? ", tbuf);
    if (!in || !*in) {
        PL()->ErasePrompt();
        return;
    }
    char *filename = pathlist::expand_path(in, false, true);
    SCD()->dumpSpiceFile(filename);

    const char *JoSim_file_linux = "JoSIM_linux_RELEASE_NONE";
    const char *JoSim_file_mac = "JoSIM_mac_RELEASE_NONE";
    const char *JoSim_file_win = "JoSIM_win_RELEASE_NONE";

    char *inJoSIM = pathlist::expand_path("/usr/local/bin", false, true);

    bool JoSIM_check_linux = pathlist::find_path_file(JoSim_file_linux, "/usr/local/bin",NULL,true);
    bool JoSIM_check_mac = pathlist::find_path_file(JoSim_file_mac, "/usr/local/bin",NULL,true);
    bool JoSIM_check_win = pathlist::find_path_file(JoSim_file_win, "/usr/local/bin",NULL,true);

    // Choose the location of JoSIM
    if(JoSIM_check_linux)
        inJoSIM = pathlist::expand_path("/usr/local/bin/JoSIM_linux_RELEASE_NONE", false, true);
    else if(JoSIM_check_mac)
        inJoSIM = pathlist::expand_path("/usr/local/bin/JoSIM_mac_RELEASE_NONE", false, true);
    else if(JoSIM_check_win)
        inJoSIM = pathlist::expand_path("/usr/local/bin/JoSIM_win_RELEASE_NONE", false, true);
    else {  
        strcpy (jbuf, "");
        char *inJo = XM()->OpenFileDlg("Open JoSIM Install Location: ", jbuf); 
        inJoSIM = pathlist::expand_path(inJo, false, true);
        if (!inJoSIM || !*inJoSIM) {
            PL()->ErasePrompt();
            return;
        }
    }

    // Test if JoSIM has been selected
    const char *loc_linux = inJoSIM + strlen(inJoSIM) - strlen(JoSim_file_linux);
    const char *loc_mac = inJoSIM + strlen(inJoSIM) - strlen(JoSim_file_mac);
    const char *loc_win = inJoSIM + strlen(inJoSIM) - strlen(JoSim_file_win);
    if ((strcmp(JoSim_file_linux, loc_linux) != 0) && (strcmp(JoSim_file_mac, loc_mac) != 0) && (strcmp(JoSim_file_win, loc_win) != 0)){
        PL()->ShowPromptV("ERROR: JOSIM Not Found");
        Errs()->sys_error("ERROR: JOSIM Not Found");
        return;
    }

    // Load plotting data
    SpiceCMD(cmd,"load output");
 
    // Fork process to run Josim
    int cpid = fork();
    if (cpid == -1) {
        Errs()->sys_error("init_local: fork");
        return;
    }
    if (!cpid) {
        execl(inJoSIM,inJoSIM,"-c", "1", "-o","output", filename, (char *) 0); 
        Errs()->sys_error("JoSIM execution failed");
        return;
    }
}
RunType Run_option();

// This is called in response to the pull-down menu of run templates.
//
void
cSced::RunCom(int Run_option,CmdDesc* cmd)
{
    switch (Run_option) {
    case runJoSIM:
        runJoSIMExec(cmd);                                                      
        break;
    case runWRspice:
        runWRspiceExec(cmd);                                                   
        break;
    default:
        if (!XM()->CheckCurMode(Electrical))
            return;
        if (!XM()->CheckCurLayer())
            return;
        if (!XM()->CheckCurCell(true, true, Electrical))
            return;
        break;
    }
}



