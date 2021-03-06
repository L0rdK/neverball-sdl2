/*
 * Copyright (C) 2003-2010 Neverball authors
 *
 * NEVERBALL is  free software; you can redistribute  it and/or modify
 * it under the  terms of the GNU General  Public License as published
 * by the Free  Software Foundation; either version 2  of the License,
 * or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT  ANY  WARRANTY;  without   even  the  implied  warranty  of
 * MERCHANTABILITY or  FITNESS FOR A PARTICULAR PURPOSE.   See the GNU
 * General Public License for more details.
 */

#include <stdio.h>
#include <stdlib.h>

#include <SDL_video.h>
#include <SDL_syswm.h>

#include "syswm.h"
#include "base_config.h"
#include "image.h"
#include "lang.h"

/*---------------------------------------------------------------------------*/

void set_SDL_icon(SDL_Window *window, const char *filename)
{
#if !defined(__APPLE__) && !defined(_WIN32)
    SDL_Surface *icon;

    if ((icon = load_surface(filename)))
    {
        SDL_SetWindowIcon(window, icon);
        free(icon->pixels);
        SDL_FreeSurface(icon);
    }
#endif
    return;
}

void set_EWMH_icon(SDL_Window *window, const char *filename)
{
#if SDL_VIDEO_DRIVER_X11 && !SDL_VIDEO_DRIVER_QUARTZ
    SDL_SysWMinfo info;

    Display *dpy;
    Window   wdw;

    unsigned char *p;
    int w, h, b;

    SDL_VERSION(&info.version);

    if (SDL_GetWindowWMInfo(window, &info) != 1)
    {
        fprintf(stderr, L_("Failed to get WM info: %s\n"), SDL_GetError());
        return;
    }

    if (info.subsystem != SDL_SYSWM_X11)
        return;

    dpy    = info.info.x11.display;
    wdw = info.info.x11.window;

    /*
     * This code loads an image and sets it as the _NET_WM_ICON window
     * property, as described in the Extended Window Manager Hints
     * specification[*].  From the spec: "This is an array of 32-bit packed
     * CARDINAL ARGB with high byte being A, low byte being B.  The first two
     * cardinals are width, height.  Data is in rows, left to right and top to
     * bottom."
     *
     * [*] http://standards.freedesktop.org/wm-spec/latest/
     */

    if ((p = image_load(filename, &w, &h, &b)))
    {
        long *data = NULL;

        if ((data = calloc(2 + w * h, sizeof (long))))
        {
            int r, c;

            data[0] = w;
            data[1] = h;

            for (r = 0; r < h; r++)
                for (c = 0; c < w; c++)
                {
                    long          *dp = &data[2 + r * w + c];
                    unsigned char *pp = &p[(h - r - 1) * w * b + c * b];

                    if (b < 3)
                    {
                        if (b == 2)
                            *dp |= pp[1] << 24;

                        *dp |= pp[0] << 16;
                        *dp |= pp[0] << 8;
                        *dp |= pp[0] << 0;
                    }
                    else
                    {
                        if (b == 4)
                            *dp |= pp[3] << 24;

                        *dp |= pp[0] << 16;
                        *dp |= pp[1] << 8;
                        *dp |= pp[2] << 0;
                    }
                }

            /* FIXME: lock_func() and unlock_func() not available */

            /*
            info.info.x11.lock_func();
            {
                Atom icon = XInternAtom(dpy, "_NET_WM_ICON", False);

                XChangeProperty(dpy, wdw, icon, XA_CARDINAL, 32,
                                PropModeReplace, (unsigned char *) data,
                                2 + w * h);
            }
            info.info.x11.unlock_func();
            */

            free(data);
        }
        else
            fputs(L_("Failed to allocate memory for EWMH icon data.\n"), stderr);

        free(p);
    }
#endif

    return;
}

/*---------------------------------------------------------------------------*/

