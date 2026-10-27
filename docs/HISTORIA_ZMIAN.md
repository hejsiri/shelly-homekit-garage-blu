# Historia zmian Garage BLU

## 2.16.1_blu.64 (2026-10-27) — tylko Shelly Plus 1

- Listy typów urządzeń pokazują same nazwy marketingowe: usunięto modele w nawiasach oraz dopiski Bindkey BLE.
- Usunięto „Mi” z nazw termometrów Xiaomi w panelu. Nazwy w oknie wykrywania termometrów pozostawiają model do identyfikacji urządzenia.
- Sposób parowania i obsługa Bindkey BLE pozostają bez zmian. Gen3 pozostaje na 2.16.1_blu.59.

## 2.16.1_blu.63 (2026-10-27) — tylko Shelly Plus 1

- Poprawiono zawieszenie podczas usuwania czujnika: puste MAC są sprawdzane przed parserem SDK. Usunięcie BLU Motion potwierdzono na fizycznym Plus 1 bez restartu; sprawdzono zachowanie po odświeżeniu. Poprawiono też obsługę spóźnionego odczytu termometru po usunięciu przypisania.
- Dodano Xiaomi Motion Sensor 2S (wariant chiński i globalny): parowanie przez Bindkey BLE, ruch, jasność i bateria. Publiczna zaszyfrowana próbka przeszła weryfikację AES-CCM i testy ochrony przed powtórzeniem. Próba fizycznego czujnika pozostaje do wykonania.
- Dodano format baterii MiBeacon `0x4803` dla MCCGQ02HL i CGPR1. Testy obejmują poprawne/błędne pakiety i zachowanie baterii po zmianie stanu. Nie przechwycono jeszcze pakietu baterii z czujników użytkownika.
- Wizualizacja pokazuje siłę Wi-Fi (RSSI) i ikonę włączonego Bluetooth; wskaźniki reagują na utratę łączności.
- Ujednolicono nazwy handlowe akcesoriów i oznaczenia modeli. Karta urządzenia podczas parowania ma białe tło.
- Testy hosta, panelu i kompilacja Plus 1. Pełna próba całego wydania na fizycznej bramie pozostaje do wykonania. Gen3 pozostaje na 2.16.1_blu.59.

## 2.16.1_blu.62 — lokalna próba

- Testowa paczka poprawki usuwania BLU Motion. Niepublikowana w OTA; zmiany włączono do blu.63.

## 2.16.1_blu.61 (2026-10-27) — usuwanie BLE poza główną pętlą, tylko Plus 1

- Kasowanie powiązania transportowego Shelly BLU wykonuje osobne zadanie FreeRTOS. Obsługa RPC nie czeka synchronicznie na magazyn NimBLE/NVS.
- Po 8 sekundach bez zakończenia panel otrzymuje błąd i zachowuje przypisanie aplikacji. Spóźniony wynik nie usuwa przypisania ani nie odpowiada drugi raz. Kolejna próba jest blokowana do wyjścia zadania.
- Usunięcie klucza aplikacji, MAC, nazwy, odczytów i odświeżenie HomeKit następuje dopiero po sukcesie zadania. Zachowano pomijanie NimBLE dla Xiaomi/Qingping i ochronę współdzielonych powiązań.
- Testy hosta obejmują kolejkę zadania, brak kasowania BLE na stosie RPC, timeout, spóźnione zakończenie, błędy oraz sześć ról. Kompilacja Plus 1; skuteczność na fizycznym Shelly wymaga potwierdzenia. Gen3 pozostaje na blu.59.

## 2.16.1_blu.60 (2026-10-27) — tylko Shelly Plus 1

- Przy usuwaniu Shelly BLU kasowanie powiązania transportowego ma limit liczby rekordów i kontrolę ich obecności. Błąd magazynu zatrzymuje operację zamiast pozwalać na nieograniczoną pętlę; pozostałe urządzenia zachowują swoje rekordy.
- Zachowano czyszczenie danych prywatności BLE i dodano logi etapów usuwania oraz odświeżenia HomeKit, bez zapisywania kluczy.
- Testy obejmują brak postępu mimo zgłaszanego sukcesu magazynu, rozdzielenie MAC/typu adresu, błędy odczytu/zapisu i wszystkie role. Skuteczność wobec fizycznego zawieszenia BLU Motion wymaga próby na Shelly.
- Wydano wyłącznie paczkę Shelly Plus 1. Ostatnia paczka Gen3 pozostaje 2.16.1_blu.59.

