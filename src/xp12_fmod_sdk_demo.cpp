/*
Author: William Good
Version 1.0
Version date 12/08/2022
Lience MIT
*/

#include <cstring>
#include <string>
#include <stdexcept>

#include "fmod.h"
#include "fmod_studio.h"
#include "fmod_errors.h"

#include "XPLMPlugin.h"
#include "XPLMProcessing.h"
#include "XPLMUtilities.h"
#include <stdint.h>
#include "XPLMSound.h"

using namespace std;


// Flight loops ----------------------------------------------------------------
static XPLMFlightLoopID flightLoopID = nullptr;
float flightLoop(float inElapsedSinceLastCall,
                 float inElapsedTimeSinceLastFlightLoop,
                 int inCounter,
                 void *inRefcon);

// FMOD Studio ------------------------------------------------------------------
FMOD_STUDIO_SYSTEM *fmodStudioPointer_sdk = nullptr;

// FMOD Core --------------------------------------------------------------------
FMOD_SYSTEM* fmod_system_sdk = nullptr;

// Custom FMOD elements ---------------------------------------------------------
FMOD_SOUND *soundFMod = nullptr;
FMOD_SOUND *soundFMod2 = nullptr;
FMOD_SOUND *soundFMod3 = nullptr;
FMOD_SOUND *soundFMod4 = nullptr;

FMOD_CHANNELGROUP *customChannelGroupFMod = nullptr;


FMOD_CHANNELGROUP *fmod_demo_exterior_aircraft_channel_group = nullptr;
FMOD_CHANNELGROUP *fmod_demo_exterior_enviroment_channel_group = nullptr;
FMOD_CHANNELGROUP *fmod_demo_interior_channel_group = nullptr;
FMOD_CHANNELGROUP *fmod_demo_ui_channel_group = nullptr;

// To be removed before publishing
FMOD_CHANNELGROUP *ChannelGroupFMod_sdk_audio_radio_com1 = nullptr;
FMOD_CHANNELGROUP *ChannelGroupFMod_sdk_audio_radio_com2 = nullptr;
FMOD_CHANNELGROUP *ChannelGroupFMod_sdk_audio_radio_pilot = nullptr;
FMOD_CHANNELGROUP *ChannelGroupFMod_sdk_audio_radio_copilot = nullptr;
FMOD_CHANNELGROUP *ChannelGroupFMod_sdk_audio_exterior_aircraft = nullptr;
FMOD_CHANNELGROUP *ChannelGroupFMod_sdk_audio_exterior_enviroment = nullptr;
FMOD_CHANNELGROUP *ChannelGroupFMod_sdk_audio_exterior_unprocessed = nullptr;
FMOD_CHANNELGROUP *ChannelGroupFMod_sdk_audio_interior = nullptr;
FMOD_CHANNELGROUP *ChannelGroupFMod_sdk_audio_ui = nullptr;
FMOD_CHANNELGROUP *ChannelGroupFMod_sdk_audio_ground = nullptr;
FMOD_CHANNELGROUP *ChannelGroupFMod_sdk_audio_master = nullptr;
bool init = true;


void FMODErrorHandler(const string &file, int line, FMOD_RESULT result)
{
    if(result != FMOD_OK)
    {
        string errorMsg = "Error in " + file + ", line " + to_string(line) + ": " + FMOD_ErrorString(result);
        throw runtime_error(errorMsg.c_str());
    }
}

