
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
 * Misc. Utilities Library                                                *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#include "help_defs.h"
#include "help_context.h"
#include "help_pkgs.h"
#include "miscutil/lstring.h"
#include "miscutil/pathlist.h"
#include "miscutil/miscutil.h"
#include "httpget/transact.h"

//
// Implementation of package management for XicTools.
//


#ifndef OSNAME
#define OSNAME "unknown"
#endif
#ifndef ARCH
#define ARCH "unknown"
#endif

namespace {
    void find_pkg(const char *prog, stringlist **l1, stringlist **l2)
    {
        while (*l1) {
            // 9 == strlen*("xictools_")
            if (lstring::prefix(prog, (*l1)->string + 9))
                break;
            *l1 = (*l1)->next;
        }
        while (*l2) {
            // 9 == strlen*("xictools_")
            if (lstring::prefix(prog, (*l2)->string + 9))
                break;
            *l2 = (*l2)->next;
        }
    }

    bool newerpkg(stringlist *loc, stringlist *avl)
    {
        if (!loc || !avl)
            return (false);
        const char *locpkg = loc->string;
        const char *avlpkg = avl->string;

        delete [] lstring::gettok(&locpkg, "-");  // xictools_prog
        delete [] lstring::gettok(&locpkg, "-");  // osname
        char *vrs = lstring::gettok(&locpkg, "-");// version
        int lgen, lmaj, lmin;
        int n = sscanf(vrs, "%d.%d.%d", &lgen, &lmaj, &lmin);
        delete [] vrs;
        if (n != 3)
            return (false);

        delete [] lstring::gettok(&avlpkg, "-");  // xictools_prog
        delete [] lstring::gettok(&avlpkg, "-");  // osname
        vrs = lstring::gettok(&avlpkg, "-");      // version
        int agen, amaj, amin;
        n = sscanf(vrs, "%d.%d.%d", &agen, &amaj, &amin);
        delete [] vrs;
        if (n != 3)
            return (false);
        if (agen > lgen)
            return (true);
        if (agen < lgen)
            return (false);
        if (amaj > lmaj)
            return (true);
        if (amaj < lmaj)
            return (false);
        if (amin > lmin)
            return (true);
        return (false);
    }
}


// Compose an HTML page listing the installed packages, and packages
// that are available on the distribution site.  The buttons allow
// initiation of downloading/installation of selected packages.
//
char *
pkgs::pkgs_page()
{
    stringlist *local = local_pkgs();
    stringlist *avail = avail_pkgs();

    const char *pkgs[] = { "adms", "fastcap", "fasthenry", "mozy", "mrouter",
        "vl", "wrspice", "xic", 0 };

    sLstr lstr;
    lstr.add("<html>\n<head>\n<body bgcolor=#ffffff>\n");
    lstr.add("<h2><i>XicTools</i> Packages</h4>\n");
    lstr.add("<blockquote>\n");

    lstr.add("<form method=get action=\":xt_download\">\n");
    lstr.add("<table cellpadding=4>\n");
    lstr.add("<tr><th>Installed</th><th>Available</th><th>download</th></tr>\n");
    for (const char **pn = pkgs; *pn;  pn++) {
        const char *prog = *pn;
        stringlist *locpkg = local;
        stringlist *avlpkg = avail;
        find_pkg(prog, &locpkg, &avlpkg);
        if (!avlpkg && !locpkg)
            continue;
        bool newer = newerpkg(locpkg, avlpkg);
        if (newer) {
            lstr.add("<tr><td bgcolor=#fff0f0>");
        }
        else {
            lstr.add("<tr><td bgcolor=#f0fff0>");
        }
        lstr.add(locpkg->string);
        lstr.add("</td><td>");
        lstr.add(avlpkg->string);
        lstr.add("</td><td>");
        lstr.add("<center><input type=checkbox name=d_");
        lstr.add(avlpkg->string);
        lstr.add("></center>");
        lstr.add("</td></tr>\n");
    }
    lstr.add("</table>\n");

    lstr.add("<input type=submit value=\"Download\">\n");
    lstr.add("<input type=submit value=\"Download and Install\">\n");
    lstr.add("</form>\n");
    lstr.add("</body>\n</html>\n");
    return (lstr.string_trim());
}