## 2.16.1_blu.59 (2026-10-27) — jeden model LYWSD03MMC

- Połączono pozycje LYWSD03MMC w jeden model na liście termometrów. Szyfrowany MiBeacon wywołuje formularz Bindkey BLE; zachowano eksperymentalny odczyt GATT dla pozostałego wariantu.
- Dotychczasowe przypisania GATT i Bindkey są pokazywane pod wspólną nazwą. Klucz nadal jest weryfikowany przed zapisaniem, a próba użycia szyfrowanego beacona bez klucza jest odrzucana.
- Testy obu ścieżek parowania, rzeczywistego RPC Bind i formularza PL; próba na fizycznym czujniku pozostaje do wykonania.

## 2.16.1_blu.58 (2026-10-27) — Qingping

- Dodano CGG1 (temperatura/wilgotność), CGH1 (kontaktron) i CGPR1 (ruch i jasność). Obsługiwany jest natywny format Qingping oraz MiBeacon; zaszyfrowany wariant wymaga zweryfikowanego Bindkey BLE.
- Panel pokazuje modele i miniaturki, wykrywa potrzebę podania klucza i zapamiętuje przypisanie. Temperatura/wilgotność zachowują godzinę ważności, a pakiety baterii i jasności nie odświeżają stanu krańcówki/ruchu.
- Usuwanie tych modeli nie dotyka powiązań transportowych BLE. Nieszyfrowane pakiety są oznaczane jako nieszyfrowane.
- Testy hosta i przeglądarki oraz kompilacje obu modeli; próby z fizycznymi czujnikami pozostają do wykonania. [Szczegóły](QINGPING.md).

## 2.16.1_blu.57 (2026-10-27) — usuwanie Xiaomi

- Usuwanie przypisań Xiaomi MiBeacon, Bindkey BLE i GATT pomija kasowanie powiązań transportowych NimBLE, których te ścieżki nie tworzą. Nadal kasuje przypisanie NVS, klucz, nazwę i odczyty oraz odświeża HomeKit.
- Shelly BLU zachowuje czyszczenie rzeczywistych powiązań BLE. Test regresyjny sprawdza rozdzielenie tych ścieżek i zachowanie pozostałych urządzeń. Potwierdzenie na fizycznym Shelly pozostaje do wykonania.

## 2.16.1-blu56 — Xiaomi Bindkey BLE

Wydanie: 2026-10-26. Testy hosta i przeglądarki; sprawdzenie Bindkey na fizycznych czujnikach pozostaje do wykonania.

- Dodano ręczne podawanie 32-znakowego Bindkey BLE dla MCCGQ02HL (krańcówki otwarcia/zamknięcia) i LYWSD03MMC (temperatura/wilgotność wewnątrz/na zewnątrz).
- Klucz jest sprawdzany na świeżym beaconie AES-CCM z MIC przed zapisaniem. Pomyłka nie zmienia przypisania. Zapis w NVS obejmuje klucz i licznik chroniący przed ponownym użyciem pakietów, także po restarcie.
- Pakiety baterii nie odświeżają stanu krańcówki. Dotychczasowe limity świeżości i blokady sterowania pozostają aktywne. Usunięcie urządzenia kasuje również Bindkey.
- Klucz nie jest zwracany w statusie, zapisywany w przeglądarce ani wypisywany w logu wysyłanych poleceń panelu. Pole klucza jest czyszczone po wysłaniu i zamknięciu okna.
- Nowy protokół nie przywraca automatycznie wycofanych wpisów blu46. Obsługa wymaga ręcznego dodania urządzenia.
- Szczegóły i ograniczenia: [Bindkey BLE](BINDKEY_BLE.md).

## 2.16.1-blu56 — rozpoznawanie urządzeń podczas parowania

