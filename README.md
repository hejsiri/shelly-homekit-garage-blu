[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](LICENSE)

# Shelly HomeKit Garage BLU

Firmware dla **Shelly Plus 1** i **Shelly 1 Gen3**, który łączy bramę garażową z Apple HomeKit
bez dodatkowego mostka. Dwa bezprzewodowe **Shelly BLU Door/Window** potwierdzają
pełne zamknięcie i otwarcie. Sterownik komunikuje się z HomeKit przez Wi-Fi,
a z urządzeniami BLU przez Bluetooth. Projekt bazuje na
[Shelly-HomeKit](https://github.com/mongoose-os-apps/shelly-homekit).

**Aktualne wydanie: 2.16.1-blu50 · Shelly Plus 1 i Shelly 1 Gen3 · 2026-10-25.**
Dla każdego modelu jest osobny ZIP, suma SHA-256 i wpis w kanale OTA.
Panel WWW ma angielskie teksty systemowe. Dokumentacja użytkownika jest po polsku,
a odnośniki do przycisków i pól zachowują ich angielskie nazwy.

[Instalacja i aktualizacje](docs/AKTUALIZACJA.md) ·
[Konfiguracja i obsługa](docs/KONFIGURACJA.md) ·
[Rozwiązywanie problemów](docs/ROZWIAZYWANIE_PROBLEMOW.md) ·
[Historia zmian](docs/HISTORIA_ZMIAN.md) ·
[Dokumentacja techniczna (EN)](README-BLU.md)

## Idea projektu

Dwie krańcówki BLE pozwalają potwierdzać oba końcowe położenia bez prowadzenia
przewodów do czujników. Zwolnienie jednego kontaktronu oznacza wyłącznie, że brama
opuściła dane położenie; drugi czujnik potwierdza osiągnięcie przeciwnego końca.
Stan może być odświeżany także wtedy, gdy bramę uruchomiono pilotem lub przyciskiem
napędu. Pozycja pomiędzy krańcówkami jest szacowana z czasu ruchu.

## Wspierany sprzęt

| Urządzenie | Funkcja i status |
| --- | --- |
| **Shelly Plus 1 (Plus1)** | Paczka Plus1 dla ESP32. Sterowanie przekaźnikiem, WWW, BLE i HomeKit. |
| **Shelly 1 Gen3 (S1G3)** | Osobna paczka S1G3 dla ESP32-C3, te same funkcje Garage BLU. Test na fizycznym Gen3 pozostaje do wykonania. |
| **BLU Door/Window SBDW-002C — 2 sztuki** | Role Closing sensor i Opening sensor. Szyfrowane odczyty dwóch fizycznych czujników potwierdzono we wcześniejszych próbach. |
| **BLU Motion SBMO-003Z — 1 sztuka** | Ruch w WWW i usługa Motion Sensor w HomeKit; odczyty obserwowano na sprzęcie. Od blu43 także Light sensor w HomeKit, z pomiarem oświetlenia w luksach z tego samego urządzenia. |
| **BLU H&T SBHT-003C — do 2 sztuk** | Role Indoor i Outdoor; temperatura i wilgotność w WWW i HomeKit. Obsługa zaimplementowana, testy fizycznych H&T pozostają do wykonania. |
| **Xiaomi LYWSD03MMC (experimental)** | Bezpośredni odczyt BLE temperatury, wilgotności i napięcia baterii z fabrycznego firmware. Bez tokenu i PIN-u; pomiar co 5 minut, utrata świeżości po minimum 15 minutach. B1.4 / 2.1.1_0159 wymaga próby fizycznej. |
| **Xiaomi LYWSDCGQ/01ZM (MJ_HT_V1)** | Temperatura, wilgotność i bateria z fabrycznych beaconów BLE. Do wyboru zamiast BLU H&T w roli Indoor lub Outdoor; łącznie dwa termometry. Bez PIN-u, chmury i zmiany firmware czujnika. Na Plus1/blu45 potwierdzono odczyty w panelu, w tym baterię 36 %. |
| **BLU Button1 — 1 sztuka** | Pojedynczy nacisk steruje bramą; beacony określają obecność przycisku w zasięgu. Dwie usługi w akcesorium Shelly: obecność i przycisk programowalny. W blu42 na Plus 1 potwierdzono ich widoczność w Domu, odczyt obecności oraz dostępność trzech rodzajów naciśnięcia. |

Od blu49 można także wypróbować **LYWSD03MMC (experimental)** na oryginalnym firmware:
krótki, bezpośredni odczyt BLE, bez PIN-u, bindkey i konta Xiaomi. Po wyborze typu
w Indoor/Outdoor H&T naciśnij **Pair** i zamknij inne aplikacje połączone z termometrem.
Shelly sprawdza model i pomiary przed zapisaniem przypisania, a potem odczytuje je co
5 minut. Bateria jest szacowana z napięcia (oznaczenie `~` w panelu). Próba fizyczna
**B1.4 / 2.1.1_0159 pozostaje do wykonania**; nie deklarujemy zgodności tej rewizji.
Blu50 poprawia odczyt tekstów zakończonych NUL (model i wersje) oraz odczytuje dane
ze zwykłych uchwytów ATT odkrytych po UUID. Błąd zawiera teraz surową odpowiedź.
Ręczne klucze i MCCGQ02HL z blu46/47 pozostają wycofane.

BLU urządzenia zachowują fabryczny firmware Shelly. Paczkę Garage BLU instaluje się
na sterowniku, a urządzenia BLU paruje w jego panelu WWW.
**Paczki Plus1 i S1G3 nie są zamienne. Nie używaj ich na Mini, 1PM, Gen4 ani innych modelach.**
Door/Window Outdoor ZB (SBDW-302C) i czujnik wycieku BLU pozostają w planach.
H&T Gen3 z ekranem e-ink korzysta z Wi-Fi i nie jest obsługiwanym tu BLU H&T.

## Shelly 1 Gen3

Od **blu38** publikowana jest osobna paczka **S1G3 / ESP32-C3** dla Shelly 1 Gen3.
Zawiera te same funkcje Garage BLU co Plus1, w tym krańcówki BLE, Motion, H&T,
Button1, parowanie w panelu, wizualizację i HomeKit.

[Poradnik Shelly 1 Gen3](docs/SHELLY_1_GEN3.md) zawiera linki do ZIP-u i SHA-256,
instrukcję pierwszego uruchomienia oraz stan weryfikacji. Paczka nie jest dla Mini
ani 1PM Gen3. Kompilacja i kontrola paczki przeszły; test fizycznego Gen3 pozostaje
do wykonania.

## Funkcje

| Funkcja | Zachowanie w blu50 |
| --- | --- |
| Parowanie w panelu | Pair otwiera modal i uruchamia wyszukiwanie. Jeden zgodny model jest wybierany automatycznie; PIN pojawia się tylko, gdy jest potrzebny. |
| Szyfrowane odczyty | AES-CCM BTHome v2, sprawdzanie MAC i klucza oraz ochrona przed ponownym użyciem pakietów. |
| Wiarygodny stan | Polecenia są blokowane przy brakujących, nieaktualnych lub sprzecznych odczytach krańcówek oraz podczas parowania. |
| Nazwy akcesoriów | Name pod Pair zapisuje nazwę urządzenia; Motion, Button1 i H&T używają jej na wizualizacji. Oddzielne Car name pod Button1 nazywa kafelek auta. Nazwa bramy pochodzi z Name w sekcji Garage Door. Krańcówki są opisane w ustawieniach. |
| Wizualizacja | Motywy Day / Night / Auto, animacja bramy, kafelki ruchu i Button1, odczyty temperatury i wilgotności. Przy włączonym haśle widok jest dostępny po zalogowaniu; podczas OTA jest ukryty. |
| Obecność auta | Nowy poprawnie odszyfrowany pakiet Button1 pokazuje auto, brak pakietów przez 45 s je ukrywa. Ręczny wybór zmienia równocześnie grafikę i kafelek Car presence do następnego pakietu lub zmiany obecności. |
| Oświetlenie | Light sensor w HomeKit korzysta wyłącznie z LUX sparowanego BLU Motion. Ma oddzielną kontrolę świeżości pomiaru i status dostępności. |
| Sterowanie Button1 | Pojedynczy nacisk: otwórz / zatrzymaj / zamknij zgodnie ze stanem bramy. W HomeKit: obecność auta i przycisk programowalny (pojedyncze / podwójne / długie naciśnięcie), we wspólnym akcesorium Shelly. Sam beacon nie uruchamia przekaźnika. |
| Kierunek po zatrzymaniu | Continue previous direction after stop: wyłączony — zmiana kierunku; włączony — kontynuacja poprzedniego. Opcję trzeba dopasować do reakcji napędu. |
| Obstruction | Przekroczenie czasu ruchu jest sygnalizowane w panelu i HomeKit. Animacja pokazuje umowne 50%; prawidłowo potwierdzona krańcówka usuwa błąd. |
| OTA | Firmware → Update → Check → Install lub Update from file → Upload. Kanał i paczki są na hejsiri.pl. |

Obecność oznacza zasięg BLE przycisku, a nie potwierdzenie miejsca zaparkowania.
Animacja nie mierzy odległości ani rzeczywistego procentu otwarcia bramy.
Blokowanie nowych impulsów nie zatrzymuje napędu już będącego w ruchu i nie zastępuje
jego zabezpieczeń.

## Szybki start

1. [Zainstaluj właściwy ZIP](docs/AKTUALIZACJA.md) na właściwym Shelly Plus 1 lub Shelly 1 Gen3.
2. W Security Settings ustaw hasło panelu. W General Settings wybierz Mode → Garage Door Opener i Save.
3. W sekcji bramy sparuj Closing sensor i Opening sensor przez Pair. Czujniki mają wykrywać swoje magnesy wyłącznie w odpowiednich skrajnych położeniach.
4. Włącz BLU sensors, ustaw czasy zgodnie z napędem i zapisz Save. Sprawdź wskazania obu krańcówek.
5. Opcjonalnie sparuj Motion, Indoor / Outdoor H&T i Button1; wpisz własne nazwy w Name i zapisz.
   Dla Xiaomi wybierz **Xiaomi LYWSDCGQ/01ZM** przy Indoor H&T albo Outdoor H&T, a potem **Pair**. Włącz czujnik i zostaw go blisko Shelly. Nie wymaga klucza, konta Xiaomi ani trybu parowania.
6. W HomeKit Settings wybierz Setup i dodaj akcesorium w aplikacji Dom za pomocą wyświetlonego kodu.

[Pełna konfiguracja krok po kroku](docs/KONFIGURACJA.md) opisuje pola,
statusy i zachowanie po zatrzymaniu. Przy problemach zacznij od
[diagnostyki](docs/ROZWIAZYWANIE_PROBLEMOW.md).

Repozytorium zawiera profile sprzętu tylko dla **Shelly Plus 1** i **Shelly 1 Gen3**.
Instrukcje budowania, testów i struktura kodu są w [README-BLU.md](README-BLU.md#build-and-test).

## Pobieranie i aktualizacja

- [Paczki i opis na hejsiri.pl](https://hejsiri.pl/firmware/garage-blu/)
- [ZIP 2.16.1-blu50 dla Shelly Plus 1](https://hejsiri.pl/firmware/garage-blu/shelly-homekit-garage-blu-ShellyPlus1-2.16.1-blu50.zip)
- [SHA-256 Plus1](https://hejsiri.pl/firmware/garage-blu/shelly-homekit-garage-blu-ShellyPlus1-2.16.1-blu50.zip.sha256)
- [ZIP 2.16.1-blu50 dla Shelly 1 Gen3](https://hejsiri.pl/firmware/garage-blu/shelly-homekit-garage-blu-Shelly1Gen3-2.16.1-blu50.zip)
- [SHA-256 Gen3](https://hejsiri.pl/firmware/garage-blu/shelly-homekit-garage-blu-Shelly1Gen3-2.16.1-blu50.zip.sha256)
- [Instrukcja instalacji, aktualizacji i kontroli po restarcie](docs/AKTUALIZACJA.md)

Obie paczki zostały skompilowane; wspólny kod sprawdzono testami hosta i panelu WWW;
zweryfikowano też publiczne ZIP-y, sumy SHA-256 i kanał OTA. Wcześniejsze wydania
potwierdziły OTA i szyfrowane odczyty na Plus 1. W blu42 potwierdzono na tym modelu
migrację i widoczność bramy, ruchu, obecności oraz przycisku w aplikacji Dom;
ruch był aktywny i gotowy, a obecność odpowiadała odczytowi BLE. Metadane blu50 nadal mają
`hardware_tested: false`: pełna próba tego wydania na fizycznej bramie pozostaje
do wykonania. Test na fizycznym Shelly 1 Gen3 nie został jeszcze wykonany.

## Projekt, zgłoszenia i rozwój

- [Repozytorium Garage BLU](https://github.com/hejsiri/shelly-homekit-garage-blu)
- [Zgłoszenia błędów i propozycje](https://github.com/hejsiri/shelly-homekit-garage-blu/issues)
- [Budowanie, testy i publikowanie (EN)](README-BLU.md)
- [Historia zmian blu20–blu50](docs/HISTORIA_ZMIAN.md)
- [Projekt bazowy Shelly-HomeKit](https://github.com/mongoose-os-apps/shelly-homekit)

W zgłoszeniu podaj wersję sterownika, model i firmware urządzenia BLU oraz dokładny
komunikat błędu. Nie publikuj haseł, PIN-ów, kodu HomeKit ani kluczy BTHome.

## Autorzy i licencja

Projekt korzysta z pracy autorów [Shelly-HomeKit](https://github.com/mongoose-os-apps/shelly-homekit)
wymienionych w [AUTHORS.md](AUTHORS.md). Licencja: [Apache 2.0](LICENSE).