// Fork a terminal and start the installation process.
//
int
pkgs::xt_install(dl_elt_t *dlist)
{
    if (!dlist)
        return (-1);
    sLstr lstr;
    lstr.add("/bin/bash ");
    for (dl_elt_t *d = dlist; d; d = d->next) {
        if (lstr.string()) {
            lstr.add_c(' ');
            lstr.add(d->filename);
        }
        else {
            lstr.add(d->filename);
//XXX
            lstr.add(" -t");
        }
    }
    // The lstr contains the command string, since the first element
    // is the wr_install script, and remaining elements are package
    // files.
    
    char *cmd = lstr.string_trim();

    int pid = miscutil::fork_terminal(cmd);
    delete [] cmd;

    if (pid > 0) {
        int status = 0;
#ifdef HAVE_SYS_WAIT_H
        for (;;) {
            int rpid = waitpid(pid, &status, 0);
            if (rpid == -1 && errno == EINTR)
                continue;
            break;
        }
#endif
        return (status);
    }
    return (-1);
}


namespace {
    char *get_osname(const char *pkgname)
    {
        const char *t = strchr(pkgname, '-');
        if (!t)
            return (0);
        t++;
        const char *e = strchr(t, '-');
        int len = e ? e-t : strlen(t);
        char *n = new char[len+1];
        strncpy(n, t, len);
        n[len] = 0;
        return (n);
    }


    char *get_version(const char *pkgname)
    {
        const char *t = strchr(pkgname, '-');
        if (!t)
            return (0);
        t++;
        t = strchr(t, '-');
        if (!t)
            return (0);
        t++;
        const char *e = strchr(t, '-');
        int len = e ? e-t : strlen(t);
        char *n = new char[len+1];
        strncpy(n, t, len);
        n[len] = 0;
        return (n);
    }


    char *get_arch(const char *pkgname)
    {
        const char *t = strchr(pkgname, '-');
        if (!t)
            return (0);
        t++;
        t = strchr(t, '-');
        if (!t)
            return (0);
        t++;
        t = strchr(t, '-');
        if (!t)
            return (0);
        t++;
        const char *e = strchr(t, '-');
        int len = e ? e-t : strlen(t);
        char *n = new char[len+1];
        strncpy(n, t, len);
        n[len] = 0;
        return (n);
    }
}


// Construct the url to download the package whose name is passed.
//
char *
pkgs::get_download_url(const char *pkgname)
{
    if (!pkgname)
        return (0);
    if (!strcmp(pkgname, "wr_install")) {
        sLstr lstr;
        lstr.add("http://wrcad.com/xictools/");
        lstr.add("scripts");
        lstr.add_c('/');
        lstr.add(pkgname);
        return (lstr.string_trim());
    }
    char *osn = get_osname(pkgname);
    if (!osn)
        return (0);
    char *arch = get_arch(pkgname);
    if (!arch) {
        delete [] osn;
        return (0);
    }
    const char *sfx = 0;
    if (lstring::prefix("Linux", osn))
        sfx = ".rpm";
    else if (lstring::prefix("Darwin", osn))
        sfx = ".pkg";
    else if (lstring::prefix("Win", osn))
        sfx = ".exe";
    else {
        delete [] osn;
        delete [] arch;
        return (0);
    }
    if (strcmp(arch, "x86_64")) {
        char *t = new char[strlen(osn) + strlen(arch) + 2];
        char *e = lstring::stpcpy(t, osn);
        delete [] osn;
        osn = t;
        *e++ = '.';
        strcpy(e, arch);
    }
    sLstr lstr;
    lstr.add("http://wrcad.com/xictools/");
    lstr.add(osn);
    lstr.add_c('/');
    lstr.add(pkgname);
    lstr.add(sfx);
    return (lstr.string_trim());
}


// Return a raw list pf packages available for download.  The first
// list element is actually the OSNAME.
//
stringlist *
pkgs::avail_pkgs()
{
    Transaction trn;

    // Create the url:
    //    http://wrcad.com/cgi-bin/cur_release.cgi?h=osname
    //
    char buf[256];
    char *e = lstring::stpcpy(buf,
        "http://wrcad.com/cgi-bin/cur_release.cgi?h=");
    e = lstring::stpcpy(e, OSNAME);
    if (strcmp(ARCH, "x86_64")) {
        *e++ = '.';
        strcpy(e, ARCH);
    }
    trn.set_url(buf);

    trn.set_timeout(5);
    trn.set_retries(0);
    trn.set_http_silent(true);

    trn.transact();
    char *s = trn.response()->data;
    stringlist *list = 0;
    if (s && *s) {
        const char *ss = s;
        const char *osn = 0;
        const char *arch = 0;
        char *t;
        while ((t = lstring::gettok(&ss)) != 0) {
            if (!osn) {
                osn = t;
                char *a = strchr(t, '.');
                if (a) {
                    *a++ = 0;
                    arch = a;
                }
                else
                    arch = "x86_64";
                continue;
            }
            char *vrs = strchr(t, '-');
            if (!vrs)
                break;
            *vrs++ = 0;
            sprintf(buf, "xictools_%s-%s-%s-%s", t, osn, vrs, arch);
            list = new stringlist(lstring::copy(buf), list);
            delete [] t;
        }
        if (list && list->next)
            stringlist::sort(list);
    }
    return (list);
}