- Lista wykrytych urządzeń pokazuje lokalną miniaturkę, nazwę modelu, opis, MAC i RSSI zamiast wyboru po samym MAC. Rozpoznawanie korzysta z identyfikatorów modeli dostarczanych przez obecny skaner.
- Katalog obejmuje Shelly BLU Button1, Door/Window, Motion i H&T oraz Xiaomi LYWSDCGQ/01ZM i LYWSD03MMC. Nie rozszerza obsługi protokołów ani modeli. Zdjęcia pokazują wygląd referencyjny; kolor obudowy nie jest wykrywany.
- Nieznany identyfikator ma neutralny opis bez przypadkowego zdjęcia. Wybór spośród kilku zgodnych urządzeń zachowuje dokładny MAC, a aktualizacje RSSI nie przebudowują kart.
- Zdjęcia producentów są dołączone do panelu jako małe WebP, bez odwołań sieciowych. Źródła: `BLU_DEVICE_PHOTOS.json`.

## 2.16.1-blu56 — polskie parowanie

- Uzupełniono polskie statusy ponownego łączenia, oczekiwania na to samo urządzenie, walidację PIN-u oraz błędy parowania i odczytu termometrów. Kody diagnostyczne pozostają dostępne.
- Teksty okna parowania są tłumaczone od razu podczas zmiany etapu. Sprawdzono 23 etapy oraz przełączanie EN / PL w przeglądarce.

## 2.16.1-blu56 — usuwanie akcesoriów BT

- Sparowane akcesoria mają szary przycisk „Usuń” zamiast „Sparuj”.
- Usunięcie kasuje klucz i licznik z NVS, powiązanie BLE, MAC, nazwę i ostatnie odczyty wybranego urządzenia. Przy usunięciu przycisku znika także nazwa auta.
- Pozostałe akcesoria zachowują swoje dane. Powiązanie BLE wspólne dla innego przypisania pozostaje do usunięcia ostatniego przypisania.
- Usunięcie czujnika krańcowego blokuje sterowanie BLU do ponownego sparowania. Opcjonalne akcesoria są usuwane również z usług HomeKit.


Bieżące opisy i instrukcje dotyczą **2.16.1_blu.61 dla Plus 1 / 2.16.1_blu.59 dla Gen3** (stan: 2026-10-27).
Zobacz [instalację i aktualizacje](AKTUALIZACJA.md), [konfigurację](KONFIGURACJA.md)
i [rozwiązywanie problemów](ROZWIAZYWANIE_PROBLEMOW.md).

Poniżej zachowano historyczne notatki z rozwoju. Polskie nazwy statusów i opisane
w starszych wersjach obejścia błędów nie stanowią instrukcji dla bieżącego panelu.
Od blu37 bazowe teksty panelu są po angielsku, a od blu54 dostępny jest wybór EN / PL; własne nazwy akcesoriów mogą być
w dowolnym języku. Od blu36 odczyty krańcówek nie są wyświetlane nad garażem.

## 2.16.1-blu56 — ważność temperatury i wilgotności

Ostatni pomiar temperatury i wilgotności wewnątrz / na zewnątrz jest ważny przez
3600 sekund (1 godzinę), niezależnie od ustawienia Sensor timeout. Dotyczy Shelly
BLU H&T oraz obu obsługiwanych protokołów Xiaomi. Każda wartość ma własny czas
ostatniego pomiaru. Pakiety z samą baterią, nieudane odczyty i odświeżanie panelu
nie przedłużają ważności pomiaru. Po godzinie bez nowego pomiaru dana wartość jest
nieaktualna w panelu i HomeKit. Restart sterownika nadal wymaga nowego odczytu.
Limity krańcówek, ruchu i obecności auta pozostają bez zmian.

Testy: rzeczywiste funkcje C++ ważności odczytów, granica 3600 sekund, niezależność
temperatury i wilgotności, pakiety baterii oraz niezmieniona blokada krańcówek.

## blu55 — separatory akcesoriów BT i wybór języka w Systemie (2026-10-26)

