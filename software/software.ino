#define MAJOR    3
#define MINOR    0
#define REVISION 0

//#define DEBUG

#ifdef DEBUG
#define assert(x) \
    if (!x) {                               \
        pinMode(LED_BUILTIN,OUTPUT);        \
        while (1) {                         \
            digitalWrite(LED_BUILTIN,HIGH); \
            delay(250);                     \
            digitalWrite(LED_BUILTIN,LOW);  \
            delay(250);                     \
        }                                   \
    }
#else
#define assert(x)
#endif

#include <EEPROM.h>
#include "pitches.h"

typedef char  s8;
typedef short s16;
typedef unsigned long u32;

#if 1
#define PIN_LEFT  4
#define PIN_RIGHT 2
#define PIN_CFG   3
#else
#define PIN_LEFT  3
#define PIN_RIGHT 4
#define PIN_CFG   2
#endif

#define PIN_TONE 11

static const int MAP[2] = { PIN_LEFT, PIN_RIGHT };

#define DEF_TIMEOUT     240         // 4 mins
#define REF_TIMEOUT     300         // 5 mins

#define REF_HITS        170
#define REF_REV         (S.reves ?  20 :   0)
#define REF_NRM         (S.reves ? 150 : 170)

#define REF_CONT        160         // 1.6%
#define REF_ABORT       15          // 15s per fall

#define HITS_MAX        650
#define HITS_REF        (HITS_NRM + HITS_REV)
#define HITS_NRM        min(REF_NRM,  max(1, REF_NRM * S.timeout / REF_TIMEOUT / 1000))
#define HITS_REV        min(REF_REV,  max(1, REF_REV * S.timeout / REF_TIMEOUT / 1000))

#define HIT_MIN_DT      235         // minimum time between two hits (125kmh)
//#define HIT_KMH_MAX   125         // to fit in s8 (changed to u8, but lets keep 125)
#define HIT_KMH_MAX     99          // safe value to avoid errors
#define HIT_KMH_50      50

#define HIT_MARK        0
#define HIT_NONE        1
#define HIT_SERV        2

#define STATE_IDLE      0
#define STATE_PLAYING   1
#define STATE_TIMEOUT   2

#define NAME_MAX        15

#define DESC_MAX        65000
#define DESC_FOLGA      5000

#define REVES_MIN       180
#define REVES_MAX       220

#define POT_BONUS       2
#define POT_VEL         50

#define REV_PCT         11/10   // do not use parenthesis (multiply before division)
#define EQU_PCT         105/100 // do not use parenthesis (multiply before division)
//#define CONT_MAX        (REF_CONT * REF_TIMEOUT / REF_ABORT)    // max PCT to loose (40%)
//#define CONT_PCT(f,t)   min(CONT_MAX, f * (((u32)REF_TIMEOUT)*REF_CONT*1000/max(1,t)))
#define CONT_PCT(f,t)   min(9999, f * (((u32)REF_TIMEOUT)*REF_CONT*1000/max(1,t)))
#define ABORT_FALLS     (S.timeout / REF_ABORT / 1000)

static int  STATE;
static bool IS_BACK;
static char STR[64];

typedef enum {
    MOD_CEL = 0,
    MOD_PC
} TMOD;

TMOD MOD;

typedef struct {
    char juiz[NAME_MAX+1];      // = "Juiz"
    char names[2][NAME_MAX+1];  // = { "Atleta ESQ", "Atleta DIR" }
    u32  timeout;               // = 180 * ((u32)1000) ms
    u16  distancia;             // = 700 cm
    s8   equilibrio;            // = sim/nao
    u8   maxima;                // = 85 kmh
    u16  reves;                 // = 180  (tempo minimo de segurar para o back)

    u16  descanso;              // cs (ms*10) (até 650s de descanso)
    u16  hit;
    s8   dts[HITS_MAX];         // cs (ms*10)
} Save;
static Save S;

enum {
    LADO_NRM = 0,
    LADO_REV,
    LADO_NRM_REV
};

typedef struct {
    u8  golpes;
    u8  minima;
    u8  maxima;
    u8  media2;
    u16 pontos;
} Lado;

// TODO: avg2, total/reves/normal->tot/rev/nrm
typedef struct {
    u16  pontos;                    // reves+normal (x100)
    Lado lados[LADO_NRM_REV];
} Jog;

typedef struct {
    // calculated when required
    u32  time;                        // ms (total time)
    u16  hits;
    u8   servs;
    s8   ritmo;                       // kmh

    u16  pontos;
    Jog  jogs[2];
} Game;
static Game G;

