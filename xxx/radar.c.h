// Sketch uses 18212 bytes (59%) of program storage space. Maximum is 30720 bytes.
// Global variables use 1716 bytes (83%) of dynamic memory, leaving 332 bytes for local variables.
// Maximum is 2048 bytes.

// Sketch uses 18922 bytes (7%) of program storage space. Maximum is 253952 bytes.
// Global variables use 1731 bytes (21%) of dynamic memory, leaving 6461 bytes for local variables.
// Maximum is 8192 bytes.

#define MIN_VEL 500
#define MIN_DT  500

typedef unsigned long u32;

typedef struct {
    char type;
    char dir;
    char peak[4];
    char none[13]; 
} Radar_S;

int tovel (Radar_S* s) {
    int ret = (s->peak[0] - '0') * 1000 +
              (s->peak[1] - '0') *  100 +
              (s->peak[2] - '0') *   10 +
              (s->peak[3] - '0');
    return ret;
}

void Radar_Setup () {
    Serial1.begin(9600);
}

void Radar_Flush () {
    while (Serial1.available()) {
        Serial1.read();
    }
}

int Radar () {
#if 1
    static u32 old = millis();
    u32 now = millis();
    u32 dt = now - old;
    if (dt > 500) {
        old = now;
        if (random(0,5) <= 2) {
            int vel = random(50,100);
            return (random(0,2)==0) ? vel : -vel;
        }
    }
    return 0;
#else
    static char dir = '\0';
    static u32  old = millis();

    if (!Serial1.available()) {
        return 0;
    }

    Radar_S s;
    Serial1.readBytes((char*)&s, sizeof(Radar_S));

    int vel = tovel(&s);
    if (vel < MIN_VEL) {
        return 0;
    }

    u32 now = millis();
    u32 dt = now - old;
    if (s.dir==dir && dt<MIN_DT) {
        return 0;
    }

    old = now;
    dir = s.dir;
    vel /= 10;
    return (dir == 'A') ? vel : -vel;
#endif
}
