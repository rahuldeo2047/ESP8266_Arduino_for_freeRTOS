/**
 * @file ESP8266.cpp
 * @brief The implementation of class ESP8266. 
 * @author Wu Pengfei<pengfei.wu@itead.cc> 
 * @date 2015.02
 * 
 * @par Copyright:
 * Copyright (c) 2015 ITEAD Intelligent Systems Co., Ltd. \n\n
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version. \n\n
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include "ESP8266.h"
#include <Regexp.h>

#define RTOS_BASED_DELAY

#if defined(RTOS_BASED_DELAY)
#include <Arduino_FreeRTOS.h>
#endif

#define LOG_OUTPUT_DEBUG            (0)
#define LOG_OUTPUT_DEBUG_PREFIX     (0)

#define logDebug(arg)\
    do {\
        if (LOG_OUTPUT_DEBUG)\
        {\
            if (LOG_OUTPUT_DEBUG_PREFIX)\
            {\
                Serial.print("[LOG Debug: ");\
                /*Serial.print((const char*)__FILE__);*/\
                /*Serial.print(",");*/\
                Serial.print((unsigned int)__LINE__);\
                Serial.print(",");\
                Serial.print((const char*)__FUNCTION__);\
                Serial.print("] ");\
            }\
            Serial.println(arg);\
        }\
    } while(0)


const char *ipd_data_states_str[] =
{
  "_IDL_",
  "_IPD_",
  "_ID_OR_LEN_",
  "_LEN_",
  "_DATA_",
  "_OVER_"
};

enum ipd_data_states
{
  IDL,
  IPD,
  ID_OR_LEN,
  LEN,
  DATA,
  OVER
}ipd_data_states ;

#define DEBUG_PRINT Serial.print(" < "); Serial.print(__LINE__), Serial.print(" > ");
#define TRACE_DATA(str_state, token, id, len, ipddata)  //Log.Info("[state (%d | %s) | tok:%s | id:%d | len:%d | ipddata:%s\t]"CR,str_state, ipd_data_states_str[str_state], token, *id, *len, ipddata );


#ifdef ESP8266_USE_SOFTWARE_SERIAL
ESP8266::ESP8266(SoftwareSerial &uart, uint32_t baud): m_puart(&uart)
{
    m_puart->begin(baud);
    rx_empty();
}
#else
ESP8266::ESP8266(HardwareSerial &uart, uint32_t baud): m_puart(&uart)
{
    m_puart->begin(baud);
    rx_empty();
}
#endif

bool ESP8266::kick(void)
{
    return eAT();
}

bool ESP8266::restart(void)
{
    unsigned long start;
    if (eATRST()) {
 logDebug(millis());
#if defined(RTOS_BASED_DELAY)

	vTaskDelay((TickType_t)pdMS_TO_TICKS(2*2000));
#else
    	delay(2000);
#endif
 logDebug(millis());
        start = millis();
        while (millis() - start < 3000) {
            if (eAT()) {
#if defined(RTOS_BASED_DELAY)
		vTaskDelay((TickType_t)pdMS_TO_TICKS(2*1500));
#else
		delay(1500); /* Waiting for stable */
#endif
                return true;
            }
#if defined(RTOS_BASED_DELAY)
            vTaskDelay((TickType_t)pdMS_TO_TICKS(2*100));
#else
    	    delay(100);
#endif
        }
    }
    return false;
}

/*
String ESP8266::getVersion(void)
{
    String version;
    eATGMR(version);
    return version;
}
*/

char * ESP8266::getVersion(void)
{
    static char version[256] = {0};
    memset(version, 0, 256);
    eATGMR(version);
    return version;
}

bool ESP8266::setOprToStation(void)
{
    uint8_t mode;
    if (!qATCWMODE(&mode)) {
        return false;
    }
    if (mode == 1) {
        return true;
    } else {
        if (sATCWMODE(1) && restart()) {
            return true;
        } else {
            return false;
        }
    }
}

