#include "sms_data.h"
#include <json-c/json.h>
#include <stdlib.h>
#include <stdio.h>
#include <hash.h>
#include <crc32.h>
#include <string.h>

const char* sms_carrier_json = 
   "{   \
	\"info\" : \"JSON array containing e-mail to SMS and MMS mappings constructed from the best available sources and DNS validations.  (C) 2013 CubicleSoft.  All Rights Reserved.\", \
	\"license\" : \"MIT or LGPL.  See http://github.com/cubiclesoft/email_sms_mms_gateways for details.\", \
	\"lastupdated\" : \"2013-02-08\", \
	\"countries\" : {   \
		\"us\" : \"United States\", \
		\"ca\" : \"Canada\", \
		\"ar\" : \"Argentina\", \
		\"aw\" : \"Aruba\", \
		\"au\" : \"Australia\", \
		\"be\" : \"Belgium\", \
		\"br\" : \"Brazil\", \
		\"bg\" : \"Bulgaria\", \
		\"cn\" : \"China\", \
		\"co\" : \"Columbia\", \
		\"cz\" : \"Czech Republic\", \
		\"dm\" : \"Dominica\", \
		\"eg\" : \"Egypt\", \
		\"fo\" : \"Faroe Islands\", \
		\"fr\" : \"France\", \
		\"de\" : \"Germany\", \
		\"gy\" : \"Guyana\", \
		\"hk\" : \"Hong Kong\", \
		\"is\" : \"Iceland\", \
		\"in\" : \"India\", \
		\"ie\" : \"Ireland\", \
		\"il\" : \"Israel\", \
		\"it\" : \"Italy\", \
		\"jp\" : \"Japan\", \
		\"mu\" : \"Mauritius\", \
		\"mx\" : \"Mexico\", \
		\"np\" : \"Nepal\", \
		\"nl\" : \"Netherlands\", \
		\"nz\" : \"New Zealand\", \
		\"pa\" : \"Panama\", \
		\"pl\" : \"Poland\", \
		\"pr\" : \"Puerto Rico\", \
		\"ru\" : \"Russia\", \
		\"sg\" : \"Singapore\", \
		\"za\" : \"South Africa\", \
		\"kr\" : \"South Korea\", \
		\"es\" : \"Spain\", \
		\"lk\" : \"Sri Lanka\", \
		\"se\" : \"Sweden\", \
		\"ch\" : \"Switzerland\", \
		\"ua\" : \"Ukraine\", \
		\"uk\" : \"United Kingdom\", \
		\"uy\" : \"Uruguay\", \
		\"europe\" : \"Europe\", \
		\"latin_america\" : \"Latin America\", \
		\"int\" : \"International\" \
	}, \
	\"sms_carriers\" : {    \
		\"us\" : {  \
			\"airfire_mobile\" : [\"Airfire Mobile\", \"{number}@sms.airfiremobile.com\"], \
			\"alltel\" : [\"Alltel\", \"{number}@message.alltel.com\"], \
			\"alltel_allied\" : [\"Alltel (Allied Wireless)\", \"{number}@sms.alltelwireless.com\"], \
			\"alaska_communications\" : [\"Alaska Communications\", \"{number}@msg.acsalaska.com\"], \
			\"ameritech\" : [\"Ameritech\", \"{number}@paging.acswireless.com\"], \
			\"assurance_wireless\" : [\"Assurance Wireless\", \"{number}@vmobl.com\"], \
			\"at_and_t\" : [\"AT&T Wireless\", \"{number}@txt.att.net\"], \
			\"at_and_t_mobility\" : [\"AT&T Mobility (Cingular)\", \"{number}@txt.att.net\", \"{number}@cingularme.com\", \"{number}@mobile.mycingular.com\"], \
			\"at_and_t_paging\" : [\"AT&T Enterprise Paging\", \"{number}@page.att.net\"], \
			\"at_and_t_global_sms\" : [\"AT&T Global Smart Messaging Suite\", \"{number}@sms.smartmessagingsuite.com\"], \
			\"bellsouth\" : [\"BellSouth\", \"{number}@bellsouth.cl\"], \
			\"bluegrass\" : [\"Bluegrass Cellular\", \"{number}@sms.bluecell.com\"], \
			\"bluesky\" : [\"Bluesky Communications\", \"{number}@psms.bluesky.as\"], \
			\"blueskyfrog\" : [\"BlueSkyFrog\", \"{number}@blueskyfrog.com\"], \
			\"boostmobile\" : [\"Boost Mobile\", \"{number}@sms.myboostmobile.com\", \"{number}@myboostmobile.com\"], \
			\"cellcom\" : [\"Cellcom\", \"{number}@cellcom.quiktxt.com\"], \
			\"cellularsouth\" : [\"Cellular South\", \"{number}@csouth1.com\"], \
			\"centennial_wireless\" : [\"Centennial Wireless\", \"{number}@cwemail.com\"], \
			\"chariton_valley\" : [\"Chariton Valley Wireless\", \"{number}@sms.cvalley.net\"], \
			\"chat_mobility\" : [\"Chat Mobility\", \"{number}@mail.msgsender.com\"], \
			\"cincinnati_bell\" : [\"Cincinnati Bell\", \"{number}@gocbw.com\"], \
			\"cingular\" : [\"Cingular (Postpaid)\", \"{number}@cingular.com\", \"{number}@mobile.mycingular.com\"], \
			\"cleartalk\" : [\"Cleartalk Wireless\", \"{number}@sms.cleartalk.us\"], \
			\"comcast_pcs\" : [\"Comcast PCS\", \"{number}@comcastpcs.textmsg.com\"], \
			\"cricket\" : [\"Cricket\", \"{number}@sms.mycricket.com\"], \
			\"cspire\" : [\"C Spire Wireless\", \"{number}@cspire1.com\"], \
			\"dtc_wireless\" : [\"DTC Wireless\", \"{number}@sms.advantagecell.net\"], \
			\"element\" : [\"Element Mobile\", \"{number}@sms.elementmobile.net\"], \
			\"esendex\" : [\"Esendex\", \"{number}@echoemail.net\"], \
			\"general_comm\" : [\"General Communications Inc.\", \"{number}@mobile.gci.net\"], \
			\"golden_state\" : [\"Golden State Cellular\", \"{number}@gscsms.com\"], \
			\"google_voice\" : [\"Google Voice\", \"{number}@txt.voice.google.com\"], \
			\"greatcall\" : [\"GreatCall\", \"{number}@vtext.com\"], \
			\"helio\" : [\"Helio\", \"{number}@myhelio.com\"], \
			\"iwireless_tmobile\" : [\"i wireless (T-Mobile)\", \"{number}.iws@iwspcs.net\"], \
			\"iwireless_sprint\" : [\"i wireless (Sprint PCS)\", \"{number}@iwirelesshometext.com\"], \
			\"kajeet\" : [\"Kajeet\", \"{number}@mobile.kajeet.net\"], \
			\"longlines\" : [\"LongLines\", \"{number}@text.longlines.com\"], \
			\"metro_pcs\" : [\"Metro PCS\", \"{number}@mymetropcs.com\"], \
			\"nextech\" : [\"Nextech\", \"{number}@sms.nextechwireless.com\"], \
			\"nextel\" : [\"Nextel Direct Connect (Sprint)\", \"{number}@messaging.nextel.com\", \"{number}@page.nextel.com\"], \
			\"pageplus\" : [\"Page Plus Cellular\", \"{number}@vtext.com\"], \
			\"pioneer\" : [\"Pioneer Cellular\", \"{number}@zsend.com\"], \
			\"psc_wireless\" : [\"PSC Wireless\", \"{number}@sms.pscel.com\"], \
			\"rogers_wireless\" : [\"Rogers Wireless\", \"{number}@sms.rogers.com\"], \
			\"qwest\" : [\"Qwest\", \"{number}@qwestmp.com\"], \
			\"simple_mobile\" : [\"Simple Mobile\", \"{number}@smtext.com\"], \
			\"solavei\" : [\"Solavei\", \"{number}@tmomail.net\"], \
			\"south_central\" : [\"South Central Communications\", \"{number}@rinasms.com\"], \
			\"southernlink\" : [\"Southern Link\", \"{number}@page.southernlinc.com\"], \
			\"sprint\" : [\"Sprint PCS (CDMA)\", \"{number}@messaging.sprintpcs.com\"], \
			\"straight_talk\" : [\"Straight Talk\", \"{number}@vtext.com\"], \
			\"syringa\" : [\"Syringa Wireless\", \"{number}@rinasms.com\"], \
			\"t_mobile\" : [\"T-Mobile\", \"{number}@tmomail.net\"], \
			\"teleflip\" : [\"Teleflip\", \"{number}@teleflip.com\"], \
			\"ting\" : [\"Ting\", \"{number}@message.ting.com\"], \
			\"tracfone\" : [\"Tracfone\", \"{number}@mmst5.tracfone.com\"], \
			\"telus_mobility\" : [\"Telus Mobility\", \"{number}@msg.telus.com\"], \
			\"unicel\" : [\"Unicel\", \"{number}@utext.com\"], \
			\"us_cellular\" : [\"US Cellular\", \"{number}@email.uscc.net\"], \
			\"usa_mobility\" : [\"USA Mobility\", \"{number}@usamobility.net\"], \
			\"verizon_wireless\" : [\"Verizon Wireless\", \"{number}@vtext.com\"], \
			\"viaero\" : [\"Viaero\", \"{number}@viaerosms.com\"], \
			\"virgin_mobile\" : [\"Virgin Mobile\", \"{number}@vmobl.com\"], \
			\"voyager_mobile\" : [\"Voyager Mobile\", \"{number}@text.voyagermobile.com\"], \
			\"west_central\" : [\"West Central Wireless\", \"{number}@sms.wcc.net\"], \
			\"xit_comm\" : [\"XIT Communications\", \"{number}@sms.xit.net\"]   \
		}, \
		\"ca\" : {  \
			\"aliant\" : [\"Aliant\", \"{number}@chat.wirefree.ca\"], \
			\"bellmobility\" : [\"Bell Mobility & Solo Mobile\", \"{number}@txt.bell.ca\", \"{number}@txt.bellmobility.ca\"], \
			\"fido\" : [\"Fido\", \"{number}@sms.fido.ca\"], \
			\"koodo\" : [\"Koodo Mobile\", \"{number}@msg.telus.com\"], \
			\"lynx_mobility\" : [\"Lynx Mobility\", \"{number}@sms.lynxmobility.com\"], \
			\"manitoba_telecom\" : [\"Manitoba Telecom/MTS Mobility\", \"{number}@text.mtsmobility.com\"], \
			\"northerntel\" : [\"NorthernTel\", \"{number}@txt.northerntelmobility.com\"], \
			\"pc_telecom\" : [\"PC Telecom\", \"{number}@mobiletxt.ca\"], \
			\"rogers_wireless\" : [\"Rogers Wireless\", \"{number}@sms.rogers.com\"], \
			\"sasktel\" : [\"SaskTel\", \"{number}@sms.sasktel.com\", \"{number}@pcs.sasktelmobility.com\"], \
			\"telebec\" : [\"Telebec\", \"{number}@txt.telebecmobilite.com\"], \
			\"telus\" : [\"Telus Mobility\", \"{number}@msg.telus.com\"], \
			\"virgin\" : [\"Virgin Mobile\", \"{number}@vmobile.ca\"], \
			\"wind_mobile\" : [\"Wind Mobile\", \"{number}@txt.windmobile.ca\"] \
		}, \
		\"ar\" : {  \
			\"claro\" : [\"CTI Movil (Claro)\", \"{number}@sms.ctimovil.com.ar\"], \
			\"movistar\" : [\"Movistar\", \"{number}@sms.movistar.net.ar\"], \
			\"nextel\" : [\"Nextel\", \"TwoWay.{number}@nextel.net.ar\"], \
			\"telecom_personal\" : [\"Telecom (Personal)\", \"{number}@alertas.personal.com.ar\"]   \
		}, \
		\"aw\" : {  \
			\"setar\" : [\"Setar Mobile\", \"{number}@mas.aw\"] \
		}, \
		\"au\" : {  \
			\"t_mobile\" : [\"T-Mobile (Optus Zoo)\", \"{number}@optusmobile.com.au\"]  \
		}, \
		\"be\" : {  \
			\"mobistar\" : [\"Mobistar\", \"{number}@mobistar.be\"] \
		}, \
		\"br\" : {  \
			\"claro\" : [\"Claro\", \"{number}@clarotorpedo.com.br\"], \
			\"viva_sa\" : [\"Vivo\", \"{number}@torpedoemail.com.br\"]  \
		}, \
		\"bg\" : {  \
			\"globul\" : [\"Globul\", \"{number}@sms.globul.bg\"], \
			\"mobiltel\" : [\"Mobiltel\", \"{number}@sms.mtel.net\"]    \
		}, \
		\"cn\" : {  \
			\"china_mobile\" : [\"China Mobile\", \"{number}@139.com\"] \
		}, \
		\"co\" : {  \
			\"comcel\" : [\"Comcel\", \"{number}@comcel.com.co\"], \
			\"movistar\" : [\"Movistar\", \"{number}@movistar.com.co\"] \
		}, \
		\"cz\" : {  \
			\"vodaphone\" : [\"Vodaphone\", \"{number}@vodafonemail.cz\"]   \
		}, \
		\"dm\" : {  \
			\"digicel\" : [\"Digicel\", \"{number}@digitextdm.com\"]    \
		}, \
		\"eg\" : {  \
			\"mobinil\" : [\"Mobinil\", \"{number}@mobinil.net\"], \
			\"vodaphone\" : [\"Vodaphone\", \"{number}@vodafone.com.eg\"]   \
		}, \
		\"fo\" : {  \
			\"foroya_tele\" : [\"Foroya tele\", \"{number}@gsm.fo\"]    \
		}, \
		\"fr\" : {  \
			\"sfr\" : [\"SFR\", \"{number}@sfr.fr\"], \
			\"bouygues\" : [\"Bouygues Telecom\", \"{number}@mms.bouyguestelecom.fr\"]  \
		}, \
		\"de\" : {  \
			\"eplus\" : [\"E-Plus\", \"{number}@smsmail.eplus.de\"], \
			\"o2\" : [\"O2\", \"{number}@o2online.de\"], \
			\"vodaphone\" : [\"Vodaphone\", \"{number}@vodafone-sms.de\"]   \
		}, \
		\"gy\" : {  \
			\"guyana_telephone\" : [\"Guyana Telephone & Telegraph\", \"{number}@sms.cellinkgy.com\"]   \
		}, \
		\"hk\" : {  \
			\"csl\" : [\"CSL\", \"{number}@mgw.mmsc1.hkcsl.com\"]   \
		}, \
		\"is\" : {  \
			\"ogvodafone\" : [\"OgVodafone\", \"{number}@sms.is\"], \
			\"siminn\" : [\"Siminn\", \"{number}@box.is\"]  \
		}, \
		\"in\" : {  \
			\"aircel\" : [\"Aircel\", \"{number}@aircel.co.in\"], \
			\"aircel_tamil_nadu\" : [\"Aircel - Tamil Nadu\", \"{number}@airsms.com\"], \
			\"airtel\" : [\"Airtel\", \"{number}@airtelmail.com\"], \
			\"airtel_andhra_pradesh\" : [\"Airtel - Andhra Pradesh\", \"{number}@airtelap.com\"], \
			\"airtel_chennai\" : [\"Airtel - Chennai\", \"{number}@airtelchennai.com\"], \
			\"airtel_karnataka\" : [\"Airtel - Karnataka\", \"{number}@airtelkk.com\"], \
			\"airtel_kerala\" : [\"Airtel - Kerala\", \"{number}@airtelkerala.com\"], \
			\"airtel_kolkata\" : [\"Airtel - Kolkata\", \"{number}@airtelkol.com\"], \
			\"airtel_tamil_nadu\" : [\"Airtel - Tamil Nadu\", \"{number}@airtelmobile.com\"], \
			\"celforce\" : [\"Celforce\", \"{number}@celforce.com\"], \
			\"escotel\" : [\"Escotel Mobile\", \"{number}@escotelmobile.com\"], \
			\"idea_cellular\" : [\"Idea Cellular\", \"{number}@ideacellular.net\"], \
			\"loop\" : [\"Loop (BPL Mobile)\", \"{number}@loopmobile.co.in\"], \
			\"orange\" : [\"Orange\", \"{number}@orangemail.co.in\"]    \
		}, \
		\"ie\" : {  \
			\"meteor\" : [\"Meteor\", \"{number}@sms.mymeteor.ie\"] \
		}, \
		\"il\" : {  \
			\"spikko\" : [\"Spikko\", \"{number}@spikkosms.com\"]   \
		}, \
		\"it\" : {  \
			\"vodaphone\" : [\"Vodaphone\", \"{number}@sms.vodafone.it\"]   \
		}, \
		\"jp\" : {  \
			\"au\" : [\"AU by KDDI\", \"{number}@ezweb.ne.jp\"], \
			\"ntt\" : [\"NTT DoCoMo\", \"{number}@docomo.ne.jp\"], \
			\"vodaphone_chuugoku\" : [\"Vodaphone - Chuugoku/Western\", \"{number}@n.vodafone.ne.jp\"], \
			\"vodaphone_hokkaido\" : [\"Vodaphone - Hokkaido\", \"{number}@d.vodafone.ne.jp\"], \
			\"vodaphone_hokuriku\" : [\"Vodaphone - Hokuriku/Central North\", \"{number}@r.vodafone.ne.jp\"], \
			\"vodaphone_kansai\" : [\"Vodaphone - Kansai/West\", \"{number}@k.vodafone.ne.jp\"], \
			\"vodaphone_kanto\" : [\"Vodaphone - Kanto\", \"{number}@k.vodafone.ne.jp\"], \
			\"vodaphone_koushin\" : [\"Vodaphone - Koushin\", \"{number}@k.vodafone.ne.jp\"], \
			\"vodaphone_kyuushu\" : [\"Vodaphone - Kyuushu\", \"{number}@q.vodafone.ne.jp\"], \
			\"vodaphone_niigata\" : [\"Vodaphone - Niigata/North\", \"{number}@h.vodafone.ne.jp\"], \
			\"vodaphone_okinawa\" : [\"Vodaphone - Okinawa\", \"{number}@q.vodafone.ne.jp\"], \
			\"vodaphone_osaka\" : [\"Vodaphone - Osaka\", \"{number}@k.vodafone.ne.jp\"], \
			\"vodaphone_shikoku\" : [\"Vodaphone - Shikoku\", \"{number}@s.vodafone.ne.jp\"], \
			\"vodaphone_tokyo\" : [\"Vodaphone - Tokyo\", \"{number}@k.vodafone.ne.jp\"], \
			\"vodaphone_touhoku\" : [\"Vodaphone - Touhoku\", \"{number}@h.vodafone.ne.jp\"], \
			\"vodaphone_toukai\" : [\"Vodaphone - Toukai\", \"{number}@h.vodafone.ne.jp\"], \
			\"willcom\" : [\"Willcom\", \"{number}@pdx.ne.jp\"] \
		}, \
		\"mu\" : {  \
			\"emtel\" : [\"Emtel\", \"{number}@emtelworld.net\"]    \
		}, \
		\"mx\" : {  \
			\"nextel\" : [\"Nextel\", \"{number}@msgnextel.com.mx\"]    \
		}, \
		\"np\" : {  \
			\"ncell\" : [\"Ncell\", \"{number}@sms.ncell.com.np\"]  \
		}, \
		\"nl\" : {  \
			\"orange\" : [\"Orange\", \"{number}@sms.orange.nl\"], \
			\"t_mobile\" : [\"T-Mobile\", \"{number}@gin.nl\"]  \
		}, \
		\"nz\" : {  \
			\"telecom\" : [\"Telecom\", \"{number}@etxt.co.nz\"], \
			\"vodafone\" : [\"Vodafone\", \"{number}@mtxt.co.nz\"]  \
		}, \
		\"pa\" : {  \
			\"mas_movil\" : [\"Mas Movil\", \"{number}@cwmovil.com\"]   \
		}, \
		\"pl\" : {  \
			\"orange_polska\" : [\"Orange Polska\", \"{number}@sms.orange.pl\"], \
			\"polkomtel\" : [\"Polkomtel\", \"+{number}@text.plusgsm.pl\"], \
			\"plus\" : [\"Plus\", \"+{number}@text.plusgsm.pl\"]    \
		}, \
		\"pr\" : {  \
			\"claro\" : [\"Claro\", \"{number}@vtexto.com\"]    \
		}, \
		\"ru\" : {  \
			\"beeline\" : [\"Beeline\", \"{number}@sms.beemail.ru\"]    \
		}, \
		\"sg\" : {  \
			\"m1\" : [\"M1\", \"{number}@m1.com.sg\"]   \
		}, \
		\"za\" : {  \
			\"mtn\" : [\"MTN\", \"{number}@sms.co.za\"], \
			\"vodacom\" : [\"Vodacom\", \"{number}@voda.co.za\"]    \
		}, \
		\"kr\" : {  \
			\"helio\" : [\"Helio\", \"{number}@myhelio.com\"]   \
		}, \
		\"es\" : {  \
			\"esendex\" : [\"Esendex\", \"{number}@esendex.net\"], \
			\"movistar\" : [\"Movistar/Telefonica\", \"{number}@movistar.net\", \"{number}@correo.movistar.net\", \"{number}@movimensaje.com.ar\"], \
			\"telefonica\" : [\"Telefonica\"], \
			\"vodaphone\" : [\"Vodaphone\", \"{number}@vodafone.es\"]   \
		}, \
		\"lk\" : {  \
			\"mobitel\" : [\"Mobitel\", \"{number}@sms.mobitel.lk\"]    \
		}, \
		\"se\" : {  \
			\"tele2\" : [\"Tele2\", \"{number}@sms.tele2.se\"]  \
		}, \
		\"ch\" : {  \
			\"sunrise\" : [\"Sunrise Communications\", \"{number}@gsm.sunrise.ch\"] \
		}, \
		\"ua\" : {  \
			\"Beeline\" : [\"Beeline\", \"{number}@sms.beeline.ua\"]    \
		}, \
		\"uk\" : {  \
			\"aql\" : [\"aql\", \"{number}@text.aql.com\"], \
			\"esendex\" : [\"Esendex\", \"{number}@echoemail.net\"], \
			\"hay_systems\" : [\"HSL Mobile (Hay Systems Ltd)\", \"{number}@sms.haysystems.com\"], \
			\"o2\" : [\"O2\", \"{number}@mmail.co.uk\"], \
			\"orange\" : [\"Orange\", \"{number}@orange.net\"], \
			\"t_mobile\" : [\"T-Mobile\", \"{number}@t-mobile.uk.net\"], \
			\"txtlocal\" : [\"Txtlocal\", \"{number}@txtlocal.co.uk\"], \
			\"unimovil\" : [\"UniMovil Corporation\", \"{number}@viawebsms.com\"]   \
		}, \
		\"uy\" : {  \
			\"movistar\" : [\"Movistar\", \"{number}@sms.movistar.com.uy\"] \
		}, \
		\"europe\" : {  \
			\"tellustalk\" : [\"TellusTalk\", \"{number}@esms.nu\"] \
		}, \
		\"latin_america\" : {   \
			\"movistar\" : [\"Movistar\", \"{number}@movimensaje.com.ar\"]  \
		}, \
		\"int\" : { \
			\"globalstar\" : [\"Globalstar satellite\", \"{number}@msg.globalstarusa.com\"], \
			\"iridium\" : [\"Iridium satellite\", \"{number}@msg.iridium.com\"] \
		}   \
	}, \
	\"mms_carriers\" : {    \
		\"us\" : {  \
			\"alltel\" : [\"Alltel\", \"{number}@mms.alltel.com\"], \
			\"alltel_allied\" : [\"Alltel (Allied Wireless)\", \"{number}@mms.alltelwireless.com\"], \
			\"at_and_t\" : [\"AT&T Wireless\", \"{number}@mms.att.net\"], \
			\"bluegrass\" : [\"Bluegrass Cellular\", \"{number}@mms.bluecell.com\"], \
			\"boostmobile\" : [\"Boost Mobile\", \"{number}@myboostmobile.com\"], \
			\"cincinnati_bell\" : [\"Cincinnati Bell\", \"{number}@mms.gocbw.com\"], \
			\"cricket\" : [\"Cricket\", \"{number}@mms.mycricket.com\"], \
			\"nextel\" : [\"Nextel (Sprint)\", \"{number}@messaging.nextel.com\"], \
			\"pageplus\" : [\"Page Plus Cellular\", \"{number}@vzwpix.com\", \"{number}@mypixmessages.com\"], \
			\"rogers_wireless\" : [\"Rogers Wireless\", \"{number}@mms.rogers.com\"], \
			\"solavei\" : [\"Solavei\", \"{number}@tmomail.net\"], \
			\"sprint\" : [\"Sprint PCS\", \"{number}@pm.sprint.com\"], \
			\"straight_talk\" : [\"Straight Talk\", \"{number}@mypixmessages.com\"], \
			\"t_mobile\" : [\"T-Mobile\", \"{number}@tmomail.net\"], \
			\"telus_mobility\" : [\"Telus Mobility\", \"{number}@mms.telusmobility.com\"], \
			\"tracfone\" : [\"Tracfone\", \"{number}@mmst5.tracfone.com\"], \
			\"us_cellular\" : [\"US Cellular\", \"{number}@mms.uscc.net\"], \
			\"verizon_wireless\" : [\"Verizon Wireless\", \"{number}@vzwpix.com\", \"{number}@mypixmessages.com\"], \
			\"viaero\" : [\"Viaero\", \"{number}@mmsviaero.com\"], \
			\"virgin_mobile\" : [\"Virgin Mobile\", \"{number}@vmpix.com\"] \
		}, \
		\"ca\" : {  \
			\"rogers_wireless\" : [\"Rogers Wireless\", \"{number}@mms.rogers.com\"], \
			\"telus_mobility\" : [\"Telus Mobility\", \"{number}@mms.telusmobility.com\"]   \
		}, \
		\"fr\" : {  \
			\"bouygues\" : [\"Bouygues Telecom\", \"{number}@mms.bouyguestelecom.fr\"]  \
		}, \
		\"hk\" : {  \
			\"csl\" : [\"CSL\", \"{number}@mgw.mmsc1.hkcsl.com\"]   \
		}   \
	}   \
}";