// Return a raw list of the installed XicTools packages.
//
stringlist *
pkgs::local_pkgs()
{
#ifdef WIN32
#else
#ifdef __APPLE__
#else
    stringlist *list = 0;
    char buf[256];
    FILE *fp = popen("rpm -qa | grep xictools_", "r");
    if (fp) {
        char *s;
        while ((s = fgets(buf, 256, fp)) != 0)
            list = new stringlist(lstring::copy(s), list);
        if (list && list->next)
            stringlist::sort(list);
        fclose(fp);
    }
    return (list);

#endif
#endif
}


// Return a page showing the XicTools packages available for download.
//
char *
pkgs::list_avail_pkgs()
{
    sLstr lstr;
    lstr.add("<html>\n<head>\n<body bgcolor=#ffffff>\n");
    lstr.add("<h4>Available Packages</h4>\n");
    lstr.add("<blockquote>\n");

    Transaction trn;

    // Create the url:
    //    http://wrcad.com/cgi-bin/cur_release.cgi?h=osname
    //
    char buf[256];
    char *e = lstring::stpcpy(buf,
        "http://wrcad.com/cgi-bin/cur_release.cgi?h=");
    const char *osname = e;
    e = lstring::stpcpy(e, OSNAME);
    if (strcmp(ARCH, "x86_64")) {
        *e++ = '.';
        strcpy(e, ARCH);
    }
    trn.set_url(buf);

    trn.set_timeout(5);
    trn.set_retries(0);
    trn.set_http_silent(true);

    trn.transact();
    char *s = trn.response()->data;
    if (s && *s) {
        const char *ss = s;
        const char *osn = 0;
        const char *arch = 0;
        char *t;
        while ((t = lstring::gettok(&ss)) != 0) {
            if (!osn) {
                osn = t;
                char *a = strchr(t, '.');
                if (a) {
                    *a++ = 0;
                    arch = a;
                }
                else
                    arch = "x86_64";
                continue;
            }
            char *vrs = strchr(t, '-');
            if (!vrs)
                break;
            *vrs++ = 0;
            sprintf(buf, "xictools_%s-%s-%s-%s", t, osn, vrs, arch);
            lstr.add("<a href=\":xt_get?f=");
            lstr.add(buf);
            lstr.add("\">");
            lstr.add(buf);
            lstr.add("</a><br>");
            delete [] t;
        }
        lstr.add("<p>Click to download / install.");
    }
    else {
        lstr.add("Sorry, prebuilt packages for ");
        lstr.add(osname);
        lstr.add(" are not currently available.");
    }
    lstr.add("</blockquote>\n");
    lstr.add("</body>\n</html>\n");
    return (lstr.string_trim());
}


// Return a page showing the currently installed XicTools packages.
//
char *
pkgs::list_cur_pkgs()
{
    sLstr lstr;
    lstr.add("<html>\n<head>\n<body bgcolor=#ffffff>\n");
    lstr.add("<h4>Currently Installed Packages</h4>\n");
    lstr.add("<blockquote>\n");
#ifdef WIN32
#else
#ifdef __APPLE__
#else
    char buf[256];
    FILE *fp = popen("rpm -qa | grep xictools_", "r");
    if (fp) {
        char *s;
        stringlist *s0 = 0;
        while ((s = fgets(buf, 256, fp)) != 0)
            s0 = new stringlist(lstring::copy(s), s0);
        stringlist::sort(s0);
        for (stringlist *sl = s0; sl; sl = sl->next) {
            lstr.add(sl->string);
            lstr.add("<br>");
        }
        stringlist::destroy(s0);
        fclose(fp);
    }
    else
        lstr.add("Error: rpm database query failed.");

#endif
#endif
    lstr.add("</blockquote>\n");
    lstr.add("</body>\n</html>\n");
    return (lstr.string_trim());
}