bool ESP8266::setOprToSoftAP(void)
{
    uint8_t mode;
    if (!qATCWMODE(&mode)) {
        return false;
    }
    if (mode == 2) {
        return true;
    } else {
        if (sATCWMODE(2) && restart()) {
            return true;
        } else {
            return false;
        }
    }
}

bool ESP8266::setOprToStationSoftAP(void)
{
    uint8_t mode;
    if (!qATCWMODE(&mode)) {
        return false;
    }
    if (mode == 3) {
        return true;
    } else {
        if (sATCWMODE(3) && restart()) {
            return true;
        } else {
            return false;
        }
    }
}

String ESP8266::getAPList(void)
{
    String list;
    eATCWLAP(list);
    return list;
}

bool ESP8266::joinAP(char * ssid, char * pwd)
{
    return sATCWJAP(ssid, pwd);
}

bool ESP8266::enableClientDHCP(uint8_t mode, boolean enabled)
{
    return sATCWDHCP(mode, enabled);
}

bool ESP8266::leaveAP(void)
{
    return eATCWQAP();
}

bool ESP8266::setSoftAPParam(String ssid, String pwd, uint8_t chl, uint8_t ecn)
{
    return sATCWSAP(ssid, pwd, chl, ecn);
}

String ESP8266::getJoinedDeviceIP(void)
{
    String list;
    eATCWLIF(list);
    return list;
}

String ESP8266::getIPStatus(void)
{
    String list;
    eATCIPSTATUS(list);
    return list;
}

char * ESP8266::getLocalIP(void)
{
    static char list[256] = {0};
    memset(list, 0, 256); 
    eATCIFSR(list);
    return list;
}

bool ESP8266::enableMUX(void)
{
    return sATCIPMUX(1);
}

bool ESP8266::disableMUX(void)
{
    return sATCIPMUX(0);
}

bool ESP8266::createTCP(char * addr, uint32_t port)
{
    return sATCIPSTARTSingle("TCP", addr, port);
}

//bool ESP8266::createTCP(String addr, uint32_t port)
//{
//    return sATCIPSTARTSingle("TCP", addr, port);
//}

bool ESP8266::releaseTCP(void)
{
    return eATCIPCLOSESingle();
}

bool ESP8266::registerUDP(String addr, uint32_t port)
{
    return sATCIPSTARTSingle("UDP", addr, port);
}

bool ESP8266::unregisterUDP(void)
{
    return eATCIPCLOSESingle();
}

bool ESP8266::createTCP(uint8_t mux_id, char * addr, uint32_t port)
{
    logDebug(addr);
    return sATCIPSTARTMultiple(mux_id, "TCP", addr, port);
}

//bool ESP8266::createTCP(uint8_t mux_id, String addr, uint32_t port)
//{
//    logDebug(addr);
//    return sATCIPSTARTMultiple(mux_id, "TCP", addr, port);
//}

bool ESP8266::releaseTCP(uint8_t mux_id)
{
    return sATCIPCLOSEMulitple(mux_id);
}

bool ESP8266::registerUDP(uint8_t mux_id, String addr, uint32_t port)
{
    return sATCIPSTARTMultiple(mux_id, "UDP", addr, port);
}

bool ESP8266::unregisterUDP(uint8_t mux_id)
{
    return sATCIPCLOSEMulitple(mux_id);
}

bool ESP8266::setTCPServerTimeout(uint32_t timeout)
{
    return sATCIPSTO(timeout);
}

bool ESP8266::startTCPServer(uint32_t port)
{
    if (sATCIPSERVER(1, port)) {
        return true;
    }
    return false;
}

bool ESP8266::stopTCPServer(void)
{
    sATCIPSERVER(0);
    restart();
    return false;
}

bool ESP8266::startServer(uint32_t port)
{
    return startTCPServer(port);
}

bool ESP8266::stopServer(void)
{
    return stopTCPServer();
}

bool ESP8266::send(const uint8_t *buffer, uint32_t len)
{
    return sATCIPSENDSingle(buffer, len);
}

bool ESP8266::send(uint8_t mux_id, const uint8_t *buffer, uint32_t len)
{
    return sATCIPSENDMultiple(mux_id, buffer, len);
}

