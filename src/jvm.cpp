 #include <jni.h>
#include "jvm.h"
#include "xplane.h"
 #include "XPLMPlugin.h"
 #include "XPLMDisplay.h"
 #include "XPLMGraphics.h"
 #include "XPLMMenus.h"
 #include "XPLMProcessing.h"
 #include "XPLMDataAccess.h"
 #include "XPLMMenus.h"
 #include "XPLMUtilities.h"
 #include "XPWidgets.h"
 #include "XPStandardWidgets.h"
 #include "XPLMScenery.h"
 #include <string.h>
 #include <stdio.h>
  #include <math.h>
  #include "Settings.h"
  //#include "vec_opps.h"
  #include "Simulation.h"
  #include "aiplane.h"
  
  #include <stdlib.h>
 #include <vector>
 #include <ctype.h>//isspace
 #if defined(__linux__) || defined(__APPLE__)
 #include <dlfcn.h>
 #include <stdlib.h>
 #endif

 const char* plugin_version = "About:0.8.72";
char gBob_debstr2[2048];
char xp_path[512];
char CONFIG_FILE_DEFAULT_AIRFRAMES[] ="Resources/plugins/java/airframes_860.txt";
 bool file_exists (const std::string& name);
 void				draw_atc_text(XPLMWindowID in_window_id, void * in_refcon);
void				draw_about_text(XPLMWindowID in_window_id, void * in_refcon);
void	            draw_jvm_text(XPLMWindowID in_window_id, void * in_refcon);
int					dummy_mouse_handler(XPLMWindowID in_window_id, int x, int y, int is_down, void * in_refcon) { return 0; }
XPLMCursorStatus	dummy_cursor_status_handler(XPLMWindowID in_window_id, int x, int y, void * in_refcon) { return xplm_CursorDefault; }
int					dummy_wheel_handler(XPLMWindowID in_window_id, int x, int y, int wheel, int clicks, void * in_refcon) { return 0; }
void				dummy_key_handler(XPLMWindowID in_window_id, char key, XPLMKeyFlags flags, char virtual_key, void * in_refcon, int losing_focus) { }
//bool hot=false;
bool enabledATCPro;
std::mutex command_mutex;
std::mutex string_mutex;
std::mutex int_mutex;
void menu_handler(void *, void *);

 XPLMDataRef  transponder_codeRef = NULL;
 XPLMDataRef  latitudeRef = NULL;
 XPLMDataRef  longitudeRef = NULL;
 XPLMDataRef  altitudeRef = NULL;
 XPLMDataRef  battery_onRef = NULL;
XPLMDataRef  com1_onRef = NULL;
XPLMDataRef  iasRef = NULL;
XPLMDataRef gyroHeadingRef = NULL;
XPLMDataRef altPressureRef = NULL;
XPLMDataRef  mslPressureRef = NULL;
XPLMDataRef fpmRef = NULL;
XPLMDataRef  pitchRef = NULL;
XPLMDataRef  rollRef = NULL;
 //XPLMDataRef  com1_freq_hzRef = NULL;
 //XPLMDataRef  com1_stdby_freq_hz = NULL;
 //const char* CONFIG_FILE ="Resources/plugins/java/jvmsettings.txt";
 String::String(const char *c) 
{ 
    size = strlen(c); 
    s = new char[size+1]; 
    strcpy(s,c); 
} 
  
