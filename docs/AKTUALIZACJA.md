# Instalacja i aktualizacja Garage BLU

Instrukcja dla **2.16.1-blu42**, stan: **2026-10-24**. Dostępne są osobne paczki
dla **Shelly Plus 1 (Plus1)** i **Shelly 1 Gen3 (S1G3 / ESP32-C3)**.
Wybierz dokładny model — paczki Plus1 i S1G3 nie są zamienne.

[Poradnik Shelly 1 Gen3](SHELLY_1_GEN3.md) ·
[Konfiguracja po instalacji](KONFIGURACJA.md) ·
[Rozwiązywanie problemów](ROZWIAZYWANIE_PROBLEMOW.md) · [Historia zmian](HISTORIA_ZMIAN.md)

## Wybór sposobu instalacji

| Co jest obecnie na sterowniku | Właściwa ścieżka |
| --- | --- |
| Garage BLU | Firmware → Update → Check; dla nowszej wersji Install. Można też użyć ZIP-u. |
| Oryginalne Shelly-HomeKit lub lokalny build bez kanału Garage BLU | Firmware → Update from file → Upload z paczką Garage BLU. |
| Fabryczny firmware Shelly | Procedura konwersji opisana niżej; nazwy opcji zależą od fabrycznej wersji. |

**Właściwy pakiet instaluje się na sterowniku Plus 1 lub Shelly 1 Gen3.** Door/Window, Motion, H&T i Button1
pozostają na swoim fabrycznym firmware. Ich parowanie jest oddzielnym procesem.
Publikacja nowej paczki na serwerze nie uruchamia aktualizacji urządzenia.

## Pobieranie