- Dodano pięć delikatnych separatorów pomiędzy sześcioma akcesoriami Bluetooth w ustawieniach Garage Door.
- Przełącznik EN / PL przeniesiono na dół panelu System, do wiersza Language / Język. Wybór nadal jest zapamiętywany w przeglądarce i obejmuje wizualizację garażu.
- Sprawdzono układ oraz działanie zmiany języka w lokalnym podglądzie. Weryfikacja na fizycznym sterowniku pozostaje do wykonania.

## blu54 — wybór języka EN / PL (2026-10-26)

- Dodano przełącznik EN / PL nad panelem. Domyślnym językiem pozostaje angielski; wybór jest zapamiętywany w przeglądarce dla danego adresu Shelly.
- Tłumaczenie obejmuje ustawienia, statusy, instrukcje parowania, komunikaty aktualizacji, okna potwierdzeń, podpowiedzi i opisy dostępności oraz wizualizację garażu.
- Zmiana języka działa bez przeładowania. Panel garażu otrzymuje tę samą wersję językową, również gdy zapis w localStorage jest niedostępny.
- Własne nazwy urządzeń i auta, nazwy w HomeKit, adresy MAC, wartości pól i żądania RPC pozostają niezmienione. Nazwa „Open” lub „Ready” wpisana przez użytkownika nie jest tłumaczona.
- Obie strony zawierają lokalnie dołączony słownik; nie korzystają z usług tłumaczeniowych ani zewnętrznych bibliotek. Dłuższe polskie etykiety zawijają się w formularzu.
- Sprawdzono testy jednostkowe, przełączanie języka i odświeżanie w przeglądarce, formularz parowania, nazwy użytkownika i widok mobilny. Test na fizycznych sterownikach pozostaje do wykonania.

## blu53 — wizualizacja odbioru Bluetooth (2026-10-26)

- Badge akcesorium w ustawieniach Garage Door świeci na niebiesko przez 650 ms po zaobserwowaniu nowego zaakceptowanego odczytu (`packet_seq`).
- Odświeżanie tego samego odczytu, pierwszy status po otwarciu panelu, nieaktualne dane i niesparowane urządzenia nie wywołują błysku.
- Kolejny odebrany pakiet przedłuża błysk; po nim wraca kolor bieżącego stanu. Zmiana przypisanego urządzenia i restart licznika resetują wskaźnik.
- Wskaźnik reaguje na otrzymanie statusu przez panel; nie jest pomiarem dokładnego czasu odbioru radiowego.
- Testy logiki panelu wykonano lokalnie. Weryfikacja nowego efektu na fizycznych sterownikach pozostaje do wykonania.

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


## blu39 — osobna nazwa auta

Pod Name sparowanego Button1 dodano Car name. Pierwsze pole nazywa wskaźnik
przycisku, drugie kafelek obecności auta. Puste pola pokazują podpowiedzi Button
i Name of car; Save zapisuje obie wartości na sterowniku. Wyczyszczenie pól
przywraca domyślne etykiety kafelków. Zmiana nazwy nie wymaga ponownego parowania
i zachowuje klucze oraz bieżące odczyty. Funkcja jest dostępna w paczkach Plus1 i S1G3.


## blu40 — obecność auta i Button1 w HomeKit

Sparowany Button1 udostępnia dwa osobne akcesoria HomeKit: Occupancy Sensor
z nazwą Car name oraz Stateless Programmable Switch z nazwą Name przycisku.
Pojedyncze, podwójne i długie naciśnięcia są przekazywane do aplikacji Dom.
Pojedynczy klik zachowuje bezpośrednie sterowanie bramą. Zwykłe beacony,
nieobsługiwane typy zdarzeń, niepoprawne i powtórzone pakiety nie wywołują akcji.

Obecność w HomeKit i panelu pochodzi z tego samego stanu firmware; 45 sekund bez
beacona oznacza brak obecności. Brak Bluetooth lub nasłuchu BLE jest raportowany jako błąd.
Akcesoria mają stałe identyfikatory i pozostają w HomeKit po utracie zasięgu.
Zmiana Car name odświeża nazwę akcesorium bez usuwania kluczy i odczytów.

Badge Closed ma czarne tło i biały tekst w jasnym i ciemnym motywie.

## blu41 — wspólny układ usług HomeKit