uint32_t ESP8266::recv(uint8_t *buffer, uint32_t buffer_size, uint32_t timeout)
{
    return recvPkg(buffer, buffer_size, NULL, timeout, NULL);
}

uint32_t ESP8266::recv(uint8_t mux_id, uint8_t *buffer, uint32_t buffer_size, uint32_t timeout)
{
    uint8_t id;
    uint32_t ret;
    ret = recvPkg(buffer, buffer_size, NULL, timeout, &id);
    if (ret > 0 && id == mux_id) {
        return ret;
    }
    return 0;
}

uint32_t ESP8266::recv(uint8_t *coming_mux_id, uint8_t *buffer, uint32_t buffer_size, uint32_t timeout)
{
    return recvPkg(buffer, buffer_size, NULL, timeout, coming_mux_id);
}

/*----------------------------------------------------------------------------*/
/* +IPD,<id>,<len>:<data> */
/* +IPD,<len>:<data> */


int isNumeric (const char * s)
{
  if (s == NULL || *s == '\0' || isspace(*s))
    return 0;
  char * p;
  strtod (s, &p);
  return *p == '\0';
}

char * getSring(byte *id, int *len, char * data, enum ipd_data_states str_state = IDL)
{
  static const char s[4] = "+,,:";
  static char *token = NULL;
  static char ipddata[256] = {0};

  switch (str_state)
  {
    case IDL:
      {
        ipddata[0] = 0;
        memset(ipddata, 0, 256);

        MatchState ms;

        ms.Target (data);

        int result = ms.Match ("IPD%p*%d*%p%d+%p.+", 0);
 
        if (REGEXP_MATCHED != result)
        { 
          return NULL;
        } 
        TRACE_DATA(str_state, token, id, len, ipddata)
        str_state = IPD;
        return getSring(id, len, data, str_state);
      } break;
      
    case IPD:
      {
        token = strtok(data, s); 
        if ( NULL != token)
        { 
          TRACE_DATA(str_state, token, id, len, ipddata)

          str_state = ID_OR_LEN;
          return getSring(id, len, NULL, str_state);
        }
      } break;
      
    case ID_OR_LEN:
      {
        token = strtok(data, s);

        if ( NULL != token)
        { 
          *id = atoi(token);
 
          TRACE_DATA(str_state, token, id, len, ipddata)

          str_state = LEN;
          return getSring(id, len, NULL, str_state);
        }
      } break;
      
    case LEN:  
      { 
        token = strtok(data, s); 

        if ( NULL != token)
        {
          if (0 == isNumeric(token))
          {
            *len = *id;
            *id = 0;

            memcpy(ipddata, token, *len);
            str_state = OVER;
            TRACE_DATA(str_state, token, id, len, ipddata)

            return getSring(id, len, NULL, str_state);
          }

          *len = atoi(token);
          if (255 < (*len))
          {
            *len = 255;
          }
          
          TRACE_DATA(str_state, token, id, len, ipddata)
          str_state = DATA; 
          return getSring(id, len, NULL, str_state);
        }
      } break;
      
    case DATA:
      {
        token = strtok(data, s);
        if ( NULL != token)
        {
          memcpy(ipddata, token, *len);
          TRACE_DATA(str_state, token, id, len, ipddata)
          str_state = OVER;
          return getSring(id, len, NULL, str_state); // WHY another step
        }
      } break;

    case OVER:
      {
        return ipddata;
      } break;

    default:
      {

      }
  }
  
}

