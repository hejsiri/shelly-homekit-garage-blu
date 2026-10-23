# Instalacja i aktualizacja Shelly HomeKit Garage BLU

Dotyczy **Shelly 1 Gen3 (S1G3)** oraz **Shelly Plus 1 (Plus1)**. Każdy model ma osobną paczkę OTA; nie są zamienne. Paczki nie są przeznaczone dla
Shelly 1 Mini Gen3, 1PM ani Gen4. Wydanie jest eksperymentalne: kompilacja i testy
lokalne przeszły, ale instalacja OTA oraz BLE na fizycznym urządzeniu wymagają
weryfikacji. Nie mamy potwierdzenia bezproblemowego przejścia z fabrycznego 2.1.0.

## Czujniki BLU pozostają na oryginalnym firmware

Czujników **Shelly BLU Door/Window nie trzeba aktualizować ani zmieniać ich
firmware**. Mają działać na oryginalnym firmware Shelly; wystarczy sparować je
z modułem przez panel WWW Garage BLU. Paczka OTA z tej instrukcji jest
przeznaczona dla sterownika **Shelly 1 Gen3 lub Shelly Plus 1**, a nie dla czujników.
Parowanie i działanie na fizycznym sprzęcie wymagają jeszcze weryfikacji.

## Pobieranie

Aktualna paczka i jej suma SHA-256: <https://hejsiri.pl/firmware/garage-blu/>.
Aktualne wydanie to **2.16.1-blu3**, z osobnymi plikami dla Gen3 i Plus 1. Pobierz ZIP bez rozpakowywania. Opcjonalnie
porównaj `shasum -a 256 nazwa-paczki.zip` z plikiem `.sha256` obok paczki.
`update.json` jest listą aktualizacji dla naszego panelu, a nie plikiem firmware.

## Przygotowanie

1. Sprawdź model urządzenia i zapisz aktualną wersję firmware oraz ustawienia
   Wi-Fi, przekaźnika i bramy. Konwersja z fabrycznego firmware nie gwarantuje
   zachowania konfiguracji, skryptów ani funkcji Shelly Cloud.
2. Zatrzymaj bramę i przeprowadzaj aktualizację, gdy nikt z niej nie korzysta.
   Zapewnij stabilne zasilanie i połączenie z lokalną siecią.
3. Jeżeli fabryczny panel proponuje aktualizację do oficjalnego stabilnego
   wydania, wykonaj ją w fabrycznym panelu i poczekaj na restart. Nie traktuj
   tego jako potwierdzenia zgodności z naszym firmware. Nie obniżaj wersji
   fabrycznej tylko po to, by użyć starszej instrukcji instalacji.

## Pierwsza instalacja z fabrycznego firmware Gen3 / Plus

1. Otwórz lokalny panel Shelly pod `http://ADRES_IP/` i zaloguj się, jeżeli
   włączona jest autoryzacja.
2. W sekcji aktualizacji firmware skorzystaj z instalacji własnej paczki ZIP
   lub adresu URL, jeżeli panel udostępnia taką opcję. Wskaż ZIP z hejsiri.pl.
3. Alternatywą dla instalacji przez URL jest fabryczne RPC `Shelly.Update`.
   Poniższy adres **rozpoczyna aktualizację** — zastąp `ADRES_IP` adresem Shelly
   i otwórz go dopiero, gdy jesteś gotowy. Przykład dotyczy **Gen3**; dla **Plus 1** użyj drugiego adresu:

   ```text
   http://ADRES_IP/rpc/Shelly.Update?url=https%3A%2F%2Fhejsiri.pl%2Ffirmware%2Fgarage-blu%2Fshelly-homekit-garage-blu-Shelly1Gen3-2.16.1-blu3.zip
   ```

   **Shelly Plus 1:**

   ```text
   http://ADRES_IP/rpc/Shelly.Update?url=https%3A%2F%2Fhejsiri.pl%2Ffirmware%2Fgarage-blu%2Fshelly-homekit-garage-blu-ShellyPlus1-2.16.1-blu3.zip
   ```

   Przy włączonej autoryzacji użyj uwierzytelnionego panelu lub klienta RPC.
   Nie wyłączaj hasła, aby ominąć błąd autoryzacji. Stare adresy `/ota?url=...`
   z instrukcji Gen1 nie są instrukcją RPC dla Gen3 / Plus.
4. Nie odłączaj zasilania. Poczekaj na zakończenie aktualizacji i restart,
   następnie ponownie otwórz panel. Sprawdź adres w routerze, jeśli się zmienił.
5. Sprawdź w panelu model i wersję `2.16.1-blu3`. Ustaw hasło administratora,
   wybierz **Garage Door Opener**, ustaw sterowanie zgodnie z napędem i sparuj
   czujniki **Closing sensor** oraz **Opening sensor**. Przetestuj oba położenia
   i działanie przekaźnika pod nadzorem przed normalnym użyciem bramy.

Parametr `url` opisuje [oficjalna dokumentacja Shelly.Update](https://shelly-api-docs.shelly.cloud/gen2/ComponentsAndServices/Shelly/#shellyupdate).
Obsługa tego RPC nie stanowi potwierdzenia zgodności naszej paczki z każdym
wydaniem fabrycznym ani układem partycji konkretnego urządzenia.

## Aktualizacja z istniejącego Shelly-HomeKit

Jeżeli panel nie zna jeszcze naszego kanału (np. oryginalne Shelly-HomeKit
lub wcześniejszy lokalny build), pobierz nasz ZIP. W panelu rozwiń **Firmware**,
wybierz **Update from file**, wskaż ZIP i kliknij **Upload**. Poczekaj na restart
i sprawdź wersję. Nie wybieraj **Revert to Stock** — to powrót do firmware fabrycznego.

## Kolejne wydania Garage BLU

W nowym panelu rozwiń **Firmware** i w wierszu **Update** kliknij **Check**.
Panel odczyta `https://hejsiri.pl/firmware/garage-blu/update.json`.
Jeżeli pojawi się nowsza wersja, przeczytaj informacje o wydaniu i kliknij
**Install**. Poczekaj na restart i sprawdź wersję oraz stany obu czujników.
Komunikat **Up to date** oznacza, że kanał nie zawiera nowszego wydania.
Możesz też użyć **Update from file → Upload** z pobraną paczką.

Aktualizacja nie jest uruchamiana przez samo opublikowanie paczki na serwerze.
Nie wykonuj resetu fabrycznego w ramach zwykłej aktualizacji — kasuje również
zapisane w Shelly powiązania i klucze BLU. Zapisz PIN-y czujników niezależnie od urządzenia.

## Gdy aktualizacja się nie powiedzie

Zapisz dokładny błąd, model i wersję. Przy błędzie pobierania sprawdź dostęp
do hejsiri.pl z komputera i spróbuj pobranego ZIP w panelu Shelly-HomeKit.
Przy błędzie modelu, manifestu lub partycji nie wymuszaj zapisu i nie wgrywaj
obrazu innego modelu. Jeżeli urządzenie nie wróci po restarcie, sprawdź listę
klientów routera i sieć konfiguracyjną Shelly; odzyskiwanie przewodowe wymaga
osobnej procedury dla konkretnego sprzętu.
