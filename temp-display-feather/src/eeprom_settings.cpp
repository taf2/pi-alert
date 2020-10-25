#include "eeprom_settings.h"
// we need this table to allow buttons to switch between timezones also we can in this way perist the timezoneIndex in EEPROM
static const int ZONES[][2] = {{14,0}, // UTC +14	Samoa and Christmas Island/Kiribati	LINT	Kiritimati
																{13,45}, // UTC +13:45	Chatham Islands/New Zealand	CHADT	Chatham Islands
																{13,0}, // UTC +13	New Zealand with exceptions and 4 more	NZDT	Auckland
																{12,0}, // UTC +12	Fiji, small region of Russia and 7 more	ANAT	Anadyr
																{11,0}, // UTC +11	much of Australia and 7 more	AEDT	Melbourne
																{10,45}, // UTC +10:30	small region of Australia	ACDT	Adelaide
																{10,0}, // UTC +10	Queensland/Australia and 6 more	AEST	Brisbane
																{9,30}, // UTC +9:30	Northern Territory/Australia	ACST	Darwin
																{9,0}, // UTC +9	Japan, South Korea and 5 more	JST	Tokyo
																{8,45}, // UTC +8:45	Western Australia/Australia	ACWST	Eucla
																{8, 0}, // UTC +8	China, Philippines and 10 more	CST	Beijing
																{7, 0}, // UTC +7	much of Indonesia, Thailand and 7 more	WIB	Jakarta
																{6, 30}, // UTC +6:30	Myanmar and Cocos Islands	MMT	Yangon
																{6,0}, // UTC +6	Bangladesh and 6 more	BST	Dhaka
																{5,45}, // UTC +5:45	Nepal	NPT	Kathmandu
																{5,30}, // UTC +5:30	India and Sri Lanka	IST	New Delhi
																{5,0}, // UTC +5	Pakistan and 8 more	UZT	Tashkent
																{4,30}, // UTC +4:30	Afghanistan	AFT	Kabul
																{4,0}, // UTC +4	Azerbaijan and 8 more	GST	Dubai
																{3,30}, // UTC +3:30	Iran	IRST	Tehran
																{3,0}, // UTC +3	Greece and 36 more	MSK	Moscow
																{2,0}, // UTC +2	Germany and 48 more	CEST	Brussels
																{1,0}, // UTC +1	United Kingdom and 22 more	BST	London
																{0,0}, // UTC +0	Iceland and 17 more	GMT	Accra
																{-1,0}, // UTC -1	Cabo Verde	CVT	Praia
																{-2,0}, //UTC -2	most of Greenland and 3 more	WGST	Nuuk
																{-2,-30}, // UTC -2:30	Newfoundland and Labrador/Canada	NDT	St. John's
																{-3,-0}, // UTC -3	most of Brazil, Argentina and 10 more	ART	Buenos Aires
																{-4,-0}, // UTC -4	regions of USA and 31 more	EDT	New York
																{-5,-0}, // UTC -5	regions of USA and 10 more	CDT	Mexico City
																{-6,-0}, // UTC -6	small region of USA and 9 more	CST	Guatemala City
																{-7,-0}, // UTC -7	regions of USA and 2 more	PDT	Los Angeles
																{-8,-0}, // UTC -8	Alaska/USA and 2 more	AKDT	Anchorage
																{-9,-0}, // UTC -9	Alaska/USA and regions of French Polynesia	HDT	Adak
																{-9,-30}, // UTC -9:30	Marquesas Islands/French Polynesia	MART	Taiohae
																{-10,-0}, // UTC -10	Hawaii/USA and 2 more	HST	Honolulu
																{-11,-0}, // UTC -11	American Samoa and 2 more	NUT	Alofi
																{-12,-0}  // UTC -12	much of US Minor Outlying Islands	AoE	Baker Island
};
//
// NOTE: maybe we can use curl "http://worldtimeapi.org/api/ip/:ipv4:.txt" to determine dst vs not?
// or at least on first boot as away to guess timezone
//
static const char *ZONE_NAMES[] = {"UTC +14	Samoa and Christmas Island/Kiribati	LINT	Kiritimati",
                            "UTC +13:45	Chatham Islands/New Zealand	CHADT	Chatham Islands",
                            "UTC +13	New Zealand with exceptions and 4 more	NZDT	Auckland",
                            "UTC +12	Fiji, small region of Russia and 7 more	ANAT	Anadyr",
                            "UTC +11	much of Australia and 7 more	AEDT	Melbourne",
                            "UTC +10:30	small region of Australia	ACDT	Adelaide",
                            "UTC +10	Queensland/Australia and 6 more	AEST	Brisbane",
                            "UTC +9:30	Northern Territory/Australia	ACST	Darwin",
                            "UTC +9	Japan, South Korea and 5 more	JST	Tokyo",
                            "UTC +8:45	Western Australia/Australia	ACWST	Eucla",
                            "UTC +8	China, Philippines and 10 more	CST	Beijing",
                            "UTC +7	much of Indonesia, Thailand and 7 more	WIB	Jakarta",
                            "UTC +6:30	Myanmar and Cocos Islands	MMT	Yangon",
                            "UTC +6	Bangladesh and 6 more	BST	Dhaka",
                            "UTC +5:45	Nepal	NPT	Kathmandu",
                            "UTC +5:30	India and Sri Lanka	IST	New Delhi",
                            "UTC +5	Pakistan and 8 more	UZT	Tashkent",
                            "UTC +4:30	Afghanistan	AFT	Kabul",
                            "UTC +4	Azerbaijan and 8 more	GST	Dubai",
                            "UTC +3:30	Iran	IRST	Tehran",
                            "UTC +3	Greece and 36 more	MSK	Moscow",
                            "UTC +2	Germany and 48 more	CEST	Brussels",
                            "UTC +1	United Kingdom and 22 more	BST	London",
                            "UTC +0	Iceland and 17 more	GMT	Accra",
                            "UTC -1	Cabo Verde	CVT	Praia",
                            "UTC -2	most of Greenland and 3 more	WGST	Nuuk",
                            "UTC -2:30	Newfoundland and Labrador/Canada	NDT	St. John's",
                            "UTC -3	most of Brazil, Argentina and 10 more	ART	Buenos Aires",
                            "UTC -4	regions of USA and 31 more	EDT	New York",
                            "UTC -5	regions of USA and 10 more	CDT	Mexico City",
                            "UTC -6	small region of USA and 9 more	CST	Guatemala City",
                            "UTC -7	regions of USA and 2 more	PDT	Los Angeles",
                            "UTC -8	Alaska/USA and 2 more	AKDT	Anchorage",
                            "UTC -9	Alaska/USA and regions of French Polynesia	HDT	Adak",
                            "UTC -9:30	Marquesas Islands/French Polynesia	MART	Taiohae",
                            "UTC -10	Hawaii/USA and 2 more	HST	Honolulu",
                            "UTC -11	American Samoa and 2 more	NUT	Alofi",
                            "UTC -12	much of US Minor Outlying Islands	AoE	Baker Island"
};


