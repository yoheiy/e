#define COLOUR_ESC "\033"
#define COLOUR_FG(x)  COLOUR_ESC "[38;5;" #x "m"
#define COLOUR_BG(x)  COLOUR_ESC "[48;5;" #x "m"
#define COLOUR_NORMAL COLOUR_ESC "[0m"

#define COLOUR_BOTTOM          COLOUR_FG(62)
#define COLOUR_CURSOR          COLOUR_FG(235) COLOUR_BG(216)
#define COLOUR_EOL             COLOUR_FG(124)
#define COLOUR_HEADLINE        COLOUR_BG(62)
#define COLOUR_HILIGHT         COLOUR_BG(56)
#define COLOUR_KEYWORD         COLOUR_FG(212)
#define COLOUR_LINE_NR         COLOUR_FG(61)
#define COLOUR_RULER           COLOUR_FG(104)
#define COLOUR_RULER_INDEX     COLOUR_FG(154)
#define COLOUR_SUBST           COLOUR_FG(122)
#define COLOUR_TABLE_V_BAR     COLOUR_FG(44)
