void PT_Bests_Lado (int n, s8* bests, Lado* lado) {
    lado->golpes = 0;
    u32 sum = 0;
    for (int i=0; i<n; i++) {
        s8 v = bests[i];
        if (v == 0) {
            break;
        }
        lado->golpes++;
        sum += v*v;
    }
    lado->minima = bests[n-1];
    lado->maxima = bests[0];
    lado->media2 = sqrt(sum/lado->golpes);
    lado->pontos = lado->media2 * lado->golpes;
}

int PT_Equ (u16* avg, u16* min_) {
    u16 p0  = G.jogs[0].pontos;
    u16 p1  = G.jogs[1].pontos;
    *avg  = (p0 + p1) / 2;
    *min_ = min(*avg, ((u32)min(p0,p1))*EQU_PCT);
    return *avg == *min_;
}

int PT_Behind (void) {
    u16 avg, min_;
    if (!PT_Equ(&avg,&min_)) {
        if (G.jogs[0].pontos < G.jogs[1].pontos) {
            return 0;   // atleta a esquerda atras
        } else {
            return 1;   // atleta a direita  atras
        }
    } else {
        return -1;      // equilibrio
    }
}

void PT_All (void) {
    G.time  = 0;
    G.hits  = 0;
    G.servs = 0;

    s8 bests[2][2][REF_BESTS]; // kmh (max 125kmh/h)
    memset(bests, 0, 2*2*REF_BESTS*sizeof(s8));

    u32 pace[2] = {0,0};        // overall   avg, avg2

    u32 volume[2] = {0,0};      // per-player avg2, avg2
    u16 hits_one[2] = {0,0};    // per-player hits

    for (int i=0 ; i<S.hit ; i++) {
        s8 dt  = S.dts[i];
        s8 kmh = KMH(i);

        if (dt == HIT_SERV) {
            G.servs++;
        }

        if (dt==HIT_NONE || dt==HIT_SERV || kmh<HIT_KMH_50) {
            continue;
        }

        if (i==S.hit-1 || S.dts[i+1]==HIT_NONE || S.dts[i+1]==HIT_SERV) {
            // ignore last hit
        }
        else
        {
            G.hits++;

            int player = 1 - (i%2);

            // bests
            s8* vec = bests[player][ dt<0 && S.reves ];
            for (int j=0; j<HITS_NRM; j++) {
                if (kmh > vec[j]) {
                    for (int k=HITS_NRM-1; k>j; k--) {
                        vec[k] = vec[k-1];
                    }
                    vec[j] = kmh;
                    break;
                }
            }
            // marca os reves somente para exibicao
            if (!S.reves && dt<0) {
                s8* vec = bests[player][0];
                for (int j=0; j<HITS_NRM; j++) {
                    if (vec[j] == 0) {
                        vec[j] = kmh;
                        break;
                    }
                }
            }
        }

        G.time += (dt>0 ? dt : -dt);
    }
    G.time *= 10;
    G.time = min(G.time, S.timeout);

    G.ritmo = (((u32)G.hits)*S.distancia*36) / G.time;

    for (int i=0; i<2; i++) {
        Jog* jog = &G.jogs[i];
        PT_Bests_Lado(HITS_NRM, bests[i][LADO_NRM], &jog->lados[LADO_NRM]);
        PT_Bests_Lado(HITS_REV, bests[i][LADO_REV], &jog->lados[LADO_REV]);
        G.jogs[i].pontos = ((u32)jog->lados[LADO_NRM].pontos*MULT_NRM +
                            (u32)jog->lados[LADO_REV].pontos*MULT_REV) / 100;
    }

    u16 avg, min_;
    PT_Equ(&avg,&min_);

    int pct    = CONT_PCT(Falls(), G.time);
    u32 pontos = (S.equilibrio ? min_ : avg);
    G.pontos   = pontos * (10000-pct) / 10000;
}
