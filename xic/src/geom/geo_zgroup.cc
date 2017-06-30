
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2007 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *  WHITELEY RESEARCH INCORPORATED PROPRIETARY SOFTWARE                   *
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
 *                                                                        *
 * XIC Integrated Circuit Layout and Schematic Editor                     *
 *                                                                        *
 *========================================================================*
 $Id: geo_zgroup.cc,v 1.8 2015/10/11 19:35:45 stevew Exp $
 *========================================================================*/

#include "cd.h"
#include "cd_types.h"
#include "geo.h"
#include "geo_zgroup.h"
#include "geo_ylist.h"
#include "geo_poly.h"
#include "timedbg.h"


//------------------------------------------------------------------------
// Zgroup functions
//------------------------------------------------------------------------

// Return a PolyList for index ix, the zoids are consumed and zeroed.
//
PolyList *
Zgroup::to_poly_list(int ix, int max_verts)
{
    TimeDbgAccum ac("zg_to_poly_list");

    if (ix < 0 || ix >= num)
        return (0);
    Zlist *zl = list[ix];
    list[ix] = 0;

    PolyList *p0 = 0;
    if (zl) {

        Poly po;
        if (!zl->next) {
            if (zl->Z.mkpoly(&po.points, &po.numpts))
                p0 = new PolyList(po, 0);
            delete zl;
        }
        else {
            Ylist *y = new Ylist(zl);

            CD()->SetIgnoreIntr(true);
            y = Ylist::repartition_group(y);
            CD()->SetIgnoreIntr(false);

            while (y) {
                if (max_verts > 0 && Zlist::JoinBreakClean) {
                    y = Ylist::to_poly(y, &po.points, &po.numpts, 0);
                    if (po.numpts > max_verts) {
                        PolyList *pn = po.divide(max_verts);
                        PolyList *px = pn;
                        while (px->next)
                            px = px->next;
                        px->next = p0;
                        p0 = pn;
                        delete [] po.points;
                    }
                    else if (po.numpts)
                        p0 = new PolyList(po, p0);
                }
                else {
                    y = Ylist::to_poly(y, &po.points, &po.numpts, max_verts);
                    if (po.numpts)
                        p0 = new PolyList(po, p0);
                }
            }
        }
    }
    return (p0);
}


// Remove all the zoids, return a concatenated list.  'This' is destroyed!
//
Zlist *
Zgroup::zoids()
{
    Zlist *z0 = 0;
    for (int i = 0; i < num; i++) {
        Zlist *ze = list[i];
        if (!ze)
            continue;
        while (ze->next)
            ze = ze->next;
        ze->next = z0;
        z0 = list[i];
        list[i] = 0;
    }
    delete this;
    return (z0);
}


// Create an unnamed cell containing polygons formed from the groups. 
// Each polygon is given a group number.  'This' is destroyed.
//
CDs *
Zgroup::mk_cell(CDl *ld)
{
    if (!ld)
        return (0);
    CDs *sd = new CDs(0, Physical);
    for (int i = 0; i < num; i++) {
        PolyList *pl = Zlist::to_poly_list(list[i]);
        list[i] = 0;
        for (PolyList *p = pl; p; p = p->next) {
            // There should be only one polygon in the list.

            Poly po;
            po.numpts = p->po.numpts;
            po.points = p->po.points;
            p->po.points = 0;
            CDpo *newp = new CDpo(ld, &po);
            newp->set_group(i);
            sd->insert(newp);
        }
        PolyList::destroy(pl);
    }
    delete this;
    return (sd);
}

