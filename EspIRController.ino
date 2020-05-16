#if defined(ESP8266) || defined(ESP32)

#include "EspIRController.h"

EspIRController::EspIRController(byte sendPin, byte recvPin) {
  mSendPin = sendPin;
  mRecvPin = recvPin;
}

EspIRController::~EspIRController() {
  if (mIRSend != NULL) {
    delete (mIRSend);
    mIRSend = NULL;
  }
  if (mIRRecv != NULL) {
    delete (mIRRecv);
    mIRRecv = NULL;
  }
}

void EspIRController::setup() {
  SPIFFS.begin();

  mIRSend = new IRsend(mSendPin);
  mIRSend->begin();

  espWiFi.registerExternalRequestHandler(&mEspIRControllerRequestHandler);
}

void EspIRController::loop() {
}

bool EspIRController::send(String data, String type, uint16_t repeat, uint16_t repeatDelay) {
  if (mIRSend == NULL)
    return false;

  int len = data.length() * 4;
  uint64_t bin = (uint64_t)strtoull(data.c_str(), NULL, 16);

  decode_type_t sendType = UNKNOWN;
  String sendTypeName;

  if ((sendType = remoteType(type)) == UNKNOWN) {
      return false;
  }

  for (uint16_t repeatCnt = 0; repeatCnt < repeat; repeatCnt++) {
    mIRSend->send(sendType, bin, len);
      
    if (repeatCnt + 1 < repeat)
      delay(repeatDelay);
  }
  
  return true;
}

String EspIRController::remoteType(decode_type_t type) {
  String result = RemoteType(UNKNOWN);

  switch (type) {
    case NEC: result = RemoteType(NEC); break;
    case SONY: result = RemoteType(SONY); break;
    case RC5: result = RemoteType(RC5); break;
    case RC6: result = RemoteType(RC6); break;
    case DISH: result = RemoteType(DISH); break;
    case SHARP: result = RemoteType(SHARP); break;
    case JVC: result = RemoteType(JVC); break;
    case SANYO: result = RemoteType(SANYO); break;
    case SANYO_LC7461: result = RemoteType(SANYO_LC7461); break;
    case MITSUBISHI: result = RemoteType(MITSUBISHI); break;
    case SAMSUNG: result = RemoteType(SAMSUNG); break;
    case WHYNTER: result = RemoteType(WHYNTER); break;
    case AIWA_RC_T501: result = RemoteType(AIWA_RC_T501); break;
    case PANASONIC: result = RemoteType(PANASONIC); break;
    case DENON: result = RemoteType(DENON); break;
    case COOLIX: result = RemoteType(COOLIX); break;
  }

  return result;
}

decode_type_t EspIRController::remoteType(String type) {
  if (type == RemoteType(NEC)) return NEC;
  if (type == RemoteType(SONY)) return SONY;
  if (type == RemoteType(RC5)) return RC6;
  if (type == RemoteType(RC6)) return RC6;
  if (type == RemoteType(DISH)) return DISH;
  if (type == RemoteType(SHARP)) return SHARP;
  if (type == RemoteType(JVC)) return JVC;
  if (type == RemoteType(SANYO)) return SANYO;
  if (type == RemoteType(SANYO_LC7461)) return SANYO_LC7461;
  if (type == RemoteType(MITSUBISHI)) return MITSUBISHI;
  if (type == RemoteType(SAMSUNG)) return SAMSUNG;
  if (type == RemoteType(WHYNTER)) return WHYNTER;
  if (type == RemoteType(AIWA_RC_T501)) return AIWA_RC_T501;
  if (type == RemoteType(PANASONIC)) return PANASONIC;
  if (type == RemoteType(DENON)) return DENON;
  if (type == RemoteType(COOLIX)) return COOLIX;

  return UNKNOWN;
}