- [Strona pobierania](https://hejsiri.pl/firmware/garage-blu/)
- [ZIP 2.16.1-blu42 — Shelly Plus 1](https://hejsiri.pl/firmware/garage-blu/shelly-homekit-garage-blu-ShellyPlus1-2.16.1-blu42.zip)
- [SHA-256 — Shelly Plus 1](https://hejsiri.pl/firmware/garage-blu/shelly-homekit-garage-blu-ShellyPlus1-2.16.1-blu42.zip.sha256)
- [ZIP 2.16.1-blu42 — Shelly 1 Gen3](https://hejsiri.pl/firmware/garage-blu/shelly-homekit-garage-blu-Shelly1Gen3-2.16.1-blu42.zip)
- [SHA-256 — Shelly 1 Gen3](https://hejsiri.pl/firmware/garage-blu/shelly-homekit-garage-blu-Shelly1Gen3-2.16.1-blu42.zip.sha256)

| Parametr paczki | Shelly Plus 1 | Shelly 1 Gen3 |
| --- | --- | --- |
| Model / aplikacja | ShellyPlus1 / Plus1 | Shelly1Gen3 / S1G3 |
| Wersja | 2.16.1-blu42 | 2.16.1-blu42 |
| Build | 20261024-171723 | 20261024-172323 |
| Rozmiar ZIP | 1 821 713 bajtów | 2 258 492 bajtów |
| SHA-256 | `0f18a31bfcc3dcc99d40c0d02559662301c3c640205624dbf1607bc6e167fd07` | `89a2c98f98462d28bb6c2c693334a4ddccef18009fc7e1da3b0d2c4e238e1198` |

Pobierz ZIP **bez rozpakowywania**. Na macOS sumę można sprawdzić poleceniem:

```sh
shasum -a 256 shelly-homekit-garage-blu-ShellyPlus1-2.16.1-blu42.zip
shasum -a 256 shelly-homekit-garage-blu-Shelly1Gen3-2.16.1-blu42.zip
```

`update.json` opisuje kanał aktualizacji; nie jest plikiem firmware do Upload.

## Przygotowanie

1. Sprawdź dokładny model i zapisz obecną wersję oraz ustawienia Wi-Fi i bramy.
2. Przeprowadzaj aktualizację przy zatrzymanej bramie, stabilnym zasilaniu i połączeniu.
3. Zaloguj się do lokalnego panelu. Nie odłączaj zasilania podczas zapisu i restartu.

Zwykła aktualizacja nie wymaga System → Reset, HomeKit Reset ani resetowania BLU.
Reset sterownika kasuje również jego zapisane klucze BLU. Konwersja z fabrycznego
firmware może zmienić dostępne funkcje i konfigurację Shelly Cloud.

## Pierwsza instalacja z fabrycznego firmware Gen3 / Plus

**Wybierz przykład i paczkę zgodne z dokładnym modelem sterownika.**
Obraz Plus1 jest dla ESP32, a S1G3 dla ESP32-C3. Nie używaj ich zamiennie.

1. Otwórz `http://ADRES_IP/` i zaloguj się do fabrycznego panelu.
2. Jeżeli panel pozwala podać własny ZIP lub URL aktualizacji, użyj paczki dla swojego modelu z tej strony.
3. Alternatywnie użyj fabrycznego RPC Shelly.Update z parametrem url. Właściwy adres poniżej rozpoczyna aktualizację po zastąpieniu ADRES_IP adresem urządzenia:

   **Shelly Plus 1:**

   ```text
   http://ADRES_IP/rpc/Shelly.Update?url=https%3A%2F%2Fhejsiri.pl%2Ffirmware%2Fgarage-blu%2Fshelly-homekit-garage-blu-ShellyPlus1-2.16.1-blu42.zip
   ```

   **Shelly 1 Gen3:**

   ```text
   http://ADRES_IP/rpc/Shelly.Update?url=https%3A%2F%2Fhejsiri.pl%2Ffirmware%2Fgarage-blu%2Fshelly-homekit-garage-blu-Shelly1Gen3-2.16.1-blu42.zip
   ```

4. Przy włączonym haśle użyj uwierzytelnionego panelu lub klienta RPC. Nie wyłączaj ochrony w celu obejścia błędu.
5. Poczekaj na restart, otwórz panel ponownie i sprawdź Model, Version oraz Build w sekcjach System i Firmware.
6. Wykonaj [konfigurację bramy i parowanie BLU](KONFIGURACJA.md).

Parametr url jest opisany w [oficjalnej dokumentacji Shelly.Update](https://shelly-api-docs.shelly.cloud/gen2/ComponentsAndServices/Shelly/#shellyupdate).
Dostępność tego RPC nie potwierdza zgodności konwersji z każdą fabryczną wersją lub
partycjonowaniem. Wcześniejsza instalacja na fizycznym Plus 1 została wykonana;
konwersja na fizycznym Gen3 oraz przejście z fabrycznego 2.1.0 do blu42 nie zostały jeszcze potwierdzone.

## Aktualizacja z istniejącego Shelly-HomeKit

1. Pobierz ZIP dla swojego modelu bez rozpakowywania.
2. W Firmware → Update from file wskaż plik i kliknij Upload.
3. Poczekaj na odpowiedź, zapis i restart. Panel może na ten czas utracić połączenie.
4. Otwórz go ponownie i sprawdź, czy Version pokazuje 2.16.1-blu42.

Revert to Stock oznacza powrót do firmware fabrycznego; nie służy do aktualizacji Garage BLU.

## Kolejne wydania Garage BLU

1. W Firmware, w wierszu Update, kliknij Check.
2. Jeśli pojawi się nowsza wersja, otwórz release notes i kliknij Install.
3. Panel pobiera ZIP z hejsiri.pl, uwierzytelnia wysyłanie do sterownika i czeka na odpowiedź.
4. Poczekaj na restart, odśwież stronę i sprawdź wersję oraz odczyty urządzeń BLU.

Up to date oznacza, że kanał nie zawiera nowszej paczki dla tego modelu.
Dla zainstalowanego blu42 samo odświeżenie dokumentacji nie tworzy nowszego firmware.
Dashboard jest ukryty w trakcie OTA; po błędzie aktualizacji odśwież panel po
sprawdzeniu stanu urządzenia.

## Kontrola po restarcie

- W Firmware porównaj Version i Build z paczką.
- W Garage Door sprawdź BLU sensors, przypisania MAC, Name, Movement Time, Pulse Time oraz kierunek po zatrzymaniu.
- Wywołaj odczyty obu krańcówek i sprawdź, czy zmieniają się ich statusy i wiek danych.
- Sprawdź stan w aplikacji Dom oraz opcjonalne urządzenia BLU.
- Zweryfikuj pracę bramy pod nadzorem przed powrotem do normalnej obsługi.

## Gdy aktualizacja się nie powiedzie

W przypadku Uploading..., timeoutu, błędu pobierania lub braku połączenia postępuj
według [diagnostyki OTA](ROZWIAZYWANIE_PROBLEMOW.md#aktualizacja-zatrzymana-na-uploading).
Przy błędzie modelu, manifestu lub partycji zachowaj komunikat i sprawdź paczkę;
nie wymuszaj obrazu innego modelu.

## Stan weryfikacji

Na fizycznym Plus 1 potwierdzono instalację blu42, właściwe identyfikatory i typy
usług w działającej bazie HAP oraz cztery pozycje w aplikacji Dom: bramę, ruch,
obecność i przycisk. Ruch był gotowy i aktywny, obecność zgadzała się ze stanem BLE,
a przycisk udostępniał trzy rodzaje naciśnięcia do przypisania akcji. Ta próba
nie obejmowała wyzwalania przekaźnika ani fizycznego testu wszystkich naciśnięć.

Dla blu42 sprawdzono kompilacje Plus1 i S1G3, testy hosta, podgląd UI, ZIP-y i sumy części,
publiczny SHA-256, CORS, żądania Range oraz metadane kanału. Wcześniej potwierdzono
OTA i szyfrowane odczyty Door/Window na fizycznym Plus 1; obserwowano też odczyty
Motion i Button1. Pełna weryfikacja blu42 na rzeczywistej bramie i fizycznych H&T
pozostaje do wykonania (`hardware_tested: false`).

Paczka Gen3 zachowuje układ partycji wcześniejszej paczki S1G3. Kompilacja i testy
nie zastępują próby instalacji, BLE i przekaźnika na fizycznym Shelly 1 Gen3;
dla tego modelu również pozostaje `hardware_tested: false`.
