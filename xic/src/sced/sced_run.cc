/*========================================================================*
 *                                                                        *
 *  Distributed by Whiteley Research Inc., Sunnyvale, California, USA     *
 *                       http://wrcad.com                                 *
 *  Copyright (C) 2017 Whiteley Research Inc., all rights reserved.       *
 *  Author: Bernard H. Venter, March 2019                                 *
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

#include "cvrt.h"
#include "editif.h"
#include "fio.h"
#include "fio_gdsii.h"
#include "fio_alias.h"
#include "cd_digest.h"
#include "scedif.h"
#include "dsp_layer.h"
#include "cvrt_menu.h"
#include "tech.h"
#include "layertab.h"
#include "tech_kwords.h"
#include "miscutil/filestat.h"
#include <sys/stat.h>

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <bits/stdc++.h>

// The pull-down menu entries.
// These keywords are the menu labels, and are also used as help system
// tags "run_spice:xxx".
// The order MUST match the RunType enum!
//

namespace {
    const char * run_spice[] = {
        "JoSIM",
        "WRSpice",
        "InductEx",
        0
    };
}


const char *const *
cSced::runList()
{
    return (run_spice);
}

namespace {
    // Return a tokens to use as the "cellname" in a filename.
    //
    void
    def_cellname(char *buf, const stringlist *list)
    {
        if (Cvt()->WriteFilename())
            strcpy(buf, Cvt()->WriteFilename());
        else {
            if (!strcmp(list->string, FIO_CUR_SYMTAB))
                strcpy(buf, "symtab_cells");
            else
                strcpy(buf, list->string);
            char *s = strrchr(buf,'.');
            if (s && s != buf)
                *s = '\0';
            if (FIO()->IsStripForExport())
                strcat(buf, ".phys");
        }
    }


    // creates and dumps a GDSII file  of the current circiut to be used
    // in InductEx. the function returns the address of the GDSII file 
    // that was created.  
    //
    char *
    outGDS( bool allcells, void*)
    {
        // // Clear pushed filename and AOI params, if any.
        // if (Cvt()->WriteFilename()) {
        //     Cvt()->SetWriteFilename(0);
        //     Cvt()->SetupWriteCut(0);
        // }

        stringlist *namelist = new stringlist(lstring::copy(
            allcells ? FIO_CUR_SYMTAB : Tstring(DSP()->CurCellName())), 0);
        GCdestroy<stringlist> gc_namelist(namelist);

        CDcbin cbin(DSP()->CurCellName());
        if (!cbin.isSubcell())
            EditIf()->assignGlobalProperties(&cbin);

        FIOcvtPrms prms;
        prms.set_scale(FIO()->WriteScale());
        prms.set_alias_mask(CVAL_CASE | CVAL_PFSF | CVAL_FILE);
        if (!allcells && FIO()->OutFlatten()) {
            prms.set_flatten(true);
            if (FIO()->OutUseWindow()) {
                prms.set_use_window(true);
                prms.set_window(FIO()->OutWindow());
                prms.set_clip(FIO()->OutClip());
            }
        }

        char buf[256];
        def_cellname(buf, namelist);
        strcat(buf, "_new.gds");
        PL()->ErasePrompt();                                      

        // save and get address of file
        char *s = pathlist::expand_path(buf, true, true);
        s = lstring::strip_space(s);
        pathlist::path_canon(s);
        char *gdsname = pathlist::expand_path(s, false, true);

        prms.set_destination(gdsname, Fgds);
        FIO()->ConvertToGds(namelist, &prms);
        return(gdsname);
    }
}

//
//-----------------------------------------------------------------------------


// The function parces a line from the XIC circuit file into one InductEX
// can use. The .param is removed and the calculated values are added inline 
// with the component name and coordinates.
//
void
Param_val(char * inLine, char *INbuffer, char *newLine, FILE *OUT,std::vector<std::string> &Ts,std::vector<std::string> &TsCoord){  
    char *tokens = strtok (inLine," =\n");
    char comp ='o';
    int coord_flag = 0;
    char Hold[20];
    char Coord[20];
    double TempPval = 0;
    
    // JJ or Inductor
    if(!(strncmp(inLine,"B",1))){               // for JJ
        tokens[0] = 'J'; 
        comp = 'B';
    }
    else if (!(strncmp(inLine,"L",1))){         // for inductor
        comp = 'L';
    }
    else if (!(strncmp(inLine,"R",1))){         // for resistor
        comp = 'R';
    }          
    strcpy(newLine,tokens);          

    //Get the coords
    for(int i = 0; i < 2 ; ++i){ 
        strcat(newLine," ");
        tokens = strtok (NULL, " =\n");
        if(!(strncmp(tokens,"P",1))){                                   //if port is given as node name 
            if(!(std::find(Ts.begin(),Ts.end(),tokens) != Ts.end())){
                Ts.push_back(tokens);
                if(i && !coord_flag && (comp == 'R')){                  // only applicable to resistors
                    TsCoord.push_back(Hold);
                }
                else
                {
                    strcpy(Coord,"N_");
                    strcat(Coord,tokens);
                    TsCoord.push_back(Coord);
                    coord_flag = 1;
                }               
            }
            strcat(newLine,"N_");
        }
        strcpy(Hold,tokens);                                            // save coord
        strcat(newLine,tokens);
    }
    strcat(newLine," ");  
    tokens = strtok (NULL, " =\n"); 

    //Get param value name
    if (comp == 'B') { 
        tokens = strtok (NULL, " =\n");
        tokens = strtok (NULL, " =\n");
        tokens = strtok (NULL, " =\n");
        strcpy(Hold,tokens);             
    } 
    else if ((comp == 'L')&&(strstr(tokens, "p") == NULL)){ 
        strcpy(Hold,tokens);             
    }
    // Rewrite component values to match InductEx input format
    if (strstr(tokens, "p") != NULL) {
        tokens[strlen(tokens)-1] = ' ';     //removes the p
        strcat(newLine,tokens);
        strcat(newLine,"\n");
    }
    else if(strncmp(Hold,"B",1)&&(comp == 'B')){ // parse values from JJ
        TempPval = atof(tokens);
        TempPval = TempPval *100;
        gcvt(TempPval,10,tokens); 
        strcat(newLine,tokens);
        strcat(newLine,"\n");
    }
    else if (strstr(tokens, "n") != NULL) {
        tokens[strlen(tokens)-1] = ' ';     //removes the n and convert to p 
        TempPval = atof(tokens);
        TempPval = TempPval * pow(10,(-3));
        gcvt(TempPval,10,tokens); 
        strcat(newLine,tokens);
        strcat(newLine,"\n");
    }
    else if (strstr(tokens, "u") != NULL) {
        tokens[strlen(tokens)-1] = ' ';     //removes the u and convert to p
        TempPval = atof(tokens);
        TempPval = TempPval * pow(10,(-6));
        gcvt(TempPval,10,tokens); 
        strcat(newLine,tokens);
        strcat(newLine,"\n");
    }
    else if (strstr(tokens, "e") != NULL) { // using e-xx in component value
        char vals[20];
        char pw[4];
        sscanf( tokens, "%[^e]e%[^\n]\n", vals, pw);     
        int powers = atoi(pw); 
        TempPval = atof(vals);
        TempPval = TempPval * pow(10,(12+powers));
        gcvt(TempPval,10,tokens); 
        strcat(newLine,tokens);
        strcat(newLine,"\n");
    }
    else if (strstr(tokens, "E") != NULL) { // using E-xx in component value
        char vals[20];
        char pw[4];
        sscanf( tokens, "%[^E]E%[^\n]\n", vals, pw);     
        int powers = atoi(pw); 
        TempPval = atof(vals);
        TempPval = TempPval * pow(10,(12+powers));
        gcvt(TempPval,10,tokens); 
        strcat(newLine,tokens);
        strcat(newLine,"\n");
    }
    else if(strstr(INbuffer,Hold)){ 
        strcat(Hold,"=");                   //last caracter must be =
        char *ParLine = strstr(INbuffer,Hold);
        char Pname[20];
        char Pval[30];
        char Pscale[20];
        char Pwr[4];
        double Cval; 

        if(comp == 'L'){ //
            if(strncmp(ParLine,"Scaling",7) == 0){
                if(strstr(tokens, "E") != NULL){
                    sscanf( ParLine, "%[^=]=%[^E]E%[^*]*", Pname, Pval, Pwr);
                }
                else{
                    sscanf( ParLine, "%[^=]=%[^e]e%[^*]*", Pname, Pval, Pwr);
                }   
            }
            else
            {
                if(strstr(tokens, "E") != NULL){
                    sscanf( ParLine, "%[^=]=%[^E]E%[^\n]\n", Pname, Pval, Pwr);
                }
                else{
                    sscanf( ParLine, "%[^=]=%[^e]e%[^\n]\n", Pname, Pval, Pwr);
                } 
            }
                    
            int power = atoi(Pwr); 
            Cval = atof(Pval);
            Cval =  Cval * pow(10,(12+power)); 
        }
        else if(comp == 'B'){
            if(strncmp(ParLine,"Scaling",7) == 0){
                sscanf( ParLine, "%[^=]=%[^*]*%[^\n]\n", Pname, Pval,Pscale);
            }
            else
            {
                sscanf( ParLine, "%[^=]=%[^\n]\n", Pname, Pval);
            }
            
            Cval = atof(Pval);
            Cval = Cval * 100; // change for ic * a           
        } 
        gcvt(Cval,10,tokens);      
        strcat(newLine,tokens);
        strcat(newLine,"\n");
    }
    // Remove resistance 
    if(!(comp == 'R'))
        fputs(newLine,OUT);
    // delete [] tokens;
    return;
}

// load the data from the XIC generated circuit file to the buffer.
//
char 
*LoadBuf(char * dataIN){
    FILE *Din = fopen (dataIN , "r");
    if (dataIN==NULL){
        PL()->ShowPromptV("ERROR: Circuit file not read");
        Errs()->sys_error("ERROR: Circuit file not read");
        return 0;
    }

    // find file size
    fseek(Din , 0 , SEEK_END);
    size_t Csize = ftell(Din);
    rewind(Din);

    // alocate memory and store data in buffer
    char * CIRbuf = (char*) malloc(sizeof(char)*Csize);
    if (CIRbuf == NULL){
        PL()->ShowPromptV("ERROR: No data to read");
        Errs()->sys_error("ERROR: No data to read");
        return 0;  
    }

    //read data    
    size_t CIRdata = fread (CIRbuf,1,Csize,Din);
    if(CIRdata != Csize){
        PL()->ShowPromptV("ERROR: Circuit file is empty");
        Errs()->sys_error("ERROR: Circuit file is empty");
        return 0;
    }

    fclose (Din);
    return (char *) CIRbuf;
} 

// The function create port for selected components.
//
void 
Port_val(int portNumber, const char * PortName,char *NewLine, char *fileLine,FILE *OUT){
    char PortNum [10];
    char Hold[20];
    sprintf(PortNum, "%d", portNumber); 
    strcpy(NewLine,PortName);
    strcat(NewLine,PortNum);
    portNumber++;                     
    strcat(NewLine," ");
    char *seg = strtok (fileLine," ");
    seg = strtok (NULL, " ");
    if(!(strncmp(PortName,"PB",2))){
        strcpy(Hold,seg);
        seg = strtok (NULL, " ");    
        strcat(NewLine,seg);
        strcat(NewLine," ");
        strcat(NewLine,Hold);
    } else if(!(strncmp(PortName,"PR",2))){
        strcat(NewLine,seg);
        strcat(NewLine," ");
        seg = strtok (NULL, " ");    
        strcat(NewLine,seg);
    } else {
        strcat(NewLine,seg);
        strcat(NewLine," 0");
    }           
    strcat(NewLine,"\n");
    fputs(NewLine,OUT);
    //delete [] seg;
    return;
}
// The function process the XIC generated circuit file and 
// write it into a format acceptable by InductEX
//
void 
inductexOUT(char * cirIN, char * cirOUT)
{
    char fileString [256];
    char ParLine [256];
    char New_line [256];
    char VecTest [256];
    int pn = 1;                                     // port count
    int rn = 1;                                     // resistor port count
    int ib = 1;                                     // curretnt port count

    char * INbuff = LoadBuf(cirIN);
    FILE *pOUT = fopen (cirOUT , "w");  
    FILE *pIN = fopen (cirIN , "r");

    //Fix rest using vector
    std::vector<std::string> PortID;
    std::vector<std::string> PortCoord;   

    while ( fgets (fileString , 256 , pIN) != NULL ){
        if(!(strncmp(fileString,"*",1))){           
            fputs(fileString,pOUT);  
        }
        if(!(strncmp(fileString,"K",1)) || !(strncmp(fileString,"k",1))){ // includes the Mutual Inductance term for inductEx          
            fputs(fileString,pOUT);  
        }
        if(!(strncmp(fileString,".",1))){           
        // do Nothing,avoids false positives from param
        }
        else if(!(strncmp(fileString,"B",1))){                         
            Param_val(fileString, INbuff,New_line, pOUT,PortID,PortCoord); 
        
        }
        else if(!(strncmp(fileString,"IB",1))){                         
            Port_val(ib,"PB",New_line,fileString,pOUT);
            ib++;
        }
        // clk is named first and is the first port always.
        else if (strstr(fileString, "CLK") != NULL){ 
            strcpy(ParLine,fileString);
            Param_val(fileString, INbuff,New_line, pOUT,PortID,PortCoord); 
            Port_val(pn,"P",New_line,ParLine,pOUT);
            pn++;                           
        }
        else if ((strstr(fileString, " IN ") != NULL) || (strstr(fileString, "IN_") != NULL)){       
            strcpy(ParLine,fileString);
            Param_val(fileString, INbuff,New_line, pOUT,PortID,PortCoord); 
            Port_val(pn,"P",New_line,ParLine,pOUT);
            pn++;                           
        }
        else if (strstr(fileString, "RP") != NULL){       
            Port_val(rn,"PR",New_line,fileString,pOUT);
            rn++;
        }
        else if ((strstr(fileString, " OUT ") != NULL) || (strstr(fileString, "OUT_") != NULL)){       
            Port_val(pn,"P",New_line,fileString,pOUT); 
            pn++; 
        }
        else if (!(strncmp(fileString,"L",1))){       
            if (strstr(fileString, "LRB") == NULL){      
                Param_val(fileString, INbuff,New_line, pOUT,PortID,PortCoord);
            }
        }
        else if (!(strncmp(fileString,"R",1)) && (strstr(fileString, " P") != NULL)){       
            Param_val(fileString, INbuff,New_line, pOUT,PortID,PortCoord); 
        }
    }
    // Add ports to output file
    int np = 0;
    std::vector<std::string>::iterator i;
    for ( i = PortID.begin() ; i < PortID.end() ; ++i ){
        strcpy(VecTest,PortID[np].c_str());
        strcat(VecTest," ");
        strcat(VecTest,PortCoord[np].c_str());
        strcat(VecTest," 0\n");
        fputs(VecTest,pOUT);
        np++;
    }
    PortID.clear();
    fclose (pIN);
    fclose (pOUT);
    free (INbuff);
    return;
}

//-----------------------------------------------------------------------------
// The Parse JoSIM Input.
// 
// Dump the .cir file in a format compatible with JoSIM.
// 

void ParseJosim(char * cirIN, char * cirOUT)
{
    char JosimString [256];
    char ParLineJJ [256];
    char PlotVol [256];
    char PlotPhase [256];
    bool CirBreak = 0;
    // char PlotCurrent [256];

    char * INbuffJJ = LoadBuf(cirIN); 
    FILE *jOUT = fopen (cirOUT , "w");  
    FILE *jIN = fopen (cirIN , "r");

    std::vector<std::string> PlotData;   
    std::vector<std::string> PlotJJ;
    // std::vector<std::string> PlotTest;
    

    while ( fgets (JosimString , 256 , jIN) != NULL ){
        if(!(strncmp(JosimString,".plot",5))){         
            strcpy(ParLineJJ,JosimString);
            char *tokens = strtok (ParLineJJ," v()\n\"");     // .plot
            tokens = strtok (NULL," v()\n\"");                // tran

            for(tokens = strtok (NULL," v()\n\"");tokens != NULL;tokens = strtok (NULL," v()\n\""))
            {
                PlotData.push_back(tokens);
            }
        }
        else
        {
            fputs(JosimString,jOUT);
        }
        if(!(strncmp(JosimString,".subckt",7))){ // skip sub-circuit 
            CirBreak = 1;
        }

        if (!(strncmp(JosimString,"B",1)) && !CirBreak){
            strcpy(ParLineJJ,JosimString);
            char *tokenb = strtok (ParLineJJ," ");
            PlotJJ.push_back(tokenb);
            tokenb = strtok (NULL," ");
            tokenb = strtok (NULL," ");
            tokenb = strtok (NULL," ");
            PlotJJ.push_back(tokenb);
        }   
    }
    // Add ports to output file
    int kp = 0;
    // strcpy(PlotCurrent,".plot ");
    
    std::vector<std::string>::iterator i;
    std::vector<std::string>::iterator j;
    for ( j = PlotData.begin() ; j < PlotData.end() ; ++j ){
        i = std::find (PlotJJ.begin(), PlotJJ.end(), PlotData[kp]); 
        if (i != PlotJJ.end())
        {
            strcpy(PlotPhase,".plot PHASE ");
            strcat(PlotPhase,PlotJJ[i - PlotJJ.begin() - 1].c_str());
            strcat(PlotPhase," \n");
            fputs(PlotPhase,jOUT);
        }
        else //add branch
        {
            strcpy(PlotVol,".plot tran ");
            strcat(PlotVol,"v(");
            strcat(PlotVol, PlotData[kp].c_str());
            strcat(PlotVol,") \n");
            fputs(PlotVol,jOUT);
        }
        kp++;
    }
    PlotData.clear();
    PlotJJ.clear();
    fclose (jIN);
    fclose (jOUT);
    free (INbuffJJ);
    return;
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
// directory and simulated using JoSIM. The location of JoSIM is selected via the
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
    PL()->ErasePrompt();
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

    char *in = pathlist::expand_path(tbuf, true, true);
    in = lstring::strip_space(in);
    pathlist::path_canon(in);

    char *filename = pathlist::expand_path(in, false, true);
    SCD()->dumpSpiceFile(filename);
    PL()->ShowPrompt("DONE");

    // create path for JoSIM output file 
    char oubuf[256];
    strcpy(oubuf, Tstring(DSP()->CurCellName()));
    char *scj;
    if ((scj = strrchr(oubuf, '.')) != 0) {
        if (!strcmp(scj, "_josim.cir"))
            strcpy(scj, "_josim.deck");
        else
            strcpy(scj, "_josim.cir");
    }
    else
        strcat(oubuf, "_josim.cir");

    char *outJJ = pathlist::expand_path(oubuf, true, true);
    outJJ = lstring::strip_space(outJJ);
    pathlist::path_canon(outJJ);

    char *cirJJ = pathlist::expand_path(outJJ, false, true);
    //delete [] sc

    const char *JoSim_file = "josim";
    const char *JoSim_file_cap = "josim-cli";
    char *inJoSIM = pathlist::expand_path("/usr/local/bin", false, true);

    bool JoSIM_check_linux = pathlist::find_path_file(JoSim_file, "/usr/local/bin",NULL,true);
    bool JoSIM_check_mac = pathlist::find_path_file(JoSim_file_cap, "/usr/local/bin",NULL,true);

    //check path
    bool inJoSIMlib = false;
    // Choose the location of JoSIM
    if(JoSIM_check_linux)
        inJoSIM = pathlist::expand_path("/usr/local/bin/josim", false, true);
    else if(JoSIM_check_mac)
        inJoSIM = pathlist::expand_path("/usr/local/bin/josim-cli", false, true);
    else {
        inJoSIMlib = true;
        // strcpy (jbuf, "");
        // char *inJo = XM()->OpenFileDlg("Open JoSIM Install Location: ", jbuf); 
        // inJoSIM = pathlist::expand_path(inJo, false, true);
        // if (!inJoSIM || !*inJoSIM) {
        //     PL()->ErasePrompt();
        //     return;
        // }
    }

    // Test if JoSIM has been selected
    const char *loc_J1 = inJoSIM + strlen(inJoSIM) - strlen(JoSim_file);
    const char *loc_J2 = inJoSIM + strlen(inJoSIM) - strlen(JoSim_file_cap);
    if ((strcmp(JoSim_file, loc_J1) != 0) && (strcmp(JoSim_file_cap, loc_J2) != 0) &&!(inJoSIMlib)){
        PL()->ShowPromptV("ERROR: JOSIM Not Found");
        Errs()->sys_error("ERROR: JOSIM Not Found");
        return;
    }

    // Load plotting data
    JoSIM_Flag = 1;
 
    // Fork process to run Josim
    int cpid = fork();
    if (cpid == -1) {
        Errs()->sys_error("init_local: fork");
        return;
    }
    if (!cpid) {
        ParseJosim(filename, cirJJ); // parse josim file
        if(inJoSIMlib){ 
            execlp("josim","josim","-c", "1", "-o","output", cirJJ, (char *) 0);
        }
        else      
            execl(inJoSIM,inJoSIM,"-c", "1", "-o","output", cirJJ, (char *) 0); 
        Errs()->sys_error("JoSIM execution failed");
        return;
    }
}
RunType Run_option();

//-----------------------------------------------------------------------------
// The RUN InductEx command  
//
// Submenu command for running InductEx. The circuit file and GDSII file is dumped  
// in the current directory. They are simulated using InductEx. The location of Inductex
// is the default as specified for windows.

void runInductExec(CmdDesc* cmd)
{
    if (!XM()->CheckCurMode(Electrical))
        return;
    if (!XM()->CheckCurCell(false, false, Electrical))
        return;
    if (CurCell()->isEmpty()) {
        PL()->ShowPrompt("No electrical data found in current cell.");
        return;
    }
    // dump spice netlist and load file name and address
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

    char *in = pathlist::expand_path(tbuf, true, true);
    in = lstring::strip_space(in);
    pathlist::path_canon(in);

    char *cirfile = pathlist::expand_path(in, false, true);
    SCD()->dumpSpiceFile(cirfile);
    //delete [] s

    // create path for IDX output file 
    char obuf[256];
    strcpy(obuf, Tstring(DSP()->CurCellName()));
    char *sc;
    if ((sc = strrchr(obuf, '.')) != 0) {
        if (!strcmp(sc, "_idx.cir"))
            strcpy(sc, "_idx.deck");
        else
            strcpy(sc, "_idx.cir");
    }
    else
        strcat(obuf, "_idx.cir");

    char *outC = pathlist::expand_path(obuf, true, true);
    outC = lstring::strip_space(outC);
    pathlist::path_canon(outC);

    char *cirIDX = pathlist::expand_path(outC, false, true);
    //delete [] sc
    
    // dump GDSII file
    if (cmd && Menu()->GetStatus(cmd->caller)) {
        if (!DSP()->CurCellName()) {
            PL()->ShowPrompt("No current cell!");
            if (cmd)
                Menu()->Deselect(cmd->caller);
            return;
        }
        CDvdb()->setVariable(VA_PCellKeepSubMasters, "");   // change the keep pcell option
        char * GDSloc = outGDS(0, 0);                       // no variables need can remove
        CDvdb()->clearVariable(VA_PCellKeepSubMasters);     // clean up pcell option
        
        // check if inductex is in default location
        const char *inductEXE = "inductex";
        bool Inductex_check = pathlist::find_path_file(inductEXE, "/usr/local/bin",NULL,true); // /utils/inductex/bin
        char *Induct = pathlist::expand_path("/usr/local/bin/inductex", false, true);       // /utils/inductex/bin

        bool inIDXlib = false; // look for in PATH

        if(!(Inductex_check)){
            inIDXlib = true;
            // PL()->ShowPromptV("ERROR: InductEx Not Found");
            // Errs()->sys_error("ERROR: InductEx Not Found");
            // return;
        }
        // open ldf file
        char Ibuf[256];
        strcpy (Ibuf, "");
        char *inI = XM()->OpenFileDlg("Open InductEx LDF File: ", Ibuf); 
        char *inIDX = pathlist::expand_path(inI, false, true);
        if (!strstr(inIDX, ".ldf")){
            PL()->ShowPromptV("ERROR: LDF File Not Found");
            Errs()->sys_error("ERROR: LDF File Not Found");
            return; 
        }
        // delete inI [];

        const char *methodIDX;
        char * met = PL()->EditPrompt("Choose Numerical Engine. TetraHenry-> t and FFH-> f : ", "");
        met = lstring::strip_space(met);
        if (met && (*met == 'f' || *met == 'F')){
            methodIDX = "-fh";
        }
        else if (met && (*met == 't' || *met == 'T')){
            methodIDX = "-th";
        }
        else{
            PL()->ShowPromptV("ERROR: Choose valid numerical engine");
            Errs()->sys_error("ERROR: Choose valid numerical engine");
            return;
        }

        PL()->ErasePrompt();
        PL()->ShowPrompt("Simulating in cmd terminal window");

        // Fork process to run Inductex
        int cpid = fork();
        if (cpid == -1) {
            Errs()->sys_error("init_local: fork");
            return;
        }
        if (!cpid) {
            inductexOUT(cirfile,cirIDX); //run cir to inductex cir
            if(inIDXlib){
                execlp("inductex","inductex",GDSloc, "-l", inIDX,"-i", "IDXout.inp",methodIDX,"-n", cirIDX,"-xic", (char *) 0); // replace cirTEST with cirfile and select ldf file option? //"mitll_sfq5ee_set1.ldf"
            }
            else
                execl(Induct,Induct,GDSloc, "-l", inIDX,"-i", "IDXout.inp",methodIDX,"-n", cirIDX,"-xic", (char *) 0); // replace cirTEST with cirfile and select ldf file option? //"mitll_sfq5ee_set1.ldf"
            Errs()->sys_error("Inductex Execution Failed");
            return;
        }   
    else   
        return;
    }   
}

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
    case runInductEx:
        runInductExec(cmd);                                                   
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


    // typedef std::map<std::string, std::string> StringMap;
    // StringMap map;
    // map.insert(StringMap::value_type("",""));
    // StringMap::iterator it = map.find("");
    // std::strimg value = it->second;


}


