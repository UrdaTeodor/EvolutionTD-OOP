# EvolutionTD

Input: (tower type) (row) (col) $ == nextwave

Momentan este un tower defence simplu, ruleaza pe tick-uri interne ce se updateaza in consola la un interval de timp. afiseaza grid-ul cu path-ul inamicii si towerele.

### Roadmap (t1)

ideea principala a jocului este un sistem de evolutie al towerelor semi random pentru a simula feeling-ul jocurilor rogue like.

voi implementa evolutii minore ce cresc stats-urile towerelor cu niste valori minore si random si evolutii majore ce schimba fundamental cum functioneaza turnul

inamicii vor avea asemenea parti ce evolueaza dar vor fi 90% legati de val, ca sa poti sa diferentiezi un run bun de un run cu inamici slabi.

Cum am spus si mai sus, va fi un sistem rogue like, adica vor fi elemente ce sunt influentate de skill (cum ar fi cumpararea evolutiilor cu credite in shop, deci defence-uri mai eficiente sunt mai bune long term) dar si de noroc (evolutii majore mai puternice si mai slabe, pe raritati eventual). practic cineva mai skilled isi face norocul singur cu mai multe evolutii cumparate dar oricine poate castiga daca joaca suficiente run-uri.

Mai multe valuri si inamici, mai multe towere, shop cu optiuni mai multe, bossi, interfata si jucat in real time (nu doar intre valuri)

### Stadiu curent (t2)

Jocul ruleaza in fereastra SFML fullscreen 1920x1080, grid 20x20. SPACE porneste val, U undo wave (checkpoint la inceput de wave), ESC stop.

**Ierarhii implementate (T2):**
- `Tower` cu 5 derivate: `AntivirusTower`, `AdblockerTower`, `HoneypotTower`, `FirewallTower`, `BytecoinMinerTower` (commit-ul de integrare). Theme-specific virtual e `update(enemies, dt)`. toate au `clone()` (virtual constructor) si NVI prin `displayDetails`.
- `Evolution` (baza abstracta) cu 3 derivate: `StatEvolution` (% buffs damage/range/speed/HP/slow), `AbilityEvolution` (8 abilitati: MULTI_TARGET, FIRE_TRAIL, KNOCKBACK_EVERY_3, REFLECTIVE_SHIELD, etc.), `MythicEvolution` (combinatie a 2 Legendary). `AbilityEvolution::apply` foloseste `dynamic_cast<HoneypotTower*>` / `FirewallTower*` pentru a verifica daca abilitatea se potriveste pe tipul concret.
- Evolution momentan nu este folosit, urmeaza o data cu implementarea shop-ului in T3
- `GameException : std::runtime_error` cu 3 derivate independente: `InsufficientFundsException`, `InvalidPlacementException`, `IncompatibleEvolutionException`. Throw-uri din constructori si factory functions (`MythicEvolution::deriveType`, `craftMythic`, `AbilityEvolution::apply`, `Game::placeTower`); catch in main pe baza polimorfica.

**Misc:**
- Snapshot/undo: `Game` tine `unique_ptr<Game> snapshot_` self-referential. La fiecare `startWave()` se salveaza state-ul (towers, $, HP, wave). Apasa U oricand (in timpul valului sau intre valuri) si revine la momentul "start val". Snapshot-ul nu se consuma si nu se copiaza in cc, deci se poate face undo repetat (anti-RNG-manipulation pentru shop-ul de la T3).
- Copy-and-swap pe `Game` cu deep-copy polimorfic prin `Tower::clone()`. Destructor out-of-line necesar pentru `unique_ptr<Game>` self-referential.
- Predictie proiectile: `Tower::calculateInterceptPoint` foloseste formula de intercept. Proiectilele vizuale in SFML folosesc aceeasi logica, deci animatia matcheaza damage-ul real.
- 5 valuri, incluzand val 5 BOSS: `ILOVEYOU`. Boss-ul are 5 sprite-uri de phase (se schimba la fiecare 20% HP pierdut).

