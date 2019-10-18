
/*========================================================================*
 *                                                                        *
 *  Distributed by Whiteley Research Inc., Sunnyvale, California, USA     *
 *                       http://wrcad.com                                 *
 *  Copyright (C) 2017 Whiteley Research Inc., all rights reserved.       *
 *  Author: Stephen R. Whiteley, except as indicated.                     *
 *  Modified: Bernard H. Venter, November 2018                            *
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
#include "sced_spiceipc.h"
#include "dsp_inlines.h"
#include "menu.h"
#include "promptline.h"
#include "events.h"
#include "errorlog.h"
#include "fio.h"

#include "miscutil/pathlist.h"  

#include <stdlib.h>
#include <string.h>
// #include <math.h>
#include <vector>
#include <iostream>
#include <bits/stdc++.h>

/***********************************************************************
 *
 * Functions for displaying spice output graphically.
 *
 ***********************************************************************/

namespace
{
void check_list(hyList **);
void remove_from_list(hyList **, hyList *);
} // namespace

// Electrical menu command prompt for a command to send to WRspice.
//
void SpiceCMD(CmdDesc *cmd, const char *in)
{
    Deselector ds(cmd);
    if (!XM()->CheckCurMode(Electrical))
        return;
    if (!XM()->CheckCurCell(true, true, Electrical))
        return;

    EV()->InitCallback();

    if (cmd && Menu()->GetStatus(cmd->caller))
    {
        if (!in)
            return;

        char *s = lstring::copy(in);
        GCarray<char *> gc_s(s);

        char *retbuf;           // Message returned.
        char *outbuf;           // Stdout/stderr returned.
        char *errbuf;           // Error message.
        unsigned char *databuf; // Command data.
        if (!SCD()->spif()->DoCmd(s, &retbuf, &outbuf, &errbuf, &databuf))
        {
            // No connection to simulator.
            if (retbuf)
            {
                PL()->ShowPrompt(retbuf);
                delete[] retbuf;
            }
            if (errbuf)
            {
                Log()->ErrorLog("spice ipc", errbuf);
                delete[] errbuf;
            }
            return;
        }
        if (retbuf)
        {
            PL()->ShowPrompt(retbuf);
            delete[] retbuf;
        }
        if (outbuf)
        {
            fputs(outbuf, stdout);
            delete[] outbuf;
        }
        if (errbuf)
        {
            Log()->ErrorLog("spice ipc", errbuf);
            delete[] errbuf;
        }
        delete[] databuf;
    }
}