uint32_t ESP8266::recvPkg(uint8_t *buffer, uint32_t buffer_size, uint32_t *data_len, uint32_t timeout, uint8_t *coming_mux_id)
{
    String data;
    char a;
    int32_t index_PIPDcomma = -1;
    int32_t index_colon = -1; /* : */
    int32_t index_comma = -1; /* , */
    int32_t len = -1;
    int8_t id = -1;
    bool has_data = false;
    uint32_t ret;
    unsigned long start;
    uint32_t i;
    
    if (buffer == NULL) {
        return 0;
    }
    
    start = millis();
    while (millis() - start < timeout) {
        if(m_puart->available() > 0) {
            a = m_puart->read();
            data += a;
        }
        
        index_PIPDcomma = data.indexOf("+IPD,");
        if (index_PIPDcomma != -1) {
            index_colon = data.indexOf(':', index_PIPDcomma + 5);
            if (index_colon != -1) {
                index_comma = data.indexOf(',', index_PIPDcomma + 5);
                /* +IPD,id,len:data */
                if (index_comma != -1 && index_comma < index_colon) { 
                    id = data.substring(index_PIPDcomma + 5, index_comma).toInt();
                    if (id < 0 || id > 4) {
                        return 0;
                    }
                    len = data.substring(index_comma + 1, index_colon).toInt();
                    if (len <= 0) {
                        return 0;
                    }
                } else { /* +IPD,len:data */
                    len = data.substring(index_PIPDcomma + 5, index_colon).toInt();
                    if (len <= 0) {
                        return 0;
                    }
                }
                has_data = true;
                break;
            }
        }
    }
    
    if (has_data) {
        i = 0;
        ret = len > buffer_size ? buffer_size : len;
        start = millis();
        while (millis() - start < 3000) {
            while(m_puart->available() > 0 && i < ret) {
                a = m_puart->read();
                buffer[i++] = a;
            }
            if (i == ret) {
                rx_empty();
                if (data_len) {
                    *data_len = len;    
                }
                if (index_comma != -1 && coming_mux_id) {
                    *coming_mux_id = id;
                }
                return ret;
            }
        }
    }
    return 0;
}

void ESP8266::rx_empty(void) 
{
    while(m_puart->available() > 0) {
        m_puart->read();
    }
}

String ESP8266::recvString(String target, uint32_t timeout)
{
    String data;
    char a;
    unsigned long start = millis();
    while (millis() - start < timeout) {
        while(m_puart->available() > 0) {
            a = m_puart->read();
			if(a == '\0') continue;
            data += a;
        }
        if (data.indexOf(target) != -1) {
            break;
        }   
    }
    return data;
}

char * ESP8266::recvString(char * target, uint32_t timeout)
{
   static char data[256] = {0};
   memset(data,0,256);

logDebug(target);

    int inx=0;
    char * pdata1;//* pdata2,* pdata3; 
    char a;
    unsigned long start = millis();
    while (millis() - start < timeout) {
        while(m_puart->available() > 0) {
            a = m_puart->read();
            if(a == '\0') continue;
            //data += a;
	    if(inx<256)
	    	data[inx++] = a;
            else
		break;

        }

	pdata1 = strstr (data, target);
        logDebug(pdata1);
	//pdata2 = strstr (data, target2.c_str());
	//pdata3 = strstr (data, target3.c_str());


        if (NULL != pdata1) {
            break;
        } //else if (NULL != pdata2) {
        //    break;
        //} else if (NULL != pdata3) {
        //    break;
        //}
    }
    logDebug(data);
    return data;
}


String ESP8266::recvString(String target1, String target2, uint32_t timeout)
{
    String data;
    char a;
    unsigned long start = millis();
    while (millis() - start < timeout) {
        while(m_puart->available() > 0) {
            a = m_puart->read();
			if(a == '\0') continue;
            data += a;
        }
        if (data.indexOf(target1) != -1) {
            break;
        } else if (data.indexOf(target2) != -1) {
            break;
        }
    }
    return data;
}

char * ESP8266::recvString(char * target1, char * target2, uint32_t timeout)
{
    static char data[256] = {0};
    memset(data,0,256);

    int inx=0;
    char * pdata1, * pdata2;//,* pdata3; 
    char a;
    unsigned long start = millis();
    while (millis() - start < timeout) {
        while(m_puart->available() > 0) {
            a = m_puart->read();
            if(a == '\0') continue; 
	    if(inx<256)
	    	data[inx++] = a;
            else
		break;

        }

	logDebug(data);
	pdata1 = strstr (data, target1);
        logDebug(pdata1);
	
	pdata2 = strstr (data, target2);
	logDebug(pdata2);
        
	//pdata3 = strstr (data, target3);
        //logDebug(pdata1);

        if (NULL != pdata1) {
            break;
        } else if (NULL != pdata2) {
            break;
        } //else if (NULL != pdata3) {
        //    break;
        //}
    }
    logDebug(data);
    return data;
}