**Pregatit pentru T3:**
Codul are deja toata logica pentru shop-ul cu evolutii RNG, doar nu e expusa la UI:
- `EvolutionFactory.cpp`: 15 factory functions (`makeMiniDamage`, `makeRareAttackSpeed`, `makeEpicMultiTarget`, `makeLegendaryDoubleShot`, `makeLegendaryReflectiveShield`, etc.) acoperind toate rarity-urile MINI/RARE/EPIC/LEGENDARY. Marcate cu `// cppcheck-suppress unusedFunction // T3` pana le foloseste shop-ul.
- `craftMythic(unique_ptr<AbilityEvolution>, unique_ptr<AbilityEvolution>)`: combinare a 2 Legendary in 1 Mythic, cu 3 perechi valide hardcodate (DOUBLE_SHOT+FIRE_TRAIL = PHOENIX_BARRAGE, KNOCKBACK_EVERY_3+MOVABLE = ROVING_BRUISER, REFLECTIVE_SHIELD+FIRE_TRAIL = INFERNO_MIRROR). Combo-uri invalide arunca exceptie.
- `AbilityEvolution::canCombine(a, b)`: helper static pentru shop sa verifice daca user-ul are 2 abilitati care se pot combina.
- Toate efectele de evolutie sunt implementate in derivatele Tower (flag-uri ca `doubleShot`, `fireTrail`, `multiTarget`, `armored`, `reflectiveShield`, `biggerAura`, `movable`) si sunt aplicate corect in `update()` cand sunt activate via `apply()`.

### Roadmap T3 planificat

1. Shop intre valuri: dupa fiecare val, fereastra cu 3 evolutii random (rarity weighted: 50% Mini, 30% Rare, 15% Epic, 5% Legendary). User cumpara cu credite si aplica pe un tower.
2. Design patterns:
   - Factory: deja exista in `EvolutionFactory`, va fi extins cu `RandomEvolutionFactory`.
   - Strategy pentru rendering: `Renderer` (abstract) + `ConsoleRenderer` + `SFMLRenderer` (codul curent mutat in `SFMLRenderer`).
   - Optional Singleton pentru `Shop` sau `Game`.
3. Class template: `RandomPool<T>` cu pondere pe raritate pentru `Evolution` si pentru random enemy spawning.
4. Function template (cerinta T3): generic `operator<<` pentru containere.
5. Date in fisier (cerinta T2+T3): mut stats Tower/Enemy + compozitie valuri in `data/*.json`, parsare cu nlohmann/json (a doua biblioteca externa pe langa SFML).
6. Mythic evolutions wired in shop: doar daca user are 2 Legendary compatibile.


### Folosiți template-ul corespunzător grupei voastre!

| Laborant  | Link template                                |
|-----------|----------------------------------------------|
| Dragoș B  | https://github.com/Ionnier/oop-template      |
| Tiberiu M | https://github.com/MaximTiberiu/oop-template |
| Marius MC | https://github.com/mcmarius/oop-template     |

### Important!

Aveți voie cu cod generat de modele de limbaj la care nu ați contribuit semnificativ doar dacă documentați riguros acest proces.
Codul generat pus "ca să fie"/pe care nu îl înțelegeți se punctează doar pentru puncte bonus, doar în contextul
în care oferă funcționalități ajutătoare și doar dacă are sens.

Codul din proiect trebuie să poată fi ușor de înțeles și de modificat de către altcineva. Pentru detalii, veniți la ore.

O cerință nu se consideră îndeplinită dacă este realizată doar prin cod generat.

- **Fără cod de umplutură/fără sens!**
- **Fără copy-paste!**
- **Fără variabile globale!**
- **Fără atribute publice!**
- **Pentru T2 și T3, fără date în cod!** Datele vor fi citite din fișier, aveți exemple destule.
- **Obligatoriu** fișiere cu date mai multe din care să citiți, obligatoriu cu biblioteci externe: fișiere (local sau server) sau baze de date
- obligatoriu (TBD) să integrați cel puțin două biblioteci externe pe lângă cele pentru stocare

### Tema 0

