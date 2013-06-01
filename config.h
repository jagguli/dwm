/* See LICENSE file for copyright and license details. */
#include "im-grid.c"
#include "push.c"
#include "X11/XF86keysym.h"
#include "togglefullscreen.c"
#define HOME_BIN(cmd) "/home/steven/bin/"#cmd
//#define URXVTC(title) "urxvtc", "-title", title, "-e"
#define URXVTC(title) "st", "-t", title, "-e"
#define SINGLEMON 0
#define TERMCMD HOME_BIN(tmx_outer)
#ifdef SINGLEMON
#define TERM1 "term1"
#define TERM2 "IPython"
#else
#define TERM1 "right"
#define TERM2 "left"
#endif
/* custom functions declarations */
static void focusstackf(const Arg *arg);
static void setltor1(const Arg *arg);
static void toggletorall(const Arg *arg);
static void togglevorall(const Arg *arg);
static void vieworprev(const Arg *arg);
static void warptosel(const Arg *arg);
static void zoomf(const Arg *arg);
static void spawnifnottitle(const Arg *arg);
static void viewtagmon(const Arg *arg);
static void shadewin(const Arg *arg);
void list_clients(const Arg *arg);

/* appearance */
static const char font[]                 = "-*-terminus-*-*-*-*-12-*-*-*-*-*-*-*";
static const unsigned int borderpx       = 1;                // border pixel of windows
static const unsigned int snap           = 5;                // snap pixel
static const unsigned int systrayspacing = 1;                // space between systray icons
static const Bool showsystray            = True;             // False means no systray
static const Bool showbar                = True;             // False means no bar
static const Bool topbar                 = True;             // False means bottom bar
static const double defaultopacity  = 1;
static Bool useicons                     = True;             // False means use ascii symbols
static Bool readin                  = True;     /* False means do not read stdin */

static const char scratchpadname[]       = "Scratchpad";
#define NUMCOLORS 21

#define star_trek_blue "#42B1FF", "#A3E4FF", "#010F2C"
#define matrix_green "#00FD00", "#00FD00", "#125B29"

static const char colors[NUMCOLORS][ColLast][21] = {
    // border     foreground background
    { "#1A1A1A", "#808080", "#020202" },  // 01 - normal
    { star_trek_blue },  // 02 - selected
    //{ "#4C4C4C", "#B3B3B3", "#020202" },  // 02 - selected
    { "#B3354C", "#B3354C", "#020202" },  // 03 - urgent

    { "#1A1A1A", "#1A1A1A", "#020202" },  // 04 - black
    { "#802635", "#802635", "#020202" },  // 05 - red
    { "#608040", "#00FD00", "#010F2C" /*"#020202"*/ },  // 06 - green
    { "#877C43", "#877C43", "#020202" },  // 07 - yellow
    { "#1C678C", "#1C678C", "#020202" },  // 08 - blue
    { "#684D80", "#684D80", "#020202" },  // 09 - magenta
    { "#000000", "#000000", "#000000" },  // unusable
    { "#337373", "#337373", "#020202" },  // 0B - cyan
    { "#808080", "#808080", "#020202" },  // 0C - light gray
    { "#4C4C4C", "#4C4C4C", "#020202" },  // 0D - gray
    { "#B3354C", "#B3354C", "#020202" },  // 0E - light red
    { "#4BA65A", "#4BA65A", "#020202" },  // 0F - light green
    { "#BF9F5F", "#BF9F5F", "#020202" },  // 10 - light yellow
    { "#3995BF", "#3995BF", "#020202" },  // 11 - light blue
    { "#A64286", "#A64286", "#020202" },  // 12 - light magenta
    { "#6C98A6", "#6C98A6", "#020202" },  // 13 - light cyan
    { "#B3B3B3", "#B3B3B3", "#020202" },  // 14 - white

    { "#802635", "#BF9F5F", "#802635" },  // 15 - warning
};