// Menu command to plot variables.  Pointing at nodes or other hot spots
// adds terms to plot string shown.  Hitting Enter brings up plot.
// Parse the JoSIM output file to a format compatible with WRspice
// plot engin.
//
void cSced::showOutputExec(CmdDesc *cmd)
{
    Deselector ds(cmd);
    if (!XM()->CheckCurMode(Electrical))
        return;
    if (!XM()->CheckCurCell(false, false, Electrical))
        return;
    if (CurCell()->isEmpty())
    {
        PL()->ShowPrompt("No electrical data found in current cell.");
        return;
    }
    if (CurCell()->cellname() != DSP()->TopCellName())
    {
        PL()->ShowPrompt("Pop to top level first.");
        return;
    }

    if (JoSIM_Flag){
        // output file
        char buf[256];
        FIO()->PGetCWD(buf, 256);
        strcat(buf, "/output"); 
        char *outJosim = pathlist::expand_path(buf, true, true);
        outJosim = lstring::strip_space(outJosim);
        pathlist::path_canon(outJosim);
        char *cirJosim = pathlist::expand_path(outJosim, false, true);  // output file

        // create path for JoSIM output file 
        char oubuf[256];
        strcpy(oubuf, Tstring(DSP()->CurCellName()));
        char *scj;
        if ((scj = strrchr(oubuf, '.')) != 0) {
            if (!strcmp(scj, ".cir"))
                strcpy(scj, ".deck");
            else
                strcpy(scj, ".cir");
        }
        else
            strcat(oubuf, ".cir");

        char *outJJ = pathlist::expand_path(oubuf, true, true);
        outJJ = lstring::strip_space(outJJ);
        pathlist::path_canon(outJJ);
        char *cirJJ = pathlist::expand_path(outJJ, false, true); // josim circuit 

        // create path for new JoSIM output file 
        char bufNew[256];
        FIO()->PGetCWD(bufNew, 256);
        strcat(bufNew, "/output_new"); 
        char *outJJnew = pathlist::expand_path(bufNew, true, true);
        outJJnew = lstring::strip_space(outJJnew);
        pathlist::path_canon(outJJnew);
        char *cirJJnew = pathlist::expand_path(outJJnew, false, true); // josim circuit out

        ////////////////////////////////////////////////////////////////////////////////////
        char JosimSTR [256];
        char StrLine [256];
        bool PltFlag = 0;

        FILE *OutJosim = fopen (cirJJ , "r"); 

        std::vector<std::string> JosimData;   // from input file
        std::vector<std::string> outData;     // new output

        //Parse Josim file
        while ( fgets (JosimSTR , 256 , OutJosim) != NULL ){

            if(!(strncmp(JosimSTR,".subckt",7))){ // skip sub-circuit 
            PltFlag = 1;
            }
            if (!(strncmp(JosimSTR,"B",1)) && !PltFlag){
                strcpy(StrLine,JosimSTR);
                char *Tokens = strtok (StrLine," ");
                JosimData.push_back(Tokens);
                Tokens = strtok (NULL," ");
                Tokens = strtok (NULL," ");
                Tokens = strtok (NULL," ");
                JosimData.push_back(Tokens);
            }   
        }
        fclose(OutJosim);

        // parse Output
        char outFill [256];
        char OutLine [256];
        char OutFinal [256];

        bool end_Flag = 0;
        int JJ = 0;

        FILE *OutJosimSim = fopen (cirJosim , "r");
        FILE *OutJosimNew = fopen (cirJJnew , "w");
        std::vector<std::string>::iterator k;

        while ( fgets (outFill , 256 , OutJosimSim) != NULL ){
            if (!(strncmp(outFill,"Values:",7))){
                end_Flag = 1;
                fputs(outFill,OutJosimNew);
            }
            else if (!(strncmp(outFill," ",1)) && !(end_Flag)){
                strcpy(OutLine,outFill);
                char *TokensO = strtok (OutLine," _p()v");
                strcpy(OutFinal," ");
                strcat(OutFinal,TokensO);
                strcat(OutFinal," ");
                TokensO = strtok (NULL," _p()v");
                outData.push_back(TokensO);

                k = std::find (JosimData.begin(), JosimData.end(), outData[JJ]);
                if (k != JosimData.end()){
                    strcat(OutFinal,"v(");
                    strcat(OutFinal,JosimData[k - JosimData.begin() + 1].c_str());
                    strcat(OutFinal,") V\n");
                    fputs(OutFinal,OutJosimNew);
                }
                else if(!(strcmp(outData[JJ].c_str(),"CURRENT"))){  
                    TokensO = strtok (NULL," _p()v");
                    strcat(OutFinal,TokensO);
                    strcat(OutFinal," A\n");
                    fputs(OutFinal,OutJosimNew);
                }
                else if(!(strcmp(outData[JJ].c_str(),"time"))){ 
                    strcat(OutFinal,"time ");
                    TokensO = strtok (NULL," _p()v");
                    strcat(OutFinal,TokensO);
                    fputs(OutFinal,OutJosimNew);
                }
                else{
                    strcat(OutFinal,"v(");
                    strcat(OutFinal,TokensO);
                    strcat(OutFinal,") V\n");
                    fputs(OutFinal,OutJosimNew);
                }
                JJ++;
            }
            else{
                fputs(outFill,OutJosimNew);
            }   
        }
        JosimData.clear();
        outData.clear();
        fclose(OutJosimNew);
        fclose(OutJosimSim);

        JoSIM_Flag = 0;
        SpiceCMD(cmd, "load output_new");
    }

    connectAll(true);
    sc_doing_plot = true;
    for (;;)
    {
        check_list(&sc_plot_hpr_list);
        check_list(&sc_iplot_hpr_list);
        hyList *htmp = PL()->EditHypertextPrompt("plot ", sc_plot_hpr_list ? sc_plot_hpr_list : sc_iplot_hpr_list, false);
        if (htmp)
        {
            char *s = hyList::string(htmp, HYcvPlain, false);
            if (s)
            {
                hyList::destroy(sc_plot_hpr_list);
                sc_plot_hpr_list = htmp;
                bool ret = spif()->ExecPlot(s);
                delete[] s;
                if (!ret)
                    break;
            }
        }
        else
            break;
    }
    sc_doing_plot = false;
}