- [x] Nume proiect (poate fi schimbat ulterior)
- [x] Scurtă descriere a temei alese, ce v-ați propus să implementați

## Tema 1

#### Cerințe
- [x] definirea a minim **3-4 clase** folosind compunere cu clasele definite de voi; moștenirile nu se iau în considerare aici
- [x] constructori de inițializare cu parametri pentru fiecare clasă
- [x] pentru o aceeași (singură) clasă: constructor de copiere, `operator=` de copiere, destructor
<!-- - [ ] pentru o altă clasă: constructor de mutare, `operator=` de mutare, destructor -->
<!-- - [ ] pentru o altă clasă: toate cele 5 funcții membru speciale -->
- [x] `operator<<` pentru **toate** clasele pentru afișare (`std::ostream`) folosind compunere de apeluri cu `operator<<`
- [x] cât mai multe `const` (unde este cazul) și funcții `private`
- [x] implementarea a minim 3 funcții membru publice pentru funcționalități netriviale specifice temei alese, dintre care cel puțin 1-2 funcții mai complexe
  - nu doar citiri/afișări sau adăugat/șters elemente într-un/dintr-un vector
- [x] scenariu de utilizare **cu sens** a claselor definite:
  - crearea de obiecte și apelarea tuturor funcțiilor membru publice în main
  - vor fi adăugate în fișierul `tastatura.txt` DOAR exemple de date de intrare de la tastatură (dacă există); dacă aveți nevoie de date din fișiere, creați alte fișiere separat
- [x] minim 52-60% din codul propriu să fie C++, `.gitattributes` configurat corect
- [x] tag de `git`: de exemplu `v0.1`
- [x] serviciu de integrare continuă (CI) cu **toate bifele**; exemplu: GitHub Actions
- [x] code review #1 2 proiecte

## Tema 2

#### Cerințe
- [x] separarea codului din clase în `.h` (sau `.hpp`) și `.cpp`
- [x] moșteniri:
  - minim o clasă de bază și **3 clase derivate** din aceeași ierarhie; cele 3 derivate moștenesc aceeași clasă de bază
  - ierarhia trebuie să fie cu bază proprie, nu derivată dintr-o clasă predefinită
  - [x] funcții virtuale (pure) apelate prin pointeri de bază din clasa care conține atributul de tip pointer de bază
    - minim o funcție virtuală va fi **specifică temei** (i.e. nu simple citiri/afișări sau preluate din biblioteci i.e. draw/update/render)
    - constructori virtuali (clone): sunt necesari, dar nu se consideră funcții specifice temei
    - afișare virtuală, interfață non-virtuală
  - [x] apelarea constructorului din clasa de bază din constructori din derivate
  - [x] clasă cu atribut de tip pointer la o clasă de bază cu derivate; aici apelați funcțiile virtuale prin pointer de bază, eventual prin interfața non-virtuală din bază
    - [x] suprascris cc/op= pentru copieri/atribuiri corecte, copy and swap
    - [x] `dynamic_cast`/`std::dynamic_pointer_cast` pentru downcast cu sens
    - [x] smart pointers (recomandat, opțional)
- [x] excepții
  - [x] ierarhie proprie cu baza `std::exception` sau derivată din `std::exception`; minim **3** clase pentru erori specifice distincte
    - clasele de excepții trebuie să trateze categorii de erori distincte (exemplu de erori echivalente: citire fișiere cu diverse extensii)
  - [x] utilizare cu sens: de exemplu, `throw` în constructor (sau funcție care întoarce un obiect), `try`/`catch` în `main`
  - această ierarhie va fi complet independentă de ierarhia cu funcții virtuale
- [x] funcții și atribute `static`
- [x] STL
- [x] cât mai multe `const`
- [x] funcții *de nivel înalt*, de eliminat cât mai mulți getters/setters/funcții low-level
- [x] minim 75-78% din codul propriu să fie C++
- [x] la sfârșit: commit separat cu adăugarea unei noi clase derivate fără a modifica restul codului, **pe lângă cele 3 derivate deja adăugate** din aceeași ierarhie
  - noua derivată nu poate fi una existentă care a fost ștearsă și adăugată din nou
  - noua derivată va fi integrată în codul existent (adică va fi folosită, nu adăugată doar ca să fie)
