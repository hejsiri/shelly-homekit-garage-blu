# Historia zmian Garage BLU

Bieżące opisy i instrukcje dotyczą **2.16.1-blu38** (stan: 2026-10-24).
Zobacz [instalację i aktualizacje](AKTUALIZACJA.md), [konfigurację](KONFIGURACJA.md)
i [rozwiązywanie problemów](ROZWIAZYWANIE_PROBLEMOW.md).

Poniżej zachowano historyczne notatki z rozwoju. Polskie nazwy statusów i opisane
w starszych wersjach obejścia błędów nie stanowią instrukcji dla bieżącego panelu.
Od blu37 teksty systemowe panelu są po angielsku; własne nazwy akcesoriów mogą być
w dowolnym języku. Od blu36 odczyty krańcówek nie są wyświetlane nad garażem.

## Wersja lokalna blu20 — BLU Button1 i zatrzymywanie

- Testowany model przycisku: Shelly BLU Button1 (SBBT-002C). Tough i warianty Zigbee nie są jeszcze objęte tym wdrożeniem.
- Parowanie w panelu: BLU Button1 → Pair; w przycisku włącz tryb parowania. Firmware zapisuje klucz szyfrowania i włącza beacon. Tylko zweryfikowane szyfrowane pakiety mogą sterować bramą.
- Pojedynczy klik: ten sam cykl co w WWW, otwórz → stop → zamknij → stop. Kolejne naciśnięcie po zatrzymaniu domyślnie zmienia kierunek. Opcja „Po zatrzymaniu kontynuuj poprzedni kierunek” dopasowuje stan do napędów kontynuujących ruch. Nie przeprogramowuje samego napędu.
- Beacony nigdy nie sterują przekaźnikiem. Obecność oznacza przycisk w zasięgu BLE, wygasa po 45 s bez zweryfikowanego pakietu; nie potwierdza miejsca zaparkowania.
- Brak krańcówki docelowej po czasie ruchu: „Blokada — limit czasu” i przeszkoda w HomeKit. Timeout nie wysyła samoczynnie dodatkowego impulsu STOP.
- Na razie wymagana weryfikacja parowania i kliknięć na fizycznym przycisku. Kompilowana jest wyłącznie paczka Shelly Plus 1; OTA nie jest instalowane automatycznie.

## blu25 — aktualizacja i parowanie

- OTA pobiera wyzwanie Digest przed wysłaniem ZIP-u. Zapis pokazuje procent postępu;
  żądanie ma limit 120 sekund. Po timeout sprawdź wersję i postęp przed ponowną próbą.
- Po wykryciu jednego pasującego czujnika panel wyświetla jego adres i wybiera go
  automatycznie. Wpisz nowy PIN, a dla zabezpieczonego czujnika także obecny PIN;
  parowanie rozpocznie się automatycznie. Przy kilku pasujących czujnikach wybierz
  właściwy z listy i zatwierdź.
- Jeśli etap zabezpieczania połączenia nie zakończy się w ciągu 15 sekund,
  panel poprosi o ponowne przytrzymanie przycisku czujnika przez 10 sekund.
  Puść przycisk i kliknij **Continue pairing**. Powiązania i klucze pozostają zachowane.
- Przejście Plus 1 z blu23 do blu24 po autoryzacji przed transferem potwierdzono
  na urządzeniu. Zmiany parowania Door/Window wymagają testu na fizycznym czujniku.


## blu27 — odzyskiwanie parowania BLU

- Czujniki Door/Window także wznawiają przerwany odczyt klucza po potwierdzonym
  zapisie PIN-u. Panel poprosi o ponowne włączenie trybu parowania i **Continue pairing**.
  Wybrany wcześniej czujnik jest rozpoznawany automatycznie; PIN nie jest ustawiany ponownie.
- Ponowne połączenie korzysta z aktualnej flagi uwierzytelniania czujnika.
  Ustawienie PIN-u szyfrowania nie wymusza już starej metody parowania BLE na BLU 1.0.22+.
  Starsze czujniki zgłaszające uwierzytelnianie nadal wymagają PIN-u i uwierzytelnionego połączenia.
- Zapisane powiązanie jest wykorzystywane do natychmiastowego wznowienia szyfrowania.
  Rozłączenie samo w sobie nie kasuje powiązania. Odczyt klucza jest wznawiany najwyżej raz.
- Diagnostyka podaje obecność zapisanego klucza połączenia i jego uwierzytelnienie;
  nie ujawnia PIN-ów ani kluczy. Brak potwierdzenia zapisu PIN-u nie oznacza jego sukcesu.