/* layout(s) */
static const float mfact      = 0.55; /* factor of master area size [0.05..0.95] */
static const int nmaster      = 1;    /* number of clients in master area */
static const Bool resizehints = False; /* False means respect size hints in tiled resizals */
#define TILE_LAYOUT 0
#define BSTACK_LAYOUT 1
#define NULL_LAYOUT 2
#define MONOCLE_LAYOUT 3
#define GRID_LAYOUT 4
static const Layout layouts[] = {
    // icon                                    symbol    arrange function
    { "/home/steven/abs/dwm/icons/tile.xbm",     "þ",      tile },
    { "/home/steven/abs/dwm/icons/bstack.xbm",   "ü",      bstack },
    { "/home/steven/abs/dwm/icons/float.xbm",    "ý",      NULL },     // no layout function means floating behavior
    { "/home/steven/abs/dwm/icons/monocle.xbm",  "ÿ",      monocle },
    { "/home/steven/abs/dwm/icons/grid.xbm",     "ú",      imgrid },
};

/* tagging */
//static const char *tags[] = { "1 work", "2 research", "3 mail", "4 remote", "5 misc", "6 misc", "7 misc", "8 misc", "9 code" };
static const Tag tags[] = {
    // name       layout           mfact    nmaster
    { "1",        &layouts[MONOCLE_LAYOUT],     -1,      -1 },
    { "2",        &layouts[MONOCLE_LAYOUT],     -1,      -1 },
    { "3",        &layouts[MONOCLE_LAYOUT],     -1,      -1 },
    { "4",        &layouts[MONOCLE_LAYOUT],   0.22,      -1 },
    { "5",        &layouts[GRID_LAYOUT],      0.65,      -1 },
    { "6",        &layouts[MONOCLE_LAYOUT],     -1,      -1 },
    { "7",        &layouts[MONOCLE_LAYOUT],     -1,      -1 },
    { "8",        &layouts[MONOCLE_LAYOUT],     -1,      -1 },
    { "9",        &layouts[MONOCLE_LAYOUT],     -1,      -1 },
};

/* window rules */
static const Rule rules[] = {
    // class                  instance  title                  tags mask  isfloating  iscentred   monitor
    { NULL,                  NULL,     "DavMail Gateway",     1 << 6,    False,       False,      1 },
    { NULL,                  NULL,     "tmux",                1 << 0,    False,                  -1 },
    { "Krusader",            NULL,      NULL,                 1 << 5,    False,       False,      1 },
    { "Dwb",                 NULL,      NULL,                 1 << 1,    False,                  -1 },
    //{ "Firefox",             NULL,      NULL,                 1 << 1,    False,                  -1 },
    { "Cr3",                 NULL,      NULL,                 1 << 2,    False,                  -1 },
    { "Skype",               NULL,      NULL,                 1 << 3,    False,                  -1 },
    { NULL,                  NULL,     "WeeChat",             1 << 3,    False,                  1 },
    { NULL,                  NULL,     "Mutt",                1 << 2,    False,                  1 },
    { NULL,                  NULL,     "AWS",                 1 << 4,    False,                  1 },
#ifdef SINGLEMON
    { NULL,                  NULL,     TERM2,                1 << 7,    False,        True,      1 },
    { NULL,                  NULL,     TERM1,                1 << 8,    False,        True,      0 },
#else
    { NULL,                  NULL,     TERM1,                1 << 8,    False,        True,      1 },
    { NULL,                  NULL,     TERM2,                1 << 8,    False,        True,      0 },
#endif
    { "VirtualBox",          NULL,     NULL,                 1 << 2,    False,        True,      1 },
    { NULL,                 NULL,     "syd-term2",           1 << 3,    False,        True,      1 },
    { "Gimp",                NULL,      NULL,                 1 << 5,    False,                  -1 },
    { "Lxappearance",        NULL,      NULL,                 0,         True,                   -1 },
    { "mplayer2",            NULL,      NULL,                 0,         True,                   -1 },
    { "Nitrogen",            NULL,      NULL,                 0,         True,                   -1 },
    { "Qalculate-gtk",       NULL,      NULL,                 0,         True,                   -1 },
    { "Qalculate",           NULL,      NULL,                 0,         True,                   -1 },
    { "Qpass",               NULL,      NULL,                 0,         True,                   -1 },
    { "Stardict",            NULL,      NULL,                 0,         True,                   -1 },
    { "Zenity",              NULL,      NULL,                 0,         True,                   -1 },
    { "feh",                 NULL,      "Notification",       0,         True,        False,     0 },
    { "Pidgin",              NULL,      NULL,             1 << 3,        False,            1 },
};