- [x] tag de `git` pe commit cu **toate bifele**: de exemplu `v0.2`
- [ ] code review #2 2 proiecte

## Tema 3

#### Cerințe
- [ ] 2 șabloane de proiectare (design patterns)
- [ ] o clasă șablon cu sens; minim **2 instanțieri**
  - [ ] preferabil și o funcție șablon (template) cu sens; minim 2 instanțieri
- [ ] minim 80-90% din codul propriu să fie C++
<!-- - [ ] o specializare pe funcție/clasă șablon -->
- [ ] tag de `git` pe commit cu **toate bifele**: de exemplu `v0.3` sau `v1.0`
- [ ] code review #3 2 proiecte

## Instrucțiuni de compilare

Proiectul este configurat cu CMake.

Instrucțiuni pentru terminal:

1. Pasul de configurare
```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
# sau ./scripts/cmake.sh configure
```

Sau pe Windows cu GCC folosind Git Bash:
```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -G Ninja
# sau ./scripts/cmake.sh configure -g Ninja
```

Pentru a configura cu ASan, avem opțiunea `-DUSE_ASAN=ON` (nu merge pe Windows cu GCC):
```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DUSE_ASAN=ON
# sau ./scripts/cmake.sh configure -e "-DUSE_ASAN=ON"
```


La acest pas putem cere să generăm fișiere de proiect pentru diverse medii de lucru.


2. Pasul de compilare
```sh
cmake --build build --config Debug --parallel 6
# sau ./scripts/cmake.sh build
```

Cu opțiunea `parallel` specificăm numărul de fișiere compilate în paralel.


3. Pasul de instalare (opțional)
```sh
cmake --install build --config Debug --prefix install_dir
# sau ./scripts/cmake.sh install
```

Vezi și [`scripts/cmake.sh`](scripts/cmake.sh).

Observație: folderele `build/` și `install_dir/` sunt adăugate în fișierul `.gitignore` deoarece
conțin fișiere generate și nu ne ajută să le versionăm.


## Instrucțiuni pentru a rula executabilul

Există mai multe variante:

1. Din directorul de build (implicit `build`). Executabilul se află la locația `./build/oop` după ce a fost rulat pasul de compilare al proiectului (`./scripts/cmake.sh build` - pasul 2 de mai sus).

```sh
./build/oop
```

2. Din directorul `install_dir`. Executabilul se află la locația `./install_dir/bin/oop` după ce a fost rulat pasul de instalare (`./scripts/cmake.sh install` - pasul 3 de mai sus).

```sh
./install_dir/bin/oop
```

3. Rularea programului folosind Valgrind se poate face executând script-ul `./scripts/run_valgrind.sh` din rădăcina proiectului. Pe Windows acest script se poate rula folosind WSL (Windows Subsystem for Linux). Valgrind se poate rula în modul interactiv folosind: `RUN_INTERACTIVE=true ./scripts/run_valgrind.sh`

Implicit, nu se rulează interactiv, iar datele pentru `std::cin` sunt preluate din fișierul `tastatura.txt`.

```sh
RUN_INTERACTIVE=true ./scripts/run_valgrind.sh
# sau
./scripts/run_valgrind.sh
```

4. Pentru a rula executabilul folosind ASan, este nevoie ca la pasul de configurare (vezi mai sus) să fie activat acest sanitizer. Ar trebui să meargă pe macOS și Linux. Pentru Windows, ar merge doar cu MSVC (nerecomandat).

Comanda este aceeași ca la pasul 1 sau 2. Nu merge combinat cu Valgrind.

```sh
./build/oop
# sau
./install_dir/bin/oop
```

## License

The project is licensed under [AGPLv3](LICENSE).

The [template repository](https://github.com/mcmarius/oop-template) itself is licensed under [Unlicense](LICENSE.template).

## Resurse

- adăugați trimiteri **detaliate** către resursele externe care v-au ajutat sau pe care le-ați folosit