String ESP8266::recvString(String target1, String target2, String target3, uint32_t timeout)
{
    String data;
    char a;
    unsigned long start = millis();
    while (millis() - start < timeout) {
        while(m_puart->available() > 0) {
            a = m_puart->read();
			if(a == '\0') continue;
            data += a;
        }
        if (data.indexOf(target1) != -1) {
            break;
        } else if (data.indexOf(target2) != -1) {
            break;
        } else if (data.indexOf(target3) != -1) {
            break;
        }
    }
    return data;
}

char * ESP8266::recvString(char * target1, char * target2, char * target3, uint32_t timeout)
{
    static char data[256] = {0};

    memset(data,0,256);

    int inx=0;
    char * pdata1, * pdata2,* pdata3; 
    char a;
    unsigned long start = millis();
    while (millis() - start < timeout) {
        while(m_puart->available() > 0) {
            a = m_puart->read();
            if(a == '\0') continue; 
	    if(inx<256)
	    	data[inx++] = a;
            else
		break;

        }

	logDebug(data);

	pdata1 = strstr (data, target1);
        logDebug(pdata1);
	
	pdata2 = strstr (data, target2);
	logDebug(pdata2);
        
	pdata3 = strstr (data, target3);
        logDebug(pdata3);

        if (NULL != pdata1) {
            break;
        } else if (NULL != pdata2) {
            break;
        } else if (NULL != pdata3) {
            break;
        }
    }
    logDebug(data);
    return data;
}

bool ESP8266::recvFind(String target, uint32_t timeout)
{
    String data_tmp;
    data_tmp = recvString(target, timeout);
    if (data_tmp.indexOf(target) != -1) {
        return true;
    }
    return false;
}


bool ESP8266::recvFind(char * target, uint32_t timeout)
{
    //char data[256] = {0}; // to be handled
    //data = (char*)malloc(256); 
    char * pdata = NULL;
    char * pdata_tmp = NULL;  

//Serial.println("char"); // fine
//Serial.println(target); // fine

logDebug(target);

    pdata = recvString(target, timeout);

logDebug(pdata);
//Serial.println("char2"); // fine
//Serial.println(pdata); // fine
    pdata_tmp = strstr (pdata, target);


logDebug(pdata_tmp);


    if (NULL != pdata_tmp) {
        return true;
    }
    return false;
}


bool ESP8266::recvFindAndFilter(String target, String begin, String end, String &data, uint32_t timeout)
{
    String data_tmp;
    data_tmp = recvString(target, timeout);
    if (data_tmp.indexOf(target) != -1) {
        int32_t index1 = data_tmp.indexOf(begin);
        int32_t index2 = data_tmp.indexOf(end);
        if (index1 != -1 && index2 != -1) {
            index1 += begin.length();
            data = data_tmp.substring(index1, index2);
            return true;
        }
    }
    data = "";
    return false;
}