String::~String() 
{ 
    delete []s; 
} 
std::vector<AirframeDef> airframeDefs;
AirframeDef::AirframeDef(void){
    
}
void AirframeDef::setData(std::string inLine){
    std::size_t found = inLine.find(",");
    std::string pathS = inLine.substr (0,found);
    std::size_t found2 = inLine.find(",",found+1);
    std::string offsetS = inLine.substr (found+1,found2);
    std::size_t found3 = inLine.find(",",found2+1);
    std::string soundS = inLine.substr (found2+1,found3);
    std::string drefStyleS = inLine.substr (found3+1);

    //strcpy(path,pathS.c_str());
   // sprintf (path, "%s", pathS.c_str());
    XPLMGetSystemPath(xp_path);
    sprintf (path, "%s%s", xp_path, pathS.c_str());
    char* end;
    //
    yOffset=strtod(offsetS.c_str(),&end);
    soundIndex=strtol(soundS.c_str(),&end,10);
    drefStyle=strtol(drefStyleS.c_str(),&end,10);

    /* std::size_t foundwt3 = inLine.find("WorldTraffic");

     if(foundwt3==std::string::npos)
        yOffset=0;
     else
        yOffset=2.0;
    //sprintf (path, "%s", inLine.c_str());*/
}
char* AirframeDef::getPath(void){
    return path;
}
double AirframeDef::getOffset(void){
    return yOffset;
}
int AirframeDef::getSound(void){
    return soundIndex;
}
int AirframeDef::getDrefStyle(void){
    return drefStyle;
}
JVM::JVM(void)
{
    logPage=0;
    lastNavAudio=0;
    lastFoundNav=-1;
    nav1_nav_id = XPLMFindDataRef("sim/cockpit2/radios/indicators/nav1_nav_id");//	byte[150]	n	string	current selected navID - nav radio 1
    nav2_nav_id = XPLMFindDataRef("sim/cockpit2/radios/indicators/nav2_nav_id");//	byte[150]	n	string	current selected navID - nav radio 2
    adf1_nav_id = XPLMFindDataRef("sim/cockpit2/radios/indicators/adf1_nav_id");//	byte[150]	n	string	current selected navID - ADF 1
    adf2_nav_id = XPLMFindDataRef("sim/cockpit2/radios/indicators/adf2_nav_id");//	byte[150]	n	string	current selected navID - ADF 2
    audio_selection_nav1 = XPLMFindDataRef("sim/cockpit2/radios/actuators/audio_selection_nav1");
    audio_selection_nav2 = XPLMFindDataRef("sim/cockpit2/radios/actuators/audio_selection_nav2");
    audio_selection_adf1 = XPLMFindDataRef("sim/cockpit2/radios/actuators/audio_selection_adf1");
    audio_selection_adf2 = XPLMFindDataRef("sim/cockpit2/radios/actuators/audio_selection_adf2");
    //data_mutex = new std::mutex();
}
JVM::~JVM(void){
    //delete data_mutex;
}
/*
addSystemClassLoaderPath
Adds a jar to an existing JVM connection
Curtosy of 
dpospisi
Dominik Pospisil, Airfoillabs
May 15 2019
See
https://forums.x-plane.org/index.php?/forums/topic/175706-sharing-the-jvm/
*/
void JVM::addSystemClassLoaderPath(const char* filePath) {
    
    // construct URL from file path
    
    jstring fpStr = env->NewStringUTF(filePath);
        
    jclass cls = env->FindClass("java/io/File");
    jmethodID mtdId = env->GetMethodID(cls, "<init>", "(Ljava/lang/String;)V");
    jobject file = env->NewObject(cls, mtdId, fpStr);
    
    mtdId = env->GetMethodID(cls, "toURI", "()Ljava/net/URI;");
    jobject uri = env->CallObjectMethod(file, mtdId);
    
    cls = env->FindClass("java/net/URI");
    mtdId = env->GetMethodID(cls, "toURL", "()Ljava/net/URL;");
    jobject url = env->CallObjectMethod(uri, mtdId);    
        
    // get system classloader
    jclass classloaderClass = env->FindClass("java/lang/ClassLoader");
    mtdId = env->GetStaticMethodID(classloaderClass, "getSystemClassLoader", "()Ljava/lang/ClassLoader;");
    jobject sysClsLoader = env->CallStaticObjectMethod(classloaderClass, mtdId);
        
    // get addURL method 
    jclass urlClassloaderClass = env->FindClass("java/net/URLClassLoader");
    mtdId = env->GetMethodID(urlClassloaderClass, "addURL", "(Ljava/net/URL;)V");
    
    // add url
    env->CallVoidMethod(sysClsLoader, mtdId, url);  
    env->DeleteLocalRef(fpStr);   
    env->DeleteLocalRef(file);  

}
/*
connectJVM
Connect to an existing JVM or create a new one
Curtosy of 
dpospisi
Dominik Pospisil, Airfoillabs
May 15 2019
(with modifications - mSparks, July 2019)
See
https://forums.x-plane.org/index.php?/forums/topic/175706-sharing-the-jvm/
*/
bool JVM::connectJVM() {
    

    //getcreatedjvms = (jint(*)(JavaVM**, jsize, jsize *)) dlsym(jvmlib, "JNI_GetCreatedJavaVMs");
#if defined(__linux__)
     sprintf(gBob_debstr2,"AutoATC:Loading jvm dll '%s' \n", lin);
     XPLMDebugString(gBob_debstr2);
     libnativehelper = dlopen(lin, RTLD_NOW);//"libjvm.so"
        if (libnativehelper==NULL) {
            hasjvm=false;
            dlerror();
            popupNoJVM();
             createMenu();
            return false;
        }
        else{
            JNI_CreateJavaVM = (JNI_CreateJavaVM_t) dlsym(libnativehelper, "JNI_CreateJavaVM");
            JNI_GetCreatedJavaVMs = (JNI_GetCreatedJavaVMs_t) dlsym(libnativehelper, "JNI_GetCreatedJavaVMs");
        }
    sprintf(gBob_debstr2,"AutoATC:Successfully loaded the jvm .so\n");
    XPLMDebugString(gBob_debstr2);
 #elif defined(_WIN64)
  sprintf(gBob_debstr2,"AutoATC:Loading jvm dll '%s' \n", win);
     XPLMDebugString(gBob_debstr2);
     SetErrorMode(0); 
    libnativehelper = LoadLibraryA(win); //"jvm.dll"
    if (!libnativehelper) {
            hasjvm=false;
            popupNoJVM();
             createMenu();
            return false;
     }
    else{
        sprintf(gBob_debstr2,"AutoATC:Successfully loaded the jvm dll\n");
        JNI_CreateJavaVM = (JNI_CreateJavaVM_t) GetProcAddress(libnativehelper, "JNI_CreateJavaVM");
        JNI_GetCreatedJavaVMs = (JNI_GetCreatedJavaVMs_t) GetProcAddress(libnativehelper, "JNI_GetCreatedJavaVMs");
    }
    sprintf(gBob_debstr2,"AutoATC:found CreateJavaVM\n");
    XPLMDebugString(gBob_debstr2);
#elif defined(__APPLE__)
    libnativehelper = dlopen(mac, RTLD_NOW);//"/usr/local/jre/lib/server/libjvm.dylib"
    if (!libnativehelper) {
        hasjvm=false;
        dlerror();
        popupNoJVM();
         createMenu();
          return false;
     }
      else{
          JNI_CreateJavaVM = (JNI_CreateJavaVM_t) dlsym(libnativehelper, "JNI_CreateJavaVM");
          JNI_GetCreatedJavaVMs = (JNI_GetCreatedJavaVMs_t) dlsym(libnativehelper, "JNI_GetCreatedJavaVMs");
    }
    
    sprintf(gBob_debstr2,"AutoATC:Successfully loaded the jvm .so\n");
    XPLMDebugString(gBob_debstr2);
#endif
    jsize nVMs;
    JNI_GetCreatedJavaVMs(NULL, 0, &nVMs); // 1. just get the required array length
    JavaVM** jvms = new JavaVM*[nVMs];
    JNI_GetCreatedJavaVMs(jvms, nVMs, &nVMs);

    if (nVMs > 0) {
        sprintf(gBob_debstr2,"AutoATC:JVM already created!\n");
        XPLMDebugString(gBob_debstr2);      
       //env = jvms[0];
        
        jint jret = jvms[0]->GetEnv((void**) &env, JNI_VERSION_1_6);
        delete [] (jvms);
        if (jret == JNI_EDETACHED) {
            sprintf(gBob_debstr2,"AutoATC:JVM detached!\n");
            XPLMDebugString(gBob_debstr2);          
            return false;
        }
        if (jret != JNI_OK) {
            sprintf(gBob_debstr2,"AutoATC:JVM GetEnv failed:%d\n",jret);
            XPLMDebugString(gBob_debstr2);          
            return false;            
        }
        jint ver = env->GetVersion();
        sprintf(gBob_debstr2,"AutoATC:Java Version %d.%d \n", ((ver>>16)&0x0f),(ver&0x0f));
        XPLMDebugString(gBob_debstr2);
        jvm= jvms[0]; 
                      
        return true;
    }
    else{
        //no existing JVM, make a new one
        delete[] jvms;
        JavaVMInitArgs vm_args;                        // Initialization arguments
       vm_args.version = JNI_VERSION_1_6;             // minimum Java version
       JavaVMOption* options = new JavaVMOption[5];
        options[0].optionString = "-XX:+UseG1GC";
        options[1].optionString = "-XX:MaxGCPauseMillis=2";
        //options[2].optionString = "-XX:+UseStringCache";
        options[2].optionString = "-XX:ParallelGCThreads=4";
        options[3].optionString = "-XX:ConcGCThreads=4";
        vm_args.nOptions = 4;
        vm_args.options = options; 
       //vm_args.nOptions = 0; 
                               // number of options
       vm_args.ignoreUnrecognized = false;     // invalid options make the JVM init fail
           //=============== load and initialize Java VM and JNI interface =============
       JNI_CreateJavaVM(&jvm, (void**)&env, &vm_args);  // YES !!
      
      
       jint ver = env->GetVersion();
       sprintf(gBob_debstr2,"AutoATC:Java Version %d.%d \n", ((ver>>16)&0x0f),(ver&0x0f));
       XPLMDebugString(gBob_debstr2);
       return true;
    }
}
void JVM::deactivateJVM(void){
    if(hasjvm){
        stop();
        destroyMenu(); 
    }   
    //hasjvm=false;  
}
void JVM::activateJVM(void){
    destroyMenu();
    try{
    
    //hasjvm=false;
    if(!loadedLibrary){
        if(!connectJVM())
            return;
        addSystemClassLoaderPath("Resources/plugins/java/AutoATCPlugin.jar");

       
       commandsClass = env->FindClass("jni/Commands");  // try to find the class
    if(commandsClass == NULL) {
        sprintf(gBob_debstr2,"AutoATC ERROR: Commands not found !\n");
        XPLMDebugString(gBob_debstr2);
    }
    else {                                  // if class found, continue
        sprintf(gBob_debstr2,"AutoATC Commands found\n");
        XPLMDebugString(gBob_debstr2);
        
        getPlaneDataMethod = env->GetStaticMethodID(commandsClass, "getPlaneData", "(I)[D");  // find method
        if(getPlaneDataMethod == NULL){
            XPLMDebugString("ERROR: method void getPlaneData() not found !\n");
            return;
        }
        midToString=env->GetStaticMethodID(commandsClass, "getData", "(Ljava/lang/String;)Ljava/lang/String;");  // find method
        if(midToString == NULL){
            XPLMDebugString("ERROR: method void getData() not found !\n");
            return;
        }
        
        
        printf("AutoATC active !\n");
        sprintf(gBob_debstr2,"AutoATC Dev1 active !\n");
         XPLMDebugString(gBob_debstr2); 
         hasjvm=true;
    }
    loadedLibrary=true;
    }
    else{
        printf("AutoATC active !\n");
        sprintf(gBob_debstr2,"AutoATC active !\n");
         XPLMDebugString(gBob_debstr2);  
         //hasjvm=true;
    }
    }
catch(...){
		printf("Exception during JVM test\n");
        sprintf(gBob_debstr2,"Exception during JVM test\n");
         XPLMDebugString(gBob_debstr2);  
		return;
	}  
    createMenu();
}


