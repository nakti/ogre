// This file is part of the OGRE project.
// It is subject to the license terms in the LICENSE file found in the top-level directory
// of this distribution and at https://www.ogre3d.org/licensing.

#include "OgreApplicationContext.h"

#include "OgreRoot.h"
#include "OgreRenderWindow.h"

#include <SDL.h>
#include <SDL_video.h>
#include <SDL_syswm.h>

#include "SDLInputMapping.h"

namespace OgreBites {

ApplicationContextSDL::ApplicationContextSDL(const Ogre::String& appName) : ApplicationContextBase(appName)
{
}

void ApplicationContextSDL::addInputListener(NativeWindowType* win, InputListener* lis)
{
    mInputListeners.insert(std::make_pair(SDL_GetWindowID(win), lis));
}


void ApplicationContextSDL::removeInputListener(NativeWindowType* win, InputListener* lis)
{
    mInputListeners.erase(std::make_pair(SDL_GetWindowID(win), lis));
}

NativeWindowPair ApplicationContextSDL::createWindow(const Ogre::String& name, Ogre::uint32 w, Ogre::uint32 h, Ogre::NameValuePairList miscParams)
{
    NativeWindowPair ret = {NULL, NULL};

    if(!SDL_WasInit(SDL_INIT_VIDEO)) {
        SDL_InitSubSystem(SDL_INIT_VIDEO);
    }

    auto p = mRoot->getRenderSystem()->getRenderWindowDescription();
    miscParams.insert(p.miscParams.begin(), p.miscParams.end());
    p.miscParams = miscParams;
    p.name = name;
	    
    if(w > 0 && h > 0)
    {
        p.width = w;
        p.height= h;
    }

	if (!p.useFullScreen) {
		ret.native = SDL_CreateWindow(name.c_str(),
			SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1, 1, 0);
		int tTop, tBot, tLef, tRig, tW, tH;
		SDL_Rect tScreen;
		if (SDL_GetDisplayUsableBounds(SDL_GetWindowDisplayIndex(ret.native), &tScreen) >= 0)
		{
			SDL_GetWindowBordersSize(ret.native, &tTop, &tLef, &tBot, &tRig);
			unsigned int tMaxW = tScreen.w - tLef - tRig;
			unsigned int tMaxH = tScreen.h - tTop - tBot;
			tW = p.width > tMaxW ? tMaxW : p.width;
			tH = p.height > tMaxH ? tMaxH : p.height;
			SDL_SetWindowSize(ret.native, tW, tH);
			auto tX = (tMaxW - tW) / 2 + tScreen.x + tLef;
			auto tY = (tMaxH - tH) / 2 + tScreen.y + tTop;
			SDL_SetWindowPosition(ret.native, tX, tY);

			SDL_SetWindowMinimumSize(ret.native, 400, 400);
			SDL_SetWindowMaximumSize(ret.native, tMaxW, tMaxH);
		}
	}
	else
		ret.native = SDL_CreateWindow(name.c_str(),
			SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, p.width, p.height, SDL_WINDOW_FULLSCREEN);

#if OGRE_PLATFORM == OGRE_PLATFORM_EMSCRIPTEN
    SDL_GL_CreateContext(ret.native);
#else
    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    SDL_GetWindowWMInfo(ret.native, &wmInfo);
#endif

#if OGRE_PLATFORM == OGRE_PLATFORM_LINUX
    p.miscParams["parentWindowHandle"] = Ogre::StringConverter::toString(size_t(wmInfo.info.x11.window));
#elif OGRE_PLATFORM == OGRE_PLATFORM_WIN32
    p.miscParams["externalWindowHandle"] = Ogre::StringConverter::toString(size_t(wmInfo.info.win.window));
#elif OGRE_PLATFORM == OGRE_PLATFORM_APPLE
    assert(wmInfo.subsystem == SDL_SYSWM_COCOA);
    p.miscParams["externalWindowHandle"] = Ogre::StringConverter::toString(size_t(wmInfo.info.cocoa.window));
#endif

    if(!mWindows.empty() || OGRE_PLATFORM == OGRE_PLATFORM_EMSCRIPTEN)
    {
        // additional windows should reuse the context (also the first on emscripten)
        p.miscParams["currentGLContext"] = "true";
    }

    ret.render = mRoot->createRenderWindow(p);
    mWindows.push_back(ret);
    return ret;
}

void ApplicationContextSDL::setWindowGrab(NativeWindowType* win, bool _grab)
{
    SDL_bool grab = SDL_bool(_grab);

    SDL_SetWindowGrab(win, grab);
    SDL_SetRelativeMouseMode(grab);
}

void ApplicationContextSDL::shutdown()
{
    ApplicationContextBase::shutdown();

    for(WindowList::iterator it = mWindows.begin(); it != mWindows.end(); ++it)
    {
        if(it->native)
            SDL_DestroyWindow(it->native);
    }
    if(!mWindows.empty()) {
        SDL_QuitSubSystem(SDL_INIT_VIDEO);
    }

    mWindows.clear();
}

void ApplicationContextSDL::pollEvents()
{
    if(mWindows.empty())
    {
        // SDL events not initialized
        return;
    }

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
        case SDL_QUIT:
            mRoot->queueEndRendering();
            break;
        case SDL_WINDOWEVENT:
            if(event.window.event != SDL_WINDOWEVENT_RESIZED)
                continue;

            for(WindowList::iterator it = mWindows.begin(); it != mWindows.end(); ++it)
            {
                if(event.window.windowID != SDL_GetWindowID(it->native))
                    continue;

                Ogre::RenderWindow* win = it->render;
                win->windowMovedOrResized();
                windowResized(win);
            }
            break;
        default:
            _fireInputEvent(convert(event), event.window.windowID);
            break;
        }
    }

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
    // hacky workaround for black window on OSX
    for(const auto& win : mWindows)
    {
        SDL_SetWindowSize(win.native, win.render->getWidth(), win.render->getHeight());
        win.render->windowMovedOrResized();
    }
#endif
}

}