bool ESP8266::recvFindAndFilter(char * target, char * begin, char * end, char * data, uint32_t timeout)
{
    //String data_tmp;
    char * data_tmp;
    char * pdata_tmp;
    char * pbegin, pend;
    
logDebug(target);
logDebug(begin);
logDebug(end);

    data_tmp = recvString(target, timeout);
    logDebug(data_tmp); // verified

    pdata_tmp = strstr (data_tmp, target);
    logDebug(pdata_tmp); // verified

//    if (data_tmp.indexOf(target) != -1) {
    if (NULL != pdata_tmp) {
	//logDebug(data_tmp);
	pbegin = strstr (data_tmp, begin);
	//logDebug(data_tmp);
	logDebug(pbegin+strlen(begin)); // it is complete string , to re-verify
	
	pend = strstr (pbegin, end); 
	//logDebug(data_tmp);
	logDebug((pend));
	//Serial.println(strlen(pend));
	//Serial.println((pend+1));

	if( (NULL != pbegin) && (NULL != pend) ) {
	     // str cat
	     memset(data, 0, 1);
	     strncat(data, pbegin+strlen(begin), strlen(pbegin));
	     return true;
	}
        //int32_t index1 = data_tmp.indexOf(begin);
        //int32_t index2 = data_tmp.indexOf(end);
        //if (index1 != -1 && index2 != -1) {
        //    index1 += begin.length();
        //    data = data_tmp.substring(index1, index2);
        //    return true;
        //}
    }
    //data = "";
    return false;
}

bool ESP8266::eAT(void)
{
    rx_empty();
    m_puart->println("AT");
    return recvFind("OK");
}

bool ESP8266::eATRST(void) 
{
    rx_empty();
    m_puart->println("AT+RST");
    return recvFind("OK");
}


/*
AT+GMR

AT version:1.1.0.0(May 11 2016 18:09:56)
SDK version:1.5.4(baaeaebb)
Ai-Thinker Technology Co. Ltd.
Jun 13 2016 11:29:20
OK

*/
/*
bool ESP8266::eATGMR(String &version)
{
    rx_empty();
    m_puart->println("AT+GMR");
    return recvFindAndFilter("OK", "+GMR", "\r\nOK", version);
    //return recvFindAndFilter("OK", "SDK version:", "\r\n\r\nOK", version); 
}
*/

bool ESP8266::eATGMR(char * version)
{
    rx_empty();
    m_puart->println("AT+GMR");
    return recvFindAndFilter("OK", "+GMR", "\r\nOK", version);
    //return recvFindAndFilter("OK", "SDK version:"/*"\r\r\n"*/, "\r\n\r\nOK", version); 
}

bool ESP8266::qATCWMODE(uint8_t *mode) 
{
    char str_mode[1]={0};
    bool ret;
    if (!mode) {
    	//logDebug("not valid");
        return false;
    }
    rx_empty();
    m_puart->println("AT+CWMODE?");
    ret = recvFindAndFilter("OK", "+CWMODE:", "\r\n\r\nOK", str_mode); 
    //logDebug(str_mode);
    if (ret) {
        *mode = (uint8_t)str_mode[0]-0x30;//.toInt();
        //logDebug(str_mode[0]);
        logDebug(*mode);
        return true;
    } else {
    	//logDebug("FALSE");
        return false;
    }
}

bool ESP8266::sATCWMODE(uint8_t mode)
{
    String data;
    rx_empty();
    m_puart->print("AT+CWMODE=");
    m_puart->println(mode);
    
    data = recvString("OK", "no change");
    if (data.indexOf("OK") != -1 || data.indexOf("no change") != -1) {
        return true;
    }
    return false;
}

bool ESP8266::sATCWJAP(String ssid, String pwd)
{
    String data;
    rx_empty();
    m_puart->print("AT+CWJAP=\"");
    m_puart->print(ssid);
    m_puart->print("\",\"");
    m_puart->print(pwd);
    m_puart->println("\"");
    
    data = recvString("OK", "FAIL", 10000);
    if (data.indexOf("OK") != -1) {
        return true;
    }
    return false;
}

bool ESP8266::sATCWJAP(char * ssid, char * pwd)
{
    char * data, *chk;
    rx_empty();
    m_puart->print("AT+CWJAP=\"");
    m_puart->print(ssid);
    m_puart->print("\",\"");
    m_puart->print(pwd);
    m_puart->println("\"");
    
    data = recvString("OK", "FAIL", 10000);
    //Serial.println(data);
    chk = strstr (data, "OK");
    logDebug(chk); //  
     
    //if (data.indexOf("OK") != -1) {
    if (NULL != chk) {
        return true;
    }
    return false;
}

