#include "CountryData.h"

#include <QMap>

static CountryInfo country(QString iso, QString name, QString capital,
                           QString population, QString gdp, QString currency,
                           QString languages, QString continent, QString area,
                           QString centralBankName = QString(),
                           QString centralBankCity = QString(),
                           QString centralBankUrl = QString(),
                           double centralBankLat = 0.0,
                           double centralBankLon = 0.0)
{
    CountryInfo info;
    info.isoCode = iso;
    info.name = name;
    info.capital = capital;
    info.population = population;
    info.gdp = gdp;
    info.currency = currency;
    info.languages = languages;
    info.continent = continent;
    info.area = area;
    info.centralBankName = centralBankName;
    info.centralBankCity = centralBankCity;
    info.centralBankUrl = centralBankUrl;
    info.centralBankLat = centralBankLat;
    info.centralBankLon = centralBankLon;
    info.hasCentralBankLocation = !centralBankName.isEmpty();
    return info;
}

CountryInfo CountryData::byIso(const QString& isoCode, const QString& fallbackName)
{
    static const QMap<QString, CountryInfo> data = {
        {"AD", country("AD", "Andora", "Andora", "ok. 80 tys.", "ok. 3.7 mld USD", "EUR", "katalonski", "Europa", "468 km2")},
        {"AE", country("AE", "Zjednoczone Emiraty Arabskie", "Abu Zabi", "ok. 10 mln", "ok. 500 mld USD", "AED", "arabski", "Azja", "83 600 km2")},
        {"AR", country("AR", "Argentyna", "Buenos Aires", "ok. 46 mln", "ok. 640 mld USD", "ARS", "hiszpanski", "Ameryka Poludniowa", "2 780 400 km2")},
        {"AT", country("AT", "Austria", "Wieden", "ok. 9.1 mln", "ok. 520 mld USD", "EUR", "niemiecki", "Europa", "83 879 km2",
                       "Oesterreichische Nationalbank", "Wieden", "https://www.oenb.at", 48.2066, 16.3656)},
        {"AU", country("AU", "Australia", "Canberra", "ok. 27 mln", "ok. 1.7 bln USD", "AUD", "angielski", "Australia i Oceania", "7 692 024 km2",
                       "Reserve Bank of Australia", "Sydney", "https://www.rba.gov.au", -33.8675, 151.2094)},
        {"BE", country("BE", "Belgia", "Bruksela", "ok. 11.8 mln", "ok. 630 mld USD", "EUR", "niderlandzki, francuski, niemiecki", "Europa", "30 528 km2")},
        {"BR", country("BR", "Brazylia", "Brasilia", "ok. 203 mln", "ok. 2.2 bln USD", "BRL", "portugalski", "Ameryka Poludniowa", "8 515 767 km2",
                       "Banco Central do Brasil", "Brasilia", "https://www.bcb.gov.br", -15.8027, -47.8796)},
        {"CA", country("CA", "Kanada", "Ottawa", "ok. 40 mln", "ok. 2.1 bln USD", "CAD", "angielski, francuski", "Ameryka Polnocna", "9 984 670 km2",
                       "Bank of Canada", "Ottawa", "https://www.bankofcanada.ca", 45.4215, -75.6972)},
        {"CH", country("CH", "Szwajcaria", "Berno", "ok. 8.9 mln", "ok. 900 mld USD", "CHF", "niemiecki, francuski, wloski, romansz", "Europa", "41 285 km2",
                       "Swiss National Bank", "Berno / Zurych", "https://www.snb.ch", 46.9480, 7.4474)},
        {"CL", country("CL", "Chile", "Santiago", "ok. 20 mln", "ok. 330 mld USD", "CLP", "hiszpanski", "Ameryka Poludniowa", "756 102 km2")},
        {"CN", country("CN", "Chiny", "Pekin", "ok. 1.41 mld", "ok. 17.8 bln USD", "CNY", "mandarynski", "Azja", "9 596 961 km2",
                       "People's Bank of China", "Pekin", "https://www.pbc.gov.cn", 39.9042, 116.4074)},
        {"CZ", country("CZ", "Czechy", "Praga", "ok. 10.9 mln", "ok. 330 mld USD", "CZK", "czeski", "Europa", "78 871 km2",
                       "Czech National Bank", "Praga", "https://www.cnb.cz", 50.0875, 14.4213)},
        {"DE", country("DE", "Niemcy", "Berlin", "ok. 84 mln", "ok. 4.5 bln USD", "EUR", "niemiecki", "Europa", "357 588 km2",
                       "Deutsche Bundesbank", "Frankfurt nad Menem", "https://www.bundesbank.de", 50.1109, 8.6821)},
        {"DK", country("DK", "Dania", "Kopenhaga", "ok. 5.9 mln", "ok. 400 mld USD", "DKK", "dunski", "Europa", "42 933 km2",
                       "Danmarks Nationalbank", "Kopenhaga", "https://www.nationalbanken.dk", 55.6761, 12.5683)},
        {"EG", country("EG", "Egipt", "Kair", "ok. 112 mln", "ok. 400 mld USD", "EGP", "arabski", "Afryka", "1 010 408 km2")},
        {"ES", country("ES", "Hiszpania", "Madryt", "ok. 48 mln", "ok. 1.6 bln USD", "EUR", "hiszpanski", "Europa", "505 990 km2",
                       "Banco de Espana", "Madryt", "https://www.bde.es", 40.4168, -3.7038)},
        {"FI", country("FI", "Finlandia", "Helsinki", "ok. 5.6 mln", "ok. 300 mld USD", "EUR", "finski, szwedzki", "Europa", "338 455 km2",
                       "Bank of Finland", "Helsinki", "https://www.suomenpankki.fi", 60.1699, 24.9384)},
        {"FR", country("FR", "Francja", "Paryz", "ok. 68 mln", "ok. 3.0 bln USD", "EUR", "francuski", "Europa", "643 801 km2",
                       "Banque de France", "Paryz", "https://www.banque-france.fr", 48.8566, 2.3522)},
        {"GB", country("GB", "Wielka Brytania", "Londyn", "ok. 68 mln", "ok. 3.3 bln USD", "GBP", "angielski", "Europa", "243 610 km2",
                       "Bank of England", "Londyn", "https://www.bankofengland.co.uk", 51.5142, -0.0889)},
        {"GR", country("GR", "Grecja", "Ateny", "ok. 10.4 mln", "ok. 240 mld USD", "EUR", "grecki", "Europa", "131 957 km2")},
        {"HU", country("HU", "Wegry", "Budapeszt", "ok. 9.6 mln", "ok. 220 mld USD", "HUF", "wegierski", "Europa", "93 030 km2",
                       "Magyar Nemzeti Bank", "Budapeszt", "https://www.mnb.hu", 47.4979, 19.0402)},
        {"ID", country("ID", "Indonezja", "Dzakarta", "ok. 280 mln", "ok. 1.4 bln USD", "IDR", "indonezyjski", "Azja", "1 904 569 km2")},
        {"IE", country("IE", "Irlandia", "Dublin", "ok. 5.3 mln", "ok. 550 mld USD", "EUR", "irlandzki, angielski", "Europa", "70 273 km2",
                       "Central Bank of Ireland", "Dublin", "https://www.centralbank.ie", 53.3498, -6.2603)},
        {"IL", country("IL", "Izrael", "Jerozolima", "ok. 10 mln", "ok. 520 mld USD", "ILS", "hebrajski", "Azja", "22 145 km2")},
        {"IN", country("IN", "Indie", "Nowe Delhi", "ok. 1.43 mld", "ok. 3.6 bln USD", "INR", "hindi, angielski", "Azja", "3 287 263 km2",
                       "Reserve Bank of India", "Mumbaj", "https://www.rbi.org.in", 18.9388, 72.8354)},
        {"IT", country("IT", "Wlochy", "Rzym", "ok. 59 mln", "ok. 2.3 bln USD", "EUR", "wloski", "Europa", "301 340 km2",
                       "Banca d'Italia", "Rzym", "https://www.bancaditalia.it", 41.9028, 12.4964)},
        {"JP", country("JP", "Japonia", "Tokio", "ok. 124 mln", "ok. 4.2 bln USD", "JPY", "japonski", "Azja", "377 975 km2",
                       "Bank of Japan", "Tokio", "https://www.boj.or.jp", 35.6860, 139.7710)},
        {"KR", country("KR", "Korea Poludniowa", "Seul", "ok. 52 mln", "ok. 1.7 bln USD", "KRW", "koreanski", "Azja", "100 210 km2",
                       "Bank of Korea", "Seul", "https://www.bok.or.kr", 37.5665, 126.9780)},
        {"MX", country("MX", "Meksyk", "Meksyk", "ok. 129 mln", "ok. 1.8 bln USD", "MXN", "hiszpanski", "Ameryka Polnocna", "1 964 375 km2",
                       "Banco de Mexico", "Meksyk", "https://www.banxico.org.mx", 19.4326, -99.1332)},
        {"NL", country("NL", "Holandia", "Amsterdam", "ok. 18 mln", "ok. 1.1 bln USD", "EUR", "niderlandzki", "Europa", "41 543 km2",
                       "De Nederlandsche Bank", "Amsterdam", "https://www.dnb.nl", 52.3676, 4.9041)},
        {"NO", country("NO", "Norwegia", "Oslo", "ok. 5.6 mln", "ok. 500 mld USD", "NOK", "norweski", "Europa", "385 207 km2",
                       "Norges Bank", "Oslo", "https://www.norges-bank.no", 59.9139, 10.7522)},
        {"NZ", country("NZ", "Nowa Zelandia", "Wellington", "ok. 5.2 mln", "ok. 250 mld USD", "NZD", "angielski, maoryski", "Australia i Oceania", "268 021 km2")},
        {"PL", country("PL", "Polska", "Warszawa", "ok. 37.6 mln", "ok. 810 mld USD", "PLN", "polski", "Europa", "312 696 km2",
                       "Narodowy Bank Polski", "Warszawa", "https://www.nbp.pl", 52.2362, 21.0146)},
        {"PT", country("PT", "Portugalia", "Lizbona", "ok. 10.5 mln", "ok. 290 mld USD", "EUR", "portugalski", "Europa", "92 212 km2")},
        {"RO", country("RO", "Rumunia", "Bukareszt", "ok. 19 mln", "ok. 350 mld USD", "RON", "rumunski", "Europa", "238 397 km2",
                       "National Bank of Romania", "Bukareszt", "https://www.bnr.ro", 44.4268, 26.1025)},
        {"RU", country("RU", "Rosja", "Moskwa", "ok. 146 mln", "ok. 2.0 bln USD", "RUB", "rosyjski", "Europa/Azja", "17 098 246 km2",
                       "Bank of Russia", "Moskwa", "https://www.cbr.ru", 55.7558, 37.6173)},
        {"SA", country("SA", "Arabia Saudyjska", "Rijad", "ok. 36 mln", "ok. 1.1 bln USD", "SAR", "arabski", "Azja", "2 149 690 km2",
                       "Saudi Central Bank", "Rijad", "https://www.sama.gov.sa", 24.7136, 46.6753)},
        {"SE", country("SE", "Szwecja", "Sztokholm", "ok. 10.6 mln", "ok. 590 mld USD", "SEK", "szwedzki", "Europa", "450 295 km2",
                       "Sveriges Riksbank", "Sztokholm", "https://www.riksbank.se", 59.3293, 18.0686)},
        {"SG", country("SG", "Singapur", "Singapur", "ok. 5.9 mln", "ok. 500 mld USD", "SGD", "angielski, malajski, mandarynski, tamilski", "Azja", "734 km2",
                       "Monetary Authority of Singapore", "Singapur", "https://www.mas.gov.sg", 1.2839, 103.8519)},
        {"TR", country("TR", "Turcja", "Ankara", "ok. 85 mln", "ok. 1.1 bln USD", "TRY", "turecki", "Europa/Azja", "783 562 km2",
                       "Central Bank of the Republic of Turkiye", "Ankara", "https://www.tcmb.gov.tr", 39.9334, 32.8597)},
        {"UA", country("UA", "Ukraina", "Kijow", "ok. 37 mln", "ok. 180 mld USD", "UAH", "ukrainski", "Europa", "603 500 km2",
                       "National Bank of Ukraine", "Kijow", "https://bank.gov.ua", 50.4501, 30.5234)},
        {"US", country("US", "Stany Zjednoczone", "Waszyngton", "ok. 335 mln", "ok. 27 bln USD", "USD", "angielski", "Ameryka Polnocna", "9 833 517 km2",
                       "Federal Reserve", "Waszyngton", "https://www.federalreserve.gov", 38.8936, -77.0450)},
        {"ZA", country("ZA", "Republika Poludniowej Afryki", "Pretoria / Kapsztad / Bloemfontein", "ok. 62 mln", "ok. 380 mld USD", "ZAR", "11 jezykow urzedowych", "Afryka", "1 221 037 km2")},
    };

    const QString iso = isoCode.trimmed().toUpper();
    if (data.contains(iso))
        return data.value(iso);

    CountryInfo unknown;
    unknown.isoCode = iso;
    unknown.name = fallbackName.isEmpty() ? iso : fallbackName;
    unknown.capital = "-";
    unknown.population = "-";
    unknown.gdp = "-";
    unknown.currency = "-";
    unknown.languages = "-";
    unknown.continent = "-";
    unknown.area = "-";
    return unknown;
}