enum {
    IN_LEFT  = 0,   // must be 0 (bc of MAP and 1-X)
    IN_RIGHT = 1,   // must be 1 (bc of MAP and 1-X)
    IN_NONE,
    IN_GO_FALL,
    IN_TIMEOUT,
    IN_RESTART,
    IN_UNDO,
    IN_RESET
};

int Falls (void) {
    return G.servs - (STATE==STATE_IDLE ? 0 : 1);
                        // after fall
}

// tom dos golpes            50-      50-60    60-70    70-80    80+
static const int NOTES[] = { NOTE_E3, NOTE_E5, NOTE_G5, NOTE_B5, NOTE_D6 };

#include "pt.c.h"
#include "serial.c.h"
#include "xcel.c.h"
#include "pc.c.h"

void Sound (s8 kmh, bool is_back) {
    int ton = NOTES[min(max(0,kmh/10-4), 4)];
    if (is_back && kmh>=HIT_KMH_50) {
        tone(PIN_TONE, ton, 20);
        delay(35);
        tone(PIN_TONE, ton, 20);
    } else {
        tone(PIN_TONE, ton, 50);
    }
}

int Await_Input (bool serial, bool hold) {
    static u32 old;
    static int pressed = 0;
    while (1) {
        if (serial) {
            int ret = Serial_Check();
            if (ret != IN_NONE) {
                return ret;
            }
        }

        u32 now = millis();

        int pin_left  = digitalRead(PIN_LEFT);
        int pin_right = digitalRead(PIN_RIGHT);

        // CFG UNPRESSED
        if (digitalRead(PIN_CFG) == HIGH)
        {
            pressed = 0;
            old = now;
            if (pin_left == LOW) {
                return IN_LEFT;
            }
            if (pin_right == LOW) {
                return IN_RIGHT;
            }
        }

        // CFG PRESSED
        else
        {
            if (!pressed) {
                tone(PIN_TONE, NOTE_C2, 50);
                pressed = 1;
            }

            // fall
            if        (now-old>= 750 && pin_left==HIGH && pin_right==HIGH) {
                old = now;
                return IN_GO_FALL;
            } else if (now-old>=3000 && pin_left==LOW  && pin_right==HIGH) {
                old = now;
                return IN_UNDO;
            } else if (now-old>=3000 && pin_left==HIGH && pin_right==LOW) {
                old = now;
                return IN_RESTART;
            } else if (now-old>=3000 && pin_left==LOW  && pin_right==LOW) {
                old = now;
                return IN_RESET;
            }
        }

        if (!hold) {
            return IN_NONE;
        }
    }
}

u8 KMH (int i) {
    s8 dt = S.dts[i];
    dt = (dt > 0) ? dt : -dt;
    u32 kmh_ = ((u32)36) * S.distancia / (dt*10);
               // prevents overflow
    u8 kmh = min(kmh_, HIT_KMH_MAX);
       kmh = min(kmh, S.maxima);
    return kmh;
}

void EEPROM_Load (void) {
    for (int i=0; i<sizeof(Save); i++) {
        ((byte*)&S)[i] = EEPROM[i];
    }
    S.hit = min(S.hit, HITS_MAX);
    S.names[0][NAME_MAX] = '\0';
    S.names[1][NAME_MAX] = '\0';
}

void EEPROM_Save (void) {
    for (int i=0; i<sizeof(Save); i++) {
        EEPROM[i] = ((byte*)&S)[i];
    }
}

void EEPROM_Default (void) {
    strcpy(S.juiz,     "?");
    strcpy(S.names[0], "Atleta ESQ");
    strcpy(S.names[1], "Atleta DIR");
    S.distancia     = 750;
    S.timeout       = DEF_TIMEOUT * ((u32)1000);
    S.equilibrio    = 1;
    S.maxima        = 85;
    S.reves         = 0;
}

void setup (void) {
    Serial.begin(9600);

    pinMode(PIN_CFG,   INPUT_PULLUP);
    pinMode(PIN_LEFT,  INPUT_PULLUP);
    pinMode(PIN_RIGHT, INPUT_PULLUP);

    delay(2000);
    if (Serial.available()) {
        MOD = Serial.read();
    } else {
        MOD = MOD_CEL;
    }

    EEPROM_Load();
}

