enum {
    PC_RESTART = 0,
    PC_SEQ     = 1,
    PC_HIT     = 2,
    PC_TICK    = 3,
    PC_FALL    = 4,
    PC_END     = 5
};

void PC_Restart (void) {
    Serial.print(PC_RESTART);         // codigo de reinicio
    Serial.print(F(";"));
    Serial.print(S.timeout/1000);     // tempo total de jogo
    Serial.print(F(";"));
    Serial.print(S.names[0]);         // atleta a esquerda
    Serial.print(F(";"));
    Serial.print(S.names[1]);         // atleta a direita
    Serial.print(F(";"));
    Serial.println();
}

void PC_Seq (void) {
    Serial.print(PC_SEQ);             // codigo de nova sequencia
    Serial.print(F(";"));
    Serial.print(G.time/1000);        // tempo jogado em ms
    Serial.print(F(";"));
    Serial.print(Falls());            // total de quedas
    Serial.print(F(";"));
    Serial.print(S.names[0]);         // atleta a esquerda
    Serial.print(F(";"));
    Serial.print(S.names[1]);         // atleta a direita
    Serial.print(F(";"));
    Serial.println();
}

void PC_Player (int I) {
    Serial.print(G.ps[I]/100);        // pontuacao
    Serial.print(F(";"));
    Serial.print(PT_Behind() == I ? 1 : 0);   // 1=atras | 0=ok
    Serial.print(F(";"));

    int Rtot1,Ravg1, Rtot2,Ravg2, Rmin_, Rmax_;
    int Ntot1,Navg1, Ntot2,Navg2, Nmin_, Nmax_;
    PT_Bests(G.bests[I][0], &Rtot1,&Ravg1, &Rtot2,&Ravg2, &Rmin_, &Rmax_);
    PT_Bests(G.bests[I][1], &Ntot1,&Navg1, &Ntot2,&Navg2, &Nmin_, &Nmax_);

    int Rtot, Ntot, Ravg, Navg, Rmax, Nmax;
    if (Ravg1 <= Navg1) {
        Rtot = Rtot2;
        Ravg = Ravg2;
        Rmax = Rmax_;
        Ntot = Ntot1;
        Navg = Navg1;
        Nmax = Nmax_;
    } else {
        Rtot = Ntot2;
        Ravg = Navg2;
        Rmax = Nmax_;
        Ntot = Rtot1;
        Navg = Ravg1;
        Nmax = Rmax_;
    }

    Serial.print(Rtot);               // total de revezes
    Serial.print(F(";"));
    Serial.print(Ravg);               // media de revez
    Serial.print(F(";"));
    Serial.print(Rmax);               // maxima de revez
    Serial.print(F(";"));

    Serial.print(Ntot);               // total normais
    Serial.print(F(";"));
    Serial.print(Navg);               // media normal
    Serial.print(F(";"));
    Serial.print(Nmax);               // maxima normal
    Serial.print(F(";"));
}

void PC_Hit (int player, int is_back, int kmh) {
    Serial.print(PC_HIT);             // codigo de golpe
    Serial.print(F(";"));
    Serial.print(player);             // 0=esquerda | 1=direita
    Serial.print(F(";"));
    Serial.print(is_back);            // 0=normal   | 1=revez
    Serial.print(F(";"));
    Serial.print(kmh);                // velocidade
    Serial.print(F(";"));
    PC_Player(player);
    Serial.println();
}

void PC_Tick (void) {
    Serial.print(PC_TICK);            // codigo de tick
    Serial.print(F(";"));
    Serial.print(G.time/1000);        // tempo jogado em ms
    Serial.print(F(";"));
    Serial.print(G.total);            // total da dupla
    Serial.print(F(";"));
    Serial.print(G.hits);             // total de golpes
    Serial.print(F(";"));
    Serial.print((int)G.pace[0]);     // media simples
    Serial.print(F(";"));
    Serial.println();
}

void PC_Fall (void) {
    Serial.print(PC_FALL);            // codigo de queda
    Serial.print(F(";"));
    Serial.print(Falls());            // total de quedas
    Serial.print(F(";"));
    Serial.println();
}

void PC_End (void) {
    Serial.print(PC_END);             // codigo de fim
    Serial.print(F(";"));
    Serial.println();
}

void PC_Nop (void) {
}