char *JVM::trim (char * s)
{
  /* Initialize start, end pointers */
  char *s1 = s, *s2 = &s[strlen (s) - 1];

  /* Trim and delimit right side */
  while ( (isspace (*s2)) && (s2 >= s1) )
    s2--;
  *(s2+1) = '\0';

  /* Trim left side */
  while ( (isspace (*s1)) && (s1 < s2) )
    s1++;

  /* Copy finished string */
  strcpy (s, s1);
  return s;
}
void JVM::getMorse (){
    int navdata[150];
    navdata[149]=0;
    char navaid[500];
    bool found=false;
    int size=XPLMGetDatab(nav1_nav_id,navdata,0,150);
    int audio=XPLMGetDatai(audio_selection_nav1);
    int thisFoundNav=-1;
    if(audio>0&&navdata[0]>0){
        found=true;
        thisFoundNav=1;
    }
    if(!found){
        size=XPLMGetDatab(nav2_nav_id,navdata,0,150);
        audio=XPLMGetDatai(audio_selection_nav2);
        if(audio>0&&navdata[0]>0){
            found=true;
            thisFoundNav=2;
        }
    }
    if(!found){
        size=XPLMGetDatab(adf1_nav_id,navdata,0,150);
        audio=XPLMGetDatai(audio_selection_adf1);
        if(audio>0&&navdata[0]>0){
            found=true;
            thisFoundNav=3;
        }
    }
    if(!found){
        size=XPLMGetDatab(adf2_nav_id,navdata,0,150);
        audio=XPLMGetDatai(audio_selection_adf2);
        if(audio>0&&navdata[0]>0){
            found=true;
            thisFoundNav=4;
        }
    }
    if(lastFoundNav>0&&lastFoundNav!=thisFoundNav){
         getData("doCommand:stopMorse");
         //printf("doCommand:stopMorse %d %d\n", lastFoundNav, thisFoundNav);
         lastFoundNav=thisFoundNav;
    }
    float nowT=getSysTime();//clock()/(CLOCKS_PER_SEC*1.0f);
    if(nowT<lastNavAudio+30)
        return;
    
    if(!found)
        lastFoundNav=-1;
    else{
        lastFoundNav=thisFoundNav;
        strcpy(navaid, (char*) &navdata);
        char com[512];
        sprintf(com,"doCommand:playMorse:%s",navaid);
        //printf("run %s\n",com);
        getData(com);
        lastNavAudio=nowT;
    }
    /*
    if(audio>0&&navdata[0]>0){
        strcpy(navaid, (char*) &navdata);
        char com[512];
        sprintf(com,"doCommand:playMorse:%s",navaid);
        getData(com);
        printf("navData 1 %s\n",com);
        found=true;
        lastFoundNav=1;
    }
    if(!found){
        size=XPLMGetDatab(nav2_nav_id,navdata,0,150);
        audio=XPLMGetDatai(audio_selection_nav2);
        if(audio>0&&navdata[0]>0){
            strcpy(navaid, (char*) &navdata);
            char com[512];
            sprintf(com,"doCommand:playMorse:%s",navaid);
            getData(com);
            printf("navData 2 %s\n",com);
            lastFoundNav=2;
            found==true;
        }
    }
    if(!found){
        size=XPLMGetDatab(adf1_nav_id,navdata,0,150);
        audio=XPLMGetDatai(audio_selection_adf1);
        if(audio>0&&navdata[0]>0){
            strcpy(navaid, (char*) &navdata);
            char com[512];
            sprintf(com,"doCommand:playMorse:%s",navaid);
            getData(com);
            printf("adfData 1 %s\n",com);
            lastFoundNav=3;
            found==true;
        }
    }
    if(!found){
        size=XPLMGetDatab(adf2_nav_id,navdata,0,150);
        audio=XPLMGetDatai(audio_selection_adf2);
        if(audio>0&&navdata[0]>0){
            strcpy(navaid, (char*) &navdata);
            char com[512];
            sprintf(com,"doCommand:playMorse:%s",navaid);
            getData(com);
            printf("adfData 2 %s\n",com);
            lastFoundNav=4;
            found==true;
        }
    }*/
    
}
/*void JVM::testExistingJVM (){
    jsize nVMs;
    JNIEnv *local_env;
    JNI_GetCreatedJavaVMs(NULL, 0, &nVMs); // 1. just get the required array length
    JavaVM** jvms = new JavaVM*[nVMs];
    JNI_GetCreatedJavaVMs(jvms, nVMs, &nVMs);
    if(nVMs==1){
        jvms[0]->AttachCurrentThread((void**)&local_env, NULL);
         if(local_env == NULL)
            printf("failed to attached \n");
            else{
                jclass apiClass = local_env->FindClass("api/XPJVMAPI");  // try to find the class
                if(apiClass != NULL)
                    printf("searched and found XPJVMAPI \n");
                else
                    printf("searched and NOT found XPJVMAPI \n");
            }
    }
}*/
void JVM::init_parameters ()
{

    strncpy (win, "jvm.dll", MAXLEN);
    strncpy (lin, "libjvm.so", MAXLEN);
    strncpy (mac, "/usr/local/jre/lib/server/libjvm.dylib", MAXLEN);
    airframeDefs.clear();
    XPLMGetSystemPath(xp_path);
    transponder_codeRef = XPLMFindDataRef("sim/cockpit/radios/transponder_code");
    latitudeRef = XPLMFindDataRef("sim/flightmodel/position/latitude");
    longitudeRef = XPLMFindDataRef("sim/flightmodel/position/longitude");
    altitudeRef = XPLMFindDataRef ("sim/flightmodel/position/elevation");
    battery_onRef = XPLMFindDataRef ("sim/cockpit/electrical/battery_array_on");
    com1_onRef = XPLMFindDataRef ("sim/cockpit2/radios/actuators/com1_power");
    com1_freq_hzRef = XPLMFindDataRef ("sim/cockpit/radios/com1_freq_hz");
    com1_stdby_freq_hz = XPLMFindDataRef ("sim/cockpit/radios/com1_stdby_freq_hz");
    iasRef = XPLMFindDataRef ("sim/cockpit2/gauges/indicators/airspeed_kts_pilot");
    gyroHeadingRef = XPLMFindDataRef ("sim/cockpit2/gauges/indicators/heading_vacuum_deg_mag_pilot");;
    altPressureRef = XPLMFindDataRef ("sim/cockpit2/gauges/indicators/altitude_ft_pilot");
    mslPressureRef = XPLMFindDataRef ("sim/cockpit2/gauges/actuators/barometer_setting_in_hg_pilot");
    fpmRef = XPLMFindDataRef ("sim/cockpit2/gauges/indicators/vvi_fpm_pilot");
    pitchRef = XPLMFindDataRef ("sim/cockpit2/gauges/indicators/pitch_vacuum_deg_pilot");
    rollRef = XPLMFindDataRef ("sim/cockpit2/gauges/indicators/roll_vacuum_deg_pilot");
    sysTimeRef = XPLMFindDataRef("sim/time/total_running_time_sec");
    standbyAirframe=AirframeDef();
    standbyAirframe.setData("Resources/default scenery/airport scenery/Aircraft/Heavy_Metal/747United.obj,0.0,0,0");
    std::ifstream t("Resources/plugins/AutoATC/notepad.txt");
    std::stringstream notepadss;
    notepadss << t.rdbuf();
    sprintf(notepad,notepadss.str().c_str());
    XPLMDebugString(notepad);
    live=false;
}
void JVM::init_parameters (char * jvmfilename)
{
 // struct file_parameters * lparms=&parms;
  strncpy (win,jvmfilename, MAXLEN);
  strncpy (lin,jvmfilename, MAXLEN);
  strncpy (mac,jvmfilename, MAXLEN);
  strncpy (device,"any", MAXLEN);
  strncpy (slave,"0", MAXLEN);
  //airframeDefs.clear();
  XPLMGetSystemPath(xp_path);
}

