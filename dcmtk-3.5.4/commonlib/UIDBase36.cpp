#include "StdAfx.h"
#include "UIDBase36.h"
using namespace std;

set<size_t> UIDBase36::MATCH_HEADER_LENGTH;
MapString2Int UIDBase36::uid2index;
MapInt2String UIDBase36::index2uid;
volatile LONG UIDBase36::REF_COUNTER = 0;
volatile bool UIDBase36::INIT_OK = false;
UIDBase36 UIDBase36::instance;

UIDBase36::UIDBase36(void)
{
	if(1 == InterlockedIncrement(&UIDBase36::REF_COUNTER))
	{
		// class init
		int code = 10 * 11 * 11;
		AddStaticDictDirect("1.3.46.670589.", --code);			//Philips
		AddStaticDictDirect("1.2.840.113619.", --code);			//GE
		AddStaticDictDirect("1.3.12.2.1107.", --code);			//Siemens
		AddStaticDictDirect("1.2.392.200036.9110.", --code);	//㈱島津製作所
		AddStaticDictDirect("1.2.392.200036.9111.", --code);	//GE ヘルスケア.ジャパン(株)（旧 ＧＥ横河）
		AddStaticDictDirect("1.2.392.200036.9117.", --code);	//東芝医用システム（旧 東芝メディカル）
		AddStaticDictDirect("1.2.392.200036.9125.", --code);	//FujiFilm Synapse
		AddStaticDictDirect("1.2.392.200046.100.", --code);		//Canon
		AddStaticDictDirect("1.2.392.200109.", --code);			//Kicnet
		AddStaticDictDirect("1.2.528.1.1001.", --code);			//GE Medical Systems Nederland BV
		AddStaticDictDirect("1.2.156.10011.", --code);			//卫生部信息中心
		AddStaticDictDirect("1.2.156.20002.", --code);			//中国互联网络信息中心
		AddStaticDictDirect("2.16.156.6879.", --code);			//Healthcare in China
		AddStaticDictDirect("1.2.156.112606.", --code);			//北京大学医学院
		AddStaticDictDirect("2.16.156.112557.", --code);		//用友医疗卫生信息系统有限公司
		AddStaticDictDirect("1.2.156.112548.", --code);			//北京亿仁赛博医疗设备公司 
		AddStaticDictDirect("1.2.392.200036.", --code);			//JIRA
		AddStaticDictDirect("1.3.12.2.", --code);				//ECMA Member Company
        AddStaticDictDirect("1.2.840.887072.", --code);         //北京同仁医院XA，厂商未知
		//AddStaticDictDirect("1.2.156.60004855.", --code);		//
		//AddStaticDictDirect("1.2.840.17998.", --code);			//

		int fallback = 3 * 11 * 11 - 1;
		AddStaticDictDirect("1.2.", ++fallback);				//ISO Member Bodies
		AddStaticDictDirect("1.3.", ++fallback);				//ISO Identified Organization
 		AddStaticDictDirect("1.2.36.", ++fallback);				//Australia
 		AddStaticDictDirect("1.2.40.", ++fallback);				//Austria
 		AddStaticDictDirect("1.2.56.", ++fallback);				//Belgium
		AddStaticDictDirect("1.2.156.", ++fallback);			//CHINA
		AddStaticDictDirect("2.16.156.", ++fallback);			//CHINA
 		AddStaticDictDirect("1.2.158.", ++fallback);			//Taiwan object identifiers
 		AddStaticDictDirect("1.2.203.", ++fallback);			//Czech Republic
 		AddStaticDictDirect("1.2.208.", ++fallback);			//Denmark
 		AddStaticDictDirect("1.2.246.", ++fallback);			//Finland
 		AddStaticDictDirect("1.2.250.", ++fallback);			//France
		AddStaticDictDirect("1.2.276.", ++fallback);			//GERMAN
 		AddStaticDictDirect("1.2.280.", ++fallback);			//Germany (in German, Bundesrepublik Deutschland)
 		AddStaticDictDirect("1.2.300.", ++fallback);			//Greece
 		AddStaticDictDirect("1.2.344.", ++fallback);			//Hong Kong Special Administrative Region of China
 		AddStaticDictDirect("1.2.372.", ++fallback);			//Ireland
		AddStaticDictDirect("1.2.392.", ++fallback);			//JAPAN
 		AddStaticDictDirect("1.2.398.", ++fallback);			//Kazakhstan
 		AddStaticDictDirect("1.2.410.", ++fallback);			//Republic of Korea
 		AddStaticDictDirect("1.2.498.", ++fallback);			//Moldova (Republic of)
 		AddStaticDictDirect("1.2.528.", ++fallback);			//Nederlands
 		AddStaticDictDirect("1.2.566.", ++fallback);			//Nigeria
 		AddStaticDictDirect("1.2.578.", ++fallback);			//Norway
 		AddStaticDictDirect("1.2.616.", ++fallback);			//Poland
 		AddStaticDictDirect("1.2.643.", ++fallback);			//Russian Federation
 		AddStaticDictDirect("1.2.702.", ++fallback);			//Singapore
 		AddStaticDictDirect("1.2.704.", ++fallback);			//Vietnam
 		AddStaticDictDirect("1.2.752.", ++fallback);			//Sweden
 		AddStaticDictDirect("1.2.804.", ++fallback);			//Ukraine
 		AddStaticDictDirect("1.2.826.", ++fallback);			//United Kingdom
		AddStaticDictDirect("1.2.840.", ++fallback);			//USA
 		AddStaticDictDirect("1.2.860.", ++fallback);			//Uzbekistan
 		AddStaticDictDirect("1.2.862.", ++fallback);			//Venezuela

		AddStaticDictDirect("1.3.2.", ++fallback);			//System Information et Repertoire des Entreprise et des...
 		AddStaticDictDirect("1.3.3.", ++fallback);			//Codification Numérique des Etablissements Financiers E...
 		AddStaticDictDirect("1.3.4.", ++fallback);			//NBS/OSI NETWORK
 		AddStaticDictDirect("1.3.5.", ++fallback);			//National Institute of Standards and Technology (...
 		AddStaticDictDirect("1.3.6.", ++fallback);			//United States Department of Defense (DoD)
 		AddStaticDictDirect("1.3.7.", ++fallback);			//Organisationsnummer
 		AddStaticDictDirect("1.3.8.", ++fallback);			//LE NUMERO NATIONAL
 		AddStaticDictDirect("1.3.9.", ++fallback);			//SIRET-CODE
 		AddStaticDictDirect("1.3.10.", ++fallback);			//Organizational Identifiers for Structured Names under...
 		AddStaticDictDirect("1.3.11.", ++fallback);			//International Code Designator for the Identification ...
 		AddStaticDictDirect("1.3.12.", ++fallback);			//European Computer Manufacturer's...
 		AddStaticDictDirect("1.3.13.", ++fallback);			//VSA FTP CODE (FTP = File Transfer Protocol)
 		AddStaticDictDirect("1.3.14.", ++fallback);			//NIST/OSI Implementors' Workshop (OIW)
 		AddStaticDictDirect("1.3.15.", ++fallback);			//Electronic Data Interchange (EDI)
 		AddStaticDictDirect("1.3.16.", ++fallback);			//European Workshop on Open Systems (EWOS)
 		AddStaticDictDirect("1.3.17.", ++fallback);			//COMMON LANGUAGE
 		AddStaticDictDirect("1.3.18.", ++fallback);			//Systems Network Architecture/Open Systems Interconnec...
 		AddStaticDictDirect("1.3.19.", ++fallback);			//Air Transport Industry Services Communications Networ...
 		AddStaticDictDirect("1.3.20.", ++fallback);			//European Laboratory for Particle Physics: CERN
 		AddStaticDictDirect("1.3.21.", ++fallback);			//SOCIETY FOR WORLDWIDE INTERBANK FINANCIAL TELECOMMUNI...
 		AddStaticDictDirect("1.3.22.", ++fallback);			//Open Software Foundation (OSF) Distributed Compu...
 		AddStaticDictDirect("1.3.23.", ++fallback);			//Nordic University and Research Network: NOR...
 		AddStaticDictDirect("1.3.24.", ++fallback);			//Digital Equipment Corporation: DEC
 		AddStaticDictDirect("1.3.25.", ++fallback);			//OSI ASIA-OCEANIA WORKSHOP
 		AddStaticDictDirect("1.3.26.", ++fallback);			//NATO ISO6523 ICDE coding sche...
 		AddStaticDictDirect("1.3.27.", ++fallback);			//International Civil Aviation Organization (ICAO...
 		AddStaticDictDirect("1.3.28.", ++fallback);			//International Standard ISO 6523
 		AddStaticDictDirect("1.3.29.", ++fallback);			//The All-Union Classifier of Enterprises and Organisat...
 		AddStaticDictDirect("1.3.30.", ++fallback);			//AT&T/OSI Network
 		AddStaticDictDirect("1.3.31.", ++fallback);			//EDI Partner Identification Code
 		AddStaticDictDirect("1.3.32.", ++fallback);			//Telecom Australia
 		AddStaticDictDirect("1.3.33.", ++fallback);			//S G W OSI Internetwork
 		AddStaticDictDirect("1.3.34.", ++fallback);			//Reuter Open Address Standard
 		AddStaticDictDirect("1.3.35.", ++fallback);			//ISO 6523 - ICD
 		AddStaticDictDirect("1.3.36.", ++fallback);			//TeleTrusT - IT Security Association German...
 		AddStaticDictDirect("1.3.37.", ++fallback);			//LY-tunnus
 		AddStaticDictDirect("1.3.38.", ++fallback);			//The Australian GOSIP Network
 		AddStaticDictDirect("1.3.39.", ++fallback);			//The OZ DOD OSI Network
 		AddStaticDictDirect("1.3.40.", ++fallback);			//Unilever Group Companies
 		AddStaticDictDirect("1.3.41.", ++fallback);			//Citicorp Global Information Network
 		AddStaticDictDirect("1.3.42.", ++fallback);			//DBP Telekom Object Identifiers
 		AddStaticDictDirect("1.3.43.", ++fallback);			//HydroNETT
 		AddStaticDictDirect("1.3.44.", ++fallback);			//Thai Industrial Standards Institute (TISI)
 		AddStaticDictDirect("1.3.45.", ++fallback);			//ICI Company Identification System
		AddStaticDictDirect("1.3.46.", ++fallback);			//Philips FUNLOC
 		AddStaticDictDirect("1.3.47.", ++fallback);			//BULL ODI/DSA/UNIX Network
 		AddStaticDictDirect("1.3.48.", ++fallback);			//OSINZ
 		AddStaticDictDirect("1.3.49.", ++fallback);			//Auckland Area Health
 		AddStaticDictDirect("1.3.50.", ++fallback);			//Firmenich
 		AddStaticDictDirect("1.3.51.", ++fallback);			//AGFA-DIS
 		AddStaticDictDirect("1.3.52.", ++fallback);			//Society of Motion Picture and Television Engin...
 		AddStaticDictDirect("1.3.53.", ++fallback);			//Migros_Network M_NETOPZ
 		AddStaticDictDirect("1.3.54.", ++fallback);			//ISO6523 - ICDPCR
 		AddStaticDictDirect("1.3.55.", ++fallback);			//Energy Net
 		AddStaticDictDirect("1.3.56.", ++fallback);			//Nokia Object Identifiers (NOI)
 		AddStaticDictDirect("1.3.57.", ++fallback);			//Saint Gobain
 		AddStaticDictDirect("1.3.58.", ++fallback);			//Siemens Corporate Network
 		AddStaticDictDirect("1.3.59.", ++fallback);			//DANZNET
 		AddStaticDictDirect("1.3.60.", ++fallback);			//Data Universal Numbering System (D-U-N-S Number...
 		AddStaticDictDirect("1.3.61.", ++fallback);			//SOFFEX OSI
 		AddStaticDictDirect("1.3.62.", ++fallback);			//KPN OVN
 		AddStaticDictDirect("1.3.63.", ++fallback);			//ascomOSINet
 		AddStaticDictDirect("1.3.64.", ++fallback);			//UTC: Uniforme Transport Code
 		AddStaticDictDirect("1.3.65.", ++fallback);			//SOLVAY OSI CODING
 		AddStaticDictDirect("1.3.66.", ++fallback);			//Roche Corporate Network
 		AddStaticDictDirect("1.3.67.", ++fallback);			//ZellwegerOSINet
 		AddStaticDictDirect("1.3.68.", ++fallback);			//Intel Corporation OSI
 		AddStaticDictDirect("1.3.69.", ++fallback);			//SITA (Societe Internationale de Telecommunicati...
 		AddStaticDictDirect("1.3.70.", ++fallback);			//DaimlerChrysler Corporate Network
 		AddStaticDictDirect("1.3.71.", ++fallback);			//LEGO /OSI NETWORK
 		AddStaticDictDirect("1.3.72.", ++fallback);			//NAVISTAR/OSI Network
 		AddStaticDictDirect("1.3.73.", ++fallback);			//ICD Formatted ATM address
 		AddStaticDictDirect("1.3.74.", ++fallback);			//ARINC
 		AddStaticDictDirect("1.3.75.", ++fallback);			//Alcanet/Alcatel-Alsthom Corporate Network
 		AddStaticDictDirect("1.3.76.", ++fallback);			//Sistema Italiano di Identificazione di ogetti...
 		AddStaticDictDirect("1.3.77.", ++fallback);			//Sistema Italiano di Indirizzamento di Reti OSI Gestit...
 		AddStaticDictDirect("1.3.78.", ++fallback);			//Mitel terminal or switching equipment
 		AddStaticDictDirect("1.3.79.", ++fallback);			//ATM Forum
 		AddStaticDictDirect("1.3.80.", ++fallback);			//UK National Health Service Scheme (EDIRA compliant)
 		AddStaticDictDirect("1.3.81.", ++fallback);			//International NSAP
 		AddStaticDictDirect("1.3.82.", ++fallback);			//Norwegian Telecommunications Authority's, NTA'S, EDI,...
 		AddStaticDictDirect("1.3.83.", ++fallback);			//Advanced Telecommunications Modules Limited Corporate...
 		AddStaticDictDirect("1.3.84.", ++fallback);			//Athens Chamber of Commerce & Industry Scheme (EDIRA c...
 		AddStaticDictDirect("1.3.85.", ++fallback);			//Swisskey Certificate Authority coding system
 		AddStaticDictDirect("1.3.86.", ++fallback);			//United States Council for International Business (USC...
 		AddStaticDictDirect("1.3.87.", ++fallback);			//National Federation of Chambers of Commerce & Industr...
 		AddStaticDictDirect("1.3.88.", ++fallback);			//Global Location Number (GLN) (previously, Europ...
 		AddStaticDictDirect("1.3.89.", ++fallback);			//The Association of British Chambers of Commerce Ltd. ...
 		AddStaticDictDirect("1.3.90.", ++fallback);			//Internet IP addressing - ISO 6523 ICD encoding ...
 		AddStaticDictDirect("1.3.91.", ++fallback);			//Cisco Systems / OSI Network
 		AddStaticDictDirect("1.3.92.", ++fallback);			//Not to be assigned (see additional comments)
 		AddStaticDictDirect("1.3.93.", ++fallback);			//Revenue Canada Business Number Registration (EDIRA co...
 		AddStaticDictDirect("1.3.94.", ++fallback);			//DEUTSCHER INDUSTRIE- UND HANDELSTAG (DIHT) Scheme (ED...
 		AddStaticDictDirect("1.3.95.", ++fallback);			//Hewlett - Packard Company Internal AM Network
 		AddStaticDictDirect("1.3.96.", ++fallback);			//DANISH CHAMBER OF COMMERCE Scheme (EDIRA compliant)
 		AddStaticDictDirect("1.3.97.", ++fallback);			//FTI - Ediforum Italia (EDIRA compliant)
 		AddStaticDictDirect("1.3.98.", ++fallback);			//CHAMBER OF COMMERCE TEL AVIV-JAFFA Scheme (EDIRA comp...
 		AddStaticDictDirect("1.3.99.", ++fallback);			//Siemens Supervisory Systems Network
 		AddStaticDictDirect("1.3.100.", ++fallback);		//PNG_ICD Scheme
 		AddStaticDictDirect("1.3.101.", ++fallback);		//South African Code Allocation
 		AddStaticDictDirect("1.3.102.", ++fallback);		//HEAG
 		AddStaticDictDirect("1.3.103.", ++fallback);		//(Reserved for later allocation)
 		AddStaticDictDirect("1.3.104.", ++fallback);		//BT - ICD Coding System
 		AddStaticDictDirect("1.3.105.", ++fallback);		//Portuguese Chamber of Commerce and Industry Scheme (...
 		AddStaticDictDirect("1.3.106.", ++fallback);		//Vereniging van Kamers van Koophandel en Fabrieken in...
 		AddStaticDictDirect("1.3.107.", ++fallback);		//Association of Swedish Chambers of Commerce and Indu...
 		AddStaticDictDirect("1.3.108.", ++fallback);		//Australian Chambers of Commerce and Industry Scheme ...
 		AddStaticDictDirect("1.3.109.", ++fallback);		//BellSouth ICD AESA (ATM End System Address)
 		AddStaticDictDirect("1.3.110.", ++fallback);		//Bell Atlantic
 		AddStaticDictDirect("1.3.111.", ++fallback);		//IEEE
 		AddStaticDictDirect("1.3.112.", ++fallback);		//ISO register for Standards Producing Organizations
 		AddStaticDictDirect("1.3.113.", ++fallback);		//OriginNet
 		AddStaticDictDirect("1.3.114.", ++fallback);		//Check Point Software Technologies
 		AddStaticDictDirect("1.3.115.", ++fallback);		//Pacific Bell Data Communications Network
 		AddStaticDictDirect("1.3.116.", ++fallback);		//PSS Object Identifiers
 		AddStaticDictDirect("1.3.117.", ++fallback);		//STENTOR-ICD CODING SYSTEM
 		AddStaticDictDirect("1.3.118.", ++fallback);		//ATM-Network ZN’96
 		AddStaticDictDirect("1.3.119.", ++fallback);		//MCI / OSI Network
 		AddStaticDictDirect("1.3.120.", ++fallback);		//Advantis
 		AddStaticDictDirect("1.3.121.", ++fallback);		//Affable Software Data Interchange Codes
 		AddStaticDictDirect("1.3.122.", ++fallback);		//BB-DATA GmbH
 		AddStaticDictDirect("1.3.123.", ++fallback);		//BASF Company ATM-Network
 		AddStaticDictDirect("1.3.124.", ++fallback);		//Identifiers for Organizations for Telecommunications...
 		AddStaticDictDirect("1.3.125.", ++fallback);		//Henkel Corporate Network (H-Net)
 		AddStaticDictDirect("1.3.126.", ++fallback);		//GTE/OSI Network
 		AddStaticDictDirect("1.3.127.", ++fallback);		//Allianz Managed Operations and Services SE (previou...
 		AddStaticDictDirect("1.3.128.", ++fallback);		//BCNR (Swiss Clearing Bank Number)
 		AddStaticDictDirect("1.3.129.", ++fallback);		//BPI (Telekurs Business Partner Identification)
 		AddStaticDictDirect("1.3.130.", ++fallback);		//European Commission
 		AddStaticDictDirect("1.3.131.", ++fallback);		//Code for the Identification of National Organization...
 		AddStaticDictDirect("1.3.132.", ++fallback);		//Certicom Object Identifiers
 		AddStaticDictDirect("1.3.133.", ++fallback);		//ISO/TC 68 (committee designated to develop int...
 		AddStaticDictDirect("1.3.134.", ++fallback);		//Infonet Services Corporation
 		AddStaticDictDirect("1.3.135.", ++fallback);		//Allocated to the italian company SIA (Societa Interb...
 		AddStaticDictDirect("1.3.136.", ++fallback);		//Cable & Wireless Global ATM End-System Address Plan
 		AddStaticDictDirect("1.3.137.", ++fallback);		//Global AESA scheme
 		AddStaticDictDirect("1.3.138.", ++fallback);		//France Telecom ATM End System Address Plan
 		AddStaticDictDirect("1.3.139.", ++fallback);		//Savvis Communications AESA
 		AddStaticDictDirect("1.3.140.", ++fallback);		//Toshiba Organizations, Partners, And Suppliers’ (TOP...
 		AddStaticDictDirect("1.3.141.", ++fallback);		//NATO Commercial and Government Entity system
 		AddStaticDictDirect("1.3.142.", ++fallback);		//SECETI S.p.A.
 		AddStaticDictDirect("1.3.143.", ++fallback);		//EINESTEINet AG
 		AddStaticDictDirect("1.3.144.", ++fallback);		//DoDAAC (Department of Defense Activity Address Code)
 		AddStaticDictDirect("1.3.145.", ++fallback);		//DGCP (Direction Générale de la Comptabilité Publique...
 		AddStaticDictDirect("1.3.146.", ++fallback);		//DGI (Direction Générale des Impots) code
 		AddStaticDictDirect("1.3.147.", ++fallback);		//Standard Company Code
 		AddStaticDictDirect("1.3.148.", ++fallback);		//ITU (International Telecommunications Union) D...
 		AddStaticDictDirect("1.3.149.", ++fallback);		//Global Business Identifier
 		AddStaticDictDirect("1.3.150.", ++fallback);		//Madge Networks Ltd- ICD ATM Addressing Scheme
 		AddStaticDictDirect("1.3.151.", ++fallback);		//Australian Business Number (ABN) Scheme
 		AddStaticDictDirect("1.3.152.", ++fallback);		//Edira Scheme Identifier Code
 		AddStaticDictDirect("1.3.153.", ++fallback);		//Concert Global Network Services ICD AESA
 		AddStaticDictDirect("1.3.154.", ++fallback);		//Identification number of economic subjects (ICO...
 		AddStaticDictDirect("1.3.155.", ++fallback);		//Global Crossing AESA (ATM End System Address)
 		AddStaticDictDirect("1.3.156.", ++fallback);		//AUNA
 		AddStaticDictDirect("1.3.157.", ++fallback);		//ITO Drager Net Intended purpose/application area: A...
 		AddStaticDictDirect("1.3.158.", ++fallback);		//Identification number of economic subject (I?O) Act ...
 		AddStaticDictDirect("1.3.159.", ++fallback);		//ACTALIS
 		AddStaticDictDirect("1.3.160.", ++fallback);		//Global Trade Item Number (GTIN) The GS1 GTIN ...
 		AddStaticDictDirect("1.3.161.", ++fallback);		//ECCMA Open Technical Dictionary A centralized dicti...
 		AddStaticDictDirect("1.3.162.", ++fallback);		//CEN/ISSS Object Identifier Scheme To allocate OIDs ...
 		AddStaticDictDirect("1.3.163.", ++fallback);		//US-EPA Facility Identifier To provide for the uniqu...
 		AddStaticDictDirect("1.3.164.", ++fallback);		//TELUS Corporation AESA Addressing Scheme for...
 		AddStaticDictDirect("1.3.165.", ++fallback);		//FIEIE Object identifiers To provide identifiers for...
 		AddStaticDictDirect("1.3.166.", ++fallback);		//Swissguide Identifier Scheme To uniquely identify o...
 		AddStaticDictDirect("1.3.167.", ++fallback);		//Priority Telecom ATM End System Address Plan
 		AddStaticDictDirect("1.3.168.", ++fallback);		//Vodafone Ireland OSI Addressing
 		AddStaticDictDirect("1.3.169.", ++fallback);		//Swiss Federal Business Identification Number Central...
 		AddStaticDictDirect("1.3.170.", ++fallback);		//Teikoku Company Code
 		AddStaticDictDirect("1.3.171.", ++fallback);		//Luxembourg CP & CPS (Certification Policy and Certif...
 		AddStaticDictDirect("1.3.172.", ++fallback);		//Project Group "Lists of Properties" (PROLIST?)
 		AddStaticDictDirect("1.3.173.", ++fallback);		//eCl@ss
 		AddStaticDictDirect("1.3.175.", ++fallback);		//Siemens AG
 		AddStaticDictDirect("1.3.177.", ++fallback);		//Odette International
 		AddStaticDictDirect("1.3.178.", ++fallback);		//Route1's MobiNET
 		AddStaticDictDirect("1.3.179.", ++fallback);		//Penango, Inc.
 		AddStaticDictDirect("1.3.180.", ++fallback);		//Lithuanian military Public Key Infrastructure (PKI)
 		AddStaticDictDirect("1.3.183.", ++fallback);		//Unique Identification Business Number (UIDB) (in Fre...

		if(code < fallback)
		{
			ostringstream strbuf;
			strbuf << "ERROR: static dictionary overflow, code: " << code << ", fallback: " << fallback;
			string errMsg(strbuf.str());
			strbuf.clear();
			cerr << errMsg << endl;
			throw errMsg.c_str();
		}
		INIT_OK = true;
	}
	else
	{
		while(!INIT_OK) Sleep(10);
	}
	// instace init
}

