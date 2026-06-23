#!/usr/bin/env python3
"""
seed_exchanges.py — generuje exchanges_data.json z danymi giełd świata.
Koordynaty oparte na badaniach adresów giełd (maj 2026).
Uruchomienie: python seed_exchanges.py > ../exchanges_data.json
"""
import json, sys

# (kraj, region, nazwa, website, typ, miasto, lat, lon, adres, rok_zal, l_spolek)
EXCHANGES = [
    ("Albania", "Europa", "Albanian Securities Exchange (ALSE)", "https://alse.al",
     "Krajowa giełda papierów wartościowych", "Tirana", 41.3289, 19.8178,
     "Rruga e Kavajës, Tirana, Albania", 1996, 2),

    ("Algieria", "Afryka", "Bourse d'Alger", "https://www.sgbv.dz",
     "Krajowa giełda papierów wartościowych", "Algier", 36.7539, 3.0588,
     "Rue Tanger, Algier, Algieria", 1997, 6),

    ("Angola", "Afryka", "BODIVA - Bolsa de Dívida e Valores de Angola", "https://www.bodiva.ao",
     "Krajowa giełda papierów wartościowych", "Luanda", -8.8383, 13.2344,
     "Rua Rainha Ginga, Luanda, Angola", 2006, 8),

    ("Antigua i Barbuda", "Ameryka Północna", "Eastern Caribbean Securities Exchange (ECSE)", "https://www.ecseonline.com",
     "Regionalna giełda dla państw ECCU/OECS", "Basseterre, Saint Kitts", 17.3026, -62.7177,
     "Bird Rock Business Park, Basseterre, Saint Kitts and Nevis", 1983, 31),

    ("Argentyna", "Ameryka Południowa", "Bolsas y Mercados Argentinos (BYMA)", "https://www.byma.com.ar",
     "Krajowa giełda papierów wartościowych", "Buenos Aires", -34.5976, -58.3720,
     "Av. Eduardo Madero 900, Buenos Aires, Argentyna", 2017, 100),

    ("Argentyna", "Ameryka Południowa", "Bolsa de Comercio de Buenos Aires", "https://www.labolsa.com.ar",
     "Giełda / instytucja rynku kapitałowego", "Buenos Aires", -34.6035, -58.3770,
     "Sarmiento 299, Buenos Aires, Argentyna", 1854, 100),

    ("Armenia", "Azja", "Armenia Securities Exchange (AMX)", "https://amx.am",
     "Krajowa giełda papierów wartościowych", "Erywań", 40.1872, 44.5152,
     "22 Khanjyan Street, Erywań, Armenia", 2001, 5),

    ("Australia", "Oceania", "Australian Securities Exchange (ASX)", "https://www.asx.com.au",
     "Krajowa giełda papierów wartościowych", "Sydney", -33.8651, 151.2099,
     "20 Bridge Street, Sydney NSW 2000, Australia", 1987, 2200),

    ("Australia", "Oceania", "Cboe Australia", "https://www.cboe.com.au",
     "Alternatywna giełda papierów wartościowych", "Sydney", -33.8688, 151.2093,
     "10 Spring Street, Sydney NSW 2000, Australia", 2011, 0),

    ("Austria", "Europa", "Vienna Stock Exchange / Wiener Börse", "https://www.wienerborse.at",
     "Krajowa giełda papierów wartościowych", "Wiedeń", 48.2136, 16.3691,
     "Wallnerstraße 8, 1010 Wiedeń, Austria", 1771, 65),

    ("Azerbejdżan", "Azja", "Baku Stock Exchange", "https://www.bfb.az",
     "Krajowa giełda papierów wartościowych", "Baku", 40.3784, 49.8469,
     "Nizami Street 91, Baku, Azerbejdżan", 2000, 20),

    ("Bahamy", "Ameryka Północna", "Bahamas International Securities Exchange (BISX)", "https://www.bisxbahamas.com",
     "Krajowa giełda papierów wartościowych", "Nassau", 25.0480, -77.3554,
     "Charlotte House, Charlotte Street, Nassau, Bahamy", 2000, 20),

    ("Bahrajn", "Azja", "Bahrain Bourse", "https://www.bahrainbourse.com",
     "Krajowa giełda papierów wartościowych", "Manama", 26.2285, 50.5860,
     "Bahrain Financial Harbour, Manama, Bahrajn", 1987, 45),

    ("Bangladesz", "Azja", "Dhaka Stock Exchange", "https://www.dsebd.org",
     "Krajowa giełda papierów wartościowych", "Dhaka", 23.7278, 90.3998,
     "9/F Motijheel C/A, Dhaka 1000, Bangladesz", 1954, 350),

    ("Bangladesz", "Azja", "Chittagong Stock Exchange", "https://www.cse.com.bd",
     "Krajowa giełda papierów wartościowych", "Chittagong", 22.3282, 91.8153,
     "CSE Building, 1080 Sheikh Mujib Road, Agrabad, Chittagong, Bangladesz", 1995, 330),

    ("Barbados", "Ameryka Północna", "Barbados Stock Exchange", "https://bse.com.bb",
     "Krajowa giełda papierów wartościowych", "Bridgetown", 13.0961, -59.6145,
     "8th Avenue Belleville, Bridgetown, Barbados", 1987, 25),

    ("Białoruś", "Europa", "Belarusian Currency and Stock Exchange", "https://www.bcse.by",
     "Krajowa giełda papierów wartościowych", "Mińsk", 53.9045, 27.5615,
     "Nezalezhnasci Avenue 5, Mińsk, Białoruś", 1998, 80),

    ("Belgia", "Europa", "Euronext Brussels", "https://live.euronext.com/en/markets/brussels",
     "Rynek giełdowy Euronext w Belgii", "Bruksela", 50.8477, 4.3520,
     "Palais de la Bourse, Place de la Bourse, 1000 Bruksela, Belgia", 1801, 140),

    ("Benin", "Afryka", "Bourse Régionale des Valeurs Mobilières (BRVM)", "https://www.brvm.org",
     "Regionalna giełda dla państw UEMOA", "Abidżan, Côte d'Ivoire", 5.3197, -4.0139,
     "Rue Joseph Anoma, Abidżan, Côte d'Ivoire", 1998, 46),

    ("Bhutan", "Azja", "Royal Securities Exchange of Bhutan", "https://www.rsebl.org.bt",
     "Krajowa giełda papierów wartościowych", "Thimphu", 27.4661, 89.6419,
     "Norzin Lam, Thimphu, Bhutan", 1993, 21),

    ("Boliwia", "Ameryka Południowa", "Bolsa Boliviana de Valores", "https://www.bbv.com.bo",
     "Krajowa giełda papierów wartościowych", "La Paz", -16.4989, -68.1187,
     "Calle Montevideo 142, La Paz, Boliwia", 1979, 20),

    ("Bośnia i Hercegowina", "Europa", "Sarajevo Stock Exchange", "https://www.sase.ba",
     "Krajowa giełda papierów wartościowych", "Sarajewo", 43.8563, 18.4131,
     "Džidžikovac 2, 71000 Sarajewo, Bośnia i Hercegowina", 2002, 50),

    ("Bośnia i Hercegowina", "Europa", "Banja Luka Stock Exchange", "https://www.blberza.com",
     "Krajowa giełda papierów wartościowych", "Banja Luka", 44.7722, 17.1909,
     "Koste Jarića 3, 78000 Banja Luka, Bośnia i Hercegowina", 2001, 80),

    ("Botswana", "Afryka", "Botswana Stock Exchange", "https://www.bse.co.bw",
     "Krajowa giełda papierów wartościowych", "Gaborone", -24.6541, 25.9087,
     "Finance House, Khama Crescent, Gaborone, Botswana", 1989, 40),

    ("Brazylia", "Ameryka Południowa", "B3 - Brasil, Bolsa, Balcão", "https://www.b3.com.br",
     "Krajowa giełda papierów wartościowych i instrumentów pochodnych", "São Paulo", -23.5489, -46.6338,
     "Praça Antonio Prado 48, São Paulo, SP 01010-901, Brazylia", 2008, 530),

    ("Bułgaria", "Europa", "Bulgarian Stock Exchange", "https://www.bse-sofia.bg",
     "Krajowa giełda papierów wartościowych", "Sofia", 42.6977, 23.3219,
     "1 Macedonia Square, 1040 Sofia, Bułgaria", 1991, 130),

    ("Burkina Faso", "Afryka", "Bourse Régionale des Valeurs Mobilières (BRVM)", "https://www.brvm.org",
     "Regionalna giełda dla państw UEMOA", "Abidżan, Côte d'Ivoire", 5.3197, -4.0139,
     "Rue Joseph Anoma, Abidżan, Côte d'Ivoire", 1998, 46),

    ("Cabo Verde", "Afryka", "Bolsa de Valores de Cabo Verde", "https://www.bvc.cv",
     "Krajowa giełda papierów wartościowych", "Praia", 14.9335, -23.5133,
     "Avenida Amilcar Cabral, Praia, Cabo Verde", 2005, 5),

    ("Kambodża", "Azja", "Cambodia Securities Exchange", "https://www.csx.com.kh",
     "Krajowa giełda papierów wartościowych", "Phnom Penh", 11.5714, 104.9191,
     "No. 217, Norodom Blvd, Phnom Penh, Kambodża", 2011, 7),

    ("Kamerun", "Afryka", "Bourse des Valeurs Mobilières de l'Afrique Centrale (BVMAC)", "https://www.bvm-ac.com",
     "Regionalna giełda dla państw CEMAC", "Libreville, Gabon", 0.3924, 9.4536,
     "Quartier Glass, Libreville, Gabon", 2003, 7),

    ("Kanada", "Ameryka Północna", "Toronto Stock Exchange / TSX Venture Exchange (TMX Group)", "https://www.tsx.com",
     "Krajowa giełda papierów wartościowych", "Toronto", 43.6458, -79.3813,
     "100 King Street West, Toronto, ON M5X 1J2, Kanada", 1852, 1900),

    ("Kanada", "Ameryka Północna", "Canadian Securities Exchange (CSE)", "https://thecse.com",
     "Krajowa giełda papierów wartościowych", "Toronto", 43.6496, -79.3811,
     "390 Bay Street, Toronto, ON M5H 2Y2, Kanada", 2004, 800),

    ("Kanada", "Ameryka Północna", "Cboe Canada", "https://www.cboe.ca",
     "Krajowa giełda papierów wartościowych", "Toronto", 43.6496, -79.3811,
     "390 Bay Street, Toronto, ON M5H 2Y2, Kanada", 2017, 0),

    ("Republika Środkowoafrykańska", "Afryka", "Bourse des Valeurs Mobilières de l'Afrique Centrale (BVMAC)", "https://www.bvm-ac.com",
     "Regionalna giełda dla państw CEMAC", "Libreville, Gabon", 0.3924, 9.4536,
     "Quartier Glass, Libreville, Gabon", 2003, 7),

    ("Czad", "Afryka", "Bourse des Valeurs Mobilières de l'Afrique Centrale (BVMAC)", "https://www.bvm-ac.com",
     "Regionalna giełda dla państw CEMAC", "Libreville, Gabon", 0.3924, 9.4536,
     "Quartier Glass, Libreville, Gabon", 2003, 7),

    ("Chile", "Ameryka Południowa", "Bolsa de Santiago", "https://www.bolsadesantiago.com",
     "Krajowa giełda papierów wartościowych", "Santiago", -33.4372, -70.6475,
     "La Bolsa 64, Santiago, Chile", 1893, 220),

    ("Chiny", "Azja", "Shanghai Stock Exchange", "https://www.sse.com.cn",
     "Krajowa giełda papierów wartościowych", "Szanghaj", 31.2307, 121.4904,
     "528 Pudong Road South, Pudong, Szanghaj, Chiny", 1990, 1700),

    ("Chiny", "Azja", "Shenzhen Stock Exchange", "https://www.szse.cn",
     "Krajowa giełda papierów wartościowych", "Shenzhen", 22.5290, 113.9402,
     "2012 Shennan Avenue, Shenzhen, Chiny", 1990, 2700),

    ("Chiny", "Azja", "Beijing Stock Exchange", "https://www.bse.cn",
     "Krajowa giełda papierów wartościowych", "Pekin", 39.9042, 116.4074,
     "2 Jinrongda Street, Xicheng, Pekin, Chiny", 2021, 250),

    ("Chiny", "Azja", "Hong Kong Exchanges and Clearing (HKEX)", "https://www.hkex.com.hk",
     "Krajowa giełda papierów wartościowych", "Hongkong", 22.2862, 114.1537,
     "8 Connaught Place, Central, Hongkong", 1891, 2600),

    ("Kolumbia", "Ameryka Południowa", "Bolsa de Valores de Colombia (bvc)", "https://www.bvc.com.co",
     "Krajowa giełda papierów wartościowych", "Bogota", 4.6553, -74.0919,
     "Carrera 7 # 71-21, Bogota, Kolumbia", 1928, 80),

    ("Kolumbia", "Ameryka Południowa", "nuam exchange", "https://www.nuamx.com",
     "Giełda / instytucja rynku kapitałowego", "Bogota", 4.6553, -74.0919,
     "Carrera 7 # 71-21, Bogota, Kolumbia", 2022, 130),

    ("Kongo", "Afryka", "Bourse des Valeurs Mobilières de l'Afrique Centrale (BVMAC)", "https://www.bvm-ac.com",
     "Regionalna giełda dla państw CEMAC", "Libreville, Gabon", 0.3924, 9.4536,
     "Quartier Glass, Libreville, Gabon", 2003, 7),

    ("Kostaryka", "Ameryka Północna", "Bolsa Nacional de Valores", "https://www.bolsacr.com",
     "Krajowa giełda papierów wartościowych", "San José", 9.9366, -84.0870,
     "Calle Central, San José, Kostaryka", 1976, 40),

    ("Wybrzeże Kości Słoniowej", "Afryka", "Bourse Régionale des Valeurs Mobilières (BRVM)", "https://www.brvm.org",
     "Regionalna giełda dla państw UEMOA", "Abidżan", 5.3197, -4.0139,
     "Rue Joseph Anoma, Abidżan, Wybrzeże Kości Słoniowej", 1998, 46),

    ("Chorwacja", "Europa", "Zagreb Stock Exchange", "https://zse.hr",
     "Krajowa giełda papierów wartościowych", "Zagrzeb", 45.8150, 15.9780,
     "Ivana Lučića 2a, 10000 Zagrzeb, Chorwacja", 1991, 130),

    ("Cypr", "Europa", "Cyprus Stock Exchange", "https://www.cse.com.cy",
     "Krajowa giełda papierów wartościowych", "Nikozja", 35.1856, 33.3823,
     "71-73 Lordou Vironos, 1096 Nikozja, Cypr", 1996, 70),

    ("Czechy", "Europa", "Prague Stock Exchange", "https://www.pse.cz",
     "Krajowa giełda papierów wartościowych", "Praga", 50.0919, 14.4043,
     "Rybná 14, 110 05 Praha 1, Czechy", 1993, 20),

    ("Czechy", "Europa", "RM-SYSTEM Czech Stock Exchange", "https://www.rmsystem.cz",
     "Alternatywna giełda papierów wartościowych", "Praga", 50.0880, 14.4120,
     "Praha, Czechy", 1993, 30),

    ("Dania", "Europa", "Nasdaq Copenhagen", "https://www.nasdaqomxnordic.com",
     "Krajowa giełda papierów wartościowych", "Kopenhaga", 55.6814, 12.5780,
     "Nikolaj Plads 6, 1067 Kopenhaga, Dania", 1808, 180),

    ("Dominika", "Ameryka Północna", "Eastern Caribbean Securities Exchange (ECSE)", "https://www.ecseonline.com",
     "Regionalna giełda dla państw ECCU/OECS", "Basseterre, Saint Kitts", 17.3026, -62.7177,
     "Bird Rock Business Park, Basseterre, Saint Kitts and Nevis", 1983, 31),

    ("Dominikana", "Ameryka Północna", "Bolsa y Mercados de Valores de la República Dominicana", "https://bvrd.com.do",
     "Krajowa giełda papierów wartościowych", "Santo Domingo", 18.4667, -69.9281,
     "Avenida 27 de Febrero, Santo Domingo, Dominikana", 1988, 15),

    ("Ekwador", "Ameryka Południowa", "Bolsa de Valores de Quito", "https://www.bolsadequito.com",
     "Krajowa giełda papierów wartościowych", "Quito", -0.1807, -78.4678,
     "Av. Río Amazonas 540, Quito, Ekwador", 1969, 50),

    ("Ekwador", "Ameryka Południowa", "Bolsa de Valores de Guayaquil", "https://www.bolsadevaloresguayaquil.com",
     "Krajowa giełda papierów wartościowych", "Guayaquil", -2.1900, -79.8875,
     "Pichincha 323, Guayaquil, Ekwador", 1969, 50),

    ("Egipt", "Afryka", "The Egyptian Exchange (EGX)", "https://www.egx.com.eg",
     "Krajowa giełda papierów wartościowych", "Kair", 30.0579, 31.2397,
     "4 El-Sherifein Street, Kair, Egipt", 1883, 260),

    ("Salwador", "Ameryka Północna", "Bolsa de Valores de El Salvador", "https://www.bolsadevalores.com.sv",
     "Krajowa giełda papierów wartościowych", "San Salvador", 13.6987, -89.2137,
     "Alameda Roosevelt, San Salvador, Salwador", 1992, 20),

    ("Gwinea Równikowa", "Afryka", "Bourse des Valeurs Mobilières de l'Afrique Centrale (BVMAC)", "https://www.bvm-ac.com",
     "Regionalna giełda dla państw CEMAC", "Libreville, Gabon", 0.3924, 9.4536,
     "Quartier Glass, Libreville, Gabon", 2003, 7),

    ("Estonia", "Europa", "Nasdaq Tallinn", "https://nasdaqbaltic.com",
     "Krajowa giełda papierów wartościowych", "Tallin", 59.4370, 24.7536,
     "Kaasiku 14, 10119 Tallin, Estonia", 1996, 18),

    ("Eswatini", "Afryka", "Eswatini Stock Exchange", "https://www.ese.co.sz",
     "Krajowa giełda papierów wartościowych", "Mbabane", -26.3054, 31.1367,
     "Swazi Plaza, Mbabane, Eswatini", 1990, 6),

    ("Etiopia", "Afryka", "Ethiopian Securities Exchange (ESX)", "https://esx.et",
     "Krajowa giełda papierów wartościowych", "Addis Abeba", 9.0320, 38.7469,
     "Addis Abeba, Etiopia", 2023, 2),

    ("Fidżi", "Oceania", "South Pacific Stock Exchange", "https://www.spse.com.fj",
     "Krajowa giełda papierów wartościowych", "Suva", -18.1416, 178.4247,
     "Level 2, Provident Plaza 1, Suva, Fidżi", 1979, 18),

    ("Finlandia", "Europa", "Nasdaq Helsinki", "https://www.nasdaqomxnordic.com",
     "Krajowa giełda papierów wartościowych", "Helsinki", 60.1699, 24.9384,
     "Fabianinkatu 14, 00100 Helsinki, Finlandia", 1912, 150),

    ("Francja", "Europa", "Euronext Paris", "https://live.euronext.com/en/markets/paris",
     "Krajowa giełda papierów wartościowych", "Paryż (La Défense)", 48.8921, 2.2372,
     "14 Place des Reflets, 92054 Paris La Défense, Francja", 1724, 740),

    ("Gabon", "Afryka", "Bourse des Valeurs Mobilières de l'Afrique Centrale (BVMAC)", "https://www.bvm-ac.com",
     "Regionalna giełda dla państw CEMAC", "Libreville", 0.3924, 9.4536,
     "Quartier Glass, Libreville, Gabon", 2003, 7),

    ("Gruzja", "Azja", "Georgian Stock Exchange", "https://gse.ge",
     "Krajowa giełda papierów wartościowych", "Tbilisi", 41.6970, 44.8015,
     "74a Chavchavadze Avenue, Tbilisi, Gruzja", 1999, 15),

    ("Niemcy", "Europa", "Frankfurt Stock Exchange / Börse Frankfurt", "https://www.boerse-frankfurt.de",
     "Krajowa giełda papierów wartościowych", "Frankfurt nad Menem", 50.1136, 8.6801,
     "Börseplatz 4, 60313 Frankfurt am Main, Niemcy", 1585, 1000),

    ("Niemcy", "Europa", "Xetra", "https://www.xetra.com",
     "Elektroniczna platforma transakcyjna Deutsche Börse", "Frankfurt nad Menem", 50.1136, 8.6801,
     "Börseplatz 4, 60313 Frankfurt am Main, Niemcy", 1997, 1100),

    ("Niemcy", "Europa", "Boerse Stuttgart", "https://www.boerse-stuttgart.de",
     "Giełda regionalna", "Stuttgart", 48.7758, 9.1829,
     "Börsenstraße 4, 70174 Stuttgart, Niemcy", 1861, 0),

    ("Niemcy", "Europa", "Börse Berlin", "https://www.boerse-berlin.com",
     "Giełda regionalna", "Berlin", 52.5200, 13.4050,
     "Fasanenstraße 85, 10623 Berlin, Niemcy", 1685, 0),

    ("Niemcy", "Europa", "Börse Düsseldorf", "https://www.boerse-duesseldorf.de",
     "Giełda regionalna", "Düsseldorf", 51.2217, 6.7762,
     "Ernst-Schneider-Platz 1, 40212 Düsseldorf, Niemcy", 1875, 0),

    ("Niemcy", "Europa", "Börse Hamburg / Börse Hannover", "https://www.boersenag.de",
     "Giełda regionalna", "Hamburg", 53.5488, 9.9872,
     "Schauenburgerstraße 49, 20095 Hamburg, Niemcy", 1558, 0),

    ("Niemcy", "Europa", "Börse München", "https://www.boerse-muenchen.de",
     "Giełda regionalna", "Monachium", 48.1351, 11.5820,
     "Karolinenplatz 6, 80333 Monachium, Niemcy", 1830, 0),

    ("Niemcy", "Europa", "Tradegate Exchange", "https://www.tradegate.de",
     "Alternatywna platforma handlowa", "Berlin", 52.5200, 13.4050,
     "Berlin, Niemcy", 2009, 0),

    ("Ghana", "Afryka", "Ghana Stock Exchange", "https://gse.com.gh",
     "Krajowa giełda papierów wartościowych", "Akra", 5.5560, -0.2169,
     "Cedi House, Liberia Road, Akra, Ghana", 1990, 40),

    ("Grecja", "Europa", "Athens Stock Exchange", "https://www.athexgroup.gr",
     "Krajowa giełda papierów wartościowych", "Ateny", 37.9838, 23.7275,
     "110 Athinon Avenue, Ateny, Grecja", 1876, 130),

    ("Grenada", "Ameryka Północna", "Eastern Caribbean Securities Exchange (ECSE)", "https://www.ecseonline.com",
     "Regionalna giełda dla państw ECCU/OECS", "Basseterre, Saint Kitts", 17.3026, -62.7177,
     "Bird Rock Business Park, Basseterre, Saint Kitts and Nevis", 1983, 31),

    ("Gwatemala", "Ameryka Północna", "Bolsa de Valores Nacional", "https://www.bvnsa.com.gt",
     "Krajowa giełda papierów wartościowych", "Guatemala City", 14.6417, -90.5150,
     "7a Av. 5-10, Zona 4, Guatemala City, Gwatemala", 1987, 15),

    ("Gwinea Bissau", "Afryka", "Bourse Régionale des Valeurs Mobilières (BRVM)", "https://www.brvm.org",
     "Regionalna giełda dla państw UEMOA", "Abidżan, Côte d'Ivoire", 5.3197, -4.0139,
     "Rue Joseph Anoma, Abidżan, Côte d'Ivoire", 1998, 46),

    ("Gujana", "Ameryka Południowa", "Guyana Association of Securities Companies and Intermediaries (GASCI)", "https://www.gasci.com",
     "Krajowa giełda papierów wartościowych", "Georgetown", 6.8013, -58.1551,
     "1 Lots 3 & 4 South Road, Georgetown, Gujana", 2003, 20),

    ("Honduras", "Ameryka Północna", "Bolsa Centroamericana de Valores", "https://www.bcv.hn",
     "Krajowa giełda papierów wartościowych", "San Pedro Sula", 15.5000, -88.0167,
     "Edificio Palmira, San Pedro Sula, Honduras", 1990, 20),

    ("Węgry", "Europa", "Budapest Stock Exchange", "https://www.bet.hu",
     "Krajowa giełda papierów wartościowych", "Budapeszt", 47.4945, 19.0566,
     "Andrássy út 2, 1062 Budapeszt, Węgry", 1864, 75),

    ("Islandia", "Europa", "Nasdaq Iceland", "https://www.nasdaqomxnordic.com",
     "Krajowa giełda papierów wartościowych", "Reykjavik", 64.1466, -21.9426,
     "Laugavegur 182, 105 Reykjavik, Islandia", 1985, 25),

    ("Indie", "Azja", "National Stock Exchange of India (NSE)", "https://www.nseindia.com",
     "Krajowa giełda papierów wartościowych", "Mumbaj (BKC)", 19.0595, 72.8654,
     "Exchange Plaza, Bandra Kurla Complex, Mumbaj 400051, Indie", 1992, 2100),

    ("Indie", "Azja", "BSE India", "https://www.bseindia.com",
     "Krajowa giełda papierów wartościowych", "Mumbaj (Dalal Street)", 18.9285, 72.8347,
     "Phiroze Jeejeebhoy Towers, Dalal Street, Mumbaj 400001, Indie", 1875, 5400),

    ("Indie", "Azja", "Metropolitan Stock Exchange of India (MSE)", "https://www.msei.in",
     "Krajowa giełda papierów wartościowych", "Mumbaj", 19.0760, 72.8777,
     "Vibgyor Towers, Bandra Kurla Complex, Mumbaj, Indie", 2008, 300),

    ("Indonezja", "Azja", "Indonesia Stock Exchange", "https://www.idx.co.id",
     "Krajowa giełda papierów wartościowych", "Dżakarta", -6.1944, 106.8229,
     "Jl. Jend. Sudirman Kav. 52-53, Dżakarta 12190, Indonezja", 1912, 900),

    ("Iran", "Azja", "Tehran Stock Exchange", "https://www.tse.ir",
     "Krajowa giełda papierów wartościowych", "Teheran", 35.7448, 51.3763,
     "No. 228, Hafez Ave, Teheran, Iran", 1967, 600),

    ("Iran", "Azja", "Iran Fara Bourse", "https://www.ifb.ir",
     "Alternatywna giełda papierów wartościowych", "Teheran", 35.7230, 51.4001,
     "Teheran, Iran", 2008, 500),

    ("Irak", "Azja", "Iraq Stock Exchange", "https://www.isx-iq.net",
     "Krajowa giełda papierów wartościowych", "Bagdad", 33.3350, 44.4005,
     "Al-Nahla Street, Bagdad, Irak", 2004, 100),

    ("Irlandia", "Europa", "Euronext Dublin", "https://live.euronext.com/en/markets/dublin",
     "Krajowa giełda papierów wartościowych", "Dublin", 53.3481, -6.2593,
     "28 Anglesea Street, Dublin 2, Irlandia", 1793, 60),

    ("Izrael", "Azja", "Tel Aviv Stock Exchange", "https://www.tase.co.il",
     "Krajowa giełda papierów wartościowych", "Tel Awiw", 32.0669, 34.7829,
     "2 Ahuzat Bayit Street, Tel Awiw 6525216, Izrael", 1935, 450),

    ("Włochy", "Europa", "Borsa Italiana", "https://www.borsaitaliana.it",
     "Krajowa giełda papierów wartościowych", "Mediolan", 45.4684, 9.1851,
     "Piazza degli Affari 6, 20123 Mediolan, Włochy", 1808, 370),

    ("Jamajka", "Ameryka Północna", "Jamaica Stock Exchange", "https://www.jamstockex.com",
     "Krajowa giełda papierów wartościowych", "Kingston", 17.9970, -76.7936,
     "40 Harbour Street, Kingston, Jamajka", 1968, 90),

    ("Japonia", "Azja", "Japan Exchange Group / Tokyo Stock Exchange", "https://www.jpx.co.jp/english/",
     "Krajowa giełda papierów wartościowych", "Tokio", 35.6817, 139.7675,
     "2-1 Nihonbashi Kabutocho, Chuo-ku, Tokio 103-8224, Japonia", 1878, 3900),

    ("Japonia", "Azja", "Nagoya Stock Exchange", "https://www.nse.or.jp",
     "Regionalna giełda papierów wartościowych", "Nagoya", 35.1815, 136.9066,
     "3-17 Sakae 3-chome, Naka-ku, Nagoya 460-0008, Japonia", 1886, 290),

    ("Japonia", "Azja", "Fukuoka Stock Exchange", "https://www.fse.or.jp",
     "Regionalna giełda papierów wartościowych", "Fukuoka", 33.5904, 130.4017,
     "14-2 Tenjin 2-chome, Chuo-ku, Fukuoka 810-0001, Japonia", 1949, 120),

    ("Japonia", "Azja", "Sapporo Securities Exchange", "https://www.sse.or.jp",
     "Regionalna giełda papierów wartościowych", "Sapporo", 43.0642, 141.3469,
     "5-14-1 Nishi, Chuo-ku, Sapporo 060-0042, Japonia", 1950, 65),

    ("Jordania", "Azja", "Amman Stock Exchange", "https://www.ase.com.jo",
     "Krajowa giełda papierów wartościowych", "Amman", 31.9519, 35.9278,
     "Arjan, Amman 11118, Jordania", 1978, 180),

    ("Kazachstan", "Azja", "Kazakhstan Stock Exchange (KASE)", "https://kase.kz",
     "Krajowa giełda papierów wartościowych", "Ałmaty", 43.2220, 76.8512,
     "97 Aiteke bi Street, Ałmaty 050000, Kazachstan", 1993, 130),

    ("Kazachstan", "Azja", "Astana International Exchange (AIX)", "https://www.aix.kz",
     "Krajowa giełda papierów wartościowych", "Astana", 51.0894, 71.4159,
     "55/17 Mangilik El, Astana, Kazachstan", 2017, 30),

    ("Kenia", "Afryka", "Nairobi Securities Exchange", "https://www.nse.co.ke",
     "Krajowa giełda papierów wartościowych", "Nairobi", -1.2799, 36.8276,
     "Exchange Square, Westlands, Nairobi, Kenia", 1954, 64),

    ("Kuwejt", "Azja", "Boursa Kuwait", "https://www.boursakuwait.com.kw",
     "Krajowa giełda papierów wartościowych", "Kuwait City", 29.3759, 47.9774,
     "Abdullah Al-Mubarak St, Kuwait City, Kuwejt", 1977, 150),

    ("Kirgistan", "Azja", "Kyrgyz Stock Exchange", "https://www.kse.kg",
     "Krajowa giełda papierów wartościowych", "Biszkek", 42.8746, 74.5698,
     "168 Kievskaya St, Biszkek, Kirgistan", 1994, 20),

    ("Laos", "Azja", "Lao Securities Exchange", "https://www.lsx.com.la",
     "Krajowa giełda papierów wartościowych", "Wientian", 17.9757, 102.6331,
     "Kaysone Phomvihane Road, Wientian, Laos", 2011, 12),

    ("Łotwa", "Europa", "Nasdaq Riga", "https://nasdaqbaltic.com",
     "Krajowa giełda papierów wartościowych", "Ryga", 56.9514, 24.1052,
     "Valdemāra iela 20a, Ryga LV-1010, Łotwa", 1993, 25),

    ("Liban", "Azja", "Beirut Stock Exchange", "https://www.bse.com.lb",
     "Krajowa giełda papierów wartościowych", "Bejrut", 33.8938, 35.5018,
     "Rue de l'Esplanade, Bejrut, Liban", 1920, 10),

    ("Libia", "Afryka", "Libyan Stock Market", "https://www.lsm.gov.ly",
     "Krajowa giełda papierów wartościowych", "Trypolis", 32.9052, 13.1800,
     "Trypolis, Libia", 2007, 15),

    ("Litwa", "Europa", "Nasdaq Vilnius", "https://nasdaqbaltic.com",
     "Krajowa giełda papierów wartościowych", "Wilno", 54.6872, 25.2797,
     "Geležinio Vilko 18A, Wilno 08104, Litwa", 1993, 32),

    ("Luksemburg", "Europa", "Luxembourg Stock Exchange", "https://www.luxse.com",
     "Krajowa giełda papierów wartościowych", "Luksemburg", 49.6117, 6.1319,
     "11 Avenue de la Porte-Neuve, Luksemburg", 1928, 50),

    ("Malawi", "Afryka", "Malawi Stock Exchange", "https://www.mse.co.mw",
     "Krajowa giełda papierów wartościowych", "Blantyre", -15.7861, 35.0058,
     "Henry Henderson Institute, Blantyre, Malawi", 1994, 15),

    ("Malezja", "Azja", "Bursa Malaysia", "https://www.bursamalaysia.com",
     "Krajowa giełda papierów wartościowych", "Kuala Lumpur", 3.1478, 101.6953,
     "Exchange Square, Bukit Kewangan, Kuala Lumpur 50200, Malezja", 1960, 1000),

    ("Malediwy", "Azja", "Maldives Stock Exchange", "https://www.stockexchange.mv",
     "Krajowa giełda papierów wartościowych", "Malé", 4.1755, 73.5093,
     "Boduthakurufaanu Magu, Malé 20094, Malediwy", 2002, 8),

    ("Mali", "Afryka", "Bourse Régionale des Valeurs Mobilières (BRVM)", "https://www.brvm.org",
     "Regionalna giełda dla państw UEMOA", "Abidżan, Côte d'Ivoire", 5.3197, -4.0139,
     "Rue Joseph Anoma, Abidżan, Côte d'Ivoire", 1998, 46),

    ("Malta", "Europa", "Malta Stock Exchange", "https://borzamalta.com.mt",
     "Krajowa giełda papierów wartościowych", "Valletta", 35.8994, 14.5132,
     "Garrison Chapel, Castille Place, Valletta VLT 1063, Malta", 1992, 25),

    ("Mauritius", "Afryka", "Stock Exchange of Mauritius", "https://www.stockexchangeofmauritius.com",
     "Krajowa giełda papierów wartościowych", "Port Louis", -20.1609, 57.4989,
     "Stock Exchange Building, Port Louis, Mauritius", 1988, 100),

    ("Meksyk", "Ameryka Północna", "Bolsa Mexicana de Valores (BMV)", "https://www.bmv.com.mx",
     "Krajowa giełda papierów wartościowych", "Mexico City", 19.4326, -99.1332,
     "Paseo de la Reforma 255, Ciudad de México 06500, Meksyk", 1894, 130),

    ("Meksyk", "Ameryka Północna", "Bolsa Institucional de Valores (BIVA)", "https://www.biva.mx",
     "Alternatywna giełda papierów wartościowych", "Mexico City", 19.4284, -99.1636,
     "Andrés Bello 45, Ciudad de México 11560, Meksyk", 2018, 50),

    ("Mongolia", "Azja", "Mongolian Stock Exchange", "https://www.mse.mn",
     "Krajowa giełda papierów wartościowych", "Ułan Bator", 47.9086, 106.9056,
     "Sukhbaatar Square 2, Ułan Bator 15160, Mongolia", 1991, 180),

    ("Czarnogóra", "Europa", "Montenegro Stock Exchange", "https://www.montenegroberza.com",
     "Krajowa giełda papierów wartościowych", "Podgorica", 42.4304, 19.2636,
     "Moskovska 77, Podgorica 81000, Czarnogóra", 1993, 80),

    ("Maroko", "Afryka", "Casablanca Stock Exchange", "https://www.casablanca-bourse.com",
     "Krajowa giełda papierów wartościowych", "Casablanca", 33.5901, -7.6094,
     "Angle Avenue des F.A.R. et Rue Moulay Youssef, Casablanca, Maroko", 1929, 77),

    ("Mozambik", "Afryka", "Bolsa de Valores de Moçambique", "https://www.bvm.co.mz",
     "Krajowa giełda papierów wartościowych", "Maputo", -25.9663, 32.5809,
     "Rua da Imprensa, Maputo, Mozambik", 1998, 5),

    ("Mjanma", "Azja", "Yangon Stock Exchange", "https://ysx-mm.com",
     "Krajowa giełda papierów wartościowych", "Rangun", 16.8661, 96.1951,
     "Sule Square, Rangun, Mjanma", 2016, 7),

    ("Namibia", "Afryka", "Namibian Stock Exchange", "https://www.nsx.com.na",
     "Krajowa giełda papierów wartościowych", "Windhoek", -22.5597, 17.0832,
     "4 Robert Mugabe Ave, Windhoek 10001, Namibia", 1992, 40),

    ("Nepal", "Azja", "Nepal Stock Exchange", "https://www.nepalstock.com.np",
     "Krajowa giełda papierów wartościowych", "Katmandu", 27.7041, 85.3132,
     "Singha Durbar Plaza, Katmandu, Nepal", 1994, 240),

    ("Holandia", "Europa", "Euronext Amsterdam", "https://live.euronext.com/en/markets/amsterdam",
     "Krajowa giełda papierów wartościowych", "Amsterdam", 52.3740, 4.8955,
     "Beursplein 5, 1012 JW Amsterdam, Holandia", 1602, 190),

    ("Nowa Zelandia", "Oceania", "NZX", "https://www.nzx.com",
     "Krajowa giełda papierów wartościowych", "Wellington", -41.2865, 174.7762,
     "11 Cable Street, Wellington 6011, Nowa Zelandia", 1974, 170),

    ("Nikaragua", "Ameryka Północna", "Bolsa de Valores de Nicaragua", "https://www.bolsanic.com",
     "Krajowa giełda papierów wartościowych", "Managua", 12.1509, -86.2684,
     "Km. 5.5, Carretera Sur, Managua, Nikaragua", 1993, 30),

    ("Niger", "Afryka", "Bourse Régionale des Valeurs Mobilières (BRVM)", "https://www.brvm.org",
     "Regionalna giełda dla państw UEMOA", "Abidżan, Côte d'Ivoire", 5.3197, -4.0139,
     "Rue Joseph Anoma, Abidżan, Côte d'Ivoire", 1998, 46),

    ("Nigeria", "Afryka", "Nigerian Exchange (NGX)", "https://ngxgroup.com",
     "Krajowa giełda papierów wartościowych", "Lagos", 6.4499, 3.4066,
     "2-4 Custom Street, Lagos, Nigeria", 1960, 155),

    ("Nigeria", "Afryka", "NASD OTC Securities Exchange", "https://www.nasdng.com",
     "Rynek OTC", "Lagos", 6.4350, 3.4200,
     "Lagos, Nigeria", 2013, 40),

    ("Nigeria", "Afryka", "FMDQ Securities Exchange", "https://www.fmdqgroup.com",
     "Rynek instrumentów dłużnych i walutowy", "Lagos", 6.4221, 3.4204,
     "13 Idowu Taylor Street, Victoria Island, Lagos, Nigeria", 2013, 50),

    ("Macedonia Północna", "Europa", "Macedonian Stock Exchange", "https://www.mse.mk",
     "Krajowa giełda papierów wartościowych", "Skopje", 41.9973, 21.4280,
     "Mito Hadzivasilev Jasmin 20, Skopje 1000, Macedonia Północna", 1995, 120),

    ("Norwegia", "Europa", "Oslo Børs", "https://live.euronext.com/en/markets/oslo",
     "Krajowa giełda papierów wartościowych", "Oslo", 59.9127, 10.7416,
     "Tollbugate 2, 0152 Oslo, Norwegia", 1819, 240),

    ("Oman", "Azja", "Muscat Stock Exchange", "https://www.msx.om",
     "Krajowa giełda papierów wartościowych", "Muskat", 23.5880, 58.3829,
     "Muttrah Business District, Muskat, Oman", 1989, 120),

    ("Pakistan", "Azja", "Pakistan Stock Exchange", "https://www.psx.com.pk",
     "Krajowa giełda papierów wartościowych", "Karaczi", 24.8548, 67.0109,
     "Stock Exchange Road, I.I. Chundrigar Road, Karaczi 74000, Pakistan", 1947, 540),

    ("Panama", "Ameryka Północna", "Bolsa Latinoamericana de Valores (Latinex)", "https://www.latinexbolsa.com",
     "Krajowa giełda papierów wartościowych", "Panama City", 8.9936, -79.5197,
     "Calle 49-B y Avenida Federico Boyd, Panama City, Panama", 1990, 40),

    ("Papua-Nowa Gwinea", "Oceania", "PNGX Markets", "https://www.pngx.com.pg",
     "Krajowa giełda papierów wartościowych", "Port Moresby", -9.4438, 147.1803,
     "Port Moresby, Papua-Nowa Gwinea", 1999, 20),

    ("Paragwaj", "Ameryka Południowa", "Bolsa de Valores y Productos de Asunción", "https://www.bvpasa.com.py",
     "Krajowa giełda papierów wartościowych", "Asunción", -25.2867, -57.6470,
     "Estrella 540, Asunción, Paragwaj", 1977, 15),

    ("Peru", "Ameryka Południowa", "Bolsa de Valores de Lima", "https://www.bvl.com.pe",
     "Krajowa giełda papierów wartościowych", "Lima", -12.0576, -77.0352,
     "Pasaje Acuña 106, Lima, Peru", 1860, 270),

    ("Filipiny", "Azja", "Philippine Stock Exchange", "https://www.pse.com.ph",
     "Krajowa giełda papierów wartościowych", "Taguig (BGC)", 14.5499, 121.0237,
     "PSE Tower, 5th Avenue corner 28th Street, Bonifacio Global City, Taguig, Filipiny", 1927, 270),

    ("Polska", "Europa", "Giełda Papierów Wartościowych w Warszawie (GPW)", "https://www.gpw.pl",
     "Krajowa giełda papierów wartościowych", "Warszawa", 52.2207, 21.0123,
     "Książęca 4, 00-498 Warszawa, Polska", 1991, 400),

    ("Portugalia", "Europa", "Euronext Lisbon", "https://live.euronext.com/en/markets/lisbon",
     "Krajowa giełda papierów wartościowych", "Lizbona", 38.7073, -9.1372,
     "Rua Soeiro Pereira Gomes 2, 1600-196 Lizbona, Portugalia", 1769, 50),

    ("Katar", "Azja", "Qatar Stock Exchange", "https://www.qe.com.qa",
     "Krajowa giełda papierów wartościowych", "Doha", 25.2854, 51.5310,
     "Grand Hamad Street, Doha, Katar", 1995, 52),

    ("Korea Południowa", "Azja", "Korea Exchange (KRX)", "https://global.krx.co.kr",
     "Krajowa giełda papierów wartościowych", "Seul (Yeouido)", 37.5229, 126.9267,
     "76 Yeouinaru-ro, Yeongdeungpo-gu, Seul 07329, Korea Południowa", 1956, 2400),

    ("Mołdawia", "Europa", "Moldova Stock Exchange", "https://www.moldse.md",
     "Krajowa giełda papierów wartościowych", "Kiszyniów", 47.0245, 28.8322,
     "202 Stefan cel Mare Blvd, Kiszyniów MD-2004, Mołdawia", 1994, 40),

    ("Rumunia", "Europa", "Bucharest Stock Exchange", "https://www.bvb.ro",
     "Krajowa giełda papierów wartościowych", "Bukareszt", 44.4312, 26.1010,
     "34-36 Carol I Blvd, Bukareszt 030167, Rumunia", 1882, 400),

    ("Rosja", "Europa", "Moscow Exchange", "https://www.moex.com",
     "Krajowa giełda papierów wartościowych", "Moskwa", 55.7561, 37.6173,
     "13 Vozdvizhenka Street, Moskwa 125009, Rosja", 1992, 200),

    ("Rosja", "Europa", "SPB Exchange", "https://spbexchange.ru",
     "Giełda / instytucja rynku kapitałowego", "Sankt Petersburg", 59.9311, 30.3609,
     "16 Mokhovaya Street, Sankt Petersburg 191186, Rosja", 1997, 50),

    ("Rwanda", "Afryka", "Rwanda Stock Exchange", "https://www.rse.rw",
     "Krajowa giełda papierów wartościowych", "Kigali", -1.9441, 30.0619,
     "KN 6 Avenue, Kigali, Rwanda", 2008, 11),

    ("Saint Kitts i Nevis", "Ameryka Północna", "Eastern Caribbean Securities Exchange (ECSE)", "https://www.ecseonline.com",
     "Regionalna giełda dla państw ECCU/OECS", "Basseterre", 17.3026, -62.7177,
     "Bird Rock Business Park, Basseterre, Saint Kitts and Nevis", 1983, 31),

    ("Saint Lucia", "Ameryka Północna", "Eastern Caribbean Securities Exchange (ECSE)", "https://www.ecseonline.com",
     "Regionalna giełda dla państw ECCU/OECS", "Basseterre, Saint Kitts", 17.3026, -62.7177,
     "Bird Rock Business Park, Basseterre, Saint Kitts and Nevis", 1983, 31),

    ("Saint Vincent i Grenadyny", "Ameryka Północna", "Eastern Caribbean Securities Exchange (ECSE)", "https://www.ecseonline.com",
     "Regionalna giełda dla państw ECCU/OECS", "Basseterre, Saint Kitts", 17.3026, -62.7177,
     "Bird Rock Business Park, Basseterre, Saint Kitts and Nevis", 1983, 31),

    ("Arabia Saudyjska", "Azja", "Saudi Exchange / Tadawul", "https://www.saudiexchange.sa",
     "Krajowa giełda papierów wartościowych", "Rijad", 24.7136, 46.6753,
     "King Fahd Road, Rijad 11211, Arabia Saudyjska", 2007, 220),

    ("Senegal", "Afryka", "Bourse Régionale des Valeurs Mobilières (BRVM)", "https://www.brvm.org",
     "Regionalna giełda dla państw UEMOA", "Abidżan, Côte d'Ivoire", 5.3197, -4.0139,
     "Rue Joseph Anoma, Abidżan, Côte d'Ivoire", 1998, 46),

    ("Serbia", "Europa", "Belgrade Stock Exchange", "https://www.belex.rs",
     "Krajowa giełda papierów wartościowych", "Belgrad", 44.8122, 20.4611,
     "Omladinskih brigada 1, Belgrad 11070, Serbia", 1894, 380),

    ("Seszele", "Afryka", "MERJ Exchange", "https://merj.exchange",
     "Krajowa giełda papierów wartościowych", "Victoria", -4.6197, 55.4513,
     "Mont Fleuri, Victoria, Seszele", 2012, 25),

    ("Singapur", "Azja", "Singapore Exchange (SGX)", "https://www.sgx.com",
     "Krajowa giełda papierów wartościowych", "Singapur", 1.2841, 103.8499,
     "2 Shenton Way, SGX Centre, Singapur 068804", 1999, 620),

    ("Słowacja", "Europa", "Bratislava Stock Exchange", "https://www.bsse.sk",
     "Krajowa giełda papierów wartościowych", "Bratysława", 48.1486, 17.1077,
     "Vysoká 17, 811 06 Bratysława, Słowacja", 1991, 15),

    ("Słowenia", "Europa", "Ljubljana Stock Exchange", "https://ljse.si",
     "Krajowa giełda papierów wartościowych", "Lublana", 46.0569, 14.5058,
     "Slovenska 56, 1000 Lublana, Słowenia", 1989, 30),

    ("Republika Południowej Afryki", "Afryka", "Johannesburg Stock Exchange (JSE)", "https://www.jse.co.za",
     "Krajowa giełda papierów wartościowych", "Johannesburg (Sandton)", -26.1052, 28.0562,
     "One Exchange Square, 2 Gwen Lane, Sandton, Johannesburg, RPA", 1887, 360),

    ("Republika Południowej Afryki", "Afryka", "Cape Town Stock Exchange", "https://www.ctexchange.co.za",
     "Alternatywna giełda papierów wartościowych", "Kapsztad", -33.9249, 18.4241,
     "Cape Town, RPA", 2021, 15),

    ("Republika Południowej Afryki", "Afryka", "A2X Markets", "https://www.a2x.co.za",
     "Alternatywna giełda papierów wartościowych", "Kapsztad", -33.9200, 18.4230,
     "Cape Town, RPA", 2017, 100),

    ("Hiszpania", "Europa", "Bolsa de Madrid / BME", "https://www.bolsasymercados.es",
     "Krajowa giełda papierów wartościowych", "Madryt", 40.4168, -3.6974,
     "Plaza de la Lealtad 1, 28014 Madryt, Hiszpania", 1831, 140),

    ("Hiszpania", "Europa", "Bolsa de Barcelona / BME", "https://www.bolsasymercados.es",
     "Regionalna giełda papierów wartościowych", "Barcelona", 41.3851, 2.1734,
     "Passeig de Gràcia 19, 08007 Barcelona, Hiszpania", 1915, 40),

    ("Hiszpania", "Europa", "Bolsa de Bilbao / BME", "https://www.bolsasymercados.es",
     "Regionalna giełda papierów wartościowych", "Bilbao", 43.2630, -2.9350,
     "José María Olabarri 1, 48001 Bilbao, Hiszpania", 1890, 20),

    ("Hiszpania", "Europa", "Bolsa de Valencia / BME", "https://www.bolsasymercados.es",
     "Regionalna giełda papierów wartościowych", "Walencja", 39.4699, -0.3763,
     "Libreros 2, 46002 Walencja, Hiszpania", 1980, 15),

    ("Sri Lanka", "Azja", "Colombo Stock Exchange", "https://www.cse.lk",
     "Krajowa giełda papierów wartościowych", "Kolombo", 6.9271, 79.8612,
     "World Trade Center, Kolombo 01, Sri Lanka", 1896, 300),

    ("Sudan", "Afryka", "Khartoum Stock Exchange", "https://www.kse.com.sd",
     "Krajowa giełda papierów wartościowych", "Chartum", 15.5514, 32.5325,
     "Chartum, Sudan", 1994, 60),

    ("Surinam", "Ameryka Południowa", "Suriname Stock Exchange", "https://www.surinamestockexchange.com",
     "Krajowa giełda papierów wartościowych", "Paramaribo", 5.8520, -55.2038,
     "Paramaribo, Surinam", 1994, 5),

    ("Szwecja", "Europa", "Nasdaq Stockholm", "https://www.nasdaqomxnordic.com",
     "Krajowa giełda papierów wartościowych", "Sztokholm", 59.3327, 18.0575,
     "Tullvaktsvägen 15, 115 56 Sztokholm, Szwecja", 1863, 990),

    ("Szwecja", "Europa", "Nordic Growth Market (NGM)", "https://www.ngm.se",
     "Alternatywna giełda papierów wartościowych", "Sztokholm", 59.3339, 18.0544,
     "Sztokholm, Szwecja", 1999, 200),

    ("Szwecja", "Europa", "Spotlight Stock Market", "https://spotlightstockmarket.com",
     "Alternatywna giełda papierów wartościowych", "Sztokholm", 59.3320, 18.0560,
     "Sztokholm, Szwecja", 2012, 300),

    ("Szwajcaria", "Europa", "SIX Swiss Exchange", "https://www.six-group.com",
     "Krajowa giełda papierów wartościowych", "Zurych", 47.3769, 8.5417,
     "Selnaustrasse 30, 8001 Zurych, Szwajcaria", 1850, 230),

    ("Szwajcaria", "Europa", "BX Swiss", "https://www.bxswiss.com",
     "Alternatywna giełda papierów wartościowych", "Bern", 46.9481, 7.4474,
     "Selnaustrasse 32, 8021 Zurych, Szwajcaria", 1888, 40),

    ("Syria", "Azja", "Damascus Securities Exchange", "http://www.dse.sy",
     "Krajowa giełda papierów wartościowych", "Damaszek", 33.5102, 36.2913,
     "Damaszek, Syria", 2009, 30),

    ("Tadżykistan", "Azja", "Central Asian Stock Exchange", "https://case.com.tj",
     "Krajowa giełda papierów wartościowych", "Duszanbe", 38.5598, 68.7738,
     "Duszanbe, Tadżykistan", 1996, 10),

    ("Tajlandia", "Azja", "Stock Exchange of Thailand (SET)", "https://www.set.or.th",
     "Krajowa giełda papierów wartościowych", "Bangkok", 13.7563, 100.5018,
     "93 Ratchadaphisek Road, Dindeang, Bangkok 10400, Tajlandia", 1975, 800),

    ("Togo", "Afryka", "Bourse Régionale des Valeurs Mobilières (BRVM)", "https://www.brvm.org",
     "Regionalna giełda dla państw UEMOA", "Abidżan, Côte d'Ivoire", 5.3197, -4.0139,
     "Rue Joseph Anoma, Abidżan, Côte d'Ivoire", 1998, 46),

    ("Trynidad i Tobago", "Ameryka Północna", "Trinidad and Tobago Stock Exchange", "https://www.stockex.co.tt",
     "Krajowa giełda papierów wartościowych", "Port of Spain", 10.6549, -61.5019,
     "Independence Square, Port of Spain, Trynidad i Tobago", 1981, 35),

    ("Tunezja", "Afryka", "Tunis Stock Exchange", "https://www.bvmt.com.tn",
     "Krajowa giełda papierów wartościowych", "Tunis", 36.8090, 10.1680,
     "Rue du Marché, 1000 Tunis, Tunezja", 1969, 80),

    ("Turcja", "Europa", "Borsa İstanbul", "https://www.borsaistanbul.com",
     "Krajowa giełda papierów wartościowych", "Stambuł", 41.0682, 29.0310,
     "Reşitpaşa Mah., Tuncay Artun Cad., Sarıyer, Stambuł 34467, Turcja", 1985, 540),

    ("Uganda", "Afryka", "Uganda Securities Exchange", "https://www.use.or.ug",
     "Krajowa giełda papierów wartościowych", "Kampala", 0.3476, 32.5825,
     "Jubilee Insurance Centre, 14 Parliament Avenue, Kampala, Uganda", 1997, 17),

    ("Ukraina", "Europa", "Ukrainian Exchange", "http://www.ux.ua",
     "Krajowa giełda papierów wartościowych", "Kijów", 50.4547, 30.5238,
     "Instrumentalna 1A, Kijów 01601, Ukraina", 2009, 100),

    ("Ukraina", "Europa", "PFTS Stock Exchange", "https://pfts.ua",
     "Giełda / instytucja rynku kapitałowego", "Kijów", 50.4450, 30.5150,
     "Kijów, Ukraina", 1996, 100),

    ("Zjednoczone Emiraty Arabskie", "Azja", "Abu Dhabi Securities Exchange (ADX)", "https://www.adx.ae",
     "Krajowa giełda papierów wartościowych", "Abu Zabi", 24.4539, 54.3773,
     "Khaleej Al Arabi Street, Abu Zabi, ZEA", 2000, 82),

    ("Zjednoczone Emiraty Arabskie", "Azja", "Dubai Financial Market (DFM)", "https://www.dfm.ae",
     "Krajowa giełda papierów wartościowych", "Dubaj", 25.2048, 55.2708,
     "Dubai Financial Market Building, Dubaj, ZEA", 2000, 76),

    ("Zjednoczone Emiraty Arabskie", "Azja", "Nasdaq Dubai", "https://www.nasdaqdubai.com",
     "Rynek giełdowy", "Dubaj (DIFC)", 25.2184, 55.2838,
     "DIFC Gate District, Dubaj, ZEA", 2005, 100),

    ("Wielka Brytania", "Europa", "London Stock Exchange", "https://www.londonstockexchange.com",
     "Krajowa giełda papierów wartościowych", "Londyn", 51.5135, -0.0928,
     "10 Paternoster Square, Londyn EC4M 7LS, Wielka Brytania", 1801, 1900),

    ("Wielka Brytania", "Europa", "Aquis Stock Exchange", "https://www.aquis.eu/aquis-stock-exchange",
     "Alternatywna giełda papierów wartościowych", "Londyn", 51.5155, -0.0922,
     "Birchin Lane, Londyn EC3V 9BW, Wielka Brytania", 2013, 100),

    ("Tanzania", "Afryka", "Dar es Salaam Stock Exchange", "https://www.dse.co.tz",
     "Krajowa giełda papierów wartościowych", "Dar es Salaam", -6.7924, 39.2083,
     "14th Floor, Golden Jubilee Towers, Dar es Salaam, Tanzania", 1998, 28),

    ("Stany Zjednoczone", "Ameryka Północna", "New York Stock Exchange (NYSE)", "https://www.nyse.com",
     "Krajowa giełda papierów wartościowych", "Nowy Jork", 40.7069, -74.0089,
     "11 Wall Street, New York, NY 10005, USA", 1792, 2300),

    ("Stany Zjednoczone", "Ameryka Północna", "Nasdaq Stock Market", "https://www.nasdaq.com",
     "Krajowa giełda papierów wartościowych", "Nowy Jork (Times Square)", 40.7580, -73.9855,
     "151 W 42nd Street, New York, NY 10036, USA", 1971, 3600),

    ("Stany Zjednoczone", "Ameryka Północna", "Cboe U.S. Equities Exchanges", "https://www.cboe.com/us/equities/",
     "Alternatywna giełda papierów wartościowych", "Chicago", 41.8786, -87.6251,
     "400 S LaSalle Street, Chicago, IL 60605, USA", 1973, 0),

    ("Stany Zjednoczone", "Ameryka Północna", "IEX Exchange", "https://www.iexexchange.io",
     "Alternatywna giełda papierów wartościowych", "Nowy Jork", 40.7128, -74.0059,
     "4 World Trade Center, New York, NY 10007, USA", 2016, 0),

    ("Stany Zjednoczone", "Ameryka Północna", "MEMX", "https://memx.com",
     "Alternatywna giełda papierów wartościowych", "Jersey City, NJ", 40.7180, -74.0680,
     "100 Montgomery Street, Jersey City, NJ 07302, USA", 2020, 0),

    ("Urugwaj", "Ameryka Południowa", "Bolsa de Valores de Montevideo", "https://www.bvm.com.uy",
     "Krajowa giełda papierów wartościowych", "Montevideo", -34.9011, -56.1645,
     "Misiones 1400, Montevideo 11000, Urugwaj", 1867, 15),

    ("Urugwaj", "Ameryka Południowa", "Bolsa Electrónica de Valores del Uruguay (BEVSA)", "https://www.bevsa.com.uy",
     "Elektroniczna platforma transakcyjna", "Montevideo", -34.9015, -56.1650,
     "Misiones 1549, Montevideo 11000, Urugwaj", 1994, 30),

    ("Uzbekistan", "Azja", "Tashkent Republican Stock Exchange", "https://www.uzse.uz",
     "Krajowa giełda papierów wartościowych", "Taszkent", 41.2995, 69.2401,
     "10 Buyuk Ipak Yuli, Taszkent 100003, Uzbekistan", 1994, 80),

    ("Wenezuela", "Ameryka Południowa", "Bolsa de Valores de Caracas", "https://www.bolsadecaracas.com",
     "Krajowa giełda papierów wartościowych", "Caracas", 10.4806, -66.9036,
     "Calle Sorocaima, Caracas, Wenezuela", 1947, 30),

    ("Wenezuela", "Ameryka Południowa", "Bolsa Pública de Valores Bicentenaria", "https://www.bpvb.gob.ve",
     "Krajowa giełda papierów wartościowych", "Caracas", 10.4700, -66.9100,
     "Caracas, Wenezuela", 2010, 30),

    ("Wietnam", "Azja", "Ho Chi Minh Stock Exchange (HOSE)", "https://www.hsx.vn",
     "Krajowa giełda papierów wartościowych", "Ho Chi Minh City", 10.7769, 106.7009,
     "16 Vo Van Kiet, District 1, Ho Chi Minh City, Wietnam", 2000, 450),

    ("Wietnam", "Azja", "Hanoi Stock Exchange (HNX)", "https://www.hnx.vn",
     "Krajowa giełda papierów wartościowych", "Hanoi", 21.0285, 105.8542,
     "2 Phan Chu Trinh, Hoàn Kiếm, Hanoi, Wietnam", 2005, 350),

    ("Wietnam", "Azja", "Vietnam Exchange (VNX)", "https://vietnamexchange.vn",
     "Konglomerat giełd Vietnam (HOSE + HNX)", "Hanoi", 21.0245, 105.8412,
     "Hanoi, Wietnam", 2023, 800),

    ("Zambia", "Afryka", "Lusaka Securities Exchange", "https://www.luse.co.zm",
     "Krajowa giełda papierów wartościowych", "Lusaka", -15.4167, 28.2833,
     "Cairo Road, Lusaka, Zambia", 1994, 20),

    ("Zimbabwe", "Afryka", "Zimbabwe Stock Exchange", "https://www.zse.co.zw",
     "Krajowa giełda papierów wartościowych", "Harare", -17.8292, 31.0522,
     "Southerton, Harare, Zimbabwe", 1946, 56),

    ("Zimbabwe", "Afryka", "Victoria Falls Stock Exchange", "https://www.vfex.exchange",
     "Alternatywna giełda papierów wartościowych", "Victoria Falls", -17.9243, 25.8573,
     "Victoria Falls, Zimbabwe", 2020, 5),

    ("Palestyna", "Azja", "Palestine Exchange", "https://www.pex.ps",
     "Krajowa giełda papierów wartościowych", "Nablus", 32.2211, 35.2544,
     "Nablus, Palestyna", 1997, 50),
]

records = [
    {
        "country":          e[0], "region":           e[1], "name":    e[2],
        "website":          e[3], "type":             e[4], "city":    e[5],
        "lat":              e[6], "lon":              e[7], "address": e[8],
        "founded_year":     e[9], "listed_companies": e[10],
    }
    for e in EXCHANGES
]

if __name__ == "__main__":
    out = sys.stdout if len(sys.argv) < 2 else open(sys.argv[1], "w", encoding="utf-8")
    json.dump(records, out, ensure_ascii=False, indent=2)
    if out is not sys.stdout:
        out.close()
    print(f"Generated {len(records)} exchange records.", file=sys.stderr)