static const MonocleNumberedIcon monoclenumberedicons[] = {
    { "/home/steven/abs/dwm/icons/monocle0.xbm" },
    { "/home/steven/abs/dwm/icons/monocle1.xbm" },
    { "/home/steven/abs/dwm/icons/monocle2.xbm" },
    { "/home/steven/abs/dwm/icons/monocle3.xbm" },
    { "/home/steven/abs/dwm/icons/monocle4.xbm" },
    { "/home/steven/abs/dwm/icons/monocle5.xbm" },
};



/* key definitions */
#define MODKEY Mod4Mask
#define TAGKEYS(KEY,TAG)                                                \
    { MODKEY,                       KEY,      vieworprev,           {.ui = 1 << TAG} }, \
  { MODKEY|ControlMask,           KEY,      togglevorall,     {.ui = 1 << TAG} }, \
  { MODKEY|ShiftMask,             KEY,      tag,            {.ui = 1 << TAG} }, \
  { MODKEY|ControlMask|ShiftMask, KEY,      toggletorall,      {.ui = 1 << TAG} },

#define MONTAGKEYS(MOD, KEY,TAG,MON)                                    \
    { MOD,                            KEY,    viewtagmon,     {.v = &(int[2]){1<<TAG,MON}} },

/* helper for spawning shell commands in the pre dwm-5.0 fashion */
#define SHCMD(cmd) { .v = (const char*[]){ "/bin/sh", "-c", cmd, NULL } }

/* commands */
static const char *termcmd[]  = { URXVTC("Terminal"), NULL };
static const char *lock[]  = { "xscreensaver-command", "-lock", NULL };
/* commands */
static const char *browsercmd[]    = { HOME_BIN(browser), NULL };
static const char *calendarcmd[]   = { "gsimplecal", NULL };
static const char *dictcmd[]       = { "stardict", NULL };
static const char *dmenucmd[]      = { "dmenu_run", "-i", "-fn", font, "-nb", colors[0][ColBG], "-nf", colors[0][ColFG],
                                       "-sb", colors[1][ColBG], "-sf", colors[1][ColFG], "-l","30", NULL };
static const char *dmenuswitcher[]      = { HOME_BIN(dwmswitcher), "-fn", font, "-nb", colors[0][ColBG], "-nf", colors[0][ColFG],
                                            "-sb", colors[1][ColBG], "-sf", colors[1][ColFG], "-l","30", NULL };
