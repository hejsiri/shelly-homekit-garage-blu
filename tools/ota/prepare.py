#!/usr/bin/env python3
"""Prepare a static OTA channel from the verified Shelly 1 Gen3 build."""
import hashlib
import html
import json
from pathlib import Path
import zipfile
from urllib.parse import quote

root = Path(__file__).resolve().parents[2]
source = root / 'build_Shelly1Gen3/fw.zip'
with zipfile.ZipFile(source) as archive:
    assert archive.testzip() is None, 'ZIP integrity failure'
    assert archive.namelist()[0] == 'manifest.json', 'Manifest must be first'
    assert archive.getinfo('manifest.json').compress_type == zipfile.ZIP_STORED
    manifest = json.loads(archive.read('manifest.json'))
assert manifest['name'] == 'S1G3', 'Wrong device image'
version = manifest['version']
assert version.startswith('2.16.1-blu'), 'Unexpected channel version'
assert all(c.isalnum() or c in '.-' for c in version)
base = 'https://hejsiri.pl/firmware/garage-blu/'
name = f'shelly-homekit-garage-blu-Shelly1Gen3-{version}.zip'
out = root / 'releases/hosting/garage-blu'
out.mkdir(parents=True, exist_ok=True)
payload = source.read_bytes()
(out / name).write_bytes(payload)
checksum = hashlib.sha256(payload).hexdigest()
(out / (name + '.sha256')).write_text(f'{checksum}  {name}\n')
channel = [['.*', {'version': version, 'rel_notes': base, 'urls': {'Shelly1Gen3': base + name}}]]
(out / 'update.json').write_text(json.dumps(channel, indent=2) + '\n')
(out / 'release.json').write_text(json.dumps({'model': 'Shelly1Gen3', 'version': version,
    'build_id': manifest.get('build_id'), 'url': base + name,
    'size': len(payload), 'sha256': checksum, 'hardware_tested': False}, indent=2) + '\n')
