
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
 * Xic Integrated Circuit Layout and Schematic Editor                     *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#ifndef SCED_MENU_H
#define SCED_MENU_H

#include "menu.h"

// Button Menu (Electrical)
enum
{
    btnElecMenu,
    btnElecMenuXform,
    btnElecMenuPlace,
    btnElecMenuDevs,
    btnElecMenuShape,
    btnElecMenuWire,
    btnElecMenuLabel,
    btnElecMenuErase,
    btnElecMenuBreak,
    btnElecMenuSymbl,
    btnElecMenuNodmp,
    btnElecMenuSubct,
    btnElecMenuTerms,
    btnElecMenuSpCmd,
    btnElecMenuRun,
    btnElecMenuDeck,
    btnElecMenuPlot,
    btnElecMenuIplot,
    btnElecMenu_END
};

// Ordering for the shapes pop-up menu.
enum ShapeType
    { ShBox, ShPoly, ShArc, ShDot, ShTri, ShTtri, ShAnd, ShOr, ShSides };

// Ordering for the run pop-up menu.
enum RunType
    { runJoSIM, runWRspice, runInductEx }; // ADDED button

#define    MenuDEVS      "devs"
#define    MenuSHAPE     "shape"
#define    MenuWIRE      "wire"
#define    MenuLABEL     "label"
#define    MenuERASE     "erase"
#define    MenuBREAK     "break"
#define    MenuSYMBL     "symbl"
#define    MenuNODMP     "nodmp"
#define    MenuSUBCT     "subct"
#define    MenuTERMS     "terms"
#define    MenuSPCMD     "spcmd"
#define    MenuRUN       "run"
#define    MenuDECK      "deck"
#define    MenuPLOT      "plot"
#define    MenuIPLOT     "iplot"
#define    MenuMUTUL     "mutul"

extern MenuFunc  M_ShowMutual;

#endif

