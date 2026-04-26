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
Il dataset utilizzato e' di tipo sintetico, cioe' e' un dataset creato appositamente per questo framework. Il motivo per cui e' sintetico e' semplice, ci serve un riferimento di base per capire se quello che stiamo facendo e' giusto o meno. Con dei dati reali non potremmo avere una stima di riferimento perche' appunto si ritorna al problema iniziale, cioe' che il conteggio esame diventa troppo oneroso al crescere dei dati.

Ogni file del dataset contiene 50 partizioni, dove ogni partizione contiene n elementi totali e d elementi distinti.
Una partizione e' strutturata esattamente come l'esempio in tabella.
Cioe' abbiamo dei blocchi che sono lunghi rho, che e' il numero di elementi totali su quelli distinti.
Ad ogni blocco introduciamo un nuovo elemento preso da una lista di elementi e poi per i restanti elementi del blocco scegliamo quelli che sono gia' venuti fuori.
In questa maniera all'inizio di ogni blocco abbiamo esattamente un elemento nuovo, questo ci permette di calcolare rapidamente il numero di elementi distinti sapendo solo quanti elementi abbiamo visto.

Inoltre, per ogni elemento del dataset abbiamo affiancato anche un bit di verita' che ci indica se l'elemento e' nuovo. La somma di questi bit ci permette anch'essa di calcolare il numero di elementi distinti visti fino ad un certo instante.

Infine questo dataset e' in un formato binario ed e' stato pure compresso, per esempio con n a 1 milione di elementi si occupano circa 1.7 GB. Aumentare anche solo di un ordine di grandezza n implica aumentare di 10x la dimensione del file.

# Slide 12 (Grafici)
Adesso andremo a vedere 3 grafici significativi sui risultati. 
Il primo va a misurare la cardinalita' stimata rispetto alla cardinalita' reale fissando il numero di elementi distinti per numero di elementi totali. 

Il secondo va a verificare quanto la stima degrada nel caso in cui si riducano il numero di registri per lo stesso algoritmo

E il terzo va a misurare quanto degrada la stima quando si effettua un operazione di merge tra due sistemi eterogenei.

# Slide 13 (Cardinalita' stimata rispetto alla cardinalita' reale)
In questi due grafici, uno in scala lineare e uno in scala logaritmica, possiamo notare che tra i 4 algoritmi analizzati, quello rosso (probabilistic counting) e' il peggiore perche' devia dal risultato ottimale.
Tra i 3 algoritmi rimasti, sembrerebbe che performino in maniera uguale, ma all'inizio della stima quando la cardinalita' degli elementi e' bassa, loglog sovrastima, e questo si puo' notare guardando il grafico in scala logaritmica. I due restanti algoritmi HLL e HLL++ performano in maniera molto simile.

# Slide 14 (Confronto tra diverse precisioni per lo stesso algoritmo)
In questo grafico possiamo notare che HLL++ performa in maniera ottimale e devia pochissimo dal risultato atteso. Infatti e' l'algoritmo di punta di Google per questi problemi. HLL++ ha queste ottime performance perche' Google ha rilasciato delle correzioni empiriche per ogni numero di registro per sistemare il bias dell'algoritmo.

HLL invece tende a performare peggio quando gli si riduce il numero di registri che e' corretto. E soprattutto non ha le stesse correzioni di HLL++.

LogLog come PC non performano in maniera ottimale, per di piu' di puo' notare che PC ha un problema di saturazione dei registri perche' le stime tendono ad appiattirsi.

Questo si vede benissimo nei grafici in scala Log che LogLog sovrastima all'inizio e PC invece tende a saturarsi in fretta. Soprattutto se gli si diminuisce la memoria.

# Slide 15 (Merge di sistemi eterogenei)
In questo grafico il contesto e' il seguente: 
1) l'algoritmo utilizzato e' HLL++
2) Il numero di registri varia da 8 a 18 con aumenti di 2.
3) A sx possiamo vedere cosa accade quando si uniscono due sketch a registri differenti normalizzando, l'errore dell'algoritmo diventa l'errore dello sketch con precisione inferiore; a dx invece unendoli senza fare alcuna normalizzazione l'errore relativo medio all'aumentare degli elementi distinti peggiora enormemente.

# Slide 16 (Conclusioni)
Ricapitolando, in questa tesi si e' sviluppato un intero framework per l'aggiunta di algoritmi che risolvono il problema del calcolo della cardinalita', completamente modulare, che si puo' aggiungere qualunque dataset, modificarne le metriche ci interessano e come deve comportarsi in caso di merge.

Abbiamo visto che HLL++ e' l'algoritmo che risulta piu' attendibile.

Nel caso del merge distribuito abbiamo verificato che il caso omogeneo equivale all'unione seriale di due dataset.

Abbiamo definito una terminologia per la compatibilita' semantica di due sketch e quando/come e' possibile unirle.

Infine abbiamo verificato che il caso recoverable richiede una normalizzazione esplicita prima di poter effettuare l'operazione di unione e che c'e' un degrado dovuto alla precisione inferiore tra le due sketch.

# Slide 17 (Limiti e sviluppi futuri)
Una delle prime cose che si puo' fare con questo framework e' l'utilizzo di dataset reali che possano prendere dei dati da un flusso continuo come ad esempio da Apache Kafka.

Si potrebbe fare della ricerca su quando convenga fare l'operazione di merge in topologie
per minimizzare le perdite di sketch.

Oppure si potrebbero implementare delle sketch che risolvono altri problemi come il problema di appartenenza di un elemento ad un insieme, o il calcolo dei moments-frequency, che vanno a stimare quanto ogni elemento di un insieme e' frequente.

# Slide 18 (Conclusioni)
Bene, ho finito, grazie per l'attenzione e se avete delle domande...
