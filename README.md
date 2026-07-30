# Sistema di Transizioni Bancarie Distribuite con Mutua Esclusione a Token Ring
Progetto che simula un sistema ATM distribuito basato su **mutua esclusione tramite Token Ring**.  
Ogni ATM è un processo indipendente che comunica tramite socket TCP.

## Tecnologie
- Linguaggio: **C++**
- Librerie: standard C++ + POSIX (`socket`, `thread`, `chrono`)

## Compilazione
```
g++ atm.cpp -o atm -pthread
```

## Esecuzione
Il sistema è composto da 4 nodi, ciascuno avviato su un terminale distinto:
```
./atm 1
./atm 2
./atm 3
./atm 4
```
- Ogni nodo ascolta sulla porta 5000 + idNodo
- I nodi sono collegati in un anello logico (token ring)
- ATM1 genera il token iniziale dopo input da tastiera

## Funzionamento
- Il token contiene il saldo condiviso e il contatore delle transazioni
- Solo il nodo che possiede il token accede alla sezione critica
- Ogni ATM esegue una transazione deterministica ---> ATM2: prelievo 200€; ATM3: deposito 100€; ATM4: prelievo 500€
- Il token viene poi inoltrato al nodo successivo
- Se un nodo non è ancora attivo, il sistema ritenta automaticamente la connessione.
