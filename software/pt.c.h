void PT_Bests_Lado (s8* bests, Lado* lado) {
    //lado->min1 = bests[HITS_BESTS-1];
    //lado->min2 = bests[HITS_BESTS/2-1];
    lado->max_ = bests[0];
    int sum1 = 0;
    int sum2 = 0;
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

void PT_Bests_Get (int i, Lado** reves, Lado** normal) {
    if (G.lados[i][0].avg1 <= G.lados[i][1].avg1) {
        *reves  = &G.lados[i][0];
        *normal = &G.lados[i][1];
    } else {
        *reves  = &G.lados[i][1];
        *normal = &G.lados[i][0];
    }
}

void PT_Bests_All (void) {
    if (! S.maximas) {
        return;
    }

    for (int i=0; i<2; i++)
    {
        // calcula para os dois lados HITS_BESTS e HITS_BESTS/2
        for (int j=0; j<2; j++) {
            PT_Bests_Lado(G.bests[i][j], &G.lados[i][j]);
        }

        Lado *reves, *normal;
        PT_Bests_Get(i, &reves, &normal);
        if (S.reves) {
            G.ps[i] += reves->avg2 * MULT_REVES;
        }
        G.ps[i] += normal->avg1 * MULT_NORMAL;
    }
}

int PT_Behind (void) {
    u32 p0  = G.ps[0];
    u32 p1  = G.ps[1];
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
    G.ps[0] = 0;
    G.ps[1] = 0;
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

                int idx = (i%2);
                volume[idx] = kmh*kmh;
                hits_one[idx]++;

                // bests
                s8* vec = G.bests[ 1-(i%2) ][ dt>0||!S.reves ];
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
                    s8* vec = G.bests[ 1-(i%2) ][0];
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

    G.volume[0] = volume[0]*100/hits_one[0];
    G.volume[1] = volume[1]*100/hits_one[1];

    G.ps[0] = (hits_one[0] == 0) ? 0 : G.volume[0] * MULT_VOLUME;
    G.ps[1] = (hits_one[1] == 0) ? 0 : G.volume[1] * MULT_VOLUME;
    PT_Bests_All();

    u32 pct   = min(990, Falls()*CONT_PCT);
    u32 avg   = (G.ps[0] + G.ps[1]) / 2;
    u32 total = (S.equilibrio ? min(avg, min(G.ps[0],G.ps[1])*11/10) : avg);
    G.total   = total * (1000-pct) / 100000;
}