struct json_object* sms_carrier_object;
carrier_data_t*  carrier_data;
hash_table_t*    phone_table;

void    sms_data_load()
{
    sms_carrier_object = json_tokener_parse(sms_carrier_json);
}

const char**  sms_data_get_country_code_list()
{
    const char** country_code_array;
    struct json_object* countries;
    if(json_object_object_get_ex(sms_carrier_object, "countries", &countries) == 1)
    {
        int length = json_object_object_length(countries);

        //printf("Country code list len: %d\n", length);
        country_code_array = calloc(length+1, sizeof(char*));
        int idx = 0;
        json_object_object_foreach(countries, key, value)
        {
            country_code_array[idx] = key;
            ++idx;
        }
    }

    return country_code_array;
}

const char**  sms_data_get_carrier_list(const char* country_code)
{
    const char** carrier_array;
    struct json_object* carriers;
    if(json_object_object_get_ex(sms_carrier_object, "sms_carriers", &carriers) == 1)
    {
        struct json_object* carriers_by_country;
        if(json_object_object_get_ex(carriers, country_code, &carriers_by_country) == 1)
        {
            int length = json_object_object_length(carriers_by_country);
            //printf("Carrier list len: %d\n", length);
            carrier_array = calloc(length+1, sizeof(char*));
            int idx = 0;
            json_object_object_foreach(carriers_by_country, key, value)
            {
                carrier_array[idx] = key;
                ++idx;
            }

            //json_object_put(carriers_by_country);
        }
        //json_object_put(carriers);
    }

    return carrier_array;
}

