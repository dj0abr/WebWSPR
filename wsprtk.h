extern int running;
extern char wavdir[256];
extern char call[30];
extern char locator[30];
extern char htmldir[256];
extern char phpdir[256];
extern char decoderfile[256];

enum SYSTYPE {
    SYSTYPE_PC_UBUNTU = 0,
    SYSTYPE_PC_SUSE,
    SYSTYPE_ARM32,
    SYSTYPE_ARM64,
    SYSTYPE_TINYCORE,
};
