#define MIN_DT  100

typedef char s8;
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
    //ret = ret / 10;
    //assert(abs(ret) < 128);
    //Serial.print("===  ");
    //Serial.println(ret);
    return ret;
}

static char dir = '\0';
static u32  old = millis();

int Radar () {
    if (!Serial1.available()) {
        return 0;
    }

    Radar_S s;
    Serial1.readBytes((char*)&s, sizeof(Radar_S));

    int vel = tovel(&s);
    //Serial.println((int)vel);
    //Serial.println(dir);

    u32 now = millis();
    u32 dt = now - old;
    if (s.dir==dir && dt<MIN_DT) {
        return;
    }

    old = now;
    dir = s.dir;
    return (dir == 'A') ? vel : -vel;
}
void setup (void) {
    Serial.begin(9600);
    Serial1.begin(9600);
    Serial.println("=== RADAR ===");
}

void loop (void) {
    static int old = 0;
    int vel = Radar();
#if 1
    if (vel != 0) {
        Serial.print((vel > 0) ? "<-" : "->");
        Serial.print(' ');
        Serial.println(abs(vel));
    }
#else
    if (vel < 0) {
        Serial.print(-vel);
        Serial.print("  <--->  ");
        Serial.println(old);
    } else if (vel>0 && vel>old) {
        old = vel;
    }
#endif
}