static const char *irccmd[]        = { URXVTC("WeeChat"), HOME_BIN(tmx),"chat", NULL };
static const char *pidgincmd[]        = { "pidgin", NULL };
static const char *mailcmd[]       = { URXVTC("Mutt"), HOME_BIN(tmx),"mail", NULL };
static const char *remotecmd[]     = { URXVTC("AWS"), "autossh", "-M", "0", "stevenjoseph.in", "-t", HOME_BIN(tmx_outer aws),"aws", NULL };
static const char *terminal1cmd[]  = { URXVTC(TERM1), TERMCMD, TERM1, NULL };
static const char *terminal2cmd[]  = { URXVTC(TERM2), TERMCMD, TERM2, NULL };
static const char *qpython[]     = { HOME_BIN(qpython), NULL, TERM2, NULL };
static const char *logoutcmd[]     = { "sudo", "killall", "X", NULL };
static const char *menucmd[]       = { "mygtkmenu", "/home/ok/.menu", NULL };
static const char *monitorcmd[]    = { "/home/ok/bin/monitor-dwm.sh", NULL };
static const char *passcmd[]       = { "qpass", NULL };
static const char *scratchpadcmd[] = { URXVTC(scratchpadname),  NULL }; //"-geometry", "70x9+400+10",
static const char *screenoffcmd[]  = { "xset", "dpms", "force", "off", NULL };
static const char *screenshotcmd[]  = { "ksnapshot", NULL };
static const char *shutdowncmd[]   = { "/home/ok/bin/dmenu-powerbutton", NULL };
static const char *tmuxcmd[]       = { URXVTC("tmux"), "/home/ok/bin/tmux.sh", NULL };
static const char *krusader[]       = { "krusader", NULL };
static const char *playcmd[]       = { "player_control", "pause", NULL };
static const char *prevcmd[]       = { "player_control", "prev", NULL };
static const char *nextcmd[]       = { "player_control", "next", NULL };
static const char *kbdlightupcmd[]       = { HOME_BIN(samctl.py), "-k", "up", NULL };
static const char *kbdlightdowncmd[]       = { HOME_BIN(samctl.py), "-k", "down", NULL };
static const char *backlightupcmd[]       = { HOME_BIN(samctl.py), "-s", "up", NULL };
static const char *backlightdowncmd[]       = { HOME_BIN(samctl.py), "-s", "down", NULL };
static const char *powermodecmd[]       = { HOME_BIN(samctl.py), "-p", "toggle", NULL };
static const char *voldowncmd[]    = { HOME_BIN(samctl.py), "-v", "down", NULL };
static const char *voltogglecmd[]  = { "amixer", "-q", "set", "Master", "toggle",  NULL };
static const char *volupcmd[]      = { HOME_BIN(samctl.py), "-v", "up",  NULL };
static const char *wificmd[]       = { URXVTC("Wifi"), "-e", "sudo", "wifi-menu", NULL };
static const int cmd1[2] = {8,1};
static const int cmd2[2] = {8,0};

#include "cycle.c"
#include "movestack.c"

