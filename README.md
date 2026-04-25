# Rapport de projet — Système Linux embarqué sur STM32MP157C-DK2

**Auteur :** Adnan
**Carte cible :** STMicroelectronics STM32MP157C-DK2 (Cortex-A7)
**Capteur :** MPU6050 (accéléromètre + gyroscope) Adafruit, branché en I²C5
**Framework graphique :** Qt5 avec plateforme `linuxfb`

---
https://github.com/user-attachments/assets/93d5aabe-76e0-4b53-9788-d2305bd78531

## 1. Introduction

L'objectif de ce projet était de **découvrir l'embarqué sous Linux**. C'était la première fois que je faisais ce genre de chose, je suis donc parti de zéro. Pour avoir une base solide, j'ai suivi la formation Bootlin *Embedded Linux system development*, qui ma permis de prendre la main depuis la toolchain de cross-compilation jusqu'à un système Linux complet sur la carte.

Une fois le système opérationnel, j'ai voulu aller plus loin sur la partie graphique. J'avais commencé par afficher des choses directement via le **framebuffer** (`/dev/fb0`), c'est-à-dire une zone mémoire qui représente l'écran pixel par pixel : on y écrit des octets, et chaque octet correspond à la couleur d'un pixel. C'est rapide à mettre en place, mais on doit tout dessiner soi-même : aucune notion de bouton, de texte, de fenêtre...

Pour avoir quelque chose de plus propre, j'ai voulu utiliser **Qt5**, qui apporte des widgets prêts à l'emploi, le support du touchscreen et un rendu 2D bien plus pratique.

Le projet final est une application Qt5 qui lit en temps réel les valeurs du MPU6050 et affiche, sur trois pages navigables au doigt par swipe :

- une page d'accueil,
- un graphique défilant des accélérations en m/s² sur les trois axes,
- une représentation 3D d'un avion dont l'orientation suit celle de la carte.
  
---
## Organisation du projet

```
Linux-stm32mp157/
├── Backup_dtb_image/
│   └── tftp/                          # DTB et image kernel pour le boot via TFTP
├── embedded-linux-stm32mp1-labs/      # Travaux issus de la formation Bootlin
│   └── ...                            # (toolchain, U-Boot, kernel, rootfs, etc.)
├── qt_src/                            # Sources des programmes Qt5
│   ├── hello_qt                       # Hello world Qt
│   ├── MPU/                           # Affichage de l'acceleration + GRAPH
│   ├── MPU_GYRO/                      # Affiche de l'angle et de la 3d de l'avion
│   └── MPU_ACCGYRO/                   # Code Final avec l'accélération et le gyroscope
├── embedded-linux-stm32mp1-labs.tar.xz  # Archive de la formation Bootlin
├── .gitignore
└── README.md                          # Le rapport
```
---

## 2. Travaux réalisés à partir de la formation Bootlin

J'ai suivi la majeure partie de la formation Bootlin *Embedded Linux system development*, en l'adaptant à ma carte (DK2 et non DK1). Voici les grandes étapes effectuées.

### 2.1. Toolchain de cross-compilation

Construction d'une toolchain ARM avec **Crosstool-NG**, ciblant Cortex-A7 / VFPv4 / EABIhf, avec **musl** comme bibliothèque C et **GCC 14.3.0**.

### 2.2. Bootloaders TF-A et U-Boot

Mise en place de la chaîne de boot en deux étages : **TF-A BL2** comme premier étage, puis **U-Boot** comme second étage. Préparation d'une carte SD partitionnée en GPT (`fsbl1`, `fsbl2`, `fip`, `bootfs`) et flashage des binaires.

### 2.3. Réseau et TFTP

Configuration du réseau côté U-Boot et installation de `tftpd-hpa` côté PC, pour charger dynamiquement le kernel et le DTB pendant les développements.

### 2.4. Kernel Linux

Récupération des sources Linux, bascule sur la branche **6.12.y**, configuration personnalisée et compilation. Boot via TFTP avec un `bootcmd` automatisé.

### 2.5. Système racine minimal avec BusyBox

Construction d'un rootfs minimal monté en NFS, avec **BusyBox** statique puis dynamique, montage des filesystems virtuels et création des scripts d'init.