Obecność auta i przycisk są teraz usługami istniejącego akcesorium Shelly,
analogicznie do ruchu i H&T. Usunięto mieszany model, w którym dwie nowe usługi
z blu40 były mostkowane, a pozostałe były częścią bramy. Zachowano AID bramy
oraz identyfikatory usług bramy i ruchu. Jednorazowa migracja zwiększa numer
konfiguracji HomeKit, bez resetu parowań i kluczy BLU.

Ruch i obecność zwracają wartości właściwego typu również przed pierwszym odczytem;
Status Fault i Status Active określają dostępność. Przy utracie odbioru zachowują
ostatni stan i zgłaszają błąd zamiast wysyłać null jako wartość czujnika.
Przycisk programowalny zachowuje wymagany przez HomeKit IP odczyt null i trzy
rodzaje zdarzeń. Sprawdź akcje przypisane do osobnego przycisku z blu40 po migracji.

Dodano test rzeczywistych klas mgos::hap i serializacji /accessories przez Apple ADK,
w tym unikalności identyfikatorów, jednej usługi głównej, typów odczytów,
statusów dostępności i układu jednego akcesorium garażu.

## blu42 — wymiana błędnie przypisanych usług BLU

W logach aplikacji Dom wykryto próbę odczytu obecności (typ 0x71, charakterystyka
5634) z usługi 5120, której firmware używał dla ruchu. HomeKit nie potrafił
dopasować charakterystyki i zgłaszał HMErrorDomain 2/3. Blu41 nie usunęło tego
przypisania mimo odświeżenia numeru konfiguracji.

Blu42 wycofuje stare identyfikatory usług ruchu, obecności i przycisku. Zastępuje
je nowymi: 0x1800, 0x1900 i 0x1a00. Pozostają one usługami akcesorium bramy,
którego identyfikatory i parowania nie zmieniają się. Jednorazowa migracja z blu41
lub starszych wersji zwiększa numer konfiguracji. Sprawdź ponownie akcje przycisku
i automatyzacje korzystające z ruchu lub obecności.

Diagnostyka Shelly.GetDebugInfo pokazuje faktycznie uruchomioną bazę HomeKit:
identyfikatory i typy usług/charakterystyk oraz gotowość źródeł BLU. Nie zawiera
kluczy parowania ani haseł. Test bazy obejmuje teraz rzeczywistą ścieżkę wyszukiwania
i odczytu w Apple ADK oraz serializację porcjami takimi jak na sterowniku.
Na fizycznym Plus 1 z tym błędem zainstalowano blu42 i odczytano uruchomioną bazę:
trzy usługi mają właściwe typy i oddzielne identyfikatory, oba źródła BLU są gotowe.
W aplikacji Dom potwierdzono cztery pozycje: bramę, gotowy i aktywny ruch, wykrytą
obecność oraz przycisk z pojedynczym, podwójnym i długim naciśnięciem do konfiguracji.
Pełne próby fizycznych naciśnięć i pracy bramy pozostają do wykonania.

## blu43 — natężenie światła z BLU Motion w HomeKit

Sparowany BLU Motion dodaje usługę Light sensor w istniejącym akcesorium Shelly.
Odczyt LUX pochodzi z uwierzytelnionego obiektu BTHome 0x05, z przeliczeniem uint24
na luksy ze skalą 0,01. Nie dodano usług światła z Door/Window.

Pomiar ma oddzielny znacznik czasu: pakiet ruchu bez LUX zachowuje odczyt, lecz nie
odświeża jego ważności. Brak pomiaru, przeterminowanie lub niedostępność źródła
ustawiają Status Fault i nieaktywny status. Wartość HomeKit mieści się w zakresie
0,0001–100000 lx i wysyła powiadomienia o zmianie.

Nowa usługa ma IID 0x1b00. Zachowano identyfikatory bramy, ruchu, obecności i przycisku;
jednorazowe odświeżenie konfiguracji nie resetuje parowań. Testy obejmują skalowanie,
brakujące/przeterminowane pomiary, granice HomeKit, powiadomienia oraz prawdziwą
ścieżkę odczytu i serializacji Apple ADK. Próba nowego pomiaru na sprzęcie pozostaje
do wykonania po instalacji.