void JVM::parse_config (char * filename)
{
   // struct file_parameters * lparms=&parms;
  char *s, buff[MAXLEN];
  FILE *fp = fopen (filename, "r");
  if (fp == NULL)
  {
    return;
  }

  /* Read next line */
  while ((s = fgets (buff, sizeof buff, fp)) != NULL)
  {
    /* Skip blank lines and comments */
    if (buff[0] == '\n' || buff[0] == '#')
      continue;

    /* Parse name/value pair from line */
    char name[MAXLEN], value[MAXLEN];
    s = strtok (buff, "=");
    if (s==NULL)
      continue;
    else
      strncpy (name, s, MAXLEN);
    s = strtok (NULL, "=");
    if (s==NULL)
      continue;
    else
      strncpy (value, s, MAXLEN);
    trim (value);

    /* Copy into correct entry in parameters struct */
    if (strcmp(name, "win")==0)
      strncpy (win, value, MAXLEN);
    else if (strcmp(name, "lin")==0)
      strncpy (lin, value, MAXLEN);
    else if (strcmp(name, "mac")==0)
      strncpy (mac, value, MAXLEN);
    else if (strcmp(name, "phone")==0)
      strncpy (device, value, MAXLEN);
    else if (strcmp(name, "isSlave")==0)
      strncpy (slave, value, MAXLEN);
    else{
        /*sprintf(gBob_debstr2,"WARNING: %s/%s: Unknown name/value pair!\n",
        name, value);
       XPLMDebugString(gBob_debstr2);*/
        AirframeDef def=AirframeDef();
        def.setData(value);
        airframeDefs.push_back(def);
    }
  }

  /* Close file */
  fclose (fp);
}
float JVM::getSysTime(void){
    float retVal=XPLMGetDataf(sysTimeRef);
    //printf("getTime %f\n",retVal);
    return retVal;
}
void JVM::start(void)
{
    if(!hasjvm)
        return;
    XPLMGetSystemPath(xp_path);
    sprintf(gBob_debstr2,"AutoATC Start!\n");
    fireTransmit=false;
    fireNewFreq=true;
    XPLMDebugString(gBob_debstr2);
     
     
    
     jmethodID startMethod=env->GetStaticMethodID(commandsClass, "start", "()V");  // find method
     env->CallStaticVoidMethod(commandsClass, startMethod);                      // call method
     setIcaov=false;
}
void JVM::joinThread(void){
    JavaVMAttachArgs args;
	args.version = JNI_VERSION_1_6; // choose your JNI version
	args.name = NULL; // you might want to give the java thread a name
	args.group = NULL; // you might want to assign the java thread to a ThreadGroup
	jvm->AttachCurrentThread((void**)&plane_env, &args);
    threadcommandsClass = plane_env->FindClass("jni/Commands");  // try to find the class
    if(threadcommandsClass == NULL) {
        sprintf(gBob_debstr2,"AutoATC ERROR: Commands not found !\n");
        XPLMDebugString(gBob_debstr2);
    }
    getPlaneDataThreadMethod = plane_env->GetStaticMethodID(threadcommandsClass, "getPlaneData", "(I)[D");  // find method
    if(getPlaneDataThreadMethod == NULL){
            XPLMDebugString("ERROR: method void getPlaneData() not found !\n");
            return;
    }
    setThreadDataMethod = plane_env->GetStaticMethodID(threadcommandsClass, "setData", "([F)V");  // find method
       
         if(setThreadDataMethod == NULL){
            XPLMDebugString("ERROR: method void setData(float[]) not found !\n");
            return;
         }
    getStndbyMethod=plane_env->GetStaticMethodID(threadcommandsClass, "getStndbyFreq", "(II)I");  // find method
     if(getStndbyMethod == NULL){
         XPLMDebugString("ERROR: method int getStndbyFreq() not found !\n");
          return;
       }
    commandString=plane_env->GetStaticMethodID(threadcommandsClass, "getCommandData", "()Ljava/lang/String;");  // find method
        if(commandString == NULL){
            XPLMDebugString("ERROR: method void getCommandData() not found !\n");
            return;
        }
    midToThreadString=env->GetStaticMethodID(threadcommandsClass, "getData", "(Ljava/lang/String;)Ljava/lang/String;");  // find method
        if(midToThreadString == NULL){
            XPLMDebugString("ERROR: method void getData() not found !\n");
            return;
        }
        threadBroadcastMethod = env->GetStaticMethodID(threadcommandsClass, "broadcast", "()V");  // find method
        
        if(threadBroadcastMethod == NULL){
            XPLMDebugString("ERROR: method void broadcast() not found !\n");
            return;
        }
    printf("thread jvm join\n");
    sprintf(gBob_debstr2,"thread jvm join\n");
    XPLMDebugString(gBob_debstr2); 
}
void JVM::updateAirframes(void){
    if(!file_exists(CONFIG_FILE_DEFAULT_AIRFRAMES)){
         printf("ERROR:AUTOATC Can't find airframes definitions");
        XPLMDebugString("ERROR:AUTOATC Can't find airframes definitions");
     }
    else{
        airframeDefs.clear();
        parse_config(CONFIG_FILE_DEFAULT_AIRFRAMES);
        for (unsigned i=0; i<airframeDefs.size(); i++){
            sprintf(gBob_debstr2,"AutoATC Airframe %d is %s. at %f \n",i, airframeDefs[i].getPath(),airframeDefs[i].getOffset());
           // printf(gBob_debstr2);
            XPLMDebugString(gBob_debstr2);
        }
        if(airframeDefs.size()==0){
            //we dont have any definitions
            printf("no airframes found, using Resources/default scenery/airport scenery/Aircraft/General_Aviation/KingAirC90B.obj as default\n");
            XPLMDebugString("no airframes found, using Resources/default scenery/airport scenery/Aircraft/General_Aviation/KingAirC90B.objj as default");
            AirframeDef def=AirframeDef();
            //def.setData("Resources/default scenery/airport scenery/Aircraft/Heavy_Metal/747United.obj,0.0,0,0");
            def.setData("Resources/default scenery/airport scenery/Aircraft/General_Aviation/KingAirC90B.obj,0.0,1,0");
            airframeDefs.push_back(def);
        }
    }
}
void JVM::stop(void)
{
    if(!hasjvm)
        return;
    //if(enabledATCPro)
    {
            //printf("JVM STOP\n");
            sprintf(gBob_debstr2,"JVM STOP\n");
            XPLMDebugString(gBob_debstr2); 
            stopPlanes();
            enabledATCPro=false;
    }
    

    systemstop();
    jvm->DetachCurrentThread();
     printf("AutoATC Stop!\n");
     sprintf(gBob_debstr2,"AutoATC Stop!\n");
            XPLMDebugString(gBob_debstr2); 
     hasjvm=false;
     //XPLMDebugString(gBob_debstr2);
}
void JVM::systemstop(void)
{
    if(!hasjvm)
        return;
    
    

    jmethodID stopMethod=env->GetStaticMethodID(commandsClass, "stop", "()V");  // find method
     if(stopMethod == NULL)
      {
     sprintf(gBob_debstr2,"Couldnt find stop method \n");
           // printf(gBob_debstr2);
            XPLMDebugString(gBob_debstr2);
     }
     else
        env->CallStaticVoidMethod(commandsClass, stopMethod);                      // call method*/
    
     //XPLMDebugString(gBob_debstr2);
}
void JVM::broadcast(void){
    if(!hasjvm)
        return;
    command_mutex.lock();  
    fireTransmit=true;
    command_mutex.unlock();
    
}
char retlogpageData[2048]={0};
char* JVM::getLogData(const char* reference){
    string_mutex.lock();
    sprintf(logpageString,"%s",reference);
    getLogTime=clock()/(CLOCKS_PER_SEC*1.0f);
    //printf("getLogData %s\n",logpageString);
    
    sprintf(retlogpageData,"%s",logpageData);
    string_mutex.unlock();
    return retlogpageData;
}
void JVM::retriveLogData(){
    if(!hasjvm){
        string_mutex.lock();
        sprintf(logpageData,"No JVM|");
        string_mutex.unlock();
        return;
    }   
    //string_mutex.lock();
    if(strlen(logpageString)<2){
        //string_mutex.unlock();
        return;
    }
    double nowT=clock()/(CLOCKS_PER_SEC*1.0f);
    if(nowT>(getLogTime+5)){
        //string_mutex.unlock();
        return;
    }
    jstring stringArg1 = plane_env->NewStringUTF(logpageString);
    //string_mutex.unlock();
    jstring jstr = (jstring) plane_env->CallStaticObjectMethod(threadcommandsClass, midToThreadString,stringArg1);
    env->DeleteLocalRef(stringArg1);
    const char* nativeString = plane_env->GetStringUTFChars(jstr, JNI_FALSE);
    string_mutex.lock();
    sprintf(logpageData,"%s",(char *)nativeString);
    //printf("retriveLogData %s\n",logpageString);
    string_mutex.unlock();
    plane_env->ReleaseStringUTFChars(jstr, nativeString);
}
jstring JVM::getData(const char* reference){
    //jmethodID 
    if(!hasjvm)
        return ( jstring)"No JVM|";
    jstring stringArg1 = env->NewStringUTF(reference);
    jstring jstr = (jstring) env->CallStaticObjectMethod(commandsClass, midToString,stringArg1);
    env->DeleteLocalRef(stringArg1);
    
    return jstr;
}
int JVM::getStndbyFreq(int roll){
    if(!hasjvm)
        return -1;
    printf("get standby %d\n",roll);  

    //jint fOnehundred = (jint) env->CallStaticIntMethod(commandsClass, getStndbyMethod,roll,logPage);
    int retVal=0;
    int_mutex.lock();
    retVal=standbyFreqInt;
    standbyRoll=roll;
    fireNewFreq=true;
    int_mutex.unlock();

    return retVal;
}
void JVM::updateStndbyFreq(){
    if(!fireNewFreq)
        return;
    int_mutex.lock();
    jint fOnehundred = (jint) plane_env->CallStaticIntMethod(threadcommandsClass, getStndbyMethod,standbyRoll,logPage);
    standbyFreqInt=fOnehundred;
    printf("set standbyRoll %d\n",fOnehundred);  
    fireNewFreq=false;
    int_mutex.unlock();
}
void JVM::setThreadData(){
    if(!hasjvm)
        return;
    jfloatArray planeDataV=plane_env->NewFloatArray(14);
    command_mutex.lock();
    plane_env->SetFloatArrayRegion(planeDataV, 0, 14, planeData);
    command_mutex.unlock();
    plane_env->CallStaticVoidMethod(threadcommandsClass, setThreadDataMethod,planeDataV); 
    plane_env->DeleteLocalRef(planeDataV);
}
void JVM::doTransmit(){
    command_mutex.lock();
    plane_env->CallStaticVoidMethod(threadcommandsClass, threadBroadcastMethod);                      // call method
    fireTransmit=false;
    command_mutex.unlock();
}
void JVM::setData(jfloat jplaneData[]){
    command_mutex.lock();
    for(int i=0;i<14;i++)
        planeData[i]=jplaneData[i];
    command_mutex.unlock();
}
PlaneData JVM::getPlaneData(int id,JNIEnv *caller_env){
    PlaneData retVal;
    if(!hasjvm){
        retVal.live=false;
        return retVal;
    }
    jmethodID method;
    jclass classV;
    if(caller_env==env){
       classV=commandsClass;
       method= getPlaneDataMethod;
    }
    else{
        classV=threadcommandsClass;
       method= getPlaneDataThreadMethod;
    }
        //return retVal;
    jdoubleArray    jplanedata   = (jdoubleArray) caller_env->CallStaticObjectMethod( classV, method,id-1 );

    jdouble *element = caller_env->GetDoubleArrayElements(jplanedata, 0);
   // jsize len = env->GetArrayLength(jplanedata);
    /*double v=element[0];
    retVal.phi=(float)v;*/
    if(element[0]==0&&element[1]==0&&element[2]==0)
        {
            retVal.live=false;
            return retVal;
        }
    retVal.live=true;
    if(element[2]==-9999.0)
        return retVal;
    XPLMWorldToLocal(element[0],element[1],element[2],&retVal.x,&retVal.y,&retVal.z);
    retVal.the=(float)element[3];
    retVal.phi=(float)element[4];
    retVal.psi=(float)element[5];
    retVal.gearDown=(float)element[6];
    retVal.throttle=element[7];
    retVal.timeStamp=element[8];
    retVal.airframe=(int)element[9];
    caller_env->ReleaseDoubleArrayElements(jplanedata,element,0);
    return retVal;
}