// Menu command to set up or unset an iplot.  If setting, the iplot string
// is shown for editing.  Pressing Enter completes editing and enables iplot.
//
void cSced::setDoIplotExec(CmdDesc *cmd)
{
    Deselector ds(cmd);
    if (!XM()->CheckCurMode(Electrical))
        return;
    if (!XM()->CheckCurCell(false, false, Electrical))
        return;
    if (CurCell()->isEmpty())
    {
        PL()->ShowPrompt("No electrical data found in current cell.");
        return;
    }

    sc_doing_iplot = cmd && Menu()->GetStatus(cmd->caller);
    if (!sc_doing_iplot)
    {
        spif()->ClearIplot();
        sc_iplot_status_changed = false;
        PL()->ShowPrompt("No plotting during simulation.");
    }
    else
    {
        if (CurCell()->cellname() != DSP()->TopCellName())
        {
            PL()->ShowPrompt("Pop to top level first.");
            sc_doing_iplot = false;
            return;
        }
        connectAll(true);
        sc_doing_plot = true;
        check_list(&sc_plot_hpr_list);
        check_list(&sc_iplot_hpr_list);
        hyList *htmp = PL()->EditHypertextPrompt("iplot ", sc_iplot_hpr_list ? sc_iplot_hpr_list : sc_plot_hpr_list, false);
        sc_doing_plot = false;
        if (htmp)
        {
            char *s = hyList::string(htmp, HYcvPlain, false);
            if (s)
            {
                char *estr = findPlotExpressions(s);
                delete[] s;
                if (estr && *estr)
                {
                    hyList::destroy(sc_iplot_hpr_list);
                    sc_iplot_hpr_list = htmp;
                    delete[] estr;
                    PL()->ShowPrompt("Will generate plot while simulating.");
                    sc_iplot_status_changed = true;
                    ds.clear();
                    return;
                }
                PL()->ShowPrompt("No plotting during simulation.");
                delete[] estr;
                hyList::destroy(sc_iplot_hpr_list);
                sc_iplot_hpr_list = 0;
            }
            hyList::destroy(htmp);
        }
        // no string entered, abort
        sc_doing_iplot = false;
    }
}

// Returns true if hent is a plot or iplot reference,
// in either case it is deleted.
//
int cSced::deletePlotRef(hyEnt *hent)
{
    if (!hent)
        return (false);
    hyList *h0 = 0, *h;
    for (h = sc_plot_hpr_list; h; h0 = h, h = h->next())
    {
        if (h->hent() == hent)
        {
            if (!h0)
                sc_plot_hpr_list = h->next();
            else
                h0->set_next(h->next());
            break;
        }
    }
    if (!h)
    {
        h0 = 0;
        for (h = sc_iplot_hpr_list; h; h0 = h, h = h->next())
        {
            if (h->hent() == hent)
            {
                if (!h0)
                    sc_iplot_hpr_list = h->next();
                else
                    h0->set_next(h->next());
                sc_iplot_status_changed = true;
                break;
            }
        }
    }
    if (h)
    {
        delete hent;
        return (true);
    }
    return (false);
}

// This is called from the hypertext editor when the plot command is
// active.  It identifies the trace index that contains each hypertext
// reference, and sets the corresponding mark color.  The mark color
// will match the trace color in the WRspice plot if colors have not
// been altered by the user - both Xic and WRspice start with the same
// defaults.
//
void cSced::setPlotMarkColors()
{
    hyList *curlist = PL()->List(false);
    if (!curlist)
        return;
    char *plstr = hyList::string(curlist, HYcvPlain, false);
    char *expr_str0 = findPlotExpressions(plstr);
    delete[] plstr;
    const char *expr_str = expr_str0;

    char *expr = expr_str ? lstring::getqtok(&expr_str) : 0;
    int tok_ix = 0;
    int clr_ix = 0;

    for (hyList *h = curlist; h; h = h->next())
    {
        if (!h->hent())
            continue;
        if (h->ref_type() != HLrefNode && h->ref_type() != HLrefBranch)
            continue;
        tok_ix++;
        if (!expr)
        {
            DSP()->SetPlotMarkColor(tok_ix, -1);
            continue;
        }
        char *estr = hyList::get_entry_string(h);
        if (!estr)
        {
            DSP()->SetPlotMarkColor(tok_ix, -1);
            continue;
        }
        if (!strstr(expr, estr))
        {
            char *nxpr;
            const char *s = expr_str;
            int ntok = 0;
            bool found = false;
            while ((nxpr = lstring::getqtok(&s)) != 0)
            {
                ntok++;
                if (strstr(nxpr, estr))
                {
                    delete[] expr;
                    expr = nxpr;
                    expr_str = s;
                    clr_ix += ntok;
                    found = true;
                    break;
                }
                delete[] nxpr;
            }
            if (!found)
            {
                // Hmmm, shouldn't happen.
                delete[] estr;
                DSP()->SetPlotMarkColor(tok_ix, -1);
                continue;
            }
        }
        delete[] estr;
        DSP()->SetPlotMarkColor(tok_ix, clr_ix);
    }
    delete[] expr;
    delete[] expr_str0;
    hyList::destroy(curlist);
}

