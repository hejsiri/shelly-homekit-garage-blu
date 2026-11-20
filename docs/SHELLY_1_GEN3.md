# Shelly 1 Gen3 — Garage BLU

**Instalacja od blu.79:** użyj [jednego adresu instalacji](INSTALACJA_JEDEN_ADRES.md)
po zaktualizowaniu fabrycznego firmware do najnowszego stabilnego (sprawdzono stock 2.2.0).
Paczka blu.78 ma poprzedni układ; bezpośrednia aktualizacja istniejącego Gen3
blu.78 do blu.79 jest zablokowana. Test blu.82 na sprzęcie pozostaje do wykonania.

Dla **2.16.1_blu.82**, stan: **2026-11-20**.

Od blu38 dostępna jest osobna paczka Garage BLU dla **Shelly 1 Gen3**. Korzysta
z platformy **ESP32-C3**, ma nazwę aplikacji **S1G3**, a w kanale OTA jest
rozpoznawana jako **Shelly1Gen3**.

[Pełna instalacja i aktualizacje](AKTUALIZACJA.md) ·
[Konfiguracja bramy i BLU](KONFIGURACJA.md) ·
[Rozwiązywanie problemów](ROZWIAZYWANIE_PROBLEMOW.md)

## Właściwa paczka

- [Pobierz ZIP 2.16.1_blu.82 dla Shelly 1 Gen3](https://hejsiri.pl/firmware/garage-blu/shelly-homekit-garage-blu-Shelly1Gen3-2.16.1_blu.82.zip)
- [Pobierz sumę SHA-256](https://hejsiri.pl/firmware/garage-blu/shelly-homekit-garage-blu-Shelly1Gen3-2.16.1_blu.82.zip.sha256)
- [Strona pobierania i metadane](https://hejsiri.pl/firmware/garage-blu/)

| Parametr | Wartość |
| --- | --- |
| Sterownik | Shelly 1 Gen3 |
| Identyfikator OTA / aplikacja | Shelly1Gen3 / S1G3 |
| Platforma | ESP32-C3 |
| Wersja | 2.16.1_blu.82 |
| Build | 20261120-172610 |
| ZIP | 2 359 375 bajtów |
| SHA-256 | `1fd6aefb947b9ba2045aea6fc2d514c355207cf762d5fee162216a05247d70b7` |

ZIP jest dla **Shelly 1 Gen3**, nie dla **Shelly 1 Mini Gen3**, **1PM Gen3**, innych
Gen3 ani Gen4. Paczka S1G3 nie jest zamienna z paczką Plus1 dla Shelly Plus 1.
Firmware wgrywa się na sterownik; urządzenia BLU zachowują fabryczny firmware Shelly.

## Co obsługuje Gen3

Paczka zawiera te same funkcje Garage BLU blu51 co wersja Plus1:

- sterowanie bramą i odczyt stanu w Apple HomeKit przez Wi-Fi;
- dwie szyfrowane krańcówki BLU Door/Window: Closing sensor i Opening sensor;
- opcjonalne Motion, dwa H&T (Shelly BLU H&T, Xiaomi LYWSDCGQ/01ZM lub eksperymentalny LYWSD03MMC przez GATT) oraz Button1;
- parowanie w modalu, automatyczny wybór jednego zgodnego urządzenia i PIN tylko wtedy, gdy jest potrzebny;
- własne nazwy urządzeń i osobną nazwę kafelka auta (Car name), angielski panel i wizualizację Day / Night / Auto;
- obecność auta na podstawie beaconów Button1 i sterowanie pojedynczym naciśnięciem;
- dwie usługi w akcesorium Shelly HomeKit: czujnik obecności auta oraz przycisk programowalny
  z pojedynczym, podwójnym i długim naciśnięciem;
- wspólny z HomeKit status Obstruction i animację 50% przy błędzie;
- aktualizacje OTA z hejsiri.pl z paczką dobieraną według modelu.

## Instalacja i pierwsze uruchomienie

1. Sprawdź na urządzeniu dokładny model Shelly 1 Gen3 i pobierz ZIP S1G3 bez rozpakowywania.
2. Przy istniejącym Gen3 blu.78 lub starszym zatrzymaj się: ma inny układ partycji. Nie wgrywaj blu.79 bezpośrednio przez Upload. Zobacz [ograniczenia aktualizacji](INSTALACJA_JEDEN_ADRES.md#istniejące-garage-blu).
3. Przy fabrycznym firmware użyj [procedury konwersji](AKTUALIZACJA.md#pierwsza-instalacja-z-fabrycznego-firmware-gen3--plus) i przykładu RPC oznaczonego Shelly 1 Gen3.
4. Poczekaj na restart i sprawdź Version oraz Build w panelu.
   W publicznym blu.79 ustaw w Wi-Fi Settings **Power saving → 1** przed
   zapisaniem domowej sieci. Na fizycznym Gen3 tryb Disabled uniemożliwiał
   połączenie; po przełączeniu na 1 urządzenie połączyło się. Domyślną wartość
   poprawiono w źródłach dla kolejnej kompilacji.
5. Ustaw hasło w Security Settings, a w General Settings wybierz Mode → Garage Door Opener i Save.
6. Sparuj obie krańcówki przez Pair, włącz BLU sensors, dopasuj czasy do napędu i zapisz Save.
7. Sprawdź oba położenia, opcjonalne BLU i konfigurację HomeKit według [pełnego poradnika](KONFIGURACJA.md).

Nie kopiuj ustawień okablowania ani obrazu firmware z innego modelu tylko dlatego,
że jego panel wygląda tak samo. Instrukcja konfiguracji funkcji jest wspólna,
a paczka i mapa sprzętu są właściwe dla danego sterownika.

## Stan weryfikacji

Kompilacja S1G3/ESP32-C3, testy wspólnego kodu, kontrola manifestu, sum części,
rozmiarów i układu partycji przeszły. Zweryfikowano publiczny ZIP, SHA-256 i mapowanie
obu modeli w OTA. Od blu.79 układ Gen3 odpowiada fabrycznemu 2.2.0 i różni się od blu.78.

2026-11-12 potwierdzono instalację i start blu.79 na fizycznym Gen3 oraz połączenie
Wi-Fi po ręcznym ustawieniu Power saving → 1. Odczyt RPC potwierdził model
Shelly1Gen3 i wersję 2.16.1_blu.79. **BLE i przekaźnik pozostają do sprawdzenia.**
Metadane nadal mają `hardware_tested: false`. Potwierdzone wcześniej próby na Plus 1
nie są testem fizycznego Gen3.