const char*   sms_data_get_sms_gw(const char* country_code, const char* carrier)
{
    struct json_object* carriers;
    if(json_object_object_get_ex(sms_carrier_object, "sms_carriers", &carriers) == 1)
    {
        struct json_object* carriers_by_country;
        if(json_object_object_get_ex(carriers, country_code, &carriers_by_country) == 1)
        {
            struct json_object* sms_gw;
            if(json_object_object_get_ex(carriers_by_country, carrier, &sms_gw) == 1)
            {
                struct json_object* gw_data;
                gw_data = json_object_array_get_idx(sms_gw, 1);

                const char* sms_gw_email = json_object_get_string(gw_data);
                return strchr(sms_gw_email, '@');
            }
        }
    }

    return NULL;
}

void    sms_data_set_carrier(char* country_code, char* carrier)
{
    free(carrier_data->country_code);
    free(carrier_data->carrier);

    carrier_data->country_code = country_code;
    carrier_data->carrier = carrier;
}

carrier_data_t* sms_data_get_carrier()
{
    return carrier_data;
}

void    sms_data_add_phone(const char* number)
{
    variant_hash_insert(phone_table, crc32(0, number, strlen(number)), variant_create_string(number));
}

void    sms_data_del_phone(const char* number)
{
    variant_hash_remove(phone_table, crc32(0, number, strlen(number)));
}