## blu44 — ręczne ukrycie auta wygasza kafelek obecności

Ikona auta i kafelek Car presence korzystają z jednego stanu wizualizacji. Ukrycie
auta natychmiast wyłącza podświetlenie kafelka i ustawia Not detected. Odpytywanie
tego samego pakietu nie cofa ręcznego wyboru; nowy beacon Button1 przywraca stan
obecności. Odczyt obecności w HomeKit nadal pochodzi z BLE.

Zachowanie sprawdzono w istniejących testach panelu i lokalnym podglądzie:
ukrycie, odświeżenia tego samego pakietu oraz powrót po nowym beaconie.

## blu45 — termometr Xiaomi LYWSDCGQ/01ZM

Dodano pasywny odbiór fabrycznego MiBeacon 0x01aa: temperatura, wilgotność i bateria.
W Indoor H&T oraz Outdoor H&T można wybrać Shelly BLU H&T lub Xiaomi LYWSDCGQ/01ZM.
Xiaomi nie wymaga PIN-u, zmiany firmware ani ponownego wprowadzania w tryb parowania.
Modal wykrywa urządzenie i kończy po potwierdzeniu temperatury oraz wilgotności.

Przypisanie Xiaomi jest trwałe i oddzielone od kluczy BTHome. Działa wyłącznie
w dwóch rolach H&T; nie autoryzuje sterowania bramą ani innych ról. Każdy pomiar
ma własny czas ważności, niezależny od beaconów baterii. WWW oraz HomeKit pokazują
te same odczyty. H&T zachowują identyfikatory usług, a brak świeżych danych zgłasza
Status Fault z poprawnie typowaną wartością HomeKit.

Sprawdzono opublikowaną próbkę LYWSDCGQ, pakiety częściowe i uszkodzone, restart,
izolację od chronionych ról, regresje BLU, prawdziwy odczyt/serializację Apple ADK
oraz lokalny podgląd modalu. Fizyczna próba Xiaomi i Gen3 pozostaje do wykonania.

## blu46 — Xiaomi LYWSD03MMC na fabrycznym firmware

Historyczne wydanie: obsługa tego modelu została wycofana w blu48.

Dodano szyfrowany MiBeacon v4/v5 modelu 0x055b. Indoor/Outdoor H&T mają opcję
Xiaomi LYWSD03MMC (stock); modal przyjmuje jeden bindkey i opcjonalny ręczny MAC.
Nie zmienia firmware ani klucza czujnika. Odszyfrowana temperatura i wilgotność trafiają do panelu i istniejących usług
H&T HomeKit. Poziom baterii jest pokazywany w panelu WWW.

Przypisanie jest zapisywane po weryfikacji obu pomiarów. Wcześniejsze urządzenie
pozostaje aktywne do tego momentu. Okno wykrywania/weryfikacji wynosi 15 minut,
bez blokowania dotychczasowego sterowania BLU. Efektywny timeout tego termometru
wynosi minimum 30 minut, z osobną ważnością temperatury i wilgotności.

Testy obejmują prawdziwe AES-CCM i opublikowaną próbkę stock Xiaomi, ochronę
przed powtórzeniem po restarcie, walidację MAC/klucza, częściowe pomiary oraz
odrzucanie nieszyfrowanych danych. Bindkey nie jest zwracany w statusie.
Sprawdzono modal w lokalnym podglądzie; fizyczna próba LYWSD03MMC jest do wykonania.
W blu45 potwierdzono na fizycznym Plus1 odczyty LYWSDCGQ/01ZM wraz z baterią 36 %.

## blu47 — Xiaomi Mi Door and Window Sensor 2 MCCGQ02HL

Historyczne wydanie: obsługa tego modelu została wycofana w blu48.

Dodano fabryczny szyfrowany MiBeacon modelu 0x098b. Można wybrać go zamiast
BLU Door/Window jako Closing/Opening sensor. Jego stan służy istniejącej bramie;
nie dodaje osobnego kontaktronu ani nowej usługi HomeKit.