unsigned long long getDelayToCurrentSoundEnd(int outputRate, FMOD_CHANNEL *playingChannel)
{
    // From the example granular_synth.cpp
    if(playingChannel != nullptr)
    {
        unsigned long long startdelay = 0;
        unsigned int soundlength = 0;
        float soundfrequency = 0.f;
        FMOD_SOUND *playingsound = nullptr;

        FMOD_RESULT result;

        // Get the channel DSP clock, which serves as a reference
        result = FMOD_Channel_GetDSPClock(playingChannel, 0, &startdelay);
        FMODErrorHandler(__FILE__, __LINE__-1, result);

        // Grab the length of the playing sound, and its frequency, so we can caluate where to place the new sound on the time line.
        result = FMOD_Channel_GetCurrentSound(playingChannel, &playingsound);
        FMODErrorHandler(__FILE__, __LINE__-1, result);
        result = FMOD_Sound_GetLength(playingsound, &soundlength, FMOD_TIMEUNIT_PCM);
        FMODErrorHandler(__FILE__, __LINE__-1, result);
        result = FMOD_Channel_GetFrequency(playingChannel, &soundfrequency);
        FMODErrorHandler(__FILE__, __LINE__-1, result);

        /*
            Now calculate the length of the sound in 'output samples'.
            Ie if a 44khz sound is 22050 samples long, and the output rate is 48khz, then we want to delay by 24000 output samples.
        */
        soundlength *= outputRate;
        soundlength /= soundfrequency;

        startdelay += soundlength; // Add output rate adjusted sound length, to the clock value of the sound that is currently playing

        return startdelay;
    }
    return 0;
}


PLUGIN_API int XPluginStart(char * outName, char * outSig, char * outDesc)
{
    const string pluginName = "Fmod_SDK_Demo";
    const string pluginSignature = "SparkerInVR.Fmod_SDK_Demo";
    const string pluginDescription = "Plugin to test FMod SDK";
#if __STDC_LIB_EXT1__
    strcpy_s(outName, 255, pluginName.c_str());
    strcpy_s(outSig, 255, pluginSignature.c_str());
    strcpy_s(outDesc, 255, pluginDescription.c_str());
#else
    strncpy(outName, pluginName.c_str(), 255);
    strncpy(outSig, pluginSignature.c_str(), 255);
    strncpy(outDesc, pluginDescription.c_str(), 255);
#endif

    XPLMEnableFeature("XPLM_USE_NATIVE_PATHS", 1);

    // Declare the flight loop which will play the sounds
    XPLMCreateFlightLoop_t flightLoopData
    {
        sizeof (XPLMCreateFlightLoop_t),
        xplm_FlightLoop_Phase_AfterFlightModel,
        flightLoop,
        nullptr
    };
    flightLoopID = XPLMCreateFlightLoop(&flightLoopData);

    return 1;
}


PLUGIN_API void XPluginStop(void)
{
    XPLMDestroyFlightLoop(flightLoopID);
}


PLUGIN_API void XPluginDisable(void)
{
}

PLUGIN_API int XPluginEnable(void)
{
    return 1;
}