static Key keys[] = {
    { MODKEY|ShiftMask,		XK_s,	   spawn,	   SHCMD("transset-df -a --dec .05") },
    { MODKEY|ShiftMask,		XK_d,	   spawn,	   SHCMD("transset-df -a --inc .05") },
    { MODKEY|ShiftMask,		XK_f,	   spawn,	   SHCMD("transset-df -a 1") },
    { MODKEY|ShiftMask,              XK_b,                     spawn,            {.v = browsercmd } },
    { Mod3Mask,                      XK_d,                     spawn,            {.v = dictcmd } },
    { MODKEY|ShiftMask,              XK_i,                     spawnifnottitle,  {.v = irccmd } },
    //{ MODKEY|ShiftMask,              XK_s,                     spawn,            {.v = pidgincmd } },
    { MODKEY,                        XK_s,                     deck,            {.i = 1 } },
    { MODKEY|ShiftMask,              XK_m,                     spawnifnottitle,  {.v = mailcmd } },
    { MODKEY|ShiftMask,              XK_r,                     spawnifnottitle,  {.v = remotecmd } },
    { 0,                             XK_Print,                 spawn,            {.v = screenshotcmd } },
    { MODKEY|ShiftMask,              XK_e,                     spawn,            {.v = logoutcmd } },
    //{ Mod4Mask,                      XK_space,               spawn,            {.v = menucmd } },
    { 0,                             XF86XK_Display,           spawn,            {.v = monitorcmd } },
    { Mod4Mask,                      XK_h,                     spawn,            {.v = passcmd} },
    { MODKEY,                        XK_F12,                   togglescratch,    {.v = scratchpadcmd} },
    { 0,                             XF86XK_Launch1,           spawn,            {.v = screenoffcmd } },
    { 0,                             XF86XK_PowerOff,          spawn,            {.v = shutdowncmd } },
    { Mod4Mask,                      XK_t,                     spawn,            {.v = tmuxcmd } },
    { 0,                             XF86XK_AudioLowerVolume,  spawn,            {.v = voldowncmd } },
    { 0,                             XF86XK_AudioMute,         spawn,            {.v = voltogglecmd } },
    { 0,                             XF86XK_AudioRaiseVolume,  spawn,            {.v = volupcmd } },
    { Mod4Mask,                      XK_w,                     spawn,            {.v = wificmd } },
    { MODKEY|ShiftMask,              XK_e,                     spawn,            {.v = krusader } },
    { 0,                             XF86XK_KbdBrightnessDown, spawn,            {.v = kbdlightdowncmd } },
    { 0,                             XF86XK_KbdBrightnessUp,   spawn,            {.v = kbdlightupcmd } },
    { 0,                             XF86XK_MonBrightnessDown, spawn,            {.v = backlightdowncmd } },
    { 0,                             XF86XK_MonBrightnessUp,   spawn,            {.v = backlightupcmd } },
    { 0,                             XF86XK_Tools,             spawn,            {.v = powermodecmd } },
    /*{ Mod4Mask,                      XK_Down,                  spawn,            {.v = playcmd } },
      { Mod4Mask,                      XK_Left,                  spawn,            {.v = prevcmd } },
      { Mod4Mask,                      XK_Right,                 spawn,            {.v = nextcmd } },*/
    /* modifier                     key        function        argument */
    { MODKEY,                       XK_F1,     spawn,          {.v = termcmd } },
    { MODKEY,                       XK_F2,     spawn,          {.v = dmenucmd } },
    { MODKEY,                       XK_F3,     list_clients,          {.v = dmenuswitcher } },
    //{ MODKEY,                       XK_F3,     spawn,          {.v = dmenuswitcher } },
    { MODKEY|ShiftMask,             XK_l,      spawn,          {.v = lock } },
    { MODKEY,                       XK_f,      togglebar,      {0} },
    { MODKEY,                       XK_j,      focusstack,     {.i = +1 } },
    { MODKEY,                       XK_k,      focusstack,     {.i = -1 } },
    { MODKEY,                       XK_i,      incnmaster,     {.i = +1 } },
    { MODKEY,                       XK_d,      incnmaster,     {.i = -1 } },
    { MODKEY,                       XK_h,      setmfact,       {.f = -0.05} },
    { MODKEY,                       XK_l,      setmfact,       {.f = +0.05} },
    { MODKEY,                       XK_Return, zoom,           {0} },
    { MODKEY,                       XK_Tab,    view,           {0} },
    { MODKEY,                       XK_q,      killclient,     {0} },
    { MODKEY,                       XK_t,      setlayout,      {.v = &layouts[TILE_LAYOUT]} },
    { MODKEY,                       XK_b,      setlayout,      {.v = &layouts[BSTACK_LAYOUT]} },
    { MODKEY,                       XK_f,      setlayout,      {.v = &layouts[NULL_LAYOUT]} },
    { MODKEY,                       XK_m,      setlayout,      {.v = &layouts[MONOCLE_LAYOUT]} },
    { MODKEY,                       XK_g,      setlayout,      {.v = &layouts[GRID_LAYOUT]} },
    { MODKEY,                       XK_space,  setlayout,      {0} },
    { MODKEY|ShiftMask,             XK_space,  togglefloating, {0} },
    { MODKEY,                       XK_0,      view,           {.ui = ~0 } },
    { MODKEY|ShiftMask,             XK_0,      tag,            {.ui = ~0 } },
    { MODKEY,                       XK_comma,  focusmon,       {.i = -1 } },
    { MODKEY,                       XK_period, focusmon,       {.i = +1 } },
    { MODKEY,                       XK_Up,  focusmon,       {.i = -1 } },
    { MODKEY,                       XK_Down, focusmon,       {.i = +1 } },
    { MODKEY|ShiftMask,             XK_comma,  tagmon,         {.i = -1 } },
    { MODKEY|ShiftMask,             XK_period, tagmon,         {.i = +1 } },
    { MODKEY|ShiftMask,             XK_j,      movestack,      {.i = +1 } },
    { MODKEY|ShiftMask,             XK_k,      movestack,      {.i = -1 } },
    TAGKEYS(                        XK_1,                      0)
    TAGKEYS(                        XK_2,                      1)
    TAGKEYS(                        XK_3,                      2)
    TAGKEYS(                        XK_4,                      3)
    TAGKEYS(                        XK_5,                      4)
    TAGKEYS(                        XK_6,                      5)
    TAGKEYS(                        XK_7,                      6)
    TAGKEYS(                        XK_8,                      7)
    TAGKEYS(                        XK_9,                      8)
#ifndef SINGLEMON
    MONTAGKEYS(0,XK_F12,8,1)
    MONTAGKEYS(0,XK_F11,8,0)
#else
    MONTAGKEYS(0,XK_F12,8,0)
    MONTAGKEYS(0,XK_F11,7,0)
#endif
    MONTAGKEYS(0,XK_F10,2,0)
    MONTAGKEYS(0,XK_F9,3,0)
    MONTAGKEYS(0,XK_F1,0,0)
    MONTAGKEYS(0,XK_F2,1,0)
    MONTAGKEYS(0,XK_F6,4,0)
#ifndef SINGLEMON
    { 0,                            XK_F11,    spawnifnottitle,     {.v = terminal2cmd } },
#else
    { 0,                            XK_F11,    spawnifnottitle,     {.v = qpython } },
#endif
    { 0,                            XK_F12,    spawnifnottitle,     {.v = terminal1cmd } },
    { Mod5Mask,                     XK_5,      focusmon,       {.i = 1 } },
    { MODKEY|ShiftMask,             XK_q,      quit,           {0} },
    { MODKEY, XK_Left, cycle, {.i = -1} },
    { MODKEY, XK_Right, cycle, {.i = +1} },
    { MODKEY|ControlMask, XK_Left, tagcycle, {.i = -1} },
    { MODKEY|ControlMask, XK_Right, tagcycle, {.i = +1} },
};

