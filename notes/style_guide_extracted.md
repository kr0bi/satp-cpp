# Style guide estratto (cap. 2-3)

Fonti analizzate:
- `/Users/daniele/CLionProjects/satp-cpp/thesis/chapters/2-background.tex`
- `/Users/daniele/CLionProjects/satp-cpp/thesis/chapters/3-analisi_stato_arte.tex`

## 1) Pattern di struttura sezione/sottosezione

- Pattern macro dei capitoli:
  - Cap.2: struttura soprattutto per `\section{...}` (9 sezioni, 0 sottosezioni), forte base teorica con molte `definition` (23).
  - Cap.3: struttura gerarchica `\section` + `\subsection` (6+6), organizzata per linea storica/algoritmo.
- Pattern ricorrente (soprattutto cap.3, per ogni algoritmo):
  - Introduzione breve del metodo.
  - `\begin{definition}[...]`.
  - `\begin{algorithm}[htbp]` (se applicabile).
  - `\paragraph{Esempio.}`.
  - `\paragraph{Complessità}`.
  - opzionale `\paragraph{Limiti ...}`.
- Pattern didattico cap.2:
  - definizione formale -> mini spiegazione discorsiva -> talvolta `\paragraph{Esempio.}`.

Evidenze brevi:
- `\section{Modello di stream di dati}` (`/Users/daniele/CLionProjects/satp-cpp/thesis/chapters/2-background.tex:7`)
- `\section{Sviluppo storico}` + `\subsection{Probabilistic Counting}` (`/Users/daniele/CLionProjects/satp-cpp/thesis/chapters/3-analisi_stato_arte.tex:46`, `/Users/daniele/CLionProjects/satp-cpp/thesis/chapters/3-analisi_stato_arte.tex:52`)
- `\paragraph{Complessità}` ricorrente (`/Users/daniele/CLionProjects/satp-cpp/thesis/chapters/3-analisi_stato_arte.tex:155`, `/Users/daniele/CLionProjects/satp-cpp/thesis/chapters/3-analisi_stato_arte.tex:256`, `/Users/daniele/CLionProjects/satp-cpp/thesis/chapters/3-analisi_stato_arte.tex:351`, `/Users/daniele/CLionProjects/satp-cpp/thesis/chapters/3-analisi_stato_arte.tex:485`, `/Users/daniele/CLionProjects/satp-cpp/thesis/chapters/3-analisi_stato_arte.tex:682`, `/Users/daniele/CLionProjects/satp-cpp/thesis/chapters/3-analisi_stato_arte.tex:738`)

## 2) Pattern lessicali e terminologici

- Registro tecnico-formale in italiano con anglicismi controllati in `\emph{}` o `\textit{}`.
- Introduzione acronimi al primo uso con forma estesa + sigla:
  - `Probabilistic Counting with Stochastic Averaging (PCSA)` (`/Users/daniele/CLionProjects/satp-cpp/thesis/chapters/3-analisi_stato_arte.tex:60`)
  - `Relative Standard Error (RSE)` (`/Users/daniele/CLionProjects/satp-cpp/thesis/chapters/2-background.tex:496`)
- Notazione matematica coerente e persistente: `F_0`, `\hat{F}_0`, `m`, `(\varepsilon,\delta)`.
- Uso sistematico di lessico operativo comune: `update`, `query`, `merge`, `bias`, `varianza`, `small-range/raw-range/large-range`.

Esempi brevi:
- "strutture compatte, chiamate **sketch**" (`/Users/daniele/CLionProjects/satp-cpp/thesis/chapters/2-background.tex:114`)
- "stato `M` ... operazioni *update*/*query*" (`/Users/daniele/CLionProjects/satp-cpp/thesis/chapters/2-background.tex:316-317`)
- "uso di *linear counting*" (`/Users/daniele/CLionProjects/satp-cpp/thesis/chapters/3-analisi_stato_arte.tex:560`)

## 3) Convenzioni su voce/tempo/persona

- Prevalenza di forma impersonale/passiva tecnica:
  - "si richiede", "si introduce", "viene mostrato", "vengono calcolati".
- Prima persona plurale usata con parsimonia, solo per scelte metodologiche o roadmap:
  - "In questa tesi adottiamo..." (`/Users/daniele/CLionProjects/satp-cpp/thesis/chapters/2-background.tex:41`)
  - "Nel seguito distinguiamo..." (`/Users/daniele/CLionProjects/satp-cpp/thesis/chapters/2-background.tex:506`)
- Tempo prevalente: presente indicativo per definizioni/proprietà; futuro usato per anticipazioni dei capitoli successivi.

