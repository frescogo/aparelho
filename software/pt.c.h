void PT_Bests_Lado (s8* bests, Lado* lado) {
    //lado->min1 = bests[HITS_BESTS-1];
    //lado->min2 = bests[HITS_BESTS/2-1];
    //lado->max_ = bests[0];
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

    G.volume[0] = (hits_one[0] == 0) ? 0 : volume[0]*100/hits_one[0];
    G.volume[1] = (hits_one[1] == 0) ? 0 : volume[1]*100/hits_one[1];

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

            G.normal[i] = normal->avg1;
            G.reves[i]  = (S.reves ? reves->avg2 : 0);
            //G.max_[i]   = max(normal->max_, reves->max_);
        }
    }

    G.ps[0] = (G.volume[0]*MULT_VOLUME + G.normal[0]*MULT_NORMAL + G.reves[0]*MULT_REVES) / MULT_DIV;
    G.ps[1] = (G.volume[1]*MULT_VOLUME + G.normal[1]*MULT_NORMAL + G.reves[1]*MULT_REVES) / MULT_DIV;

    u32 pct   = min(990, Falls()*CONT_PCT);
    u32 avg   = (G.ps[0] + G.ps[1]) / 2;
    u32 total = (S.equilibrio ? min(avg, min(G.ps[0],G.ps[1])*11/10) : avg);
    G.total   = total * (1000-pct) / 100000;
}
