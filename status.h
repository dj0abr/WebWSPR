void fsave_status(int id, int value);

enum _STATIDS_ {
    PEAKLEVEL = 0,
    PTT = 1,
    NEXTTXTIME = 2,
    RXTXFREQ = 3,
    FREQNEXTINTV = 4,
    NEXTTXFREQU = 5,
    SYSALIVE = 6,
    SNDALIVE = 7,
    DECODER = 8,
    MY2WAY = 9,
    OT2WAY = 10,
    MAXSTATUSLINES
};