String EspIRController::Uint64toString(uint64_t input, uint8_t base) {
  char buf[8 * sizeof(input) + 1];  // Assumes 8-bit chars plus zero byte.
  char *str = &buf[sizeof(buf) - 1];

  *str = '\0';

  // prevent crash if called with base == 1
  if (base < 2) base = 10;

  do {
    char c = input % base;
    input /= base;

    *--str = c < 10 ? c + '0' : c + 'A' - 10;
  } while (input);

  std::string s(str);
  return s.c_str();
}

bool EspIRController::EspIRControllerRequestHandler::canHandle(HTTPMethod method, String uri) {
  if (method == HTTP_GET && (uri == getRemoteJsUri() || uri == getRemoteCssUri()))
    return true;
  if (method == HTTP_GET && uri.startsWith(getRemoteDirUri()))
    return SPIFFS.exists(uri);
  if (method == HTTP_POST && uri == getRecvIRUri())
    return true;
  if (method == HTTP_POST && uri == getSendIRUri())
    return true;
  if (method == HTTP_POST && canUpload(uri))
    return true;

  return false;
}

#ifdef ESP8266
bool EspIRController::EspIRControllerRequestHandler::canHandle(ESP8266WebServer& server) {
#endif
#ifdef ESP32
bool EspIRController::EspIRControllerRequestHandler::canHandle(WebServer& server) {
#endif
  if (canHandle(server.method(), server.uri()))
    return true;

  if (server.method() == HTTP_POST && server.uri() == getConfigUri() && server.hasArg(menuIdentifierIR()) && server.arg(menuIdentifierIR()) == "")
    return true;

  return false;
}

bool EspIRController::EspIRControllerRequestHandler::canUpload(String uri) {
  return uri == getUploadUri();
}

#ifdef ESP8266
bool EspIRController::EspIRControllerRequestHandler::handle(ESP8266WebServer& server, HTTPMethod method, String uri) {
#endif
#ifdef ESP32
bool EspIRController::EspIRControllerRequestHandler::handle(WebServer& server, HTTPMethod method, String uri) {
#endif
  DBG_PRINT("EspIRControllerRequestHandler::handle: " + uri + " ");
  if (method == HTTP_POST && uri == getRecvIRUri())
    return httpHandleRecvIRCmd(server);
    
  if (method == HTTP_POST && uri == getSendIRUri())
    return httpHandleSendIRCmd(server);

  if (method == HTTP_POST && uri == getConfigUri() && server.hasArg(menuIdentifierIR()) && server.arg(menuIdentifierIR()) == "") {
    String action = server.arg("action"), remote = "";
    if (server.hasArg("remote"))
      remote = server.arg("remote");

    server.client().setNoDelay(true);
    server.send(200, "text/javascript", getIRForm(action, remote));
    
    return (httpRequestProcessed = true);
  }

  if (method == HTTP_GET && uri == getRemoteJsUri()) {
    String js = F("var bL;function initIR(r,rC){if(typeof rC==='undefined')rC=0;if(typeof window[r]==='undefined'){if(rC<10)wT('initIR(\\\''+r+'\\\')',rC+1);else alert('load failed '+rC+' times');return;}bL=window[r];var d=document.getElementById(\"rm\");while(d.hasChildNodes())d.removeChild(d.firstChild);for(var i=0;i<bL[5].length;i++){var b = bL[5][i];B(d,b[0],b[1],b[2],b[3],(b.length>=5?b[4]:bL[2]));}I(d, bL[0], bL[1]);}");
    js += F("function B(d,t,c,x,y,s){var nD=E(d,'div');nD.id='B'+c;A(nD,'style','top:'+x+'px;left:'+y+'px;');var nA=E(nD,'a');A(nA,'style','width:'+s+'px;height:'+s+'px;');A(nA,'onclick',\"C('\"+c+\"')\");A(nA,'title',t);}");
    js += F("function I(d,s,w){var img=E(d,'img');A(img,'src','data:'+s);A(img,'style','width:'+w+'px;');}");
    js += F("function A(t,n,v){t.setAttribute(n,v);}");
    js += F("function E(d,e){var c=document.createElement(e);d.appendChild(c);return c;}");
    js += F("function wT(s,t){if(typeof t==='undefined')t=100;window.setTimeout(s,t);}");
    js += F("function uR(){var f=gE('rUF');if(f.elements['remote'].value == ''){alert('Remote-Description File required!');return;}f.submit();lL();}");
    js += F("function rR(r){var rN=event.srcElement.parentNode.firstChild.innerHTML;var url='/config?ChipID='+cI()+'&action=remove&remote='+rN+'&ir=';if (typeof sReq('POST',url)!==\"undefined\")lL();}");
    js += F("function lR(r){var rN='/remote/'+r+'.js';var rNS=r+'.Remote';var rmS=document.querySelector('script[data-name=\"'+rNS+'\"]');if(typeof rmS===\"undefined\" || rmS == null)aSU(rN,gE('rm').parentNode,rNS);wT('initIR(\\\''+r+'\\\')');}");
    js += F("function lF(a){rcvSa();a=(a===undefined?'form':a);modDlg(true,false,'ir',a);}");
    js += F("function lL(){lF('list');}");
    js += F("function lU(){lF('upload');}");
    js += F("function lS(){lF('sample');}");
    js += F("function C(cmd){var url='/sendIRCmd?ChipID='+cI()+'&data='+bL[4]+cmd+'&type='+bL[3];sReq('POST',url);}");
    js += F("function cI(){return '");
    js += EspWiFi::getChipID();
    js += F("';}");
    js += F("function sReq(m,u){try{var xmlHttp=new XMLHttpRequest();xmlHttp.open(m,u,false);xmlHttp.send(null);if(xmlHttp.status!=200){alert('Fehler: '+xmlHttp.statusText);return null;}else return xmlHttp;} catch(err) {return null;}}");
    js += F("var rcvXhr=null;var rcvC=0;\n");
    js += F("function rcvSa(){if(rcvXhr!=null)rcvS();}");
    js += F("function rcvS(){var aB=gE('sIR');if(rcvXhr!=null){rcvXhr.abort();rcvXhr=null;aB.innerHTML='Start';modDlgEn(true);return;}aB.innerHTML='Stop';modDlgEn(false);var o=gE('rdtO');while(o.childNodes.length>0)o.removeChild(o.lastChild);\nrcvC=0;rcvXhr=new XMLHttpRequest();rcvXhr.onprogress=rcvH;rcvXhr.onerror=rcvH;rcvXhr.open('POST','");
    js += getRecvIRUri();
    js += F("',true);rcvXhr.send(null);}\n");
    js += F("function rcvH(e){var o=gE('rdtO');if(o){var rT=rcvXhr.responseText.split(':::');var i;for(i=0;i<rT.length;i++){if(i>=rcvC&&rT[i]!=''){");
    js += F("rcvC+=1;var tr=cE('tr');var s;var d=rT[i].split(':');for(s=0;s<d.length;s++)if(d[s]!=''){var td=cE('td');td.innerHTML=d[s];\naC(tr,td);}aC(o,tr);}}window.setTimeout('scD(\\\'rd\\\')', 100);}}");
    js += F("function scD(n){var o=gE(n);o.scrollTop=o.scrollHeight;}\n");

    server.sendHeader("Cache-Control", "public, max-age=86400");
    server.client().setNoDelay(true);
    server.send(200, "text/javascript", js);
    
    return (httpRequestProcessed = true);
  }

  if (method == HTTP_GET && uri == getRemoteCssUri()) {
    String css=F(".rm{position:relative;margin:0px 10px 0px 10px;width:min-content;}\n");
    css += F(".rm div{position:absolute;}\n");
    css += F(".rm img{z-index:20;}\n");
    css += F(".rm div a{position:absolute;border-radius:50%;border:1px solid #000;z-index:30;opacity:0.1;}\n");

    css += F(".rL {margin:10px 10px 0px 10px;}\n");
    css += F(".rL .dR{border-radius:5px;border:1px solid #A0A0A0;color:#777;text-align:center;padding:0px 5px 0px 5px;cursor:pointer;}\n");
    css += F(".rL .dR:hover{border:1px solid #5F5F5F;background-color:#D0D0D0;}\n");
    css += F(".rL th{text-align:left;}\n");
    css += F(".rL form{margin:5px 5px 0px 5px;}\n");
    css += F(".rL form input{min-width:180px;max-width:180px;}\n");

    css += F(".rd{min-width:200px;max-height:250px;overflow-y:scroll;margin:10px 10px 0px 10px;}\n");
    css += F(".rdt th{border:1px solid #444;border-radius:3px;background:#ccc;text-align:left;position:sticky;top:0;}\n");
    css += F(".rdt tbody td:nth-child(3){text-align:right;}\n");
    css += F(".rdt tbody td:nth-child(4){white-space:nowrap;}\n");
    css += F(".sIR {float:right;margin-right:10px;}\n");

    server.sendHeader("Cache-Control", "public, max-age=86400");
    server.client().setNoDelay(true);
    server.send(200, "text/css", css);
    
    return (httpRequestProcessed = true);
  }

  if (method == HTTP_GET && uri.startsWith(getRemoteDirUri())) {
    File sFile;
    
    if ((sFile = SPIFFS.open(uri, "r"))) {
      DBG_PRINT("serve " + uri);
      server.sendHeader("Cache-Control", "public, max-age=3600");
      server.streamFile(sFile, "text/javascript");
      DBG_PRINT(" size " + String(sFile.size()) + " ");
      sFile.close();
      mLastLoadedRemote = getRemoteFromUri(uri);
      
      return (httpRequestProcessed = true);
    } else {
      DBG_PRINT("file " + uri + " not found");
    }
  }

  if (method == HTTP_POST && uri == getUploadUri()) {
    server.client().setNoDelay(true);
    server.sendHeader("Location", "/");
    server.send(303, "text/plain", "See Other");
    httpRequestProcessed = true;
    return true;
  }

  return false;
}

#ifdef ESP8266
void EspIRController::EspIRControllerRequestHandler::upload(ESP8266WebServer& server, String uri, HTTPUpload& upload) {
#endif
#ifdef ESP32
void EspIRController::EspIRControllerRequestHandler::upload(WebServer& server, String uri, HTTPUpload& upload) {
#endif
  if (upload.status == UPLOAD_FILE_START) {
    DBG_PRINT("upload: " + upload.filename + " ");
    String uploadFilename = getRemoteDirUri() + upload.filename;
    
    if (SPIFFS.exists(uploadFilename)) {
      DBG_PRINT("delete ");
      SPIFFS.remove(uploadFilename);
    }
    if (!(uploadFile = SPIFFS.open(uploadFilename.c_str(), "w"))) {
      DBG_PRINT("open failed!");
      DBG_FORCE_OUTPUT();
    }
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (uploadFile.write(upload.buf, upload.currentSize) != upload.currentSize) {
      DBG_PRINTLN("ERROR: UPLOAD_FILE_WRITE");
      DBG_FORCE_OUTPUT();
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    uploadFile.flush();
    uploadFile.close(); 
    DBG_PRINTF("wrote size: %d", upload.totalSize);
  } else {
    DBG_PRINTLN("ERROR: UPLOAD_FILE_END");
    DBG_FORCE_OUTPUT();
  }
}

#ifdef ESP8266
bool EspIRController::EspIRControllerRequestHandler::httpHandleSendIRCmd(ESP8266WebServer& server) {
#endif
#ifdef ESP32
bool EspIRController::EspIRControllerRequestHandler::httpHandleSendIRCmd(WebServer& server) {
#endif
  if (server.args() == 0 || server.argName(0) != "ChipID" || server.arg(0) != EspWiFi::getChipID()) {
    server.client().setNoDelay(true);
    server.send(403, "text/plain", "Forbidden");
    httpRequestProcessed = true;
    return true;
  }

  if (server.hasArg("data") && server.arg("data") != "" && server.hasArg("type") && server.arg("type") != "") {
    String data = server.arg("data"), type = server.arg("type");
    uint16_t repeat = 1, repeatDelay=10;

    if (server.hasArg("repeat") && server.arg("repeat") != "")
      repeat = strtoul(server.arg("repeat").c_str(), NULL, 10);
    if (server.hasArg("delay") && server.arg("delay") != "")
      repeatDelay = strtoul(server.arg("delay").c_str(), NULL, 10);

    espIRController.send(data, type, repeat, repeatDelay);

    server.client().setNoDelay(true);
    server.send(200, "text/plain", "OK");
    
    return (httpRequestProcessed = true);
  }
  
  return false;
}

#ifdef ESP8266
bool EspIRController::EspIRControllerRequestHandler::httpHandleRecvIRCmd(ESP8266WebServer& server) {
#endif
#ifdef ESP32
bool EspIRController::EspIRControllerRequestHandler::httpHandleRecvIRCmd(WebServer& server) {
#endif

  DBG_PRINT("httpHandleRecvIRCmd ");
  if (server.method() == HTTP_POST) {
    server.client().setNoDelay(true);
    server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    server.send(200, "text/html", "");

    IRrecv *irrecv = espIRController.startRecv();

    while (server.client().connected()) {
      decode_results  results;
      
      if (irrecv->decode(&results)) {
        String rType = espIRController.remoteType(results.decode_type);
        String rData = espIRController.Uint64toString(results.value, 16);
        String data = rType + ":" +  rData + ":" + String(results.bits, DEC), extra = "";

        if (results.decode_type == UNKNOWN)
          extra += "unknown type detected";
        if (results.overflow)
          extra += String(extra == "" ? "" : ", ") + "truncated";

        if (extra != "")
          data+= ":" + extra;
          
        data += ":::";
        server.sendContent(data);

        irrecv->resume();
      }

      yield();
    }

    espIRController.stopRecv();
    
    server.sendContent("");
    httpRequestProcessed = true;

    return true;
  }

  return false;
}

String EspIRController::EspIRControllerRequestHandler::getIRForm(String action, String remote) {
  // list of remotes
  String dir = getRemoteDirUri(), result, loadName;
  Dir remoteDir = SPIFFS.openDir(dir);
  bool hasRemote=(remoteDir.next());

  if (action == "remove" && remote != "")
    return String(SPIFFS.remove(getRemoteDevUri(remote)) ? "deleted" : "not found"); 
  
  if (action == "form" && hasRemote) {
    result = "<script>var o=gE('mDCC');";
    result += "function wLS(r,c){if(typeof c==='undefined')c=0;console.log('wLS: '+r+' '+c);if(c>=10){alert('error loading script');return;}if(typeof initIR==='undefined')window.setTimeout('wLS(\\\''+r+'\\\','+(c+1)+')',100);else window.setTimeout('initIR(\\\''+r+'\\\')',100);}";
    result += "o.innerHTML='<div class=\"menu\">";

    // list of remotes
    do {
      String remoteName = remoteDir.fileName().substring(dir.length());
      if (remoteName.indexOf(".") > 1)
        remoteName = remoteName.substring(0, remoteName.indexOf("."));
      if (loadName == "" || mLastLoadedRemote == remoteName)
        loadName = remoteName;
      result += "<a class=\"dc\" onclick=\"lR(\\\'" + remoteName + "\\\')\">" + remoteName + "</a>";
    } while(remoteDir.next());

    // upload remote & close menu
    result += "<sp><a class=\"dc\" onclick=\"lL()\">...</a></sp></div>";
    result += "<link rel=\"stylesheet\" type=\"text/css\" href=\"/static/remote.css\"><div id=\"rm\" class=\"rm\"></div>';";
    result += "aSU('/static/remote.js',o);aSU('" + getRemoteDevUri(loadName) + "',o,'" + loadName + ".Remote');window.setTimeout('wLS(\\\'" + loadName + "\\\')');</script>";

    return result;
  }
  
  if (action == "list" && hasRemote) {
    String upload = htmlInput("remote", "file", "", 0, "", "", "");
    String html = F("<link rel=\"stylesheet\" type=\"text/css\" href=\"/static/remote.css\">");
    html += F("<div class=\"menu\"><a class=\"dc\" onclick=\"lU()\">Add</a><a class=\"dc\" onclick=\"lS()\">Sample</a><sp><a class=\"dc\" onclick=\"lF()\">...</a></sp></div>");
    html += F("<table class=\"rL\">");
    html += F("<tr><th>Name</th><th>Size</th><th/></tr>");
    
    do {
      String remoteName = remoteDir.fileName().substring(dir.length());
      if (remoteName.indexOf(".") > 1)
        remoteName = remoteName.substring(0, remoteName.indexOf("."));
      html += "<tr><td>" + remoteName + "</td><td>" + String(remoteDir.fileSize() / 1024.0f) + "k</td><td class=\"dR\" onclick=\"rR()\">&ndash;</td></tr>";
    } while(remoteDir.next());

//    html += F("<tr><td colspan=\"2\"></td><td class=\"uR aB\" onclick=\"lU()\">+</td></tr>");
    html += F("</table>");
    
    result = "<script>var o=gE('mDCC');";
    result += "o.innerHTML='" + html + "';";
    result += "aSU('/static/remote.js',o);</script>";
    return result;
  }

  if (action == "sample") {
    String html = F("<link rel=\"stylesheet\" type=\"text/css\" href=\"/static/remote.css\">");
    html += F("<div class=\"menu\"><sp><a class=\"dc\" onclick=\"lL()\">...</a></sp></div>");
    html += F("<h4>sample IR Codes</h4>");
    html += F("<div id=\"sIR\" class=\"dc sIR\" onclick=\"rcvS()\">Start</div><br><div id=\"rd\" class=\"rd\"><table class=\"rdt\"><thead><tr><th>Type</th><th>Data</th><th>Size</th><th/></thead><tbody id=\"rdtO\"/></table></div>");
    
    result = "<script>var o=gE('mDCC');";
    result += "o.innerHTML='" + html + "';";
    result += "aSU('/static/remote.js',o);";
//    result += "window.setTimeout('rcvS()', 100);//autostart\n";
    result += "</script>";

    return result;
  }

  if (action == "upload" || ((action == "form" || action == "list") && !hasRemote)) {
    String action = F("/remote/upload_");
    action += EspWiFi::getChipID();
    action += "?ir=&remote=";
    
    String upload = htmlInput("remote", "file", "", 0, "", "", "");
    String html = F("<div class=\"menu\">");
    if (!hasRemote)
      html += F("<a class=\"dc\" onclick=\"lS()\">Sample</a>");
    if (hasRemote)
      html += F("<sp><a class=\"dc\" onclick=\"lL()\">...</a></sp>");
    html += F("</div>");
    html += F("<link rel=\"stylesheet\" type=\"text/css\" href=\"/static/remote.css\">");
    html += F("<h4>Upload</h4>");
    html += htmlForm(upload, action, "post", "submitForm", "multipart/form-data", "");

    result = "<script>var o=gE('mDCC');";
    result += "o.innerHTML='" + html + "';";
    if (!hasRemote)
      result += "aSU('/static/remote.js',o);";
    result += "</script>";

    return result;
  }
  
  return result;
}

String EspIRController::EspIRControllerRequestHandler::menuHtml() {
  return htmlMenuItem(menuIdentifierIR(), "Remotes");
}

uint8_t EspIRController::EspIRControllerRequestHandler::menuIdentifiers() {
  return 1;
}

String EspIRController::EspIRControllerRequestHandler::menuIdentifiers(uint8_t identifier) {
  switch (identifier) {
    case 0: return menuIdentifierIR();break;
  }

  return "";
}

#endif  // ESP8266 || ESP32

