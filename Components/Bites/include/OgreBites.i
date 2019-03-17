%module(package="Ogre", directors="1") Bites
%{
/* Includes the header in the wrapper code */
#include "Ogre.h"
#include "OgreBuildSettings.h"
#include "OgreApplicationContextBase.h"
#include "OgreApplicationContext.h"
#include "OgreSGTechniqueResolverListener.h"
#include "OgreCameraMan.h"
#include "OgreTrays.h"
#include "OgreAdvancedRenderControls.h"
#include "OgreUnifiedHighLevelGpuProgram.h"
#include "OgrePredefinedControllers.h"
%}

%include std_string.i
%include exception.i 
%include stdint.i
%import "Ogre.i"

#define _OgreBitesExport

%include "OgreSGTechniqueResolverListener.h"
%feature("director") OgreBites::ApplicationContextBase;
%feature("director") OgreBites::InputListener;
%include "OgreInput.h"

#ifdef __ANDROID__
%{
#include <android/native_window_jni.h>
#include <android/asset_manager_jni.h>

JNIEnv* OgreJNIGetEnv();
%}

%ignore OgreBites::ApplicationContextAndroid::initApp;
%ignore OgreBites::ApplicationContextAndroid::initAppForAndroid(AAssetManager*, ANativeWindow*);
%extend OgreBites::ApplicationContextAndroid {
    void initAppForAndroid(jobject assetManager, jobject surface) {
        OgreAssert(assetManager, "assetManager is NULL");
        OgreAssert(surface, "surface is NULL");

        AAssetManager* assetMgr = AAssetManager_fromJava(OgreJNIGetEnv(), assetManager);
        ANativeWindow* nativeWnd = ANativeWindow_fromSurface(OgreJNIGetEnv(), surface);
        $self->initAppForAndroid(assetMgr, nativeWnd);
    }
}
%feature("director") OgreBites::ApplicationContextAndroid;
%rename(ApplicationContext) ApplicationContextAndroid;
#else
%feature("director") OgreBites::ApplicationContextSDL;
%rename(ApplicationContext) ApplicationContextSDL; // keep the pre 1.12 name
#endif

%include "OgreApplicationContextBase.h"
%include "OgreApplicationContext.h"
%include "OgreCameraMan.h"
// deprecated
%ignore OgreBites::TrayManager::getWidget(TrayLocation, unsigned int);
%ignore OgreBites::TrayManager::getNumWidgets(TrayLocation);
%ignore OgreBites::TrayManager::getWidgetIterator;
%ignore OgreBites::SelectMenu::getItemsCount;
#ifndef SWIGCSHARP
%include "OgreTrays.h"
%include "OgreAdvancedRenderControls.h"
#endif