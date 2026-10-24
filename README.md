[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](LICENSE)

# Shelly HomeKit Garage BLU

Firmware dla **Shelly Plus 1**, który łączy bramę garażową z Apple HomeKit
bez dodatkowego mostka. Dwa bezprzewodowe **Shelly BLU Door/Window** potwierdzają
pełne zamknięcie i otwarcie. Sterownik komunikuje się z HomeKit przez Wi-Fi,
a z urządzeniami BLU przez Bluetooth. Projekt bazuje na
[Shelly-HomeKit](https://github.com/mongoose-os-apps/shelly-homekit).

**Aktualne wydanie: 2.16.1-blu38 · Shelly Plus 1 · 2026-10-24.**
Kod dla Shelly 1 Gen3 pozostaje w repozytorium; nowe paczki i testy Gen3 są wstrzymane.
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
| **Shelly Plus 1 (Plus1)** | Bieżący target i jedyna paczka publikowana dla blu38. Sterowanie przekaźnikiem, WWW, BLE i HomeKit. |
| **Shelly 1 Gen3 (S1G3)** | Kod zachowany; kompilacje i testy sprzętowe tego targetu są wstrzymane. |
| **BLU Door/Window SBDW-002C — 2 sztuki** | Role Closing sensor i Opening sensor. Szyfrowane odczyty dwóch fizycznych czujników potwierdzono we wcześniejszych próbach. |
| **BLU Motion SBMO-003Z — 1 sztuka** | Ruch w WWW i usługa Motion Sensor w HomeKit; odczyty obserwowano na sprzęcie. |
| **BLU H&T SBHT-003C — do 2 sztuk** | Role Indoor i Outdoor; temperatura i wilgotność w WWW i HomeKit. Obsługa zaimplementowana, testy fizycznych H&T pozostają do wykonania. |
| **BLU Button1 — 1 sztuka** | Pojedynczy nacisk steruje bramą; beacony określają obecność przycisku w zasięgu. Szyfrowane odczyty obserwowano na sprzęcie. |

BLU urządzenia zachowują fabryczny firmware Shelly. Paczkę Garage BLU instaluje się
na sterowniku, a urządzenia BLU paruje w jego panelu WWW.
**Nie używaj paczki Plus1 na Mini, 1PM, Gen3, Gen4 ani innych modelach.**
Door/Window Outdoor ZB (SBDW-302C) i czujnik wycieku BLU pozostają w planach.
H&T Gen3 z ekranem e-ink korzysta z Wi-Fi i nie jest obsługiwanym tu BLU H&T.

## Funkcje

| Funkcja | Zachowanie w blu38 |
| --- | --- |
| Parowanie w panelu | Pair otwiera modal i uruchamia wyszukiwanie. Jeden zgodny model jest wybierany automatycznie; PIN pojawia się tylko, gdy jest potrzebny. |
| Szyfrowane odczyty | AES-CCM BTHome v2, sprawdzanie MAC i klucza oraz ochrona przed ponownym użyciem pakietów. |
| Wiarygodny stan | Polecenia są blokowane przy brakujących, nieaktualnych lub sprzecznych odczytach krańcówek oraz podczas parowania. |
| Nazwy akcesoriów | Name pod Pair zapisuje nazwę urządzenia; Motion, Button1 i H&T używają jej na wizualizacji. Nazwa bramy pochodzi z Name w sekcji Garage Door. Krańcówki są opisane w ustawieniach. |
| Wizualizacja | Motywy Day / Night / Auto, animacja bramy, kafelki ruchu i Button1, odczyty temperatury i wilgotności. Przy włączonym haśle widok jest dostępny po zalogowaniu; podczas OTA jest ukryty. |
| Obecność auta | Nowy poprawnie odszyfrowany pakiet Button1 pokazuje auto, brak pakietów przez 45 s je ukrywa. Ręczny wybór działa do następnego pakietu lub zmiany obecności. |
| Sterowanie Button1 | Pojedynczy nacisk: otwórz / zatrzymaj / zamknij zgodnie ze stanem bramy. Sam beacon nie uruchamia przekaźnika. |
| Kierunek po zatrzymaniu | Continue previous direction after stop: wyłączony — zmiana kierunku; włączony — kontynuacja poprzedniego. Opcję trzeba dopasować do reakcji napędu. |
| Obstruction | Przekroczenie czasu ruchu jest sygnalizowane w panelu i HomeKit. Animacja pokazuje umowne 50%; prawidłowo potwierdzona krańcówka usuwa błąd. |
| OTA | Firmware → Update → Check → Install lub Update from file → Upload. Kanał i paczki są na hejsiri.pl. |

Obecność oznacza zasięg BLE przycisku, a nie potwierdzenie miejsca zaparkowania.
Animacja nie mierzy odległości ani rzeczywistego procentu otwarcia bramy.
Blokowanie nowych impulsów nie zatrzymuje napędu już będącego w ruchu i nie zastępuje
jego zabezpieczeń.

## Szybki start

1. [Zainstaluj właściwy ZIP](docs/AKTUALIZACJA.md) na Shelly Plus 1.
2. W Security Settings ustaw hasło panelu. W General Settings wybierz Mode → Garage Door Opener i Save.
3. W sekcji bramy sparuj Closing sensor i Opening sensor przez Pair. Czujniki mają wykrywać swoje magnesy wyłącznie w odpowiednich skrajnych położeniach.
4. Włącz BLU sensors, ustaw czasy zgodnie z napędem i zapisz Save. Sprawdź wskazania obu krańcówek.
5. Opcjonalnie sparuj Motion, Indoor / Outdoor H&T i Button1; wpisz własne nazwy w Name i zapisz.
6. W HomeKit Settings wybierz Setup i dodaj akcesorium w aplikacji Dom za pomocą wyświetlonego kodu.

[Pełna konfiguracja krok po kroku](docs/KONFIGURACJA.md) opisuje pola,
statusy i zachowanie po zatrzymaniu. Przy problemach zacznij od
[diagnostyki](docs/ROZWIAZYWANIE_PROBLEMOW.md).

## Pobieranie i aktualizacja

- [Paczki i opis na hejsiri.pl](https://hejsiri.pl/firmware/garage-blu/)
- [ZIP 2.16.1-blu38 dla Shelly Plus 1](https://hejsiri.pl/firmware/garage-blu/shelly-homekit-garage-blu-ShellyPlus1-2.16.1-blu38.zip)
- [SHA-256 paczki](https://hejsiri.pl/firmware/garage-blu/shelly-homekit-garage-blu-ShellyPlus1-2.16.1-blu38.zip.sha256)
- [Instrukcja instalacji, aktualizacji i kontroli po restarcie](docs/AKTUALIZACJA.md)

Bieżąca paczka została skompilowana, sprawdzona testami hosta i podglądem WWW;
zweryfikowano też publiczny ZIP, sumę SHA-256 i kanał OTA. Wcześniejsze wydania
potwierdziły OTA i szyfrowane odczyty na Plus 1. Metadane blu38 nadal mają
`hardware_tested: false`: pełna próba tego wydania na fizycznej bramie pozostaje
do wykonania.

## Projekt, zgłoszenia i rozwój

- [Repozytorium Garage BLU](https://github.com/hejsiri/shelly-homekit-garage-blu)
- [Zgłoszenia błędów i propozycje](https://github.com/hejsiri/shelly-homekit-garage-blu/issues)
- [Budowanie, testy i publikowanie (EN)](README-BLU.md)
- [Historia zmian blu20–blu38](docs/HISTORIA_ZMIAN.md)
- [Projekt bazowy Shelly-HomeKit](https://github.com/mongoose-os-apps/shelly-homekit)

W zgłoszeniu podaj wersję sterownika, model i firmware urządzenia BLU oraz dokładny
komunikat błędu. Nie publikuj haseł, PIN-ów, kodu HomeKit ani kluczy BTHome.

## Autorzy i licencja

Projekt korzysta z pracy autorów [Shelly-HomeKit](https://github.com/mongoose-os-apps/shelly-homekit)
wymienionych w [AUTHORS.md](AUTHORS.md). Licencja: [Apache 2.0](LICENSE).