bool ESP8266::sATCWDHCP(uint8_t mode, boolean enabled)
{
	String strEn = "0";
	if (enabled) {
		strEn = "1";
	}
	
	
    String data;
    rx_empty();
    m_puart->print("AT+CWDHCP=");
    m_puart->print(strEn);
    m_puart->print(",");
    m_puart->println(mode);
    
    data = recvString("OK", "FAIL", 10000);
    if (data.indexOf("OK") != -1) {
        return true;
    }
    return false;
}

bool ESP8266::eATCWLAP(String &list)
{
    String data;
    rx_empty();
    m_puart->println("AT+CWLAP");
    return recvFindAndFilter("OK", "\r\r\n", "\r\n\r\nOK", list, 10000);
}

bool ESP8266::eATCWQAP(void)
{
    String data;
    rx_empty();
    m_puart->println("AT+CWQAP");
    return recvFind("OK");
}

bool ESP8266::sATCWSAP(String ssid, String pwd, uint8_t chl, uint8_t ecn)
{
    String data;
    rx_empty();
    m_puart->print("AT+CWSAP=\"");
    m_puart->print(ssid);
    m_puart->print("\",\"");
    m_puart->print(pwd);
    m_puart->print("\",");
    m_puart->print(chl);
    m_puart->print(",");
    m_puart->println(ecn);
    
    data = recvString("OK", "ERROR", 5000);
    if (data.indexOf("OK") != -1) {
        return true;
    }
    return false;
}

bool ESP8266::eATCWLIF(String &list)
{
    String data;
    rx_empty();
    m_puart->println("AT+CWLIF");
    return recvFindAndFilter("OK", "\r\r\n", "\r\n\r\nOK", list);
}
bool ESP8266::eATCIPSTATUS(String &list)
{
    String data;
#if defined(RTOS_BASED_DELAY)
    vTaskDelay((TickType_t)pdMS_TO_TICKS(2*100));
#else
    delay(100);
#endif
    rx_empty();
    m_puart->println("AT+CIPSTATUS");
    return recvFindAndFilter("OK", "\r\r\n", "\r\n\r\nOK", list);
}

bool ESP8266::sATCIPSTARTSingle(char * type, char * addr, uint32_t port)
{
    char * data, chk_ok, chk_alrdycon;
    rx_empty();
    m_puart->print("AT+CIPSTART=\"");
    m_puart->print(type);
    m_puart->print("\",\"");
    m_puart->print(addr);
    m_puart->print("\",");
    m_puart->println(port);
    
    data = recvString("OK", "ERROR", "ALREADY CONNECT", 10000);
    
    chk_ok = strstr(data, "OK");
    chk_alrdycon = strstr(data, "ALREADY CONNECT");

    if ( (NULL != chk_ok ) || (NULL != chk_alrdycon) ) {
        return true;
    }
    return false;
}

bool ESP8266::sATCIPSTARTSingle(String type, String addr, uint32_t port)
{
    String data;
    rx_empty();
    m_puart->print("AT+CIPSTART=\"");
    m_puart->print(type);
    m_puart->print("\",\"");
    m_puart->print(addr);
    m_puart->print("\",");
    m_puart->println(port);
    
    data = recvString("OK", "ERROR", "ALREADY CONNECT", 10000);
    if (data.indexOf("OK") != -1 || data.indexOf("ALREADY CONNECT") != -1) {
        return true;
    }
    return false;
}


bool ESP8266::sATCIPSTARTMultiple(uint8_t mux_id, char * type, char * addr, uint32_t port)
{
    char * data, chk_ok, chk_alrdycon;
    rx_empty();
    m_puart->print("AT+CIPSTART=");
    m_puart->print(mux_id);
    m_puart->print(",\"");
    m_puart->print(type);
    m_puart->print("\",\"");
    m_puart->print(addr);
    m_puart->print("\",");
    m_puart->println(port);
    
    data = recvString("OK", "ERROR", "ALREADY CONNECT", 10000);
        
    chk_ok = strstr(data, "OK");
    chk_alrdycon = strstr(data, "ALREADY CONNECT");

    if ( (NULL != chk_ok ) || (NULL != chk_alrdycon) ) {
        return true;
    }
    return false;
}