PLUGIN_API void XPluginReceiveMessage(XPLMPluginID inFrom, int inMessage, void *inParam)
{
    if(XPLM_PLUGIN_XPLANE == inFrom)
    {
        if(XPLM_MSG_FMOD_BANK_LOADED == inMessage && xplm_RadioBank == reinterpret_cast<uintptr_t>(inParam))
        {
            // Use the X-Plane 12 SDK sound API to get a pointer to the FMOD studio system
            fmodStudioPointer_sdk = XPLMGetFMODStudio();
            if(nullptr == fmodStudioPointer_sdk)
            {
                XPLMDebugString("Fmod_SDK_Demo: No pointer from SDK for Studio system\n");
                return;
            }

            try {
                FMOD_RESULT result;

                // Get a pointe to the FMOD core system
                result = FMOD_Studio_System_GetCoreSystem(XPLMGetFMODStudio(), &fmod_system_sdk);
                FMODErrorHandler(__FILE__, __LINE__-1, result);

                // Create a custom channel
                ChannelGroupFMod_sdk_audio_exterior_aircraft = XPLMGetFMODChannelGroup(xplm_AudioExteriorAircraft);
                result = FMOD_System_CreateChannelGroup(fmod_system_sdk, "Fmod_Demo_Exterior_Aircraft_Channel", &fmod_demo_exterior_aircraft_channel_group);
                FMODErrorHandler(__FILE__, __LINE__-1, result);
                result = FMOD_ChannelGroup_AddGroup(ChannelGroupFMod_sdk_audio_exterior_aircraft, fmod_demo_exterior_aircraft_channel_group, true, nullptr);
                FMODErrorHandler(__FILE__, __LINE__-1, result);


                // Create a custom channel
                ChannelGroupFMod_sdk_audio_exterior_enviroment = XPLMGetFMODChannelGroup(xplm_AudioExteriorEnvironment);
                result = FMOD_System_CreateChannelGroup(fmod_system_sdk, "Fmod_Demo_Exterior_Enviroment_Channel", &fmod_demo_exterior_enviroment_channel_group);
                FMODErrorHandler(__FILE__, __LINE__-1, result);
                result = FMOD_ChannelGroup_AddGroup(ChannelGroupFMod_sdk_audio_exterior_enviroment, fmod_demo_exterior_enviroment_channel_group, true, nullptr);
                FMODErrorHandler(__FILE__, __LINE__-1, result);


                // Create a custom channel
                ChannelGroupFMod_sdk_audio_interior = XPLMGetFMODChannelGroup(xplm_AudioInterior);
                result = FMOD_System_CreateChannelGroup(fmod_system_sdk, "Fmod_Demo_Interior_Channel", &fmod_demo_interior_channel_group);
                FMODErrorHandler(__FILE__, __LINE__-1, result);
                result = FMOD_ChannelGroup_AddGroup(ChannelGroupFMod_sdk_audio_interior, fmod_demo_interior_channel_group, true, nullptr);
                FMODErrorHandler(__FILE__, __LINE__-1, result);


                // Create a custom channel
                ChannelGroupFMod_sdk_audio_ui = XPLMGetFMODChannelGroup(xplm_AudioInterior);
                result = FMOD_System_CreateChannelGroup(fmod_system_sdk, "Fmod_Demo_UI_Channel", &fmod_demo_ui_channel_group);
                FMODErrorHandler(__FILE__, __LINE__-1, result);
                result = FMOD_ChannelGroup_AddGroup(ChannelGroupFMod_sdk_audio_ui, fmod_demo_ui_channel_group, true, nullptr);
                FMODErrorHandler(__FILE__, __LINE__-1, result);


                // Get the X-Plane root path
                string xPlaneDirectory;
                char cPath[1024];
                XPLMGetSystemPath(cPath);
                xPlaneDirectory.assign(cPath);

                // Load a first sound
                string soundPath = xPlaneDirectory + "/Resources/sounds/alert/10ft.wav";
                result = FMOD_System_CreateSound(fmod_system_sdk, soundPath.c_str(), FMOD_DEFAULT, nullptr, &soundFMod);
                FMODErrorHandler(__FILE__, __LINE__-1, result);

                // Load a second sound
                string sound2Path = xPlaneDirectory + "/Resources/sounds/alert/20ft.wav";
                result = FMOD_System_CreateSound(fmod_system_sdk, sound2Path.c_str(), FMOD_DEFAULT, nullptr, &soundFMod2);
                FMODErrorHandler(__FILE__, __LINE__-1, result);

                // Load a third sound
                string sound3Path = xPlaneDirectory + "/Resources/sounds/alert/30ft.wav";
                result = FMOD_System_CreateSound(fmod_system_sdk, sound3Path.c_str(), FMOD_DEFAULT, nullptr, &soundFMod3);
                FMODErrorHandler(__FILE__, __LINE__-1, result);

                // Load a forth sound
                string sound4Path = xPlaneDirectory + "/Resources/sounds/alert/40ft.wav";
                result = FMOD_System_CreateSound(fmod_system_sdk, sound4Path.c_str(), FMOD_DEFAULT, nullptr, &soundFMod4);
                FMODErrorHandler(__FILE__, __LINE__-1, result);


            }
            catch (exception const& e)
            {
                string errorMsg = "Fmod_SDK_Demo:: " + string(e.what()) + "\n";
                XPLMDebugString(errorMsg.c_str());
                return; // The plugin is failing, so do not attempt to play sounds
            }

            XPLMScheduleFlightLoop(flightLoopID, 1.f, 1); // The FMOD bank is loaded, the system is ready to play some sound
        }
        if(XPLM_MSG_FMOD_BANK_UNLOADING == inMessage && xplm_RadioBank == reinterpret_cast<uintptr_t>(inParam))
        {
            fmodStudioPointer_sdk = nullptr;
            fmod_system_sdk = nullptr;
            ChannelGroupFMod_sdk_audio_exterior_aircraft = nullptr;
            ChannelGroupFMod_sdk_audio_exterior_enviroment = nullptr;
            ChannelGroupFMod_sdk_audio_interior = nullptr;
            ChannelGroupFMod_sdk_audio_ui = nullptr;
            customChannelGroupFMod = nullptr;

            // Unload the sounds if they were loaded
            if(nullptr != soundFMod)
            {
                FMOD_Sound_Release(soundFMod);
                soundFMod = nullptr;
            }
            if(nullptr != soundFMod2)
            {
                FMOD_Sound_Release(soundFMod2);
                soundFMod2 = nullptr;
            }

            XPLMScheduleFlightLoop(flightLoopID, 0.f, 1); // Stop the flight loop until the FMOD bank is loaded again
        }
    }
}

