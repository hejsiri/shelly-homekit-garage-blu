# Garage BLU: jeden adres instalacji

Stan: 2026-11-11. Obsługiwane są wyłącznie Shelly Plus 1 i Shelly 1 Gen3.

1. W fabrycznym panelu Shelly zainstaluj najnowszy **stabilny** firmware producenta.
2. Otwórz [instalator Garage BLU](https://hejsiri.pl/firmware/garage-blu/update/),
   wpisz lokalny adres IPv4 Shelly i kliknij Sprawdź zgodność.
   Link pojawi się dopiero po potwierdzeniu modelu sprzętowego, generacji,
   aplikacji i dokładnej wersji fabrycznego firmware.
3. Otwórz wygenerowany link Rozpocznij instalację. Poczekaj na zapis i restart.

Szablon adresu, wspólny dla obu modeli:

```text
http://ADRES_IP/ota?url=https%3A%2F%2Fhejsiri.pl%2Ffirmware%2Fgarage-blu%2Fupdate%2F
```

Jeśli dana wersja panelu nie udostępnia `/ota`, użyj fabrycznego RPC z tym samym
adresem źródłowym:

```text
http://ADRES_IP/rpc/Shelly.Update?url=https%3A%2F%2Fhejsiri.pl%2Ffirmware%2Fgarage-blu%2Fupdate%2F
```

Przy włączonym haśle zaloguj się do panelu lub użyj uwierzytelnionego klienta RPC.
ZIP można też pobrać ze strony pobierania i wysłać ręcznie bez rozpakowywania.

## Wersje fabryczne i zgodność

Oficjalne API aktualizacji producenta na 2026-11-11 zwraca:

| Model | Najnowszy stabilny firmware | Build | Układ aplikacji / systemu plików |
| --- | --- | --- | --- |
| Shelly Plus 1 | 1.7.6 | 20261026-143003/1.7.6-g01e9e18 | 0x190000 / 0x60000 |
| Shelly 1 Gen3 | 2.2.0 | 20261109-150519/2.2.0-g7968dca | 0x300000 / 0xE0000 |

Porównano kompletne tablice partycji ZIP-ów Garage BLU i rzeczywistych obrazów
fabrycznych. Sumy oficjalnych ZIP-ów:

- Plus 1 1.7.6: `c405248b2257144f3e0517457c795c7e6cfb0089e5e93195851df8b0aa182cc2`.
- Gen3 2.2.0: `7c6f565fc225f005b6100045fa1bfadb3f4cb72e59a7b3fea52078971927f76c`.

Instalacja i działanie blu.79 na fizycznych urządzeniach pozostają do potwierdzenia.
Nowsza wersja fabryczna może ponownie zmienić układ; zgodność trzeba wtedy sprawdzić.

## Jak dobierana jest paczka

Shelly pobiera `/update/`, wysyłając `X-MGOS-Device-ID`. Endpoint rozpoznaje
prefiks `shellyplus1` albo `shelly1g3` (bez rozróżniania wielkości liter) i zwraca
HTTP 302 do jednego bieżącego ZIP-a dla danego modelu, opisanego w `install.json`,
po sprawdzeniu wersji z `X-MGOS-FW-Version`.
Mini, PM i Gen4 są odrzucane. Nieznany model daje 404, niezgodna lub nieznana
wersja stock 412, a brak poprawnej paczki 503. Nie są utrzymywane starsze warianty.

Przeglądarka próbuje odczytać `Shelly.GetDeviceInfo` przez WebSocket `/rpc`.
Jeśli zasady dostępu do lokalnej sieci lub połączeń HTTP blokują odczyt, instalator
udostępnia link do `http://ADRES_IP/rpc/Shelly.GetDeviceInfo` i pole na skopiowaną
odpowiedź JSON. Odczyt tej metody nie wymaga podania hasła; dane nie są wysyłane
na serwer hejsiri.pl. Przy wklejaniu użytkownik musi pobrać odpowiedź z właściwego IP.

Wspólne reguły `update/compatibility.json` dopuszczają wyłącznie:
`SNSW-001X16EU / Gen2 / Plus1 / 1.7.6` i `S3SW-001X16EU / Gen3 / S1G3 / 2.2.0`.
Niezgodny model, generacja, aplikacja, wersja lub niepełny identyfikator stock
blokują wygenerowanie linku. Zmiana IP lub danych usuwa wcześniej wygenerowany link.
Nowsza wersja stock także jest blokowana do czasu sprawdzenia zgodności.

Aktualizacja ZIP-ów i `install.json` zmienia bieżące paczki bez zmiany adresu.
Samo otwarcie `/update/` w przeglądarce wyświetla instrukcję, nie uruchamia zapisu.

## Komunikaty o problemach

Instalator objaśnia niezgodność przed pokazaniem linku. Serwer dodatkowo odrzuca
pobieranie przy niezgodnej wersji stock. Weryfikację samego obrazu wykonuje Shelly.
Jego panel może pokazać tylko własny komunikat,
np. `PT update is req'd but unsupported by the new fw` / `pt check failed`,
zamiast tekstowej odpowiedzi endpointu. Instalator objaśnia ten błąd:
sprawdź najnowszy stabilny firmware i model; jeśli są zgodne z tabelą,
zachowaj pełny log i zgłoś problem. Nie wymuszaj zapisu ani zmiany bootloadera.

## Istniejące Garage BLU

Ten endpoint służy do pierwszej konwersji z fabrycznego firmware. Kolejne aktualizacje
korzystają z Firmware → Check i `update.json`.

Gen3 blu.78 i starsze mają poprzedni układ partycji. Kanał OTA nie oferuje im nowego
ZIP-a Gen3. Nowy panel potrafi wyświetlić powód blokady; stary panel może pokazać
ogólne Check failed. W takim przypadku nie wgrywaj nowego ZIP-a ręcznie.
Przejście przez Revert to Stock i najnowszy stock wymaga osobnej weryfikacji,
zwłaszcza gdy obraz fabryczny nie mieści się w starym slocie.

Nie instaluj przypadkowego obrazu fabrycznego przez Revert to Stock w celu obejścia
tej blokady. Dla działającego Plus 1 układ partycji nie zmienia się.

## Utrzymanie

`tools/ota/prepare.py` przygotowuje po jednym bieżącym ZIP-ie na model, metadane,
endpoint PHP i instrukcje. Zmiana układu Gen3 jest od blu.79 domyślna; dawny wariant
`GEN3_STOCK_22` nie jest już potrzebny. Nie publikuj równoległych wariantów stock22.

Przed publikacją porównaj obrazy z aktualnym stock przy pomocy
`tools/ota/check_stock_layout.py`, uruchom testy endpointu, walidacji i kanału OTA.
Wyślij ZIP-y i sumy, zweryfikuj publiczne pliki, następnie opublikuj metadane
i endpoint. `update.json` przełącz na końcu.

## Bieżące paczki blu.79

- ShellyPlus1: build `20261111-223031`, 1850290 bajtów, SHA-256 `79fb8933ca9065070ae49114b22926b3f4f4e6f1b77df65931379c7cd46dd070`.
- Shelly1Gen3: build `20261111-223036`, 2355039 bajtów, SHA-256 `03e07839866f3e5e7c34dd07112c84a9826c427f58dc175c9d1c74026f86bab1`.