bool ESP8266::sATCIPSTARTMultiple(uint8_t mux_id, String type, String addr, uint32_t port)
{
    String data;
    rx_empty();
    m_puart->print("AT+CIPSTART=");
    m_puart->print(mux_id);
    m_puart->print(",\"");
    m_puart->print(type);
    m_puart->print("\",\"");
    m_puart->print(addr);
    m_puart->print("\",");
    m_puart->println(port);
    
    data = recvString("OK", "ERROR", "ALREADY CONNECT", 10000);
    if (data.indexOf("OK") != -1 || data.indexOf("ALREADY CONNECT") != -1) {
        return true;
    }
    return false;
}
bool ESP8266::sATCIPSENDSingle(const uint8_t *buffer, uint32_t len)
{
    rx_empty();
    m_puart->print("AT+CIPSEND=");
    m_puart->println(len);
    if (recvFind(">", 5000)) {
        rx_empty();
        for (uint32_t i = 0; i < len; i++) {
            m_puart->write(buffer[i]);
        }
        return recvFind("SEND OK", 10000);
    }
    return false;
}
bool ESP8266::sATCIPSENDMultiple(uint8_t mux_id, const uint8_t *buffer, uint32_t len)
{
    rx_empty();
    m_puart->print("AT+CIPSEND=");
    m_puart->print(mux_id);
    m_puart->print(",");
    m_puart->println(len);
    if (recvFind(">", 5000)) {
        rx_empty();
        for (uint32_t i = 0; i < len; i++) {
            m_puart->write(buffer[i]);
        }
        return recvFind("SEND OK", 10000);
    }
    return false;
}
bool ESP8266::sATCIPCLOSEMulitple(uint8_t mux_id)
{
    String data;
    rx_empty();
    m_puart->print("AT+CIPCLOSE=");
    m_puart->println(mux_id);
    
    data = recvString("OK", "link is not", 5000);
    if (data.indexOf("OK") != -1 || data.indexOf("link is not") != -1) {
        return true;
    }
    return false;
}
bool ESP8266::eATCIPCLOSESingle(void)
{
    rx_empty();
    m_puart->println("AT+CIPCLOSE");
    return recvFind("OK", 5000);
}
bool ESP8266::eATCIFSR(char * list)
{
    rx_empty();
    m_puart->println("AT+CIFSR");
    return recvFindAndFilter("OK", "\r\r\n", "\r\n\r\nOK", list);
}
bool ESP8266::sATCIPMUX(uint8_t mode)
{
    char * data = NULL;
    rx_empty();
    m_puart->print("AT+CIPMUX=");
    m_puart->println(mode);
    
    data = recvString("OK", "Link is builded");
    
    //Serial.println(data);
    
    if ((NULL != data) ) {
        return true;
    }
    return false;
}
bool ESP8266::sATCIPSERVER(uint8_t mode, uint32_t port)
{
    char *data = NULL;
    char *ok = NULL;
    char *nc = NULL ;
    
    if (mode) {
        rx_empty();
        m_puart->print("AT+CIPSERVER=1,");
        m_puart->println(port);
        
        data = recvString("OK", "no change");
        Serial.print(__LINE__);
        Serial.print(" ");
        Serial.println(data);
        
        ok = strstr (data, "OK");
        Serial.print(__LINE__);
        Serial.print(" ");
        Serial.println(ok);
         
        nc = strstr (data, "no change");
        Serial.print(__LINE__);
        Serial.print(" ");
        Serial.println(nc);

	if( (NULL != ok) || (NULL != nc) ) {
	     return true;
        }
        return false;
    } else {
        rx_empty();
        m_puart->println("AT+CIPSERVER=0");
        return recvFind("\r\r\n");
    }
}
bool ESP8266::sATCIPSTO(uint32_t timeout)
{
    rx_empty();
    m_puart->print("AT+CIPSTO=");
    m_puart->println(timeout);
    return recvFind("OK");
}

