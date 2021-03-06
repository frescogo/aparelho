void PT_Bests_Lado (int n, s8* bests, Lado* lado) {
    u32 sum = 0;
    for (int i=0; i<n; i++) {
        s8 v = bests[i];
        if (v == 0) {
            break;
        }
        sum += v;
    }
    int golpes = min(n, lado->golpes);
    lado->minima = bests[n-1];
    lado->maxima = bests[0];
    lado->media1 = (golpes == 0) ? 0 : sum*100/golpes;
    lado->pontos = sum;
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
    G.atas  = 0;
    G.servs = 0;

    static s8 bests[2][2][REF_HITS]; // kmh (max 125kmh/h)
    memset(bests, 0, 2*2*REF_HITS*sizeof(s8));

    for (int i=0; i<2; i++) {
        for (int j=0; j<2; j++) {
            G.jogs[i].lados[j].golpes = 0;
        }
    }

//Serial.println("---");
    for (int i=0 ; i<S.hit ; i++) {
        s8 dt  = S.dts[i];
//Serial.println((int)dt);
        s8 kmh = KMH(i);

        if (dt == HIT_SERV) {
            G.servs++;
        }

        if (dt==HIT_NONE || dt==HIT_SERV) {
            continue;
        }

        G.time += (dt>0 ? dt : -dt);

        if (i==S.hit-1 || S.dts[i+1]==HIT_NONE || S.dts[i+1]==HIT_SERV) {
            continue; // ignore last hit
        }

        G.hits++;

        if (kmh < HIT_KMH_50) {
            continue;
        }

        G.atas++;

        int is_rev = (dt<0 && S.reves);
        int player = 1 - (i%2);

        G.jogs[player].lados[is_rev].golpes++;

        // bests
        s8* vec = bests[player][is_rev];
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
    G.time *= 10;
    G.time = min(G.time, S.timeout);

    G.ritmo = (((u32)G.hits)*S.distancia*36) / G.time;

    for (int i=0; i<2; i++) {
        Jog* jog = &G.jogs[i];
        PT_Bests_Lado(HITS_NRM, bests[i][LADO_NRM], &jog->lados[LADO_NRM]);
        PT_Bests_Lado(HITS_REV, bests[i][LADO_REV], &jog->lados[LADO_REV]);
        G.jogs[i].pontos = (u32)jog->lados[LADO_NRM].pontos +
                           (u32)jog->lados[LADO_REV].pontos * (S.reves ? 1 : 0);
//Serial.println((int)jog->lados[LADO_NRM].golpes);
    }

    u16 avg, min_;
    PT_Equ(&avg,&min_);

    int pct    = Falls() * CONT_PCT;
    u32 pontos = (S.equilibrio ? min_ : avg);
    G.pontos   = pontos * (10000-pct) / 10000;
}