float flightLoop(float, float, int, void *)
{
    /* This is purely an experiment!
     * Don't copy directly this code, since it is not safe: most pointers are not checked and error codes are ignored!
     */

    try
    {
        // To be removed before publishing
        if(init)
        {
            init = false;

            // Using the X-Plane Fmod SDK to find and test if valid
            // Currently Com1, Com2, Pilot and Ground are not being found

            ChannelGroupFMod_sdk_audio_radio_com1 = XPLMGetFMODChannelGroup(xplm_AudioRadioCom1);
            if(nullptr == ChannelGroupFMod_sdk_audio_radio_com1)
            {
                XPLMDebugString("Fmod_SDK_Demo: No xplm_AudioRadioCom1 bus found\n");
            }

            ChannelGroupFMod_sdk_audio_radio_com2 = XPLMGetFMODChannelGroup(xplm_AudioRadioCom2);
            if(nullptr == ChannelGroupFMod_sdk_audio_radio_com2)
            {
                XPLMDebugString("Fmod_SDK_Demo: No xplm_AudioRadioCom2 bus found\n");
            }

            ChannelGroupFMod_sdk_audio_radio_pilot = XPLMGetFMODChannelGroup(xplm_AudioRadioPilot);
            if(nullptr == ChannelGroupFMod_sdk_audio_radio_pilot)
            {
                XPLMDebugString("Fmod_SDK_Demo: No xplm_AudioRadioPilot bus found\n");
            }

            ChannelGroupFMod_sdk_audio_radio_copilot = XPLMGetFMODChannelGroup(xplm_AudioRadioCopilot);
            if(nullptr == ChannelGroupFMod_sdk_audio_radio_copilot)
            {
                XPLMDebugString("Fmod_SDK_Demo: No xplm_AudioRadioCopilot bus found\n");
            }

            ChannelGroupFMod_sdk_audio_exterior_aircraft = XPLMGetFMODChannelGroup(xplm_AudioExteriorAircraft);
            if(nullptr == ChannelGroupFMod_sdk_audio_exterior_aircraft)
            {
                XPLMDebugString("Fmod_SDK_Demo: No xplm_AudioExteriorAircraft bus found\n");
            }

            ChannelGroupFMod_sdk_audio_exterior_enviroment = XPLMGetFMODChannelGroup(xplm_AudioExteriorEnvironment);
            if(nullptr == ChannelGroupFMod_sdk_audio_exterior_enviroment)
            {
                XPLMDebugString("Fmod_SDK_Demo: No xplm_AudioExteriorEnvironment bus found\n");
            }

            ChannelGroupFMod_sdk_audio_exterior_unprocessed = XPLMGetFMODChannelGroup(xplm_AudioExteriorUnprocessed);
            if(nullptr == ChannelGroupFMod_sdk_audio_exterior_unprocessed)
            {
                XPLMDebugString("Fmod_SDK_Demo: No xplm_AudioExteriorUnprocessed bus found\n");
            }

            ChannelGroupFMod_sdk_audio_interior = XPLMGetFMODChannelGroup(xplm_AudioInterior);
            if(nullptr == ChannelGroupFMod_sdk_audio_interior)
            {
                XPLMDebugString("Fmod_SDK_Demo: No xplm_AudioInterior bus ound\n");
            }

            ChannelGroupFMod_sdk_audio_ui = XPLMGetFMODChannelGroup(xplm_AudioUI);
            if(nullptr == ChannelGroupFMod_sdk_audio_ui)
            {
                XPLMDebugString("Fmod_SDK_Demo: No xplm_AudioUI bus found\n");
            }

            ChannelGroupFMod_sdk_audio_ground = XPLMGetFMODChannelGroup(xplm_AudioGround);
            if(nullptr == ChannelGroupFMod_sdk_audio_ground)
            {
                XPLMDebugString("Fmod_SDK_Demo: No xplm_AudioGround bus found\n");
            }

            ChannelGroupFMod_sdk_audio_master = XPLMGetFMODChannelGroup(xplm_Master);
            if(nullptr == ChannelGroupFMod_sdk_audio_master)
            {
                XPLMDebugString("Fmod_SDK_Demo: No xplm_Master bus found\n");
            }
        }

        FMOD_RESULT result;

        // The channel on which the sound will be played
        FMOD_CHANNEL* channel = nullptr;
        // Actually play the sound
        result = FMOD_System_PlaySound(fmod_system_sdk, soundFMod, fmod_demo_exterior_aircraft_channel_group, false, &channel);
        FMODErrorHandler(__FILE__, __LINE__-1, result);

        FMOD_CHANNEL* channel2 = nullptr;
        // Actually play the sound
        result = FMOD_System_PlaySound(fmod_system_sdk, soundFMod2, fmod_demo_exterior_enviroment_channel_group, false, &channel2);
        FMODErrorHandler(__FILE__, __LINE__-1, result);

        FMOD_CHANNEL* channel3 = nullptr;
        result = FMOD_System_PlaySound(fmod_system_sdk, soundFMod3, fmod_demo_interior_channel_group, true, &channel3);
        FMODErrorHandler(__FILE__, __LINE__-1, result);

        FMOD_CHANNEL* channel4 = nullptr;
        result = FMOD_System_PlaySound(fmod_system_sdk, soundFMod4, fmod_demo_ui_channel_group, true, &channel4);
        FMODErrorHandler(__FILE__, __LINE__-1, result);

        int outputRate = 0;
        result = FMOD_System_GetSoftwareFormat(fmod_system_sdk, &outputRate, 0, 0);
        FMODErrorHandler(__FILE__, __LINE__-1, result);

        unsigned long long delay = getDelayToCurrentSoundEnd(outputRate, channel);
        result = FMOD_Channel_SetDelay(channel2, delay, 0, true);
        FMODErrorHandler(__FILE__, __LINE__-1, result);
        result = FMOD_Channel_SetPaused(channel2, false);
        FMODErrorHandler(__FILE__, __LINE__-1, result);

        unsigned long long delay2 = getDelayToCurrentSoundEnd(outputRate, channel2);
        unsigned long long delay_sum;
        // delay_sum = delay + delay2;
        delay_sum = delay2 + 20000;
        result = FMOD_Channel_SetDelay(channel3, delay_sum, 0, true);
        FMODErrorHandler(__FILE__, __LINE__-1, result);
        result = FMOD_Channel_SetPaused(channel3, false);
        FMODErrorHandler(__FILE__, __LINE__-1, result);

        unsigned long long delay3 = getDelayToCurrentSoundEnd(outputRate, channel3);
        unsigned long long delay_sum2;
        // delay_sum2 = delay + delay2 + delay3;
        delay_sum2 = delay3 + 40000;
        result = FMOD_Channel_SetDelay(channel4, delay_sum2, 0, true);
        FMODErrorHandler(__FILE__, __LINE__-1, result);
        result = FMOD_Channel_SetPaused(channel4, false);
        FMODErrorHandler(__FILE__, __LINE__-1, result);

    }
    catch (exception const& e)
    {
        string errorMsg = "Fmod_SDK_Demo: " + string(e.what()) + "\n";
        XPLMDebugString(errorMsg.c_str());
        return 0.f; // The plugin is failing, so do not execute any further loop
    }

    // Play again every 5 sec
    return 5.f;
}