#include "tilemovemouse.c"
/* button definitions */
/* click can be ClkTagBar, ClkTagButton, ClkLtSymbol, ClkStatusText, ClkWinTitle, ClkClientWin, or ClkRootWin */
static Button buttons[] = {
    /* click                event mask      button          function        argument */
    { ClkLtSymbol,          0,              Button1,        setltor1,       {.v = &layouts[0]} },
    { ClkLtSymbol,          0,              Button2,        setmfact,       {.f = 1.65} },
    { ClkLtSymbol,          0,              Button3,        setltor1,       {.v = &layouts[2]} },
    { ClkLtSymbol,          0,              Button4,        setmfact,       {.f = +0.05} },
    { ClkLtSymbol,          0,              Button5,        setmfact,       {.f = -0.05} },
    { ClkStatusText,        0,              Button2,        spawn,          {.v = termcmd } },
    //  { ClkStatusText,        Button3Mask,    Button1,        killclient,     {0} },
    { ClkWinTitle,          0,              Button1,        warptosel,      {0} },
    { ClkWinTitle,          0,              Button1,        movemouse,      {0} },
    { ClkWinTitle,          0,              Button2,        zoomf,          {0} },
    { ClkWinTitle,          0,              Button3,        resizemouse,    {0} },
    { ClkWinTitle,          0,              Button4,        focusstackf,    {.i = -1 } },
    { ClkWinTitle,          0,              Button5,        focusstackf,    {.i = +1 } },
    { ClkRootWin,           0,              Button1,        warptosel,      {0} },
    { ClkRootWin,           0,              Button1,        movemouse,      {0} },
    { ClkRootWin,           0,              Button3,        resizemouse,    {0} },
    { ClkRootWin,           0,              Button4,        focusstackf,    {.i = -1 } },
    { ClkRootWin,           0,              Button5,        focusstackf,    {.i = +1 } },
    { ClkClientWin,         MODKEY,         Button1,        movemouse,      {0} },
    { ClkClientWin,         MODKEY,         Button2,        zoomf,          {0} },
    { ClkClientWin,         MODKEY,         Button3,        resizemouse,    {0} },
    { ClkTagBar,            0,              Button1,        vieworprev,     {0} },
    { ClkTagBar,            0,              Button3,        togglevorall,   {0} },
    { ClkTagBar,            0,              Button4,        focusstackf,    {.i = -1 } },
    { ClkTagBar,            0,              Button5,        focusstackf,    {.i = +1 } },
    { ClkTagBar,            Button2Mask,    Button1,        tag,            {0} },
    { ClkTagBar,            Button2Mask,    Button3,        toggletorall,   {0} },
};