Modal wymaga bindkey tylko raz, opcjonalnie MAC, a następnie ruchu magnesem.
Przypisanie jest zatwierdzane dopiero po odszyfrowaniu stanu kontaktronu. Pakiet
baterii lub światła nie zmienia ani nie odświeża tego stanu. Reset zgłoszony przez
czujnik unieważnia odczyt. Po restarcie Shelly trzeba odebrać nowy stan magnesu.
Timeout krańcówek pozostaje konfigurowalny; stare lub sprzeczne odczyty blokują
sterowanie tak jak dla BLU Door/Window.

Sprawdzono opublikowaną próbkę MCCGQ02HL, rzeczywiste AES-CCM, stany magnesu,
powtórzone/uszkodzone pakiety, rozdzielenie ról i ważności danych oraz odbiór
krańcówki bez odświeżania jej stanu przez baterię. Regresje istniejących usług
HomeKit przeszły testy z Apple ADK. Podgląd WWW korzysta z danych przykładowych.
Testy fizycznego Xiaomi i Gen3 są nadal do wykonania.

## blu48 — parowanie bez ręcznego klucza Xiaomi

Wycofano eksperymentalne LYWSD03MMC i MCCGQ02HL z panelu i parowania przez RPC.
Usunięto formularz bindkey oraz instrukcje pozyskiwania klucza z aktualnej
konfiguracji. Xiaomi LYWSDCGQ/01ZM pozostaje dostępny w Indoor/Outdoor H&T:
wykrywanie i odbiór z fabrycznych beaconów, bez PIN-u i konta Xiaomi.

Zachowano szyfrowane parowanie Shelly BLU, HomeKit, nazwy, wizualizację i odczyt
baterii. Nie obniżono szyfrowania wycofanych modeli do trybu nieszyfrowanego.
Zapisane rekordy urządzeń nie są kasowane; wycofane modele trzeba zastąpić
obsługiwanymi. Panel respektuje również zgłoszoną przez firmware niegotowość
urządzenia przy wyświetlaniu nieaktualnego statusu.

Testy obejmują odrzucanie wycofanych modeli we wszystkich rolach, zachowanie
obsługiwanych trybów discovery, odczyty LYWSDCGQ, szyfrowanie BLU i istniejące
usługi HomeKit. Pełny test tej wersji na fizycznej bramie i Gen3 pozostaje do wykonania.

## blu49 — eksperymentalny LYWSD03MMC bez tokenów

Nowy klient GATT odczytuje fabryczny LYWSD03MMC bez PIN-u, konta Xiaomi i bindkey.
Panel udostępnia typ **Xiaomi LYWSD03MMC (experimental)** w dwóch rolach H&T;
modal wykrywa urządzenie i automatycznie próbuje odczytu. Zapis następuje dopiero
po sprawdzeniu modelu i poprawnego pięciobajtowego pomiaru. Nieudana próba nie
zastępuje wcześniejszego przypisania.

Klient wyszukuje cechy po UUID zamiast stałych uchwytów ATT. Czyta model, opcjonalne
wersje firmware/sprzętu i temperaturę, wilgotność oraz napięcie. Obsługuje zapasową
cechę danych, timeouty, anulowanie i spóźnione odpowiedzi. Nie inicjuje SMP i nie
wysyła zapisów GATT. Bateria jest oznaczona jako szacunek z napięcia.

Odczyt co 5 minut, po jednym termometrze; ponowienia 1/2/5 minut, minimum 15 minut
świeżości. Ruch bramy odkłada lub przerywa odczyt, a krańcówki zachowują swój próg
świeżości. Istniejące usługi HomeKit i ich identyfikatory pozostają bez zmian.
Poprawiono także zapis nazw nieszyfrowanych termometrów w panelu.

Testy hosta obejmują rzeczywisty klient GATT z asynchronicznym adapterem BLE,
walidację danych, UUID, odrzucone zabezpieczenie, timeouty, anulowanie, odłożenie
podczas ruchu, zapisy dopiero po sukcesie oraz regresję BLU/HomeKit/panelu.
Próba fizyczna **B1.4 / 2.1.1_0159** pozostaje do wykonania; przykład źródłowy
potwierdza to firmware na B1.5. MCCGQ02HL i szyfrowane beacony z ręcznym kluczem
nadal nie są obsługiwane. Obrazy Plus1/Gen3 mają `hardware_tested: false`.