u32 alarm (void) {
    u32 left = S.timeout - G.time;
    if (left < 5000) {
        return S.timeout - 0;
    } else if (left < 10000) {
        return S.timeout - 5000;
    } else if (left < 30000) {
        return S.timeout - 10000;
    } else if (left < 60000) {
        return S.timeout - 30000;
    } else {
        return (G.time/60000 + 1) * 60000;
    }
}

#define XMOD(xxx,yyy)   \
    switch (MOD) {      \
        case MOD_CEL:   \
            xxx;        \
            break;      \
        case MOD_PC:    \
            yyy;        \
            break;      \
    }

void Desc (u32 now, u32* desc0, bool desconto) {
    u32 diff = now - *desc0;
    *desc0 = now;

    if (!desconto || diff>DESC_FOLGA) {     // 5s de folga
        u32 temp = S.descanso + diff/10;
        S.descanso = min(DESC_MAX, temp);   // limita a 65000
        XMOD(CEL_Nop(), PC_Desc());
    }
}

void loop (void)
{
// RESTART
    STATE = STATE_IDLE;
    PT_All();

    XMOD(CEL_Restart(), PC_Restart());

    while (1)
    {
// GO
        PT_All();

        if (G.time >= S.timeout) {
            goto _TIMEOUT;          // if reset on ended game
        }
        if (Falls() >= ABORT_FALLS) {
            goto _TIMEOUT;
        }

        int got;
        while (1) {
            got = Await_Input(true,true);
            switch (got) {
                case IN_RESET:
                    EEPROM_Default();
                    goto _RESTART;
                case IN_RESTART:
                    goto _RESTART;
                case IN_UNDO:
                    if (S.hit > 0) {
_UNDO:
                        while (1) {
                            S.hit -= 1;
                            if (S.hit == 0) {
                                break;
                            } else if (S.dts[S.hit] == HIT_SERV) {
                                if (S.dts[S.hit-1] == HIT_NONE) {
                                    S.hit -= 1;
                                }
                                break;
                            }
                        }
                        tone(PIN_TONE, NOTE_C2, 100);
                        delay(110);
                        tone(PIN_TONE, NOTE_C3, 100);
                        delay(110);
                        tone(PIN_TONE, NOTE_C4, 300);
                        delay(310);
                        EEPROM_Save();
                        PT_All();
                        XMOD(Serial_Score(), PC_Atualiza());
                    }
                case IN_GO_FALL:
                    goto _BREAK1;
            }
        }
_BREAK1:

        u32 desc0 = millis();               // comeca a contar o descanso
        XMOD(Serial_Score(), PC_Nop());

_SERVICE:
        tone(PIN_TONE, NOTE_C7, 500);

// SERVICE
        while (1) {
            got = Await_Input(true,false);
            switch (got) {
                case IN_RESET:
                    EEPROM_Default();
                    goto _RESTART;
                case IN_RESTART:
                    goto _RESTART;
                case IN_LEFT:
                case IN_RIGHT:
                    goto _BREAK2;

                case IN_GO_FALL:
                    Desc(millis(), &desc0, false);
                    goto _SERVICE;

                case IN_NONE:
                    u32 now = millis();
                    if (now - desc0 >= 10000) {
                        Desc(now, &desc0, true);
                        tone(PIN_TONE, NOTE_C7, 500);
                    }
                    break;
            }
        }
_BREAK2:

        u32 t0 = millis();

        Desc(t0, &desc0, true);

        if (got != S.hit%2) {
            S.dts[S.hit++] = HIT_NONE;
        }
        S.dts[S.hit++] = HIT_SERV;
        STATE = STATE_PLAYING;

        tone(PIN_TONE, NOTES[0], 50);

        IS_BACK = false;

        XMOD(CEL_Service(got), PC_Hit(1-got,false,0));

        PT_All();
        delay(HIT_MIN_DT);

        int nxt = 1 - got;
        while (1)
        {
            // wait "got" pin to unpress
            while (digitalRead(MAP[got]) == LOW)
                ;

// HIT
            u32 t1;
            int dt;
            while (1) {
                got = Await_Input(true,true);
                if (got == IN_RESET) {
                    EEPROM_Default();
                    goto _RESTART;
                } else if (got == IN_RESTART) {
                    goto _RESTART;
                } else if (got == IN_TIMEOUT) {
                    goto _TIMEOUT;
                } else if (got == IN_GO_FALL) {
                    goto _FALL;
                } else if (got==IN_LEFT || got==IN_RIGHT) {
                    t1 = millis();
                    dt = (t1 - t0);
                    if (got==nxt || dt>=3*HIT_MIN_DT) {
                        break;
                    } else {
                        // ball cannot go back and forth so fast
                    }
                }
            }
            //ceu_arduino_assert(dt>50, 2);

            t0 = t1;

            bool skipped = (nxt != got);
            if (skipped) {
                dt = dt / 2;
            }
            dt = min(dt/10, 127); // we don't have space for dt>1270ms,so we'll
                                  // just assume it since its already slow

            u32 kmh_ = ((u32)36) * S.distancia / (dt*10);
                       // prevents overflow
            s8 kmh = min(kmh_, HIT_KMH_MAX);
            kmh = min(kmh_, S.maxima);

            // nao pode ser "skipped" (eh uma maneira de corrigir um erro de marcacao)
            // 10% mais forte que golpe anterior
            bool is_back = IS_BACK && !skipped && (kmh >= KMH(S.hit-1)*REV_PCT);

            u8 al_now = 0;
            if (G.time+dt*10 > alarm()) {
                tone(PIN_TONE, NOTE_C7, 250);
                al_now = 1;
            } else {
                Sound(kmh, is_back);
            }

            if (is_back) {
                S.dts[S.hit] = -dt;
            } else {
                S.dts[S.hit] = dt;
            }
            S.hit++;
            if (skipped) {
                S.dts[S.hit]  = dt;
                S.hit++;
            }
            nxt = 1 - got;

            PT_All();
            XMOD(CEL_Nop(), PC_Tick());

// TIMEOUT
            if (G.time >= S.timeout) {
                goto _TIMEOUT;
            }
            if (S.hit >= HITS_MAX-5) {
                goto _TIMEOUT;
            }

            // sleep inside hit to reach S.reves
            {
                int sensibilidade = (!S.reves) ? REVES_MIN : S.reves;
                u32 dt_ = millis() - t1;
                if (sensibilidade > dt_) {
                    delay(sensibilidade-dt_);
                }
                if (got == 0) {
                    IS_BACK = (digitalRead(PIN_LEFT)  == LOW);
                } else {
                    IS_BACK = (digitalRead(PIN_RIGHT) == LOW);
                }
                if (!al_now)
                {
                    if (IS_BACK) {
                        tone(PIN_TONE, NOTE_C4, 30);
                    } else if (S.equilibrio) {
                        // desequilibrio
                        if (G.time >= 30000) {
                            if (PT_Behind() == nxt) {
                                tone(PIN_TONE, NOTE_C2, 30);
                            }
                        }
                    }
                }
            }

            if (skipped) {
                XMOD(CEL_Hit(  got,IS_BACK,kmh), PC_Hit(  got,IS_BACK,kmh));
                XMOD(CEL_Hit(1-got,false,  kmh), PC_Hit(1-got,false,  kmh));
            } else {
                XMOD(CEL_Hit(1-got,IS_BACK,kmh), PC_Hit(1-got,IS_BACK,kmh));
            }

            // sleep inside hit to reach HIT_MIN_DT
            {
                u32 dt_ = millis() - t1;
                if (HIT_MIN_DT > dt_) {
                    delay(HIT_MIN_DT-dt_);
                }
            }
        }
_FALL:
        STATE = STATE_IDLE;

        tone(PIN_TONE, NOTE_C4, 100);
        delay(110);
        tone(PIN_TONE, NOTE_C3, 100);
        delay(110);
        tone(PIN_TONE, NOTE_C2, 300);
        delay(310);

        PT_All();
        XMOD(CEL_Fall(), PC_Fall());
        XMOD(CEL_Nop(),  PC_Tick());
        EEPROM_Save();

        if (Falls() >= ABORT_FALLS) {
            S.dts[S.hit++] = HIT_SERV;  // simulate timeout after service
        }
    }

_TIMEOUT:
    STATE = STATE_TIMEOUT;
    tone(PIN_TONE, NOTE_C2, 2000);
    PT_All();
    XMOD(CEL_Nop(), PC_Tick());
    XMOD(CEL_End(), PC_End());
    EEPROM_Save();

    while (1) {
        int got = Await_Input(true,true);
        if (got == IN_RESET) {
            EEPROM_Default();
            goto _RESTART;
        } else if (got == IN_RESTART) {
            goto _RESTART;
        } else if (got == IN_UNDO) {
            STATE = STATE_IDLE;
            if (Falls() >= ABORT_FALLS) {
                S.hit--;    // reverse above
            }
            goto _UNDO;
        }
    }

_RESTART:
    tone(PIN_TONE, NOTE_C5, 2000);
    S.hit = 0;
    S.descanso = 0;
    EEPROM_Save();
}
