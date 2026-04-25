# Slide 1
Buongiorno a tutti, sono Daniele Ferroli e la mia tesi tratta
la stima della cardinalita' nei flussi di dati distribuiti, in particolare andremo a 
vedere come si comportano gli algoritmi di sketching quando si vuole aggregarli.

# Slide 2 (Sketch)
Una sketch e' una struttura dati probabilistica compatta che va a riassumere
delle informazioni su un certo dataset per poter calcolare una stima.

In questo caso il problema trattato e' la stima della cardinalita', cioe' contare gli
elementi distinti all'interno di un flusso di dati.

Questa stima viene fatta con una approssimazione eps., delta che significa
quanto la stima si discosta dal valore reale con una certa precisione epsilon
e una certa probabilita' delta.

# Slide 3 (Applicazioni)
Il problema della stima della cardinalita' ha molteplici utilizzi, i principali possono essere il monitoraggio dei client unici all'interno di una rete Wi-Fi pubblica in aereoporto, oppure l'aggregazione del conteggio dei like all'interno di un video di YouTube distribuiti su piu' nodi o infine la previsione di costo di una join tra due tabelle.

# Slide 4 (Contribuiti)
I contributi della mia tesi riguardano l'implementazione dei piu' famosi algoritmi
di sketching per la stima di cardinalita' come Probabilistic Counting, LogLog e tutta la sua famiglia di migliorie.
Per implementare questi algoritmi si e' dovuto costruire un framework sperimentale,
modulare ed estendibile per l'input/output, la gestione degli algoritmi e le metriche utilizzate.
Inoltre e' stata fatta un'analisi empirica sistematica su questi algoritmi e in particolare sul merge...

# Slide 5 (Merge)
In questo diagramma a sinistra corrisponde il merge distribuito, si parte da n dataset
in cui per ognuno si calcola lo sketch equivalente e poi si uniscono ottenendo la stima.
In un contesto ottimale si vuole che il merge seriale, cioe' l'unione seriale dei dataset da cui poi si calcola lo sketch, corrisponda al merge distribuito.

Per effettuare un operazione di merge pero' si considerano diverse assunzioni, cioe' che vengano utilizzati gli stessi parametri algoritmici, le stesse funzioni di hash e il seed.

Nel caso in cui una tra queste caratteristiche differisca si parla di sistemi eterogenei. Uno dei casi piu' classici di questi sistemi, sono i sistemi IoT dove i nodi del sistema possono differire per spazio o tecnologia.

# Slide 3 (Nuovamente Contribuiti)
In particolare in questa tesi si e' data una classificazione operativa sui tipi di merge che si possono effettuare e cosa accade quando si prova a fare un merge in condizioni non ottimali.

# Slide 6 (Sketch come struttura dinamica)
Ora quando vogliamo implementare una sketch vogliamo diverse caratteristiche per questa struttura dati.
1) L'aggiornamento deve avvenire il piu' velocemente possibile, quindi usando la notazione asintotica si vuole che l'operazione di aggiornamento richieda O(1) tempo
2) L'operazione di query sullo sketch ha come requisito che la stima sia unbiased
3) Questa struttura dati deve occupare pochissima memoria, in particolare si puo' notare che partendo dall'upper-bound sia m il numero di elementi del flusso e N grande il numero di elementi dell'universo, significherebbe che bisogna salvare la codifica degli elementi dell'universo cioe' logN e moltiplicare questa codifica salvando ogni elemento distinto. Invece il lower bound accade, banalmente, quando ci sono solo due elementi distinti e basta salvare una sola delle due codifiche. Un algoritmo del 2010 ottimale, dice che per il caso di algoritmi di sketching questo lower-bound coincide con il risultato ottimale.

# Slide 7 (Sketch LogLog)
Ora vediamo come funziona uno degli algoritmi piu' semplici di sketching, ho scelto
di presentare LogLog.
Dato k che e' la precisione richiesta, si calcola il numero di registri, essi vengono tutti inizializzati a zero.
Il diagramma partendo dall'elemento x viene calcolato il suo hash e si calcola il prefisso lungo k bit e il suffisso lungo L - k.
Per effettuare l'update si calcola il numero di zeri del suffisso (denominato rho(w)) + 1 e lo confronta con M(r) dove r corrisponde all'indice corrispondente calcolato dal prefisso dell'hash, da cui si prende il piu' grande tra i due.
Mentre per il calcolo della stima si va ad utilizzare la formula a destra, dove alpha_m e' una costante di calibrazione definita per il numero di registri, m e' il numero di registri e A e' la media dei valori dei registri. 

Questo algoritmo funziona perche' va a calcolare la probabilita' che un certo evento accada. L'evento e' la rarita' che venga generato un hash con tanti zeri consecutivi di fila. Essenzialmente l'idea e' che se viene visto un suffisso con tanti zeri consecutivi si saranno gia' visti i numeri prima.

# Slide 8 (Versioni moderne)
LogLog era l'algoritmo piu' semplice che c'era, esistono miglioramenti di questo algoritmo. Questi miglioramenti riguardando l'utilizzo della media armonica al posto della media aritmetica, la correzione dei bias dati da sovrastime quando la cardinalita' e' bassa o l'utilizzo di funzioni di hash con piu' bit.

# Slide 9 (Compatibilita' semantica del merge)
Prima abbiamo parlato di quando si puo' effettuare l'operazione di merge, ora possiamo definire 3 casi.
Il primo caso e' quello ottimale, cioe' tutto coincide e si puo' fare direttamente l'operazione di merge, questo caso si chiama valid.
Il secondo e' quando differisce solo la precisione dell'algoritmo e il resto coincide, in questo caso, basta effettuare una normalizzazione e poi si puo' effettuare il merge.
L'ultimo e' quando differisce la funzione di hash, in questo caso non si puo' effettuare alcun tipo di merge, perche' significherebbe cercare di unire due sketch che per lo stesso elemento viene usato un risultato diverso. E' come cercare di unire due lingue diverse.

# Slide 10 (Framework)
Questa e' l'architettura del sistema software, in verde si possono notare gli elementi esterni al framework, come la generazione dei dataset, gli script utilizzati per effettuare gli esperimenti e i notebooks python per la generazione dei grafici.
Al centro dell'architettura c'e' il dataset che viene dato in pasto al framework che va ad effettuare le sue stime e va a scrivere in un CSV i risultati dell'esecuzione.

# Slide 11 (Dataset)
Il dataset utilizzato e' di tipo sintetico, cioe' e' un dataset creato appositamente per questo framework. Il motivo per cui e' sintetico e' semplice, 