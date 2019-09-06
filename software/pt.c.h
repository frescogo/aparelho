void PT_Bests_Lado (s8* bests, Lado* lado) {
    lado->min1 = bests[HITS_BESTS-1];
    lado->min2 = bests[HITS_BESTS/2-1];
    lado->max_ = bests[0];
    int sum1 = 0;
    int sum2 = 0;
    lado->tot1 = HITS_BESTS;
    lado->tot2 = HITS_BESTS/2;
    for (int i=0; i<HITS_BESTS; i++) {
        s8 v = bests[i];
        if (v == 0) {
            lado->tot1 = i;
            lado->tot2 = min(i, HITS_BESTS/2);
            break;
        }
        sum1 += v;
        if (i < HITS_BESTS/2) {
            sum2 += v;
        }
    }
    lado->avg1 = sum1/HITS_BESTS;
    lado->avg2 = sum2/(HITS_BESTS/2);
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

        // pega a maior avg1 e a menor avg2
        int avg;
        if (G.lados[i][0].avg1 < G.lados[i][1].avg1) {
            avg = (G.lados[i][0].avg2 + G.lados[i][1].avg1*2) / 3;
        } else {
            avg = (G.lados[i][1].avg2 + G.lados[i][0].avg1*2) / 3;
        }

        G.ps[i] += avg*avg * POT_BONUS;
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

    u32 pace[2] = {0,0};

    for (int i=0 ; i<S.hit ; i++) {
    //for (int i=0 ; i<600 ; i++) {
        s8  dt  = S.dts[i];
        s8  kmh = G.kmhs[i];
        u16 pt  = ((u16)kmh)*((u16)kmh);

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
                G.ps[1-(i%2)] += pt;
                pace[0] += kmh;
                pace[1] += kmh*kmh;

                // bests
                s8* vec = G.bests[ 1-(i%2) ][ dt>0 ];
                for (int j=0; j<HITS_BESTS; j++) {
                    if (kmh > vec[j]) {
                        for (int k=HITS_BESTS-1; k>j; k--) {
                            vec[k] = vec[k-1];
                        }
                        vec[j] = kmh;
                        break;
                    }
                }
            }

            G.time += (dt>0 ? dt : -dt);
        }
    }
    G.time *= 10;

    G.pace[0] = pace[0]/G.hits;
    G.pace[1] = sqrt(pace[1]/G.hits);

    PT_Bests_All();

    u32 pct   = min(990, Falls()*CONT_PCT);
    u32 avg   = (G.ps[0] + G.ps[1]) / 2;
    u32 total = (S.equilibrio ? min(avg, min(G.ps[0],G.ps[1])*11/10) : avg);
    G.total   = total * (1000-pct) / 100000;
}