// Clear all plot and iplot references.
//
void cSced::clearPlots()
{
    if (!DSP()->CurCellName())
        return;
    if (sc_plot_hpr_list)
    {
        hyList::destroy(sc_plot_hpr_list);
        sc_plot_hpr_list = 0;
    }
    if (sc_iplot_hpr_list)
    {
        hyList::destroy(sc_iplot_hpr_list);
        sc_iplot_hpr_list = 0;
        sc_iplot_status_changed = true;
    }
    Menu()->MenuButtonSet("side", "iplot", false);
}

// Return the plot string.  If ascii is true, return as ascii hypertext,
// otherwise as plain text.
//
char *
cSced::getPlotCmd(bool ascii)
{
    if (!sc_plot_hpr_list)
        return (0);
    check_list(&sc_plot_hpr_list);
    return (hyList::string(sc_plot_hpr_list, ascii ? HYcvAscii : HYcvPlain,
                           false));
}

// Set the current plot list, input is ascii format hypertext.
//
void cSced::setPlotCmd(const char *cmd)
{
    CDs *cursde = CurCell(Electrical);
    if (cursde)
    {
        if (sc_plot_hpr_list)
            hyList::destroy(sc_plot_hpr_list);
        sc_plot_hpr_list = new hyList(cursde, cmd, HYcvAscii);
    }
}

// Return the iplot string.  If ascii is true, return as ascii hypertext,
// otherwise as plain text.
//
char *
cSced::getIplotCmd(bool ascii)
{
    if (!sc_iplot_hpr_list)
        return (0);
    check_list(&sc_iplot_hpr_list);
    return (hyList::string(sc_iplot_hpr_list, ascii ? HYcvAscii : HYcvPlain,
                           false));
}

// Set the current iplot list, input is ascii format hypertext.
//
void cSced::setIplotCmd(const char *cmd)
{
    CDs *cursde = CurCell(Electrical);
    if (cursde)
    {
        if (sc_iplot_hpr_list)
            hyList::destroy(sc_iplot_hpr_list);
        sc_iplot_hpr_list = new hyList(cursde, cmd, HYcvAscii);
    }
}

// Return the current plot list.
//
hyList *
cSced::getPlotList()
{
    return (sc_plot_hpr_list);
}

// Return the current iplot list.
//
hyList *
cSced::getIplotList()
{
    return (sc_iplot_hpr_list);
}

namespace
{
// Remove and free any bogus references.
//
void check_list(hyList **list)
{
    hyList *hn;
    for (hyList *h = *list; h; h = hn)
    {
        hn = h->next();
        if (h->ref_type() == HLrefText)
            continue;
        if (h->hent() && h->hent()->ref_type() != HYrefBogus)
        {
            char *s = hyList::get_entry_string(h);
            if (s)
            {
                delete[] s;
                continue;
            }
        }
        remove_from_list(list, h);
    }
}

// Remove hx from the list, and also the preceding space char if
// any if hx is hypertext, and free the removed elements.
//
void remove_from_list(hyList **list, hyList *hx)
{
    if (!*list)
        return;
    hyList *hprev = 0, *hprev2 = 0;
    for (hyList *h = *list; h; h = h->next())
    {
        if (h == hx)
        {
            if (h->ref_type() == HLrefText)
            {
                if (hprev)
                    hprev->set_next(h->next());
                else
                    *list = h->next();
                delete h;
                return;
            }
            if (hprev && hprev->ref_type() == HLrefText)
            {
                if (!hprev->text() ||
                    (isspace(hprev->text()[0]) && !hprev->text()[1]))
                {
                    if (hprev2)
                        hprev2->set_next(h->next());
                    else
                        *list = h->next();
                    delete hprev;
                    delete h;
                    return;
                }
                int len = strlen(hprev->text());
                if (isspace(hprev->text()[len - 1]))
                    hprev->etext()[len - 1] = 0;
            }
            if (hprev)
                hprev->set_next(h->next());
            else
                *list = h->next();
            delete h;
            return;
        }
        hprev2 = hprev;
        hprev = h;
    }
}
} // namespace
