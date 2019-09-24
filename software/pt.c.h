typedef struct {
    u16 avg1;   // media de velocidade considerando HITS_BESTS (x100)
    u16 avg2;   // media de velocidade considerando HITS_BESTS/2 (x100)
    //int max_;   // maior velocidade
    //int tot1;   // total de golpes     considerando HITS_BESTS
    //int min1;   // menor velocidade    considerando HITS_BESTS
    //int tot2;   // total de golpes     considerando HITS_BESTS/2
    //int min2;   // menor velocidade    considerando HITS_BESTS/2
} Lado;

void PT_Bests_Lado (s8* bests, Lado* lado) {
    //lado->min1 = bests[HITS_BESTS-1];
    //lado->min2 = bests[HITS_BESTS/2-1];
    //lado->max_ = bests[0];
    u32 sum1 = 0;
    u32 sum2 = 0;
    //lado->tot1 = HITS_BESTS;
    //lado->tot2 = HITS_BESTS/2;
    for (int i=0; i<HITS_BESTS; i++) {
        s8 v = bests[i];
        if (v == 0) {
            //lado->tot1 = i;
            //lado->tot2 = min(i, HITS_BESTS/2);
            break;
        }
        sum1 += v;
        if (i < HITS_BESTS/2) {
            sum2 += v;
        }
    }
    lado->avg1 = sum1*100/HITS_BESTS;
    lado->avg2 = sum2*100/(HITS_BESTS/2);
}

int PT_Behind (void) {
    u32 p0  = G.jogs[0].total;
    u32 p1  = G.jogs[1].total;
    u32 avg = (p0 + p1) / 2;
    u32 m   = min(p0,p1);
    if (m * 11/10 < avg) {
        if (p0 < p1) {
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

    memset(G.bests, 0, 2*2*HITS_BESTS_MAX*sizeof(s8));

    u32 pace[2] = {0,0};        // overall   avg, avg2

    u32 volume[2] = {0,0};      // per-player avg2, avg2
    u16 hits_one[2] = {0,0};    // per-player hits

    for (int i=0 ; i<S.hit ; i++) {
    //for (int i=0 ; i<600 ; i++) {
        s8 dt  = S.dts[i];
        s8 kmh = G.kmhs[i];

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
                pace[0] += kmh;
                pace[1] += kmh*kmh;

                int player = 1 - (i%2);
                volume[player] += kmh*kmh;
                hits_one[player]++;

                // bests
                s8* vec = G.bests[player][ dt>0||!S.reves ];
                for (int j=0; j<HITS_BESTS; j++) {
                    if (kmh > vec[j]) {
                        for (int k=HITS_BESTS-1; k>j; k--) {
                            vec[k] = vec[k-1];
                        }
                        vec[j] = kmh;
                        break;
                    }
                }
                // marca os reves somente para exibicao
                if (!S.reves && dt<0) {
                    s8* vec = G.bests[player][0];
                    for (int j=0; j<HITS_BESTS; j++) {
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

    G.pace[0] = pace[0]/G.hits;
    G.pace[1] = pace[1]/G.hits;

    G.jogs[0].volume = (hits_one[0] == 0) ? 0 : sqrt(volume[0]*10000/hits_one[0]);
    G.jogs[1].volume = (hits_one[1] == 0) ? 0 : sqrt(volume[1]*10000/hits_one[1]);

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

    u32 avg   = (G.jogs[0].total + G.jogs[1].total) / 2;
    u32 pct   = Falls() * CONT_PCT;
    u32 total = (!S.equilibrio ? avg : min(avg, ((u32)min(G.jogs[0].total,G.jogs[1].total))*11/10));
    G.total   = total * (1000-pct) / 1000;
}
