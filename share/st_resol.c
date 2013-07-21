/*
 * Copyright (C) 2003 Robert Kooima - 2006 Jean Privat
 * Part of the Neverball Project http://icculus.org/neverball/
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


#include <string.h>

#include <SDL.h>

#include "gui.h"
#include "geom.h"
#include "part.h"
#include "audio.h"
#include "config.h"
#include "video.h"

#include "st_resol.h"

extern struct state  st_null;
static struct state *st_back;

static SDL_DisplayMode **modes;

/*---------------------------------------------------------------------------*/

enum
{
    RESOL_BACK = 1,
    RESOL_SELECT
};

static int resol_action(int tok, int val)
{
    int r = 1;

    audio_play("snd/menu.ogg", 1.0f);

    switch (tok)
    {
    case RESOL_BACK:
        goto_state(st_back);
        st_back = NULL;
        break;

    case RESOL_SELECT:
        goto_state(&st_null);
        r = video_mode(config_get_d(CONFIG_FULLSCREEN),
                       modes[val]->w, modes[val]->h);
        goto_state(&st_resol);
        break;
    }

    return r;
}

static int fill_row(int id, SDL_DisplayMode **modes, int i, int n)
{
    int complete;

    if (n == 0)
        return 1;

    if (modes[i])
    {
        char label[20];
        int btn;

        sprintf(label, "%d x %d", modes[i]->w, modes[i]->h);

        complete = fill_row(id, modes, i + 1, n - 1);

        btn = gui_state(id, label, GUI_SML, RESOL_SELECT, i);

        gui_set_hilite(btn, (config_get_d(CONFIG_WIDTH)  == modes[i]->w &&
                             config_get_d(CONFIG_HEIGHT) == modes[i]->h));
    }
    else
    {
        for (; n; gui_space(id), n--);
        complete = 0;
    }

    return complete;
}

static int resol_gui(void)
{
    int id, jd;

    if ((id = gui_vstack(0)))
    {
        if ((jd = gui_harray(id)))
        {
            gui_label(jd, _("Resolution"), GUI_SML, 0, 0);
            gui_space(jd);
            gui_start(jd, _("Back"),       GUI_SML, RESOL_BACK, 0);
        }

        gui_space(id);

        if (modes)
        {
            int i;

            for (i = 0; fill_row(gui_harray(id), modes, i, 4); i += 4);
        }

        gui_layout(id, 0, 0);
    }

    return id;
}

static void list_modes()
{
    int i, nmodes;
    SDL_DisplayMode mode;
    SDL_DisplayMode cur_mode;

    SDL_GetCurrentDisplayMode(0, &cur_mode);

    nmodes = 0;
    modes = NULL;
    for (i = 0; i < SDL_GetNumDisplayModes(0); ++i)
    {
        SDL_GetDisplayMode(0, i, &mode);
        if (!mode.w || !mode.h)
        {
            return;
        }
        if (SDL_BITSPERPIXEL(mode.format) !=
            SDL_BITSPERPIXEL(cur_mode.format))
        {
            continue;
        }
        if (nmodes > 0 && modes[nmodes - 1]->w == mode.w
            && modes[nmodes - 1]->h == mode.h)
        {
            continue;
        }

        modes = SDL_realloc(modes, (nmodes + 2) * sizeof(*modes));
        if (!modes)
        {
            return;
        }

        modes[nmodes] = SDL_malloc(sizeof(SDL_DisplayMode));
        if (!modes[nmodes])
        {
            return;
        }
        *modes[nmodes] = mode;
        ++nmodes;
    }

    if (modes) {
        modes[nmodes] = NULL;
    }
}

static void free_modes()
{
    int i;

    for (i = 0; modes[i]; ++i) {
        SDL_free(modes[i]);
    }

    SDL_free(modes);
}

static int resol_enter(struct state *st, struct state *prev)
{
    if (!st_back)
    {
        /* Note the parent screen if not done yet. */

        st_back = prev;
    }

    back_init("back/gui.png");

    list_modes();

    audio_music_fade_to(0.5f, "bgm/inter.ogg");

    return resol_gui();
}

static void resol_leave(struct state *st, struct state *next, int id)
{
    back_free();
    gui_delete(id);
    free_modes();
}

static void resol_paint(int id, float st)
{
    video_push_persp((float) config_get_d(CONFIG_VIEW_FOV), 0.1f, FAR_DIST);
    {
        back_draw_easy();
    }
    video_pop_matrix();
    gui_paint(id);
}

static void resol_timer(int id, float dt)
{
    gui_timer(id, dt);
}

static void resol_point(int id, int x, int y, int dx, int dy)
{
    gui_pulse(gui_point(id, x, y), 1.2f);
}

static void resol_stick(int id, int a, float v, int bump)
{
    gui_pulse(gui_stick(id, a, v, bump), 1.2f);
}

static int resol_click(int b, int d)
{
    if (gui_click(b, d))
    {
        int active = gui_active();

        return resol_action(gui_token(active), gui_value(active));
    }
    return 1;
}

static int resol_keybd(int c, int d)
{
    return (d && c == SDLK_ESCAPE) ? resol_action(RESOL_BACK, 0) : 1;
}

static int resol_buttn(int b, int d)
{
    if (d)
    {
        int active = gui_active();

        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_A, b))
            return resol_action(gui_token(active), gui_value(active));
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_B, b))
            return resol_action(RESOL_BACK, 0);
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_EXIT, b))
            return resol_action(RESOL_BACK, 0);
    }
    return 1;
}

/*---------------------------------------------------------------------------*/

struct state st_resol = {
    resol_enter,
    resol_leave,
    resol_paint,
    resol_timer,
    resol_point,
    resol_stick,
    NULL,
    resol_click,
    resol_keybd,
    resol_buttn
};

