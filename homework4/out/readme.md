# Yocto/Particle: Tiny Particle Simulator

In questo homework, impareremo i principi di base della simulazione
implementando un semplice simulatore a particelle. In particolare, impareremo come

- scrivere un simulatore usando mass-springs,
- scrivere un simulatore usando position-based dynamics,

## Framework

Il codice utilizza la libreria [Yocto/GL](https://github.com/xelatihy/yocto-gl),
che è inclusa in questo progetto nella directory `yocto`.
Si consiglia di consultare la documentazione della libreria che si trova
all'inizio dei file headers. Inoltre, dato che la libreria verrà migliorata
durante il corso, consigliamo di mettere star e watch su github in modo che
arrivino le notifiche di updates.

Il codice è compilabile attraverso [Xcode](https://apps.apple.com/it/app/xcode/id497799835?mt=12)
su OsX e [Visual Studio 2019](https://visualstudio.microsoft.com/it/vs/) su Windows,
con i tools [cmake](www.cmake.org) e [ninja](https://ninja-build.org)
come mostrato in classe e riassunto, per OsX,
nello script due scripts `scripts/build.sh`.
Per compilare il codice è necessario installare Xcode su OsX e
Visual Studio 2019 per Windows, ed anche i tools cmake e ninja.
Come discusso in classe, si consiglia l'utilizzo di
[Visual Studio Code](https://code.visualstudio.com), con i plugins
[C/C++](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools) e
[CMake Tools](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cmake-tools)
extensions, che è già predisposto per lavorare su questo progetto.

Questo homework consiste nello sviluppare vari simulazioni che, per semplicità,
sono tutti contenuti nel file `yocto_particle.cpp`. Il simulatore è invocato
sia da riga di comando che da in una applicazioen interattiva.

Questo repository contiene anche tests che sono eseguibili da riga di comando
come mostrato in `run.sh`. Le immagini generate dal runner sono depositate
nella directory `out/`. Questi risultati devono combaciare con le immagini nella
directory `check/`.

## Funzionalità (26 punti)

In questo homework dovrete implementare le seguenti features:

- **Inizializzazione del Simulatore** nella funzione `init_simulation()`
  - inizializza lo state delle particelle dato lo stato iniziale `xxx = initial_xxx`
  - per ogni vertice in `pinned` setta la `invmass` a 0, per bloccare il punto
  - aggiungi una velocità random alle particelle settata come `s * rngscale * r`
    dove `s` è un punto random sulla sfera, vedi `sample_sphere()`, e
    `r` un numero casuale tra 0 e 1
  - crea le molle tra se `spring_coeff > 0`
    - metti una molla per ogni lato dei quad
    - ricordati di avere lati non ripetuti, ad esempio usando `shape::get_edges()`
    - metti una molla per ogni diagonale di ogni quad (2 oer quad)
  - inizializza il bvh di ogni collider, vedi `shape::make_xxx_bvh()`
- **Simulatore Mass-Spring** nella funzione `simulate_massspring()`:
  - implementare un simulatore mass-spring seguendo le slides e la traccia nel codice
- **Simulatore Position-Based** nella funzione `simulate_pbd()`:
  - implementare un simulatore position-based seguendo le slides e la traccia nel codice

## Extra Credit (8 punti)

- **AYOP**, add your own particle (2 punti per effetto):
  - aggiungete altri effetti al particle systems come forze addizionali,
    vento, vortici, etc.
  - cerca su Google o Shadertoy effetti interessanti o guarda demo
    del particle system di blender
- **AYOE**, add your own effect (4 punti per effetto):
  - aggiungete altri effetti al simulatore position-based seguendo i links
    dati nelle slides o in [Flex](http://blog.mmacklin.com/project/flex/)
- **AYOC**, add your own collisions (8 punti):
  - implementare le collisioni tra particelle come descritto in
    [Flex](http://blog.mmacklin.com/project/flex/);
    per calcolare le intersezioni, potete usare `hash_grid` in Yocto/Shape

## Istruzioni

Per consegnare l'homework è necessario inviare uno ZIP che include il codice e
le immagini generate, cioè uno zip _con le sole directories `yocto_particle` e `out`_.
_In `out` includete anche degli preview dell'animazione_ fatte con `yparticleviews`,
come trovate in `check/*,mp4`.
Per chi fa l'extra credit, includere anche `apps` e ulteriori immagini.
Il file va chiamato `<cognome>_<nome>_<numero_di_matricola>.zip`
e vanno escluse tutte le altre directory. Inviare il file su Google Classroom.

Per registrarle potete usare QuickTime sul mac, Game bar su Windows 10, etc.
Il file deve essere in formato MP4. Se trovate buone soluzioni per Windows
o Linux, inviatele su Classroom in modo che tutti le possano usare.

Nei video, _non vi preoccupate della velocità della simulazione_, nè del
colore e della dimensione delle particelle.
Dipende molto dal computer che avete e non stiamo facendo alcun tipo di
ottimizzazione.
