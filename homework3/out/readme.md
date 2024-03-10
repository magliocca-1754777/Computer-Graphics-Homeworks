# Yocto/Modeling: Tiny Procedural Modeler

In questo homework, impareremo i principi di base del procedural modeling.
In particolare, impareremo come

- generare terreni,
- generare capelli,
- generare instanze

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

Questo homework consiste nello sviluppare varie procedure che, per semplicità,
sono tutte contenute nel file `yscenegen.cpp`. A differenze degli homework
precedenti non abbiamo definito una libreria e una applicazione, ma solo una
applicazione. L'applicazione genera piccole scene prodecurali che vanno salvate e
visualizzate con un renderer. Includiamo anche i renderer di yocto in
`yscenetrace.cpp` come interfaccia a riga di comando, e da `ysceneitraces.cpp`
che mostra una semplice interfaccia grafica.

Questo repository contiene anche tests che sono eseguibili da riga di comando
come mostrato in `run.sh`. Le immagini generate dal runner sono depositate
nella directory `out/`, mentre le scene sono depositate in `outs/`.
Questi risultati devono combaciare con le immagini nella
directory `check/`.

## Funzionalità (26 punti)

In questo homework verranno implementate le seguenti funzionalità:

- **Procedural Terrain** nella funziont `make_terrain()`:
  - creare il terraon spostando i vertici lungo la normale
  - l'altezza di ogni vertice va calcolata con un `ridge()` noise,
    che dovete implementare, moltiplicato per `1 - (pos - center) / size`
  - applicare poi i colori ai vertici in base all'altezza, `bottom`
    per il 33%, `middle` dal 33% al 66% e `top` il resto
  - alla fine calcolate vertex normals usando le funzioni in Yocto/Shape
- **Procedural Dispalcement** nella funzione `make_displacement()`:
  - spostare i vertici lungo la normale per usando `turbulence()` noise,
    che dovete implementare
  - colorare ogni vertice in base all'altezza tra `bottom` e `top`
  - alla fine calcolate vertex normals usando le funzioni in Yocto/Shape
- **Procedural Hair** nella funzione `make_hair()`:
  - generare `num` capelli a partire dalla superficie in input
  - ogni capello è una linea di `steps` segmenti
  - ogni capello inizia da una punto della superficie ed e' direzionato
    lungo la normale
  - il vertice successivo va spostato lungo la direzione del capello di
    `length/steps`, perturbato con `noise()` e spostato lungo l'asse y
    di una quantità pari alla gavità
  - il colore del capelli varia lungo i capelli da `bottom` a `top`
  - per semplicità abbiamo implementato la funzione `sample_shape()` per
    generare punti sulla superficie e `add_polyline()` per aggiungere un
    "capello" ad una shape
  - alla fine calcolate vertex tangents usando le funzioni in Yocto/Shape
- **Procedural Grass** nella funzione `trace_texcoord()`:
  - generare `num` fili d'erba instanziando i modelli dati in input
  - per ogni modello, le instanze vanno salvate in `object->instance`
  - per ogni filo d'erba, scegliere a random l'oggetto da instanziare e
    metterlo sulla superficie in un punto samplato e orientato lungo la normale
  - per dare variazione transformare l'instanza applicando, in questo ordine,
    uno scaling random tra 0.9 e 1.0, una rotazione random attorno all'asse
    z tra 0.1 e 0.2, e una rotazione random attorno all'asse y tra 0 e 2 pi.

## Extra Credit (8 punti)

Anche in questo homework suggeriamo extra credit. Scegliete gli extra credit
che desiderate. Il punteggio sarà la somma dei punteggi degli extra credits
corrtti, fino al massimo indicato sopra.

- **Better Procedurals**, make it better – nope, not Man in Black (4 punti):
  - scrivere una nuova procedure che migliora i risultati ottenuti dai
    generatori precedenti, come ad esempio
    - displacement modellati su superfici reali
    - capelli più realistici, ad esempio controllandone la densità
      attraverso textures
    - capelli molto lunghi con forme interessanti, tipo flow noise
    - erba migliore, ad esmepio controllandone la densità e
      mischiandola con fiori e rocce
- **Sample Elimination** (6 punti):
  - migliorare il posizionamento di punti sulla superficie;
    mostrare i risulati mettendo punti o piccole sfere sulla superficie e
    confrontando random sampling e questi metodi
  - seguire la pubblicazione [sample elimination](http://www.cemyuksel.com/research/sampleelimination/)
  - potete basarvi sull'implementazione dell'autore, ma deve essere tutta
    portata in Yocto/GL, e non linkata
  - l'algoritmo richiede di trovare dei nearest neighbors; per farlo usate
    o `hash_grid` in Yocto/Shape o `nanoflann` su GitHub
- **Poisson Point Set**, Poisson point set in 2D (4 punti)
  - seguire la pubbliacazione in
    [poisson sampling](https://www.cct.lsu.edu/~fharhad/ganbatte/siggraph2007/CD2/content/sketches/0250.pdf)
  - trovate varie implementazioni di questo metodo, inclusa una dell'autore
  - dimostrate il risultato mettendo punti su in piano
- **Procedural Noises** (2 punti per ogni pattern):
  - implementate altri patterns di tipo procedurale prendendo spunto da ShaderToy
  - esempi possibile [Voronoise](https://www.iquilezles.org/www/articles/voronoise/voronoise.htm),
    [PhasorNoise](https://www.shadertoy.com/view/WtjfzW),
    [CellNoise](https://www.iquilezles.org/www/articles/voronoilines/voronoilines.htm),
    [SmoothVoronoi](https://www.iquilezles.org/www/articles/smoothvoronoi/smoothvoronoi.htm)
  - dimostrate il risultato su una superficie

## Istruzioni

Per consegnare l'homework è necessario inviare una ZIP che include il codice e
le immagini generate, cioè uno zip _con le sole directories `yscenegen` e `out`_.
Per chi fa l'extra credit, includere anche `apps`, ulteriori immagini, e un
file **`readme.md`** che descrive la lista di extra credit implementati.
Per chi crea nuovi modelli, includere anche le directory con i nuovo modelli creati.
Il file va chiamato `<cognome>_<nome>_<numero_di_matricola>.zip`
e vanno escluse tutte le altre directory. Inviare il file su Google Classroom.