**NFS** (*Network File System*) est un protocole qui permet à la carte de monter un dossier du PC comme s'il était son propre disque. Très pratique pour dévelloper rapidement.

### 2.6. Accès au matériel

- **GPIO** : exploration via `/sys/class/gpio` puis via `libgpiod`.
- **I²C** : activation du bus I²C5 via une modification du Device Tree (création d'un fichier `stm32mp157c-dk2-custom.dts`).
- **MPU6050** : ajout du nœud `mpu6050@68` dans le Device Tree, ce qui permet au kernel de charger automatiquement le driver `inv-mpu6050` et d'exposer le capteur via le sous-système IIO dans `/sys/bus/iio/devices/iio:device0/`.

### 2.7. Bibliothèques tierces

Compilation manuelle (sans build system) de `alsa-lib`, `alsa-utils`, `libgpiod` et `ipcalc` en cross-compilation, avec séparation entre un répertoire *staging* (pour la compilation) et un répertoire *target* (pour le déploiement).

### 2.8. Programmes personnels avec le framebuffer

J'ai écrit plusieurs petits programmes en C compilés avec ma toolchain, notamment un programme de **dessin sur framebuffer** piloté par le touchscreen. Le principe :

- lecture des événements tactiles sur `/dev/input/event0` (`EV_ABS`, `BTN_TOUCH`),
- écriture directe des pixels dans `/dev/fb0` via `mmap`.

**Premier problème rencontré :** au début, le pixel s'affichait au mauvais endroit. Les coordonnées renvoyées par le touchscreen (plage brute du contrôleur tactile) ne correspondent pas du tout à la résolution de l'écran (480×800). Il a fallu **calibrer manuellement** : déterminer les valeurs minimales et maximales renvoyées par le tactile (pour cela j'ai fait un programme qui affiche 4 carrer dans les angles et qui me retourne les valeurs de calibration), puis appliquer une mise à l'échelle pour les ramener dans l'espace de coordonnées de l'écran.

> Réponse du programme de calibration

```bash
=== RESULTAT === 
X_min = 50 
X max = 455 
Y min = 40 
Y max = 771 
=== A UTILISER === 
#define TS_X_MIN 50 
#define TS_X_MAX 455 
#define TS_Y_MIN 40 
#define TS_Y_MAX 771
```

C'est précisément la lourdeur de ce type de programme (tout coder à la main, gérer la calibration, ne même pas pouvoir afficher du texte simplement) qui m'a poussé à passer à Qt.

---

## 3. Qu'est-ce que Qt et pourquoi l'utiliser ?

**Qt** est un framework C++ multiplateforme orienté interfaces graphiques. Sur Linux embarqué, ses avantages sont nombreux :

- gestion native des **widgets** (boutons, labels, layouts),
- prise en charge directe du **touchscreen** via le plugin `evdev` (plus besoin de calibrer à la main, Qt s'en charge),
- plusieurs **plateformes d'affichage**, dont `linuxfb` qui écrit directement sur `/dev/fb0` sans avoir besoin d'X11 ni de Wayland (X11 et Wayland sont les deux serveurs graphiques classiques sous Linux desktop, qui gèrent les fenêtres, le clavier, la souris... 
  C'est lourd et inutile pour mon utilisation en embarquée),
- système de **signaux/slots** et de **timers** très pratique pour les interfaces qui se mettent à jour en continu (typiquement, l'affichage du capteur),
- classes graphiques (`QPainter`, `QPolygonF`) qui permettent de faire de la 2D et de la pseudo-3D rapidement.

La STM32MP157 n'a pas de GPU exploitable. On utilise donc le rendu **logiciel** via le plugin `linuxfb`, ce qui reste largement suffisant pour mon application.

---

## 4. Mise en place de Qt5 dans Buildroot

Compiler Qt à la main aurait été beaucoup trop lourd à cause des nombreuses dépendances. J'ai donc utilisé **Buildroot** pour générer un rootfs complet incluant Qt5.

### 4.1. Pourquoi un nouveau Buildroot ?

Mon système précédent reposait sur **musl** et BusyBox init. Or **Qt5 ne supporte pas la musl** : il faut une toolchain **glibc**. J'ai donc mis en place un Buildroot dédié, avec :

- la toolchain externe **Bootlin armv7-eabihf glibc**,
- mon kernel 6.12.74 avec mon patch custom (DTS de la DK2 + activation I²C5 + nœud `mpu6050`).

### 4.2. Activation de Qt5

Dans `make menuconfig`, j'ai activé sous *Target packages → Qt5* :

- `qt5base` avec les modules `gui` et `widgets`,
- la plateforme `linuxfb`,
- le support `fontconfig` et `PNG`,
- les **polices DejaVu** (sans elles, le texte n'apparaît pas dans Qt, j'ai compris cela apres quelque échec a voulair afficher "hello" sur l'écran).

### 4.3. Problèmes rencontrés

- **Patch kernel non appliqué.** Il fallait renseigner le chemin **absolu** du `.patch` dans Buildroot.
- **TFTP en timeout** : suite de `T T T T` sous U-Boot. Cause : pare-feu UFW activé. Résolu avec `sudo ufw disable`. (J'ai été confronté a ce bug car je faisait un autre projet sur la meme machine et j'avais du activé le parfeu)
- **Mauvais NFS root.** Le `bootargs` U-Boot pointait encore vers l'ancien rootfs. Modification de `nfsroot=...` pour pointer vers `buildroot/nfsroot`.

### 4.4. Compilation et déploiement

Compilation des programmes Qt depuis le PC avec `qmake` fourni par Buildroot :

```sh
~/.../buildroot/output/host/bin/qmake projet.pro
make
cp binaire ~/.../buildroot/nfsroot/root/
```

Lancement sur la cible :

```sh
QT_QPA_PLATFORM=linuxfb /root/binaire
```

---

## 5. Lecture du MPU6050

Une fois le capteur déclaré dans le Device Tree, le kernel l'expose via le sous-système **IIO** (*Industrial I/O*). Tout devient accessible comme de simples fichiers :

```
/sys/bus/iio/devices/iio:device0/
├── in_accel_x_raw       ← accélération brute X
├── in_accel_y_raw       ← accélération brute Y
├── in_accel_z_raw       ← accélération brute Z
├── in_accel_scale       ← coefficient de conversion
├── in_anglvel_x_raw     ← vitesse angulaire brute X
├── in_anglvel_y_raw     ← vitesse angulaire brute Y
├── in_anglvel_z_raw     ← vitesse angulaire brute Z
├── in_anglvel_scale     ← coefficient de conversion
└── in_temp_raw          ← température
```

Pour obtenir les vraies valeurs physiques, on multiplie la valeur brute par le facteur d'échelle :

- `in_accel_scale = 0.000598` → valeur brute × 0.000598 = accélération en **m/s²**,
- `in_anglvel_scale = 0.001064724` → valeur brute × 0.001064724 = vitesse angulaire en degrés/seconde.

À noter : le driver `inv-mpu6050` renvoie l'accélération en **m/s²** et non en *g*. Au repos, posée à plat, la carte mesure donc Z ≈ **9.81 m/s²** (la gravité), et non 1 g comme on pourrait s'y attendre.

Dans le programme, c'est très simple : on ouvre le fichier, on lit l'entier, on le ferme.

```c
int read_value(const char *path) {
    FILE *f = fopen(path, "r");
    int val;
    fscanf(f, "%d", &val);
    fclose(f);
    return val;
}

float ax = read_value(".../in_accel_x_raw") * 0.000598f;  // en m/s²
```

Le kernel se charge de tout l'I²C, on n'a qu'à lire un fichier.


## 6. Programmes développés

### 6.1. Étape 1 — *Hello world* Qt

<img width="325" height="500" alt="WhatsApp Image 2026-04-25 at 19 34 45" src="https://github.com/user-attachments/assets/3e99aa8d-b7ed-4362-a6c9-fad32b814411" />

Premier test minimal : un `QLabel` plein écran avec un fond rouge. Objectif : valider que `qmake`, la compilation Qt et le plugin `linuxfb` fonctionnent. Ce test a permis de détecter l'absence des polices au début (rectangle rouge sans aucun texte affiché).

### 6.2. Étape 2 — Affichage numérique des trois axes

Trois `QLabel` colorés (rouge/vert/bleu pour X/Y/Z) mis à jour toutes les 500 ms par un `QTimer`. Affichage des valeurs brutes lues dans IIO.

### 6.3. Étape 3 — Conversion en m/s²

Application du facteur d'échelle `0.000598`. La valeur Z passe à environ **+9.81 m/s²** quand la carte est à plat, ce qui correspond bien à la gravité terrestre et valide la lecture.

### 6.4. Étape 4 — Graphique défilant

Ajout d'un widget `GraphWidget` personnalisé qui maintient trois files (`QQueue<float>`) limitées à 100 points. À chaque tick (100 ms) un point est ajouté et le plus ancien retiré. Tracé via `QPainter::drawLine` avec antialiasing, grille de fond pour −9.81 m/s² / 0 / +9.81 m/s², et légende colorée.

### 6.5. Étape 5 — Représentation 3D d'un avion

Pour exploiter le gyroscope en plus de l'accéléromètre, ajout d'un widget qui dessine un avion en 3D (fuselage, ailes, empennages). Le pipeline est entièrement maison :

1. **Rotations** successives autour des trois axes appliquées aux sommets.
2. **Projection perspective** simple `1 / (dist - z)` vers les coordonnées 2D.
3. **Rendu** des polygones avec `QPainter::drawPolygon`.

Pour estimer l'orientation, j'utilise un **filtre complémentaire** :

- l'**accéléromètre** donne le pitch et le roll absolus mais bruités,
- le **gyroscope** donne une variation angulaire lisse mais qui dérive avec le temps,
- la combinaison `α·(angle + gyro·dt) + (1−α)·angle_accel` avec `α = 0.96` donne un résultat à la fois stable et réactif.

**Limite à connaître :** le MPU6050 n'a pas de **magnétomètre** (l'équivalent d'une boussole, qui donne une direction absolue par rapport au nord magnétique). Sans cette référence, le *yaw* (rotation autour de l'axe vertical) est calculé uniquement à partir du gyroscope et **dérive** avec le temps. De même, si on bouge la carte trop vite ou trop brusquement, l'accéléromètre ne mesure plus uniquement la gravité (les accélérations parasites du mouvement s'ajoutent), et l'avion peut **perdre temporairement son orientation de référence**. Il faut alors reposer la carte à plat quelques secondes pour qu'elle se recale.

### 6.6. Étape 6 — Application multi-pages avec swipe (programme final)

Le programme final regroupe les étapes précédentes dans une seule application avec navigation tactile. Il est composé de **trois pages** :

1. une **page d'accueil** avec le titre du projet, la date et mon nom, plus une flèche indiquant qu'il faut glisser pour commencer,
2. la page du **graphique défilant** des accélérations,
3. la page de l'**avion 3D** dont l'orientation suit celle de la carte.

**Architecture :**

- un `QStackedWidget` contient les trois pages,
- la classe principale `SwipeWidget` capte les événements souris/tactiles,
- un seuil de **80 px** sur l'axe X distingue un swipe d'un simple tap,
- trois points (●/○/○) en bas de l'écran indiquent la page active,
- un seul `QTimer` central lit le MPU6050 toutes les 50 ms et pousse les données vers les pages concernées.

Cette architecture permet de garder une boucle de lecture unique pour les pages capteur, ce qui assure la régularité du `dt` utilisé dans le filtre complémentaire.

---

## 7. Bilan

### Compétences acquises

- Mise en œuvre complète d'une chaîne de boot embarquée (TF-A, U-Boot, kernel, rootfs, NFS, TFTP).
- Compréhension de Buildroot : toolchains, packages, patchs kernel, overlays rootfs.
- Modification du Device Tree pour intégrer un capteur I²C et utilisation du sous-système IIO.
- Cross-compilation manuelle puis automatisée de bibliothèques tierces.
- Programmation Qt5 sur cible embarquée sans GPU (rendu logiciel, plugin `linuxfb`).
- Manipulation simple de la 3D (rotations, projection) et fusion de capteurs (filtre complémentaire).

---

*Fin du rapport.*