//static int mode = 0; // Button A
int EEPROMSettings::timezoneOffset() {
  int hours = ZONES[timezoneIndex][0];
  int minutes = ZONES[timezoneIndex][1];
  int totalMinutes = (hours * 60 + minutes);
  int totalSeconds = totalMinutes * 60;
	return totalSeconds;
}

void EEPROMSettings::fetchQuote(NTPClient &timeClient) {
  WiFiClient client;
  HTTPClient http;
  // curl -v  -i -X GET http://quotes.rest/qod.json?category=management
  String url = "http://quotes.rest/qod.json?category=management";
  http.setConnectTimeout(20000);// timeout in ms
  http.setTimeout(20000); // 20 seconds
  http.setUserAgent("Fun-Clock");
  http.begin(client, url);
  http.addHeader("Content-Type", "application/json");
  int r =  http.GET();
	if (r < 0) {
		Serial.println("error fetching quote");
		return;
	}
  //http.writeToStream(&Serial);
  String body = http.getString();
  http.end();
  DynamicJsonDocument doc(body.length());
  DeserializationError error = deserializeJson(doc, body);
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  }

  JsonObject obj = doc.as<JsonObject>();
  // contents -> { quotes -> [ { quote: "...", length: , author: ".."  },  ] }
  JsonObject contents = obj[String("contents")].as<JsonObject>();
  JsonArray quotes = contents["quotes"].as<JsonArray>();
  bool didFetchSuccess = false;
  for (JsonVariant value : quotes) {
    //Serial.println(value.as<char*>());
    JsonObject quoteObj = value.as<JsonObject>();
    const char *_quote = quoteObj["quote"].as<String>().c_str();
    const char *_author = quoteObj["author"].as<String>().c_str();
    strncpy(this->quote, _quote, 127);
    strncpy(this->author, _author, 31);
    didFetchSuccess = true;
    break; // break since we just need the first one
  }
  if (didFetchSuccess) {
		Serial.println(this->quote);
		Serial.println(this->author);
    lastQuoteFetchDay = timeClient.getEpochTime() / 60 / 60 / 24;
    save();
  }
}

void EEPROMSettings::loadQuote(NTPClient &timeClient) {
  const int currentDay = timeClient.getEpochTime() / 60 / 60 / 24;
  if (currentDay > lastQuoteFetchDay) {
    Serial.println("fetching a new quote for a new day");
    fetchQuote(timeClient);
  } else {
		//Serial.println("we already have a quote:");
		//Serial.println(quote);
//		Serial.println(author);
	}
}

void EEPROMSettings::adjustTimezone(Adafruit_SSD1306 &display, NTPClient &timeClient) {
  // 24 times zones
  // smallest timezone of 24 is -12 e.g. 43200
  int offsetSeconds = timezoneOffset();
  display.clearDisplay();
  display.setCursor(0,0);
  display.setTextColor(SSD1306_WHITE);
  display.print(ZONE_NAMES[timezoneIndex]);

  yield();

  display.display();

  Serial.print("set timezone offset:");
  Serial.println(offsetSeconds);
  timeClient.setTimeOffset(offsetSeconds);
  yield();
  save();
}

void EEPROMSettings::nextZone(Adafruit_SSD1306 &display, NTPClient &timeClient) {
	++timezoneIndex;
	if (timezoneIndex >= (sizeof(ZONES)/sizeof(int))) {
		timezoneIndex = 0;
	}
	adjustTimezone(display, timeClient);
}
void EEPROMSettings::prevZone(Adafruit_SSD1306 &display, NTPClient &timeClient) {
	--timezoneIndex;
	if (timezoneIndex < 0) {
		timezoneIndex = (sizeof(ZONES)/sizeof(int)) - 1;
	}
	adjustTimezone(display, timeClient);
}

int EEPROMSettings::load() {
	int ret = EEPROM_readAnything<EEPROMSettings>(3, *this);
	if (timezoneIndex >= (sizeof(ZONES) / sizeof(int))) { // out of bounds
		timezoneIndex = 28; // EDT
	}
	return ret;
}

int EEPROMSettings::save() {
	hasSetting = false;
	int ret =  EEPROM_writeAnything<EEPROMSettings>(3, *this);
	EEPROM.commit();
	return ret;
}