void JVM::getThreadCommandData(){
    if(!hasjvm)
        return;
    jstring jstr = (jstring) plane_env->CallStaticObjectMethod(threadcommandsClass, commandString);
    //char buffer[1024];
    const char* nativeString = plane_env->GetStringUTFChars(jstr, JNI_FALSE);
    size_t length = strlen(nativeString);
    if(length>0){
       command_mutex.lock();
       String *s=new String(nativeString);
       //printf("got command %s\n",s->s);
       commandsList.push_back(s);
       command_mutex.unlock();
    }
    plane_env->ReleaseStringUTFChars(jstr, nativeString);
}
void JVM::getCommandData(){
    command_mutex.lock();
    //printf("commandsList size %d\n",commandsList.size());
    if(commandsList.size()>0){
        
        for(int i=0;i<commandsList.size();i++){
            std::string commandData (commandsList[i]->s); 
             //printf("in commandsList %d\n",commandsList.size());
            std::size_t found = commandData.find("=");
            //printf("found %d\n",found);
            if (found==std::string::npos){
                 //printf("fire command %s\n",commandsList[i]->s);
                XPLMCommandRef cmd= XPLMFindCommand(commandsList[i]->s);
           
                if(cmd)
                    XPLMCommandOnce(cmd);
            }
            else{
                //printf("dataref %s\n",commandsList[i]->s);
                std::string dataref = commandData.substr (0,found);
               // printf("dataref %s %s\n",commandsList[i]->s,dataref.c_str());
                XPLMDataRef dRef = XPLMFindDataRef(dataref.c_str());
                
                if(dRef!=NULL){
                   // printf("found dataref %s\n",dataref.c_str());
                    std::string value = commandData.substr (found+1);
                    std::size_t decimalF = value.find(".");
                    char* end;
                    if (decimalF==std::string::npos){
                        long xpvalue=strtol(value.c_str(),&end,10);
                        XPLMSetDatai(dRef, (int)xpvalue);
                        //printf("set dataref %s to %d\n",dataref.c_str(),xpvalue);

                    }
                    else{
                        double xpvalue=strtod(value.c_str(),&end);
                        XPLMSetDataf(dRef, (float)xpvalue);
                        //printf("set dataref %s to %f\n",dataref.c_str(),xpvalue);
                    }
                }
            }
            delete commandsList[i];
        }
        commandsList.clear();
    }
    
    command_mutex.unlock();
}
char* JVM::getModel(int id){
    //if(id>=airframeDefs.size()){
        //updateAirframes();

    //}
    if(id>=airframeDefs.size()||id<0)
        return standbyAirframe.getPath();
    return airframeDefs[id].getPath();
}
acModelDef* JVM::getModelPart(int id,int PartID){
    //if(id>=airframeDefs.size()){
        //updateAirframes();

    //}
    if(id>=airframeDefs.size()||id<0)
        return &standbyAirframe.acDefs[PartID];
    return &airframeDefs[id].acDefs[PartID];
}
double JVM::getOffset(int id){
    /*if(id>=airframeDefs.size()){
        updateAirframes();

    }*/
    if(id>=airframeDefs.size()||id<0)
        return standbyAirframe.getOffset();
    return airframeDefs[id].getOffset();
}
v JVM::getOffset(int id,int PartID){
    
    return airframeDefs[id].acDefs[PartID].partoffsets;
}
int JVM::getSound(int id){
    /*if(id>=airframeDefs.size()){
        updateAirframes();

    }*/
    if(id>=airframeDefs.size()||id<0)
        return standbyAirframe.getSound();
    return airframeDefs[id].getSound();
}
int JVM::getDrefStyle(int id){
    /*if(id>=airframeDefs.size()){
        updateAirframes();

    }*/
    if(id>=airframeDefs.size()||id<0)
        return standbyAirframe.getDrefStyle();
    return airframeDefs[id].getDrefStyle();
}
static JVM jvmO;

