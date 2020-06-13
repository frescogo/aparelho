typedef struct {
    u16 avg1;   // media de velocidade considerando HITS_NRM (x100)
    u16 avg2;   // media de velocidade considerando HITS_REV (x100)
    //int max_;   // maior velocidade
    //int tot1;   // total de golpes     considerando HITS_NRM
    //int min1;   // menor velocidade    considerando HITS_NRM
    //int tot2;   // total de golpes     considerando HITS_REV
    //int min2;   // menor velocidade    considerando HITS_REV
} Lado;

void PT_Bests_Lado (s8* bests, Lado* lado) {
    //lado->min1 = bests[HITS_NRM-1];
    //lado->min2 = bests[HITS_REV-1];
    //lado->max_ = bests[0];
    u32 sum1 = 0;
    u32 sum2 = 0;
    //lado->tot1 = HITS_NRM;
    //lado->tot2 = HITS_REV;
    for (int i=0; i<HITS_NRM; i++) {
        s8 v = bests[i];
        if (v == 0) {
            //lado->tot1 = i;
            //lado->tot2 = min(i, HITS_REV);
            break;
        }
        sum1 += v;
        if (i < HITS_REV) {
            sum2 += v;
        }
    }
    lado->avg1 = sum1*100/HITS_NRM;
    lado->avg2 = sum2*100/HITS_REV;
}

int PT_Equ (u16* avg, u16* min_) {
    u16 p0  = G.jogs[0].total;
    u16 p1  = G.jogs[1].total;
    *avg  = (p0 + p1) / 2;
    *min_ = min(*avg, ((u32)min(p0,p1))*EQU_PCT);
    return *avg == *min_;
}

int PT_Behind (void) {
    u16 avg, min_;
    if (!PT_Equ(&avg,&min_)) {
        if (G.jogs[0].total < G.jogs[1].total) {
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

    memset(G.bests, 0, 2*2*REF_BESTS*sizeof(s8));

    u32 pace[2] = {0,0};        // overall   avg, avg2

    u32 volume[2] = {0,0};      // per-player avg2, avg2
    u16 hits_one[2] = {0,0};    // per-player hits

    for (int i=0 ; i<S.hit ; i++) {
    //for (int i=0 ; i<600 ; i++) {
        s8 dt  = S.dts[i];
        s8 kmh = KMH(i);

        if (dt == HIT_SERV) {
            G.servs++;
        }

        if (dt!=HIT_NONE && dt!=HIT_SERV) {
            if (i==S.hit-1 || S.dts[i+1]==HIT_NONE || S.dts[i+1]==HIT_SERV) {
                // ignore last hit
            }
            else
            {
                G.hits++;

                int player = 1 - (i%2);
                volume[player] += kmh*kmh;
                hits_one[player]++;

                // bests
                s8* vec = G.bests[player][ dt>0||!S.reves ];
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
                    s8* vec = G.bests[player][0];
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
    }
    G.time *= 10;
    G.time = min(G.time, S.timeout);

    G.ritmo = (((u32)G.hits)*S.distancia*36) / G.time;

    G.jogs[0].volume = (hits_one[0] == 0) ? 0 : sqrt(volume[0]*1000/hits_one[0]*10);
    G.jogs[1].volume = (hits_one[1] == 0) ? 0 : sqrt(volume[1]*1000/hits_one[1]*10);

    {
        for (int i=0; i<2; i++)
        {
            Lado lados[2];
            PT_Bests_Lado(G.bests[i][0], &lados[0]);
            PT_Bests_Lado(G.bests[i][1], &lados[1]);

            Lado *reves, *normal;
            if (lados[0].avg1 <= lados[1].avg1) {
                reves  = &lados[0];
                normal = &lados[1];
            } else {
                reves  = &lados[1];
                normal = &lados[0];
            }

            G.jogs[i].normal = normal->avg1;
            G.jogs[i].reves  = (S.reves ? reves->avg2 : 0);
            //G.max_[i]   = max(normal->max_, reves->max_);
        }
    }

    G.jogs[0].total = (((u32)G.jogs[0].volume)*MULT_VOLUME + ((u32)G.jogs[0].normal)*MULT_NORMAL + ((u32)G.jogs[0].reves)*MULT_REVES) / MULT_DIV;
    G.jogs[1].total = (((u32)G.jogs[1].volume)*MULT_VOLUME + ((u32)G.jogs[1].normal)*MULT_NORMAL + ((u32)G.jogs[1].reves)*MULT_REVES) / MULT_DIV;

    u16 avg, min_;
    PT_Equ(&avg,&min_);

    {
        int pct   = CONT_PCT(Falls(), G.time);
        u32 total = (S.equilibrio ? min_ : avg);
        G.total   = total * (10000-pct) / 10000;
    }

    {
        u32 ti = ((STATE != STATE_TIMEOUT) ? G.time : S.timeout) / 100;
        u32 to = ((S.hit >= HITS_MAX-5)    ? G.time : S.timeout) / 100;
        G.acum = ((u32)G.total)*G.total / (36*5) * ti / to * REF_TIMEOUT / 10000;
    }
}
