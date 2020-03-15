# libc_AlloKation
Librairie C pour remplacer l'allocateur mémoire de la libc.

Codé par Alan Vaquet et Killian Babilotte.
## Installation
Pour installer la librairie lancer depuis ce répertoire la commande `make`, le
`.so` constituant la librairie se trouvera installé dans le répertoire `./build/`
sous le nom `libmAlloK.so` et son fichier header se trouve dans `./src/header/`
sous le nom `mAlloK.h`.
En lançant la commande `sudo make install` ces fichiers seront copié dans les
reportoires respectifs `/usr/local/lib/` et `/usr/local/include` et peuvent être
supprimé en lançant la commande `sudo make uninstall`.

## LD_PRELOAD
Pour préloader la librairie sur un programme ou une commande système définir la
variable d'environment LD_PRELOAD comme étant le chemin jusqu'au fichier
`libmAlloK_preload_wrapper.so` dans le dossier `./build/` 
Exemple :
```bash
   $ LD_PRELOAD=./build/libmAlloK_preload_wrapper.so apropos
```

## Lancer un test de performance 
Ce déplacer dans le répertoire `./test/perf_test/` et lancer le script `test.sh`
puis une fois la commande terminée, pour visualiser les résultats, lancer la
commande `gnuplot plot.sh`, faire défiler les résultats en appuyant sur la touche
entrée.

Il est aussi possible de comaparer les performances de notre bibliothèque avec
celle de la libc en utilisant la commande time et LD_PRELOAD.
Exemple :
```bash
   $ time LD_PRELOAD=./build/libmAlloK_preload_wrapper.so apropos
apropos comment ?

real	0m0,139s
user	0m0,138s
sys	0m0,002s

   $ time apropos
apropos comment ?

real	0m0,071s
user	0m0,071s
sys	0m0,000s
```