## blu50 — poprawka weryfikacji modelu Xiaomi

Blu49 na fizycznym B1.4 / 2.1.1_0159 wykrył urządzenie i doszedł do odczytu GATT,
lecz odrzucił odpowiedź modelu przy statusie `0x0`. Poprzedni komunikat nie zawierał
bajtów odpowiedzi, więc nie potwierdzono jej dokładnej zawartości. Testem odtworzono
odrzucenie poprawnej nazwy `LYWSD03MMC` z końcowym bajtem NUL.

Blu50 normalizuje końcowy NUL i dopełnienie w tekstach modelu oraz wersji firmware/
sprzętu. Nadal odrzuca inny model, dodatkowy sufiks i NUL wewnątrz tekstu. Klient
wykrywa cechy po UUID, sprawdza ich jednoznaczność, a potem odczytuje wartości zwykłym
ATT Read z dynamicznych uchwytów, zamiast Read By Type. Nie używa stałych adresów ATT,
PIN-u, bindkey, zapisów GATT ani SMP. Budżet transakcji wynosi 20 sekund, timeout
połączenia pozostaje 3 sekundy; odczyt nadal jest przerywany podczas ruchu bramy.

Przy błędzie diagnostyka podaje UUID, uchwyt, pierwotną długość i do 32 bajtów
odpowiedzi w hex. Nieudany odczyt obu cech danych pokazuje również błąd pierwotnej
cechy. Wcześniejsze przypisanie i pomiary nie są zastępowane nieudaną próbą.

Regresja obejmuje nazwy z NUL/dopełnieniem, błędne i zbyt długie teksty, zmienne
uchwyty oraz kolejność cech, brak i duplikaty UUID, odmowę odczytu, zapasową cechę,
timeout i spóźnione odpowiedzi po anulowaniu. Sprawdzono też wspólne testy BLU,
HomeKit i panelu. Odczyt T/H na fizycznym B1.4 / 2.1.1_0159 nadal czeka na próbę
blu50; obie paczki zachowują `hardware_tested: false`.

## Porządki w repozytorium — 2026-10-25

Usunięto nieużywane profile innych sterowników, ich sterowniki pomiarowe i dawny
kod mock dla Ubuntu, stare grafiki garażu, niepodłączony obraz dokumentacji oraz
odziedziczone narzędzia masowego flashowania i serwera aktualizacji upstream.
Zostały wspólne funkcje HomeKit, migracje, reset, czujniki i aktywny symulator.

Makefile i GitHub Actions obejmują tylko Plus 1 oraz 1 Gen3. Usunięto dawny bot
zamykania zgłoszeń i pomocnicze pliki starego CI; obecne zadania sprawdzają panel,
logikę urządzeń i budowanie obu paczek. Materiały koncepcyjne oraz cache pozostają
lokalne. Kopiowanie źródeł do katalogu kompilacji usuwa też wycofane pliki, aby
stare obrazy nie trafiały ponownie do firmware. Profile GPIO, schemat konfiguracji
i układ partycji obu modeli zostały zachowane.

To porządkowanie kodu na GitHubie, bez zastępowania opublikowanych paczek blu50.

## blu51 — czytelny czas od ostatniego pomiaru

Panel czujników BLU i Xiaomi pokazuje czas od ostatniej wiadomości jako
`21:43 temu`, `01:21:43 temu` lub `2 dni 12:32:33 temu`.
Dni są pomijane poniżej doby, a godziny przy wartości zero. Minuty i sekundy
są zawsze widoczne; jeden dzień ma zapis `1 dzień`.

Sprawdzono formatowanie, składnię JavaScript oraz regresję panelu. Wydanie nie
zmienia protokołów czujników. Test na fizycznych urządzeniach pozostaje do wykonania.

## blu52 — wyśrodkowane odczyty HT

- Dane temperatury i wilgotności są wyśrodkowane w obszarze nieba nad garażem.
- Wydanie zawiera czytelny czas od ostatniego pomiaru z blu51.
- Paczki dla Shelly Plus 1 oraz Shelly 1 Gen3; weryfikacja sprzętowa pozostaje do wykonania.