void initJVM(){
    
    jvmO=JVM();

}

JVM* getJVM(){

    return &jvmO;
}

void JVM::createMenu(){
    if(g_menu_container_idx==-1)
        g_menu_container_idx = XPLMAppendMenuItem(XPLMFindPluginsMenu(), "AutoATC", 0, 0);
	g_menu_id = XPLMCreateMenu("AutoATC", XPLMFindPluginsMenu(), g_menu_container_idx, menu_handler, NULL);

	if(hasjvm){
        XPLMAppendMenuItem(g_menu_id, "Toggle Log", (void *)"Menu Item 1", 1);
    
        XPLMAppendMenuItem(g_menu_id, "Restart Radio", (void *)"Menu Item 2", 2);
    }
    XPLMAppendMenuItem(g_menu_id, "Settings", (void *)"Menu Item 5", 3);
    XPLMAppendMenuItem(g_menu_id, "About", (void *)"Menu Item 4", 4);
    
    
}
void JVM::registerFlightLoop(){
    if(hasjvm&&!flightLoopActive){
               setICAO();
               XPLMRegisterFlightLoopCallback(	SendATCData,	/* Callback */10,					/* Interval */NULL);					/* refcon not used. */
               sprintf(gBob_debstr2,"Register flight loop\n");
                XPLMDebugString(gBob_debstr2); 
                flightLoopActive=true;
               //initPlanes();
    }
}
void JVM::unregisterFlightLoop(){
    if(hasjvm&&flightLoopActive){
        XPLMUnregisterFlightLoopCallback(SendATCData, NULL);
        sprintf(gBob_debstr2,"Destroy flight loop\n");
        XPLMDebugString(gBob_debstr2); 
        flightLoopActive=false;
    }
}
void JVM::destroyMenu(){
    XPLMDestroyMenu(g_menu_id);
    
}
void JVM::setICAO(){
   //JVM* jvmO=getJVM();
    char icao[256];
    XPLMDataRef	dr_plane_ICAO = XPLMFindDataRef ("sim/aircraft/view/acf_ICAO");
    XPLMGetDatab(dr_plane_ICAO,icao,0,40);
                
    jstring jstr = getData(icao);

    const char* nativeString = env->GetStringUTFChars(jstr, JNI_FALSE);
   // char* astring=(char *)nativeString;   
    env->ReleaseStringUTFChars(jstr, nativeString);
}
void JVM::LogPageWindowPlus(){
    logPage++;
    if(logPage>4)
        logPage=0;
}
void JVM::toggleLogWindow(){
    XPLMDataRef vr_dref =XPLMFindDataRef("sim/graphics/VR/enabled");
    const bool vr_is_enabled = XPLMGetDatai(vr_dref);
    if(log_window==NULL||vr_is_enabled!=loginVR){
         XPLMCreateWindow_t params;
        params.structSize = sizeof(params);
        params.visible = 1;
        params.drawWindowFunc = draw_atc_text;
        // Note on "dummy" handlers:
        // Even if we don't want to handle these events, we have to register a "do-nothing" callback for them
        params.handleMouseClickFunc = dummy_mouse_handler;
        params.handleRightClickFunc = dummy_mouse_handler;
        params.handleMouseWheelFunc = dummy_wheel_handler;
        params.handleKeyFunc = dummy_key_handler;
        params.handleCursorFunc = dummy_cursor_status_handler;
        params.refcon = NULL;
        params.layer = xplm_WindowLayerFloatingWindows;

        params.decorateAsFloatingWindow = xplm_WindowDecorationRoundRectangle;
        
        // Set the window's initial bounds
        // Note that we're not guaranteed that the main monitor's lower left is at (0, 0)...
        // We'll need to query for the global desktop bounds!
        int left, bottom, right, top;
        XPLMGetScreenBoundsGlobal(&left, &top, &right, &bottom);
        params.left = left + 50;
        params.bottom = bottom + 150;
        params.right = params.left + 400;
        params.top = params.bottom + 400;
        
        log_window = XPLMCreateWindowEx(&params);
        
        // Position the window as a "free" floating window, which the user can drag around
        loginVR=vr_is_enabled;
        if(vr_is_enabled)
        {
            XPLMSetWindowPositioningMode(log_window, xplm_WindowVR, 0);
        }
        else
            XPLMSetWindowPositioningMode(log_window, xplm_WindowPositionFree, -1);
        // Limit resizing our window: maintain a minimum width/height of 100 boxels and a max width/height of 300 boxels
        XPLMSetWindowResizingLimits(log_window, 200, 200, 500, 500);
        XPLMSetWindowTitle(log_window, "AutoATC Pad");
        log_visible=true;
    }
    else if(!log_visible){
        XPLMSetWindowIsVisible(log_window,1);
        log_visible=true;
    }
     else{
         XPLMSetWindowIsVisible(log_window,0);
        log_visible=false;
     }
}
void menu_handler(void * in_menu_ref, void * in_item_ref)
{
    JVM* jvmO=getJVM();
	if(!strcmp((const char *)in_item_ref, "Menu Item 1"))
	{
		 jvmO->toggleLogWindow();
   
	}
   
	if(!strcmp((const char *)in_item_ref, "Menu Item 2")){
        if(jvmO->hasjvm){
            if(enabledATCPro){
            printf("JVM MENU STOP\n");
            sprintf(gBob_debstr2,"JVM MENU STOP\n");
            XPLMDebugString(gBob_debstr2); 
                jvmO->systemstop();
            }
           jvmO->start();
        }
        else
            jvmO->popupNoJVM();
    }

    if(!strcmp((const char *)in_item_ref, "Menu Item 4"))
	{
		 
    XPLMCreateWindow_t params;
	params.structSize = sizeof(params);
	params.visible = 1;
	params.drawWindowFunc = draw_about_text;
	// Note on "dummy" handlers:
	// Even if we don't want to handle these events, we have to register a "do-nothing" callback for them
	params.handleMouseClickFunc = dummy_mouse_handler;
	params.handleRightClickFunc = dummy_mouse_handler;
	params.handleMouseWheelFunc = dummy_wheel_handler;
	params.handleKeyFunc = dummy_key_handler;
	params.handleCursorFunc = dummy_cursor_status_handler;
	params.refcon = NULL;
	params.layer = xplm_WindowLayerFloatingWindows;

	params.decorateAsFloatingWindow = xplm_WindowDecorationRoundRectangle;
	
	// Set the window's initial bounds
	// Note that we're not guaranteed that the main monitor's lower left is at (0, 0)...
	// We'll need to query for the global desktop bounds!
	int left, bottom, right, top;
	XPLMGetScreenBoundsGlobal(&left, &top, &right, &bottom);
	params.left = left + 50;
	params.bottom = bottom + 150;
	params.right = params.left + 400;
	params.top = params.bottom + 400;
	
	 XPLMWindowID	g_window = XPLMCreateWindowEx(&params);
	
	// Position the window as a "free" floating window, which the user can drag around
    XPLMDataRef vr_dref =XPLMFindDataRef("sim/graphics/VR/enabled");
    const bool vr_is_enabled = XPLMGetDatai(vr_dref);
    if(vr_is_enabled)
    {
        XPLMSetWindowPositioningMode(g_window, xplm_WindowVR, 0);
    }
    else
	    XPLMSetWindowPositioningMode(g_window, xplm_WindowPositionFree, -1);
	// Limit resizing our window: maintain a minimum width/height of 100 boxels and a max width/height of 300 boxels
	    XPLMSetWindowResizingLimits(g_window, 200, 200, 500, 500);
	    XPLMSetWindowTitle(g_window, "About AutoATC");
	}
    if(!strcmp((const char *)in_item_ref, "Menu Item 5")){
        //printf("called me 1\n");
        settings.init();//open a settings window
    }
         
}
void	draw_about_text(XPLMWindowID in_window_id, void * in_refcon)
{
	// Mandatory: We *must* set the OpenGL state before drawing
	// (we can't make any assumptions about it)
	XPLMSetGraphicsState(
						 0 /* no fog */,
						 0 /* 0 texture units */,
						 0 /* no lighting */,
						 0 /* no alpha testing */,
						 1 /* do alpha blend */,
						 1 /* do depth testing */,
						 0 /* no depth writing */
						 );
	
	int l, t, r, b;
	XPLMGetWindowGeometry(in_window_id, &l, &t, &r, &b);
	
	float col_white[] = {1.0, 1.0, 1.0}; // red, green, blue
    int ww=r-l;
    JVM* jvmO=getJVM();
    if(jvmO->hasjvm){
        jstring jstr = jvmO->getData(plugin_version);
        const char* nativeString = jvmO->env->GetStringUTFChars(jstr, JNI_FALSE);
    
        char* astring=(char *)nativeString;   
	    XPLMDrawString(col_white, l + 10, t - 20, astring, &ww, xplmFont_Proportional);
        jvmO->env->ReleaseStringUTFChars(jstr, nativeString);
    
    }
    else{
        char text[]="Java VM not found, see:\n (X-Plane 11/Resources/plugins/java/defaultjvm.txt)\n For installation instructions \n";
        XPLMDrawString(col_white, l + 10, t - 20, text, &ww, xplmFont_Proportional);
    }
}
  void	draw_atc_text(XPLMWindowID in_window_id, void * in_refcon)
{
	// Mandatory: We *must* set the OpenGL state before drawing
	// (we can't make any assumptions about it)
	XPLMSetGraphicsState(
						 0 /* no fog */,
						 0 /* 0 texture units */,
						 0 /* no lighting */,
						 0 /* no alpha testing */,
						 1 /* do alpha blend */,
						 1 /* do depth testing */,
						 0 /* no depth writing */
						 );
	
	int l, t, r, b;
	XPLMGetWindowGeometry(in_window_id, &l, &t, &r, &b);
	
	float col_white[] = {1.0, 1.0, 1.0}; // red, green, blue
    int ww=r-l;
    JVM* jvmO=getJVM();
    if(jvmO->hasjvm){
        //jstring jstr;
        char* nativeString;
        if(jvmO->logPage==3)
         nativeString= jvmO->getLogData("Console:vor");
        else if(jvmO->logPage==4)
         nativeString= jvmO->getLogData("Console:ndb");
        else if(jvmO->logPage==2)
         nativeString= jvmO->getLogData("Console:freq");
        else
         nativeString= jvmO->getLogData("Console");
        //const char* nativeString = jvmO->env->GetStringUTFChars(jstr, JNI_FALSE);
    
        //char* astring=(char *)nativeString; 
        char* astring;
        if(jvmO->logPage==0||jvmO->logPage>1)
            astring=(char *)nativeString; 
        else 
           astring =(char *)jvmO->notepad; 
        

            
	    XPLMDrawString(col_white, l + 10, t - 20, astring, &ww, xplmFont_Proportional);
        //jvmO->env->ReleaseStringUTFChars(jstr, nativeString);
    
    }
    else{
        char text[]="Java VM not found, see:\n (X-Plane 11/Resources/plugins/java/defaultjvm.txt)\n For installation instructions \n";
        XPLMDrawString(col_white, l + 10, t - 20, text, &ww, xplmFont_Proportional);
    }
}
int rolls=0;
float SendATCData(float                inElapsedSinceLastCall,    
                                   float                inElapsedTimeSinceLastFlightLoop,    
                                   int                  inCounter,    
                                   void *               inRefcon)
{
   
    JVM* jvmO=getJVM();
    if(!jvmO->hasjvm)
        return -1;
    //if(jvmO->hasjvm)
    //    return -1;

    if(jvmO->hasjvm){
        
            jfloat tplaneData[14]={};

            tplaneData[0]=(jfloat)(XPLMGetDatai(transponder_codeRef)*1.0);
            tplaneData[1]=XPLMGetDataf(latitudeRef);
            tplaneData[2]=XPLMGetDataf(longitudeRef);
            tplaneData[3]=XPLMGetDataf(altitudeRef)*3.28084;
            tplaneData[4]=(jfloat)(XPLMGetDatai(com1_freq_hzRef)*1.0);
            tplaneData[5]=(jfloat)(XPLMGetDatai(com1_stdby_freq_hz)*1.0);
            tplaneData[6]=XPLMGetDataf(iasRef);
            tplaneData[7]=XPLMGetDataf(gyroHeadingRef);
            tplaneData[8]=XPLMGetDataf(altPressureRef);
            tplaneData[9]=XPLMGetDataf(mslPressureRef);
            tplaneData[10]=XPLMGetDataf(fpmRef);
            tplaneData[11]=XPLMGetDataf(pitchRef);
            tplaneData[12]=XPLMGetDataf(rollRef);
            int battery_array[4];
            XPLMGetDatavi(battery_onRef, battery_array, 0, 4);
            float b_on=0.0;
            for(int i=0;i<4;i++)
                if(battery_array[i]>0)
                    b_on=1.0;   
            tplaneData[13]=(jfloat)(XPLMGetDatai(com1_onRef)*b_on);
            jvmO->setData(tplaneData);
            //PlaneData localData=jvmO->getPlaneData(-1,jvmO->env);
            //jvmO->live=localData.live;
            jvmO->getCommandData();
            if(!jvmO->setIcaov){
                jvmO->setICAO();
                jvmO->setIcaov=true;
            }
            float nowT=jvmO->getSysTime();//clock()/(CLOCKS_PER_SEC*1.0f);
            if(jvmO->lastFoundNav>0||nowT>jvmO->lastNavAudio+30){
                //jvmO->getData("doCommand:playMorse:VAL");
                jvmO->getMorse();
            }
            if(!enabledATCPro){
                
                if(jvmO->live){ 
                    jvmO->updateAirframes();
                    initPlanes();
                    enabledATCPro=true;
                }
            }
          
       
    }
    
    aircraftLoop();
    return -1;
}
void JVM::popupNoJVM(){
     XPLMCreateWindow_t params;
	params.structSize = sizeof(params);
	params.visible = 1;
	params.drawWindowFunc = draw_jvm_text;
	// Note on "dummy" handlers:
	// Even if we don't want to handle these events, we have to register a "do-nothing" callback for them
	params.handleMouseClickFunc = dummy_mouse_handler;
	params.handleRightClickFunc = dummy_mouse_handler;
	params.handleMouseWheelFunc = dummy_wheel_handler;
	params.handleKeyFunc = dummy_key_handler;
	params.handleCursorFunc = dummy_cursor_status_handler;
	params.refcon = NULL;
	params.layer = xplm_WindowLayerFloatingWindows;

	params.decorateAsFloatingWindow = xplm_WindowDecorationRoundRectangle;
	
	// Set the window's initial bounds
	// Note that we're not guaranteed that the main monitor's lower left is at (0, 0)...
	// We'll need to query for the global desktop bounds!
	int left, bottom, right, top;
	XPLMGetScreenBoundsGlobal(&left, &top, &right, &bottom);
	params.left = left + 50;
	params.bottom = bottom + 150;
	params.right = params.left + 200;
	params.top = params.bottom + 200;
	
	XPLMWindowID	g_window = XPLMCreateWindowEx(&params);
	
	// Position the window as a "free" floating window, which the user can drag around
	XPLMSetWindowPositioningMode(g_window, xplm_WindowPositionFree, -1);
	// Limit resizing our window: maintain a minimum width/height of 100 boxels and a max width/height of 300 boxels
	XPLMSetWindowResizingLimits(g_window, 200, 200, 300, 300);
	XPLMSetWindowTitle(g_window, "Java Error");
 }
 
 char* JVM::getDevice(){
     return device;
 }
 long JVM::getisSlave(){

     return atol(slave);
 }
void JVM::setDevice( char* newdevice){
    strncpy (device,newdevice, MAXLEN);
 
 }
 void JVM::setisSlave(long newslave){
     sprintf(slave, "%ld", newslave);
 }
  void	draw_jvm_text(XPLMWindowID in_window_id, void * in_refcon)
{
	// Mandatory: We *must* set the OpenGL state before drawing
	// (we can't make any assumptions about it)
	XPLMSetGraphicsState(
						 0 /* no fog */,
						 0 /* 0 texture units */,
						 0 /* no lighting */,
						 0 /* no alpha testing */,
						 1 /* do alpha blend */,
						 1 /* do depth testing */,
						 0 /* no depth writing */
						 );
	
	int l, t, r, b;
	XPLMGetWindowGeometry(in_window_id, &l, &t, &r, &b);
	
	float col_white[] = {1.0, 1.0, 1.0}; // red, green, blue
    int ww=r-l;
        char text[]="Java VM not found, see:\n (X-Plane 11/Resources/plugins/java/defaultjvm.txt)\n For installation instructions \n";
        XPLMDrawString(col_white, l + 10, t - 20, text, &ww, xplmFont_Proportional);
    
}