(out / 'AKTUALIZACJA.md').write_text((root / 'docs/AKTUALIZACJA.md').read_text())
(out / '.htaccess').write_text('''Options -Indexes
AddType application/zip .zip
AddType application/json .json
<IfModule mod_headers.c>
  Header always set Access-Control-Allow-Origin "*"
  Header always set Access-Control-Allow-Methods "GET, HEAD, OPTIONS"
  Header always set Access-Control-Allow-Headers "X-Model, X-Current-Version, X-Current-Build, X-Device-ID"
  Header always set Cache-Control "no-cache"
  Header always set X-Content-Type-Options "nosniff"
</IfModule>
<IfModule mod_rewrite.c>
  RewriteEngine On
  RewriteCond %{REQUEST_METHOD} =OPTIONS
  RewriteRule ^ - [R=204,L]
</IfModule>
''')
(out / 'index.html').write_text(f'''<!doctype html>
<html lang="pl"><meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1">
<title>Shelly HomeKit Garage BLU — firmware OTA</title>
<body style="font:18px/1.6 system-ui;max-width:850px;margin:48px auto;padding:0 24px">
<h1>Shelly HomeKit Garage BLU</h1>
<p>Shelly HomeKit Garage BLU to projekt firmware do sterowania bramą garażową przez Apple HomeKit
z wykorzystaniem Shelly 1 Gen3. Bazuje na otwartym projekcie
<a href="https://github.com/mongoose-os-apps/shelly-homekit">mongoose-os-apps/shelly-homekit</a>,
który udostępnia urządzenia Shelly w aplikacji Dom bez dodatkowego mostka.</p>
<p>W tej wersji dodano obsługę dwóch czujników <strong>Shelly BLU Door/Window</strong> przez Bluetooth:
jeden potwierdza pełne zamknięcie, drugi pełne otwarcie bramy. Rozszerzenie obejmuje parowanie
z panelu WWW bez aplikacji Shelly, szyfrowane komunikaty BTHome z ochroną przed ponownym użyciem
pakietów, blokowanie poleceń przy braku wiarygodnego stanu czujników oraz własny kanał aktualizacji OTA
na hejsiri.pl. HomeKit komunikuje się z modułem przez Wi-Fi.</p>
<h2>Idea projektu</h2>
<p>Chciałem stworzyć prosty sposób na dodanie bramy garażowej do Apple HomeKit
<strong>bez plątaniny kabli prowadzących do czujników</strong>. Bezprzewodowe czujniki BLU
mają ułatwić i przyspieszyć montaż, szczególnie przy istniejącym napędzie,
gdzie prowadzenie dodatkowego okablowania bywa kłopotliwe.</p>
<p>Kluczowe są <strong>dwa czujniki — po jednym dla każdego położenia krańcowego</strong>.
Pierwszy potwierdza całkowite zamknięcie bramy, a drugi całkowite otwarcie.
W rozwiązaniu z jednym czujnikiem zamknięcia jego zwolnienie oznacza tylko
„brama nie jest zamknięta”; nie potwierdza pełnego otwarcia. Brama może być
w ruchu albo zatrzymać się w połowie.</p>
<p>Zależało mi na tym, by potwierdzenie obu pozycji pochodziło z czujników,
niezależnie od tego, czy bramę uruchomiono z aplikacji Dom, przyciskiem na ścianie
czy zwykłym pilotem radiowym. Naprzemienne korzystanie z HomeKit i pilota
nie wymaga wtedy zgadywania położenia na podstawie ostatniej komendy z aplikacji.
<strong>Do takiego potwierdzania obu stanów projekt wymaga dwóch czujników.</strong>
Nie jest to ciągły pomiar położenia bramy między krańcówkami.</p>
<p>W założeniach projektu ważna jest również możliwość montażu czujników na zewnątrz.
Kierunkiem rozwoju jest obsługa
<a href="https://www.shelly.com/pl/products/shelly-blu-door-window-outdoor-zb-ivory">Shelly BLU Door/Window Outdoor ZB</a>,
który według producenta ma obudowę IP55 oraz łączność Bluetooth i Zigbee.
To model <strong>SBDW-302C</strong>, inny niż obecnie zaimplementowany SBDW-002C.
Wymaga osobnego dostosowania i testów; nie jest jeszcze obsługiwany w aktualnym wydaniu.</p>
<h2>Wspierane urządzenia</h2>
<div style="overflow-x:auto"><table style="border-collapse:collapse;width:100%" border="1" cellpadding="10">
<thead><tr><th scope="col">Urządzenie</th><th scope="col">Status Garage BLU</th></tr></thead>
<tbody>
<tr><td><strong>Shelly 1 Gen3 (S1G3)</strong></td><td>Jedyny model z przygotowaną paczką firmware. Kompilacja i testy lokalne przeszły; testy na sprzęcie pozostają do wykonania.</td></tr>
<tr><td><strong>Shelly BLU Door/Window (SBDW-002C)</strong></td><td>Dwa czujniki: pełne zamknięcie i pełne otwarcie bramy. Zachowują fabryczny firmware. Zaimplementowane parowanie i odbiór BLE wymagają testów na sprzęcie.</td></tr>
<tr><td>Shelly 1 Mini Gen3, 1PM Gen3, pozostałe Gen3, Plus, Gen1 i Pro</td><td>Brak paczek i obsługi Garage BLU dla tych modeli w tym wydaniu.</td></tr>
<tr><td>Shelly Gen4</td><td>Brak portu Garage BLU dla platformy Gen4.</td></tr>
<tr><td>Shelly BLU H&amp;T (temperatura i wilgotność), Shelly BLU Button, czujnik ruchu Shelly BLU Motion oraz czujnik wycieku Shelly BLU</td><td>Wkrótce — planowane wsparcie. Aktualne wydanie jeszcze ich nie obsługuje.</td></tr>
<tr><td>Inne czujniki BLE</td><td>Obsługa nie została dodana.</td></tr>
</tbody></table></div>
<p>Sama obecność Bluetooth nie oznacza zgodności. Kod innych urządzeń z bazowego Shelly-HomeKit
został zachowany, ale nie oznacza to obsługi rozszerzenia Garage BLU na tych modelach.</p>
<h2>Czujniki BLU — bez zmiany firmware</h2>
<p><strong>Czujników Shelly BLU Door/Window nie trzeba aktualizować ani zmieniać ich firmware.</strong>
Mają działać na oryginalnym firmware Shelly — wystarczy sparować je z modułem
przez panel WWW Garage BLU. Naszą paczkę OTA instaluje się wyłącznie na sterowniku
<strong>Shelly 1 Gen3</strong>, a nie na czujnikach. Parowanie i działanie na sprzęcie wymagają jeszcze weryfikacji.</p>
<h2>Wkrótce</h2>
<p>Planuję dodać obsługę <strong>Shelly BLU H&amp;T</strong> (temperatura i wilgotność),
przycisku <strong>Shelly BLU Button</strong>, czujnika ruchu <strong>Shelly BLU Motion</strong> oraz czujnika wycieku Shelly BLU. Te urządzenia nie są jeszcze obsługiwane
w aktualnym wydaniu firmware.</p>
<p>Firmware <strong>{html.escape(version)}</strong> wyłącznie dla <strong>Shelly 1 Gen3</strong> (S1G3).
Nie instaluj na Mini, Gen4 ani innych modelach.</p>
<p>Wersja eksperymentalna: kompilacja i testy lokalne sprawdzone; parowanie BLE i działanie na fizycznym urządzeniu wymagają weryfikacji.</p>
<p><a href="{name}">Pobierz paczkę OTA ({len(payload):,} bajtów)</a> · <a href="{name}.sha256">SHA-256</a></p>
<p>Adres paczki do pola aktualizacji przez URL:</p>
<pre style="white-space:pre-wrap;overflow-wrap:anywhere">{base}{name}</pre>
<h2>Instrukcja instalacji i aktualizacji</h2>
<p><a href="AKTUALIZACJA.md">Pełna instrukcja (PL)</a></p>
<h3>Przed aktualizacją</h3>
<p>Sprawdź model i zapisz ustawienia oraz aktualną wersję. Zatrzymaj bramę, zapewnij stabilne zasilanie i Wi-Fi.
Nie mamy potwierdzenia instalacji tej paczki na fizycznym urządzeniu ani przejścia z fabrycznego 2.1.0.</p>
<h3>Pierwsza instalacja z fabrycznego Gen3</h3>
<ol>
<li>Otwórz lokalny panel Shelly pod <code>http://ADRES_IP/</code> i zaloguj się.
Jeżeli panel proponuje oficjalne stabilne wydanie fabryczne, zaktualizuj je i poczekaj na restart.
Nie obniżaj wersji według starych instrukcji.</li>
<li>W sekcji aktualizacji wskaż nasz ZIP lub jego URL, jeżeli panel udostępnia instalację własnego firmware.
Alternatywnie użyj fabrycznego RPC <code>Shelly.Update</code> z parametrem <code>url</code>.
Poniższy adres rozpoczyna aktualizację — najpierw zastąp <code>ADRES_IP</code> adresem urządzenia:</li>
</ol>
<pre style="white-space:pre-wrap;overflow-wrap:anywhere">http://ADRES_IP/rpc/Shelly.Update?url={quote(base + name, safe='')}</pre>
<p>Przy włączonej autoryzacji użyj uwierzytelnionego panelu lub klienta RPC. Nie wyłączaj hasła.
Adres <code>/ota?url=...</code> z instrukcji Gen1 nie jest instrukcją RPC dla Gen3.
Konwersja nie gwarantuje zachowania ustawień ani funkcji Shelly Cloud.</p>
<ol start="3"><li>Nie odłączaj zasilania. Poczekaj na restart, otwórz panel ponownie i sprawdź wersję.
Jeżeli adres się zmienił, znajdź Shelly w routerze.</li>
<li>Ustaw hasło administratora, wybierz <strong>Garage Door Opener</strong>, skonfiguruj sterowanie i sparuj
<strong>Closing sensor</strong> oraz <strong>Opening sensor</strong>. Przetestuj oba położenia i przekaźnik pod nadzorem.</li></ol>
<p>Parametr URL opisuje <a href="https://shelly-api-docs.shelly.cloud/gen2/ComponentsAndServices/Shelly/#shellyupdate">dokumentacja Shelly.Update</a>.
Nie potwierdza to zgodności naszej paczki z każdym wydaniem fabrycznym i układem partycji.</p>
<h3>Z oryginalnego Shelly-HomeKit lub wcześniejszego lokalnego buildu</h3>
<p>Pobierz ZIP bez rozpakowywania. W panelu rozwiń <strong>Firmware</strong>, wybierz
<strong>Update from file</strong>, wskaż ZIP i kliknij <strong>Upload</strong>.
Poczekaj na restart i sprawdź wersję. Paczki oryginalnego projektu z rojer.me nie zawierają naszych zmian BLU.</p>
<h3>Kolejne wydania Garage BLU</h3>
<p>W panelu: <strong>Firmware → Update → Check</strong>. Jeśli dostępna jest nowsza wersja,
przeczytaj informacje o wydaniu i kliknij <strong>Install</strong>. Panel korzysta z naszego kanału OTA.
Po restarcie sprawdź wersję i oba czujniki. <strong>Up to date</strong> oznacza brak nowszego wydania.
Alternatywnie użyj <strong>Update from file → Upload</strong>.</p>
<p>Nie wybieraj <strong>Revert to Stock</strong> podczas zwykłej aktualizacji.
Nie wykonuj resetu fabrycznego — kasuje powiązania i klucze BLU. Zachowaj PIN-y czujników.
Samo opublikowanie paczki nie uruchamia aktualizacji urządzenia.</p>
<h3>Błąd aktualizacji</h3>
<p>Zapisz błąd, model i wersję. Przy błędzie pobierania spróbuj pobranego ZIP w panelu Shelly-HomeKit.
Przy błędzie modelu, manifestu lub partycji nie wymuszaj zapisu i nie wgrywaj obrazu innego modelu.
Po nieudanym restarcie sprawdź router i sieć konfiguracyjną urządzenia.</p>
<p><a href="update.json">Kanał aktualizacji</a> · <a href="release.json">Metadane wydania</a> ·
<a href="https://github.com/hejsiri/shelly-homekit-garage-blu">Repozytorium</a></p>
</body></html>
''')
print(json.dumps({'directory': str(out), 'url': base + name, 'sha256': checksum, 'size': len(payload)}, indent=2))
