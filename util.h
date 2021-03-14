#ifndef UTIL_H
#define UTIL_H

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <locale.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/cursorfont.h>
#include <X11/Xmu/WinUtil.h>
#include <glib.h>
#include "lib/c-vector/cvector.h"

/* if this is defined, then the vector will double in capacity each
 * time it runs out of space. if it is not defined, then the vector will
 * be conservative, and will have a capcity no larger than necessary.
 * having this defined will minimize how often realloc gets called.
 */
#define CVECTOR_LOGARITHMIC_GROWTH
#define MAX_PROPERTY_VALUE_LEN 4096

#define p_verbose(...)                \
    if (1)                            \
    {                                 \
        fprintf(stderr, __VA_ARGS__); \
    }

static gboolean envir_utf8;

struct Win
{
    char *id;
    unsigned long pid;
    int is_minimized;
};
typedef struct Win Win;

gchar *get_property(Display *disp, Window win,
                    Atom xa_prop_type, gchar *prop_name, unsigned long *size)
{
    Atom xa_prop_name;
    Atom xa_ret_type;
    int ret_format;
    unsigned long ret_nitems;
    unsigned long ret_bytes_after;
    unsigned long tmp_size;
    unsigned char *ret_prop;
    gchar *ret;

    xa_prop_name = XInternAtom(disp, prop_name, False);

    /* MAX_PROPERTY_VALUE_LEN / 4 explanation (XGetWindowProperty manpage):
     *
     * long_length = Specifies the length in 32-bit multiples of the
     *               data to be retrieved.
     */
    if (XGetWindowProperty(disp, win, xa_prop_name, 0, MAX_PROPERTY_VALUE_LEN / 4, False,
                           xa_prop_type, &xa_ret_type, &ret_format,
                           &ret_nitems, &ret_bytes_after, &ret_prop) != Success)
    {
        p_verbose("Cannot get %s property.\n", prop_name);
        return NULL;
    }

    if (xa_ret_type != xa_prop_type)
    {
        p_verbose("Invalid type of %s property.\n", prop_name);
        XFree(ret_prop);
        return NULL;
    }

    /* null terminate the result to make string handling easier */
    tmp_size = (ret_format / (32 / sizeof(long))) * ret_nitems;
    ret = g_malloc(tmp_size + 1);
    memcpy(ret, ret_prop, tmp_size);
    ret[tmp_size] = '\0';

    if (size)
    {
        *size = tmp_size;
    }

    XFree(ret_prop);
    return ret;
}

gchar *get_window_title(Display *disp, Window win)
{
    gchar *title_utf8;
    gchar *wm_name;
    gchar *net_wm_name;

    wm_name = get_property(disp, win, XA_STRING, "WM_NAME", NULL);
    net_wm_name = get_property(disp, win,
                               XInternAtom(disp, "UTF8_STRING", False), "_NET_WM_NAME", NULL);

    if (net_wm_name)
    {
        title_utf8 = g_strdup(net_wm_name);
    }
    else
    {
        if (wm_name)
        {
            title_utf8 = g_locale_to_utf8(wm_name, -1, NULL, NULL, NULL);
        }
        else
        {
            title_utf8 = NULL;
        }
    }

    g_free(wm_name);
    g_free(net_wm_name);

    return title_utf8;
}

gchar *get_output_str(gchar *str, gboolean is_utf8)
{
    gchar *out;

    if (str == NULL)
    {
        return NULL;
    }

    if (envir_utf8)
    {
        if (is_utf8)
        {
            out = g_strdup(str);
        }
        else
        {
            if (!(out = g_locale_to_utf8(str, -1, NULL, NULL, NULL)))
            {
                p_verbose("Cannot convert string from locale charset to UTF-8.\n");
                out = g_strdup(str);
            }
        }
    }
    else
    {
        if (is_utf8)
        {
            if (!(out = g_locale_from_utf8(str, -1, NULL, NULL, NULL)))
            {
                p_verbose("Cannot convert string from UTF-8 to locale charset.\n");
                out = g_strdup(str);
            }
        }
        else
        {
            out = g_strdup(str);
        }
    }

    return out;
}