size_t UIDBase36::compress(const string &uid, char *outputBuffer, size_t bufLen)
{
	if(uid.length() == 0 || uid.length() > UID_LEN) return 0;
	size_t cursor = 0;
	char buff[UID_LEN + 2], b36buff[UID_LEN * 2 + 1];
	char *start = buff;

	for(set<size_t>::reverse_iterator it = MATCH_HEADER_LENGTH.rbegin(); it != MATCH_HEADER_LENGTH.rend(); ++it)
	{
		string test(uid.substr(0, *it));
		MapString2Int::iterator entry = uid2index.find(test);
		if(uid2index.end() != entry)
		{
			cursor = *it;
#ifdef _DEBUG
			if(entry->second < HEADER_WARNING) cerr << "WARNING: header match fallback: " << uid << endl;
#endif
			_itoa_s(entry->second, start, UID_LEN, 11);
			start += 3;
			break;
		}
	}
#ifdef _DEBUG
	// can't match any header
	if(cursor == 0) cerr << "WARNING: header can't match: " << uid << endl;
#endif
	for_each(uid.begin() + cursor, uid.end(), [&start](const char c) {
		switch(c)
		{
		case '0':
			*start++ = 'a';
			break;
		case '.':
			*start++ = '0';
			break;
		default:
			*start++ = c;
		}
	});
	*start = '\0';
	//cout << buff << endl;
	start = &(b36buff[UID_LEN]);
	if(base11_to_base37(buff, start)) return 0;
	size_t b37len = strlen(start);
#ifdef _DEBUG
	if(b37len > COMPRESS_LEN) cerr << "OVERFLOW: " << uid << endl;
#endif
	while(b37len < COMPRESS_LEN)
	{
		*--start = '0';
		++b37len;
	}
	if(strcpy_s(outputBuffer, bufLen, start))
		return 0;
	else
		return b37len;
}

size_t UIDBase36::uncompress(const char *uid, char *outputBuffer)
{
	if(uid == NULL) return 0;
	while(*uid == '0') ++uid;
	char b11buff[UID_LEN + 2];
	if(base37_to_base11(uid, b11buff)) return 0;
	//cout << b11buff << endl;
	char *src = b11buff, *dest = outputBuffer;
	if(b11buff[0] != '1' && b11buff[0] != '2' && b11buff[0] != 'A')
	{
		*reinterpret_cast<int*>(outputBuffer) = *reinterpret_cast<int*>(b11buff);
		outputBuffer[3] = '\0';
		int code = 0;
		if(base11_to_int(outputBuffer, &code)) return 0;
		MapInt2String::iterator it = index2uid.find(code);
		if(it == index2uid.end()) return 0;
		it->second.copy(outputBuffer, it->second.length());
		src = &b11buff[3];
		dest = outputBuffer + it->second.length();
	}
	while(*src != '\0')
	{
		switch(*src)
		{
		case 'A':
			*dest++ = '0';
			++src;
			break;
		case '0':
			*dest++ = '.';
			++src;
			break;
		default:
			*dest++ = *src++;
		}
	}
	*dest = '\0';
	return dest - outputBuffer;
}
