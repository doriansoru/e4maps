# Esempio di utilizzo della modalità "assone" in e4maps

## Descrizione
Dal momento che abbiamo aggiunto la possibilità di visualizzare i collegamenti come "assi" o "rami" nello stile "assone", ecco come puoi utilizzare questa nuova funzionalità:

## Come attivare lo stile "assone"

### Modificare un tema esistente

Per modificare un tema affinché utilizzi lo stile "assone":

1. Aprire il file del tema (solitamente con estensione `.theme`)
2. Trovare le sezioni `<style>` che definiscono lo stile per i livelli
3. Aggiungere o modificare l'attributo `conn_type` impostandolo a `1` invece che a `0`

### Esempio di modifica del tema:

Nel file XML del tema:

```xml
<style level="0" bg="#ccddff" border="#4466aa" font="Sans Bold 18" conn_type="1"/>
<style level="1" bg="#e6f3ff" border="#6688cc" font="Sans Bold 14" conn_type="1"/>
<style level="2" bg="#f0f8ff" border="#7799dd" font="Sans 12" conn_type="1"/>
```

### Spiegazione dei valori:
- `conn_type="0"`: Collegamenti come frecce (modalità tradizionale)
- `conn_type="1"`: Collegamenti come rami/assi nello stile "assone" (nuova funzionalità)

## Caratteristiche dello stile "assone"

- I collegamenti vengono disegnati come curve organiche e fluide
- Le linee hanno curve più arrotondate e naturali rispetto alle frecce tradizionali
- Ogni ramo termina con un piccolo cerchio che indica la connessione al nodo figlio
- Lo spessore e il colore delle linee rispettano le impostazioni del tema
- L'intensità della curvatura diminuisce all'aumentare del livello di profondità

## Implementazione tecnica

La nuova funzionalità è stata implementata aggiungendo:

1. Una nuova proprietà `connectionType` alla struttura `NodeStyle` in `Theme.hpp`
2. La logica per disegnare le curve "organiche" attraverso la funzione `drawTonyBuzanBranch` in `MindMapDrawer.hpp`
3. Supporto per la serializzazione/deserializzazione del nuovo parametro nei file di tema

## Importante

Lo stile predefinito ora è "assone" (conn_type=1), quindi tutte le nuove mappe utilizzeranno automaticamente questa modalità. Per tornare alle frecce classiche, impostare conn_type=0 nei temi.

Non è difficile implementare questo stile, come abbiamo visto! La struttura del codice era già ben organizzata per consentire queste modifiche.