gchar *get_window_class(Display *disp, Window win)
{
    gchar *class_utf8;
    gchar *wm_class;
    unsigned long size;

    wm_class = get_property(disp, win, XA_STRING, "WM_CLASS", &size);
    if (wm_class)
    {
        gchar *p_0 = strchr(wm_class, '\0');
        if (wm_class + size - 1 > p_0)
        {
            *(p_0) = '.';
        }
        class_utf8 = g_locale_to_utf8(wm_class, -1, NULL, NULL, NULL);
    }
    else
    {
        class_utf8 = NULL;
    }

    g_free(wm_class);

    return class_utf8;
}

Window *get_client_list(Display *disp, unsigned long *size)
{
    Window *client_list;

    if ((client_list = (Window *)get_property(disp, DefaultRootWindow(disp),
                                              XA_WINDOW, "_NET_CLIENT_LIST", size)) == NULL)
    {
        if ((client_list = (Window *)get_property(disp, DefaultRootWindow(disp),
                                                  XA_CARDINAL, "_WIN_CLIENT_LIST", size)) == NULL)
        {
            fputs("Cannot get client list properties. \n"
                  "(_NET_CLIENT_LIST or _WIN_CLIENT_LIST)"
                  "\n",
                  stderr);
            return NULL;
        }
    }

    return client_list;
}

Win *get_windows()
{
    Display *disp;

    if (!(disp = XOpenDisplay(NULL)))
    {
        fputs("Cannot open display.\n", stderr);
        return EXIT_FAILURE;
    }

    Window *client_list;
    unsigned long client_list_size;
    int i;
    int max_client_machine_len = 0;

    if ((client_list = get_client_list(disp, &client_list_size)) == NULL)
    {
        return EXIT_FAILURE;
    }

    /* find the longest client_machine name */
    for (i = 0; i < client_list_size / sizeof(Window); i++)
    {
        gchar *client_machine;
        if ((client_machine = get_property(disp, client_list[i],
                                           XA_STRING, "WM_CLIENT_MACHINE", NULL)))
        {
            max_client_machine_len = strlen(client_machine);
        }
        g_free(client_machine);
    }

    /* returned in the end */
    cvector_vector_type(Win) windows = NULL;

    /* Fetch the window details */
    for (i = 0; i < client_list_size / sizeof(Window); i++)
    {
        gchar *title_utf8 = get_window_title(disp, client_list[i]); /* UTF8 */
        gchar *title_out = get_output_str(title_utf8, TRUE);
        gchar *client_machine;
        gchar *class_out = get_window_class(disp, client_list[i]); /* UTF8 */
        unsigned long *pid;
        unsigned long *desktop;
        int x, y, junkx, junky;
        unsigned int wwidth, wheight, bw, depth;
        Window junkroot;

        /* desktop ID */
        if ((desktop = (unsigned long *)get_property(disp, client_list[i],
                                                     XA_CARDINAL, "_NET_WM_DESKTOP", NULL)) == NULL)
        {
            desktop = (unsigned long *)get_property(disp, client_list[i],
                                                    XA_CARDINAL, "_WIN_WORKSPACE", NULL);
        }

        /* client machine */
        client_machine = get_property(disp, client_list[i],
                                      XA_STRING, "WM_CLIENT_MACHINE", NULL);

        /* pid */
        pid = (unsigned long *)get_property(disp, client_list[i],
                                            XA_CARDINAL, "_NET_WM_PID", NULL);

        /* geometry */
        XGetGeometry(disp, client_list[i], &junkroot, &junkx, &junky,
                     &wwidth, &wheight, &bw, &depth);
        XTranslateCoordinates(disp, client_list[i], junkroot, junkx, junky,
                              &x, &y, &junkroot);

        /* window id */
        const char *window_id = sprintf("0x%.8lx", client_list[i]);

        /* window is minimized */
        gchar *window_state = get_property(disp, client_list[i], XA_STRING, "_NET_WM_STATE", NULL);
        printf("%s", window_state);

        /* TODO: Fix this
         * Change `0` to the proper window state's maximized or minimized
         * value
         */
        Win win = {window_id, *pid, 0};
        cvector_push_back(windows, win);

        g_free(title_utf8);
        g_free(title_out);
        g_free(desktop);
        g_free(client_machine);
        g_free(class_out);
        g_free(pid);
    }
    g_free(client_list);
}

#endif