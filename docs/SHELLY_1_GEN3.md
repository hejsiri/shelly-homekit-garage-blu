# Shelly 1 Gen3 — Garage BLU

Dla **2.16.1-blu41**, stan: **2026-10-24**.

Od blu38 dostępna jest osobna paczka Garage BLU dla **Shelly 1 Gen3**. Korzysta
z platformy **ESP32-C3**, ma nazwę aplikacji **S1G3**, a w kanale OTA jest
rozpoznawana jako **Shelly1Gen3**.

[Pełna instalacja i aktualizacje](AKTUALIZACJA.md) ·
[Konfiguracja bramy i BLU](KONFIGURACJA.md) ·
[Rozwiązywanie problemów](ROZWIAZYWANIE_PROBLEMOW.md)

## Właściwa paczka

- [Pobierz ZIP 2.16.1-blu41 dla Shelly 1 Gen3](https://hejsiri.pl/firmware/garage-blu/shelly-homekit-garage-blu-Shelly1Gen3-2.16.1-blu41.zip)
- [Pobierz sumę SHA-256](https://hejsiri.pl/firmware/garage-blu/shelly-homekit-garage-blu-Shelly1Gen3-2.16.1-blu41.zip.sha256)
- [Strona pobierania i metadane](https://hejsiri.pl/firmware/garage-blu/)

| Parametr | Wartość |
| --- | --- |
| Sterownik | Shelly 1 Gen3 |
| Identyfikator OTA / aplikacja | Shelly1Gen3 / S1G3 |
| Platforma | ESP32-C3 |
| Wersja | 2.16.1-blu41 |
| Build | 20261024-162848 |
| ZIP | 2,257,964 bajtów |
| SHA-256 | `e07f841f942a81b4f93c4ffc5abca8361420e3114ae95b9fc0d3b1c1c4046c05` |

ZIP jest dla **Shelly 1 Gen3**, nie dla **Shelly 1 Mini Gen3**, **1PM Gen3**, innych
Gen3 ani Gen4. Paczka S1G3 nie jest zamienna z paczką Plus1 dla Shelly Plus 1.
Firmware wgrywa się na sterownik; urządzenia BLU zachowują fabryczny firmware Shelly.

## Co obsługuje Gen3

Paczka zawiera te same funkcje Garage BLU blu41 co wersja Plus1:

- sterowanie bramą i odczyt stanu w Apple HomeKit przez Wi-Fi;
- dwie szyfrowane krańcówki BLU Door/Window: Closing sensor i Opening sensor;
- opcjonalne Motion, dwa H&T oraz Button1;
- parowanie w modalu, automatyczny wybór jednego zgodnego urządzenia i PIN tylko wtedy, gdy jest potrzebny;
- własne nazwy urządzeń i osobną nazwę kafelka auta (Car name), angielski panel i wizualizację Day / Night / Auto;
- obecność auta na podstawie beaconów Button1 i sterowanie pojedynczym naciśnięciem;
- dwie usługi w akcesorium Shelly HomeKit: czujnik obecności auta oraz przycisk programowalny
  z pojedynczym, podwójnym i długim naciśnięciem;
- wspólny z HomeKit status Obstruction i animację 50% przy błędzie;
- aktualizacje OTA z hejsiri.pl z paczką dobieraną według modelu.

## Instalacja i pierwsze uruchomienie

1. Sprawdź na urządzeniu dokładny model Shelly 1 Gen3 i pobierz ZIP S1G3 bez rozpakowywania.
2. Jeśli działa już Shelly-HomeKit, użyj Firmware → Update from file → Upload. W starszym Garage BLU można też wybrać Check → Install, gdy kanał zgłosi blu41 dla tego modelu.
3. Przy fabrycznym firmware użyj [procedury konwersji](AKTUALIZACJA.md#pierwsza-instalacja-z-fabrycznego-firmware-gen3--plus) i przykładu RPC oznaczonego Shelly 1 Gen3.
4. Poczekaj na restart i sprawdź Version oraz Build w panelu.
5. Ustaw hasło w Security Settings, a w General Settings wybierz Mode → Garage Door Opener i Save.
6. Sparuj obie krańcówki przez Pair, włącz BLU sensors, dopasuj czasy do napędu i zapisz Save.
7. Sprawdź oba położenia, opcjonalne BLU i konfigurację HomeKit według [pełnego poradnika](KONFIGURACJA.md).

Nie kopiuj ustawień okablowania ani obrazu firmware z innego modelu tylko dlatego,
że jego panel wygląda tak samo. Instrukcja konfiguracji funkcji jest wspólna,
a paczka i mapa sprzętu są właściwe dla danego sterownika.

## Stan weryfikacji

Kompilacja S1G3/ESP32-C3, testy wspólnego kodu, kontrola manifestu, sum części,
rozmiarów i układu partycji przeszły. Zweryfikowano publiczny ZIP, SHA-256 i mapowanie
obu modeli w OTA. Pakiet zachowuje układ partycji wcześniejszej paczki S1G3.

**Instalacja, BLE i przekaźnik na fizycznym Shelly 1 Gen3 pozostają do sprawdzenia.**
Metadane nadal mają `hardware_tested: false`. Potwierdzone wcześniej próby na Plus 1
nie są testem fizycznego Gen3.