Esempi brevi:
- "si introduce la nozione..." (`/Users/daniele/CLionProjects/satp-cpp/thesis/chapters/2-background.tex:85`)
- "In \cite{...} viene mostrato..." (`/Users/daniele/CLionProjects/satp-cpp/thesis/chapters/3-analisi_stato_arte.tex:38`)
- "verrà verificata empiricamente" (`/Users/daniele/CLionProjects/satp-cpp/thesis/chapters/2-background.tex:632`)

## 4) Convenzioni di citazione

- Comando usato: quasi esclusivamente `\cite{...}` (non emergono `\autocite`/`\textcite`).
- Posizionamento tipico:
  - a fine frase prima del punto;
  - dentro la frase in forma narrativa ("In `\cite{...}` ...");
  - anche in titolo di `definition` quando rilevante.
- Citazioni multiple: chiavi separate da virgola nello stesso `\cite{...}`.
- Collegamento costante tra claim teorico e fonte primaria.

Esempi brevi:
- fine frase: `... \cite{Muthukrishnan_2005}.` (`/Users/daniele/CLionProjects/satp-cpp/thesis/chapters/2-background.tex:151`)
- narrativa: `In \cite{Kane_Nelson_Woodruff_2010} ...` (`/Users/daniele/CLionProjects/satp-cpp/thesis/chapters/3-analisi_stato_arte.tex:38`)
- multipla: `\cite{Flajolet_Fusy_Gandouet_Meunier_2007,Agarwal_Cormode_Huang_Phillips_Wei_Yi_2012}` (`/Users/daniele/CLionProjects/satp-cpp/thesis/chapters/2-background.tex:578`)

## 5) Convenzioni figure/tabelle/caption

- Float quasi sempre con `[htbp]`.
- Ordine interno coerente: `\caption{...}` prima di `\label{...}`.
- Riferimenti nel testo con spazio non separabile:
  - `Figura~\ref{...}`, `Tabella~\ref{...}`, `Capitolo~\ref{...}`, `Sezione~\ref{...}`.
- Cap.3 usa tabelle operative con setup tipico:
  - `\small`, `\setlength{\tabcolsep}{...}`, `\renewcommand{\arraystretch}{...}`, `\resizebox{\textwidth}{!}{...}`.
- Algoritmi: ambiente `algorithm` + `algorithmic`, caption spesso "adapted from ..." e contenuto pseudocodice in inglese.
- Eccezione stilistica presente: una `wrapfigure` con `\captionsetup{...}` dedicato (`/Users/daniele/CLionProjects/satp-cpp/thesis/chapters/2-background.tex:340-347`).

Esempi brevi:
- Figura: `\caption{Pipeline ...}` + `\label{fig:streaming-pipeline}` (`/Users/daniele/CLionProjects/satp-cpp/thesis/chapters/2-background.tex:164-165`)
- Tabella: `\caption{Esempio operativo di LogLog ...}` (`/Users/daniele/CLionProjects/satp-cpp/thesis/chapters/3-analisi_stato_arte.tex:246`)
- Algoritmo: `\caption{HyperLogLog++ (adapted from Fig. 6 in \cite{...})}` (`/Users/daniele/CLionProjects/satp-cpp/thesis/chapters/3-analisi_stato_arte.tex:416`)

## 6) Anti-pattern da evitare (allineamento a cap.2-3)

- Evitare tono colloquiale/conversazionale:
  - esempio da non replicare: "Adesso andremo a definire..." (`/Users/daniele/CLionProjects/satp-cpp/thesis/chapters/2-background.tex:385`).
- Evitare cambi di lingua non motivati nello stesso blocco (italiano + pseudo-inglese) se non standardizzati.
- Evitare caption non uniformi nello stesso capitolo (con/senza punto finale): scegliere una regola e mantenerla.
- Evitare periodi troppo lunghi con molte subordinate quando la stessa informazione può essere spezzata in 2 frasi.
- Evitare acronimi introdotti senza espansione iniziale.
- Evitare affermazioni teoriche senza citazione adiacente.

## Da applicare a cap.4-5

- [ ] Mantieni struttura per-sezione con flusso: contesto -> definizione/formula -> esempio -> complessità/limiti.
- [ ] Usa forma impersonale tecnica come default; usa "noi" solo per scelte metodologiche esplicite.
- [ ] Introduci ogni acronimo alla prima occorrenza (nome esteso + sigla).
- [ ] Mantieni notazione coerente con cap.2-3 (`F_0`, `\hat{F}_0`, `m`, `\varepsilon`, `\delta`).
- [ ] Inserisci `\cite{...}` vicino a ogni claim teorico o valore numerico non originale.
- [ ] Per float: usa `[htbp]`, poi `\caption{...}`, poi `\label{...}`.
- [ ] Richiama sempre oggetti con `Figura~\ref`, `Tabella~\ref`, `Capitolo~\ref`, `Sezione~\ref`.
- [ ] Standardizza lingua e punteggiatura delle caption all'interno dello stesso capitolo.