/* custom functions */
void
focusstackf(const Arg *arg) {
    Client *c = NULL, *i;

    if(!selmon->sel)
        return;
    if(selmon->lt[selmon->sellt]->arrange) {
        if (arg->i > 0) {
            for(c = selmon->sel->next; c && (!ISVISIBLE(c) || c->isfloating != selmon->sel->isfloating); c = c->next);
            if(!c)
                for(c = selmon->clients; c && (!ISVISIBLE(c) || c->isfloating == selmon->sel->isfloating); c = c->next);
        }
        else {
            for(i = selmon->clients; i != selmon->sel; i = i->next)
                if(ISVISIBLE(i) && i->isfloating == selmon->sel->isfloating)
                    c = i;
            if(!c)
                for(i =  selmon->sel; i; i = i->next)
                    if(ISVISIBLE(i) && i->isfloating != selmon->sel->isfloating)
                        c = i;
        }
    }
    if(c) {
        focus(c);
        restack(selmon);
    }
    else
        focusstack(arg);
}

void
setltor1(const Arg *arg) {
    Arg a = {.v = &layouts[1]};

    setlayout((selmon->lt[selmon->sellt] == arg->v) ? &a : arg);
}

void
toggletorall(const Arg *arg) {
    Arg a;

    if(selmon->sel && ((arg->ui & TAGMASK) == selmon->sel->tags)) {
        a.ui = ~0;
        tag(&a);
    }
    else
        toggletag(arg);
}

void
togglevorall(const Arg *arg) {
    Arg a;

    if(selmon->sel && ((arg->ui & TAGMASK) == selmon->tagset[selmon->seltags])) {
        a.ui = ~0;
        view(&a);
    }
    else
        toggleview(arg);
}

void
vieworprev(const Arg *arg) {
    Arg a = {0};

    view(((arg->ui & TAGMASK) == selmon->tagset[selmon->seltags]) ? &a : arg);
}

void
warptosel(const Arg *arg) {
    XEvent ev;

    if(selmon->sel)
        XWarpPointer(dpy, None, selmon->sel->win, 0, 0, 0, 0, 0, 0);
    XSync(dpy, False);
    while(XCheckMaskEvent(dpy, EnterWindowMask, &ev));
}

void
zoomf(const Arg *arg) {
    if(selmon->sel && (selmon->lt[selmon->sellt]->arrange != tile || selmon->sel->isfloating)) 
        togglefloating(NULL);
    else
        zoom(NULL);
}

void 
spawnifnottitle(const Arg *arg){
    /* spawn command if title is not found */
    char *title = ((char **)arg->v)[2];
    Monitor *m = NULL;
    Client *c;
    for(m = mons; m; m = m->next){
        for(c = m->clients; c; c = c->next) {
            if(title && strstr(c->name, title)){
                return;
            }
        }
    }
    spawn(arg);
}

void
viewtagmon(const Arg *arg){
    /* switch to tag on monitor, or previous tag */
    Arg a;
    a.ui = (int)((int *)arg->v)[0];

#ifndef SINGLEMON
    Arg i = {.i = (int)((int *)arg->v)[1] };
    if(selmon->num != i.i){
        focusmon(&i);
        return;
    }
#endif

    vieworprev(&a);
}

void
shadewin(const Arg *arg){
    unsigned int n = 0;
    Client *c;

    for(c = selmon->clients; c; c = c->next)
        if(ISVISIBLE(c) && selmon->sel == c){
            fprintf(stderr, "dwm: ok %s\n", c->name);
            resizeclient(c, c->x, c->y, c->w, 20);
            n++;
        }
    arrange(selmon);
}
#include <stdio.h> 
void
list_clients(const Arg *arg){
    Monitor *m = NULL;
    Client *c;
    FILE *file = popen("/usr/bin/dmenu  -i -p 'Switch:' -l 30", "w");
    for(m = mons; m; m = m->next){
        for(c = m->clients; c; c = c->next) {
            if(strlen(c->name) >0){
                int titlelen = strlen(c->name)+2;
                char* title = malloc(titlelen);
                strcpy(title, c->name);
                fwrite(strcat(title, "\n"), sizeof(char), titlelen-1, file);
                free(title);
            }
        }
    }
    //fwrite("\n", sizeof(char), 1, file);
    pclose(file);
}
    