- Testy lokalne i kompilacja nie zastępują próby radiowej: rozwiązanie błędu 0x505
  Button1 oraz ponowne parowanie Door/Window wymagają potwierdzenia na sprzęcie.
- Źródło zmiany protokołu: [changelog Shelly BLU](https://shelly-api-docs.shelly.cloud/docs-ble/changelog/).


## blu28 — uporządkowane parowanie w panelu

1. Otwórz **Pair**, wpisz nowy PIN szyfrowania i kliknij **Start pairing**.
   Jeśli znasz dotychczasowy PIN czujnika, możesz rozwinąć **Sensor already has a PIN?**
   i wpisać go w tym samym kroku.
2. Przytrzymaj przycisk czujnika przez ponad 10 sekund i puść. Jeden pasujący czujnik
   jest wykrywany i parowany automatycznie; wybór z listy pojawia się tylko przy kilku urządzeniach.
3. Modal pokazuje wykryte urządzenie i postęp. Pole nowego PIN-u znika po rozpoczęciu
   wyszukiwania i nie wraca podczas parowania, wznowienia ani po jego zakończeniu.

Jeżeli starszy czujnik wymaga obecnego PIN-u, którego nie podano wcześniej, panel pyta
wyłącznie o ten PIN. Nowy PIN pozostaje zachowany dla tej próby. **Continue pairing**
po ponownym obudzeniu urządzenia nie wymaga ponownego wpisywania PIN-ów.
Anulowanie czeka na trwające żądanie, następnie zatrzymuje parowanie i czyści PIN-y.
Spóźnione odpowiedzi nie cofają kroku ani nie zmieniają nowo otwartego modala.


## blu29 — odnowienie parowania i spójny tekst na kafelkach

- Brama garażowa, czujnik ruchu, obecność auta i przycisk mają wspólną typografię:
  nagłówki 13 px / 600 i statusy 14 px / 400, z jednakowym odstępem między wierszami.
- Kolory tekstu są zgodne z kafelkiem ruchu w trybie jasnym, nocnym i na aktywnym tle.
- Stan **Brak danych** nie przygasza tekstu całego kafelka bramy. Przygaszona ikona
  i opis nadal wskazują niedostępność; blokada sterowania pozostaje aktywna.
- Wygląd sprawdzono w podglądzie panelu, a istniejące testy dashboardu i przycisku przeszły.

- Po timeout wznowienia szyfrowania z zapisanym powiązaniem wybierz **Continue pairing**
  po ponownym przytrzymaniu przycisku czujnika przez ponad 10 sekund i puszczeniu go.
  Nowe zgłoszenie trybu parowania od wybranego czujnika uruchamia jednorazowe odnowienie
  jego powiązania BLE, zamiast kolejnej próby z tym samym kluczem połączenia.
- Odnowienie usuwa wyłącznie powiązanie BLE tego czujnika; zachowuje jego zapisany
  klucz BTHome i powiązania pozostałych urządzeń. Niepowodzenie odnowienia kończy próbę
  z osobnym komunikatem. W logu **pairing_start** odróżnia nowe parowanie od **security_start**.
- Testy odtwarzają timeout, potwierdzenie w panelu, wybór właściwego czujnika i limit
  jednego odnowienia. Skuteczność na fizycznym czujniku wymaga potwierdzenia.

## blu30 — wznowienie po rozłączeniu i rozróżnienie parowania od odczytów

- Po rozłączeniu `0x213` podczas nowego parowania BLE sterownik przez 5 sekund czeka
  na nowe zgłoszenie trybu parowania od tego samego czujnika. Może ponowić połączenie
  jeden raz, bez ponownego wpisywania PIN-u ani usuwania kolejnych powiązań.
- Jeśli czujnik nie zgłosi się sam, modal prosi o przytrzymanie przycisku przez ponad
  10 sekund, puszczenie go i wybranie **Continue pairing**.
- Odrzucenie zabezpieczeń, anulowanie i timeout wznowienia zapisanego LTK nie są
  objęte automatycznym ponowieniem. Wymagania szyfrowania pozostają zachowane.
- Po błędzie modal osobno informuje o aktualnych zaszyfrowanych odczytach wybranego
  czujnika, jeśli jego zapisany klucz nadal działa. Nie oznacza to sukcesu nowego
  parowania ani zastosowania nowego PIN-u. Komunikat wymaga zgodnego MAC i odczytu
  sprzed najwyżej 15 sekund, mieszczącego się także w skonfigurowanym limicie.
- Na blu29 potwierdzono świeże odczyty obu Door/Window pomimo błędu nowego parowania.
  Testy blu30 obejmują rozłączenie, limit ponowienia, anulowanie i rozróżnienie tych
  stanów. Nowa ścieżka ponawiania wymaga jeszcze weryfikacji radiowej.
- [Changelog Shelly BLU](https://shelly-api-docs.shelly.cloud/docs-ble/changelog/)
  opisuje dodatkową próbę po usunięciu powiązania od 1.0.21 oraz usunięcie PIN bonding
  w 1.0.23. Sam kod `0x213` nie rozstrzyga przyczyny zamknięcia połączenia przez czujnik.


## blu31 — parowanie bez zbędnego PIN-u i potwierdzania wznowienia

1. Kliknij **Pair**. Modal od razu uruchamia wyszukiwanie, bez formularza PIN-u i przycisku Start.
2. Przytrzymaj przycisk czujnika przez ponad 10 sekund, następnie go puść.
   Jeden pasujący czujnik jest wybierany automatycznie; przy kilku trzeba wskazać właściwy.
3. Sterownik zachowuje istniejący klucz szyfrowania. Przy ponownym parowaniu tego samego
   czujnika najpierw czeka 3 sekundy na nowy, poprawnie odszyfrowany odczyt — taki odczyt
   potwierdza działającą konfigurację bez ponownego zestawiania połączenia BLE.
4. Gdy brak działającego klucza, sterownik łączy się i odczytuje klucz bez zmiany PIN-u.
   Czujnik z pustym kluczem i obsługą zapisu klucza może otrzymać losowy 128-bitowy klucz
   automatycznie. Zakończenie nadal wymaga potwierdzonego zaszyfrowanego pakietu.
5. PIN pojawia się tylko, jeśli czujnik zgłasza starsze uwierzytelnianie BLE albo wymaga
   PIN-u do aktywacji szyfrowania zamiast bezpośredniego zapisu klucza.
6. Jeśli czujnik wyłączy radio, modal poprosi o ponowne przytrzymanie przycisku.
   Po wykryciu tego samego czujnika parowanie wznawia się samo — bez **Continue pairing**.

Sprawdzono testy protokołu, walidacji, zachowania klucza i licznika antyreplay,
warunkowych pól PIN-u, anulowania i spóźnionych odpowiedzi panelu. Nowy przebieg
wymaga jeszcze weryfikacji na fizycznych czujnikach. Zmiana nie usuwa wymogów
szyfrowania, powiązania BLE ani uwierzytelnienia wymaganego przez starsze urządzenia.


## blu32 — wspólne komunikaty parowania urządzeń

Wspólne instrukcje, etapy, pola PIN-u, wybór urządzenia, potwierdzenia i błędy
parowania używają teraz określenia **device** zamiast **sensor**. Komunikaty
pasują również do BLU Button1. Nagłówek nadal wskazuje konkretny rodzaj urządzenia,
np. **Pair BLU Button1**. Jest to zmiana tekstów; przebieg parowania pozostaje bez zmian.


## blu33 — status przeszkody na kafelku bramy

Status **Blokada — limit czasu** zmieniono na **Przeszkody**, z ikoną wypełnionego
trójkąta z wykrzyknikiem. Ikona pojawia się obok statusu tylko dla dotychczasowego
sygnału przekroczenia czasu ruchu; znika przy innym stanie lub braku aktualnych danych.
Zmiana dotyczy prezentacji statusu i nie zmienia wykrywania ani sterowania bramą.


## blu34 — garaż dostępny po zalogowaniu

Panel garażu jest domyślnie ukryty, a jego ramka ładuje się dopiero po poprawnej
odpowiedzi chronionego RPC GetInfoExt. Wylogowanie, odrzucone logowanie oraz utrata
połączenia usuwają ramkę i zapamiętane dane dashboardu. Spóźnione odpowiedzi po
wylogowaniu nie przywracają widoku. Podczas aktualizacji garaż pozostaje ukryty,
a zakończenie aktualizacji bez autoryzacji nie pokazuje go ponownie.
Urządzenia bez ustawionego hasła zachowują dostęp po pobraniu pełnego statusu.
Sprawdzono testy autoryzacji, wylogowania, opóźnionych odpowiedzi, komunikacji ramki
oraz aktualizacji OTA. Podgląd ekranu logowania nie wyświetla wizualizacji garażu.


## blu35 — nazwy urządzeń, obecność auta i zgodność stanu

Wspólne przełączniki w całym panelu zmniejszono z 57 × 32 do 40 × 24 px,
z proporcjonalnie mniejszym suwakiem i dopasowanym przesunięciem w pozycji włączonej.
Zmiana obejmuje między innymi BLU sensors, kontynuację kierunku i ustawienia Wi-Fi.

Po sparowaniu każdego urządzenia pole **Name** pojawia się w nowym wierszu pod
przyciskiem **Pair**, przed statusem. Wpisz nazwę i użyj **Save** w sekcji bramy.
Nazwa jest zapisywana na Shelly i używana na wizualizacji, także przy odczytach
krańcówek i temperatury. Nazwa głównego kafelka pochodzi z **Name** w sekcji
Garage Door. Usługi Motion oraz H&T przekazują tę nazwę również do HomeKit;
nazwy ręcznie nadane w aplikacji Dom mogą pozostać jej lokalnymi nazwami.

Nowy, poprawnie odszyfrowany pakiet Button1 pokazuje auto na wizualizacji.
Po przekroczeniu limitu obecności (domyślnie 45 s) auto znika. Ręczna zmiana
przyciskiem u góry obowiązuje do następnego pakietu lub zmiany obecności.
Samo odświeżenie statusu nie cofa ręcznej zmiany. Obecność oznacza zasięg BLE,
a nie pewność, że samochód stoi w garażu; beacon nie uruchamia bramy.

Potwierdzone osiągnięcie którejkolwiek krańcówki po przekroczeniu czasu ruchu
usuwa zarówno flagę timeoutu panelu, jak i przeszkodę w HomeKit. Nieaktualne
lub sprzeczne odczyty nadal blokują zmianę stanu. Sprawdzono to w testach
wykonujących produkcyjną maszynę stanów; test na fizycznej bramie pozostaje do wykonania.

Scena jest odsłaniana dopiero po załadowaniu i dekodowaniu kompletu grafik
wybranego motywu. Usunięto początkowe odwołania do starszego domu i samochodu,
które mogły mignąć podczas otwierania strony. Przy statusie **Przeszkody**
animacja pokazuje bramę na **50%**. Jest to umowna wizualizacja błędu;
potwierdzona krańcówka przywraca widok pełnego otwarcia lub zamknięcia.


## blu36 — prostszy widok nad garażem

Usunięto nazwy i statusy czujników pełnego otwarcia i zamknięcia znad wizualizacji
garażu. Pozostają tam odczyty temperatury i wilgotności. Krańcówki nadal są
konfigurowane w sekcji Garage Door i określają stan bramy.


## blu37 — angielski panel WWW

Przetłumaczono pozostałe systemowe teksty panelu i wizualizacji na angielski:
opis beaconów Button1, wskazówkę kierunku po zatrzymaniu, powiadomienie aktualizacji,
statusy bramy, ruchu i obecności, etykiety motywów oraz opisy dostępności.
Domyślne nazwy BLU są po angielsku; własne nazwy zapisane w polach Name pozostają
bez zmian. Status Przeszkody nazywa się teraz **Obstruction**.


## blu38 — odnośniki do repozytorium Garage BLU

Główny link projektu i zgłaszanie błędów w panelu prowadzą do
https://github.com/hejsiri/shelly-homekit-garage-blu oraz jego Issues.
Pomoc dla Garage Door i Firmware prowadzi do dokumentacji Garage BLU.
Usunięto komunikat Donation odsyłający do projektu bazowego.
Stopka zachowuje wskazanie autorów Shelly-HomeKit z listą AUTHORS.md w tym repozytorium.


## blu38 — paczka Shelly 1 Gen3

Dodano osobny ZIP S1G3/ESP32-C3 z tymi samymi funkcjami blu38 co Plus1.
Build: 20261024-150713; ZIP: 2,255,644 bajtów.
SHA-256: `59170d3bf0ad317b11e6ea3a7947dc03142201683d87910caad7c0845c78812e`.

Kanał OTA zawiera oba modele i wybiera adres ZIP-u według identyfikatora urządzenia.
Paczka Plus1 pozostała bez zmian. Publikator domyślnie uwzględnia oba targety
i odrzuca niezgodne wersje, platformy lub sumy części przed zapisaniem metadanych.
Test na fizycznym Gen3 pozostaje do wykonania.
