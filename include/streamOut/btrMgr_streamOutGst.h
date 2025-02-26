/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2016 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/
/**
 * @file btrMgr_streamOutGst.h
 *
 * @description This file defines bluetooth manager's data streaming interfaces using GStreamer to external BT devices
 *
 */

/**
 * @addtogroup  Stream_Out
 * @{
*
 */

#ifndef __BTR_MGR_STREAMOUT_GST_H__
#define __BTR_MGR_STREAMOUT_GST_H__

#ifdef UNIT_TEST
#include <glib.h>
#endif

typedef void* tBTRMgrSoGstHdl;

#define BTRMGR_AUDIO_SFMT_SIGNED_8BIT       "S8"
#define BTRMGR_AUDIO_SFMT_SIGNED_LE_16BIT   "S16LE"
#define BTRMGR_AUDIO_SFMT_SIGNED_LE_24BIT   "S24LE"
#define BTRMGR_AUDIO_SFMT_SIGNED_LE_32BIT   "S32LE"
// Add additional sampling formats as supported by Gst SO layer

#define BTRMGR_AUDIO_CHANNELMODE_MONO       "mono"
#define BTRMGR_AUDIO_CHANNELMODE_DUAL       "dual"
#define BTRMGR_AUDIO_CHANNELMODE_STEREO     "stereo"
#define BTRMGR_AUDIO_CHANNELMODE_JSTEREO    "joint"
// Add additional chennel modes as supported by Gst SO layer

typedef enum _eBTRMgrSOGstRet {
   eBTRMgrSOGstFailure,
   eBTRMgrSOGstFailInArg,
   eBTRMgrSOGstSuccess
} eBTRMgrSOGstRet;

typedef enum _eBTRMgrSOGstStatus {
    eBTRMgrSOGstStInitialized,
    eBTRMgrSOGstStDeInitialized,
    eBTRMgrSOGstStPaused,
    eBTRMgrSOGstStPlaying,
    eBTRMgrSOGstStUnderflow,
    eBTRMgrSOGstStOverflow,
    eBTRMgrSOGstStCompleted,
    eBTRMgrSOGstStStopped,
    eBTRMgrSOGstStWarning,
    eBTRMgrSOGstStError,
    eBTRMgrSOGstStUnknown
} eBTRMgrSOGstStatus;


/* Fptr Callbacks types */
typedef eBTRMgrSOGstRet (*fPtr_BTRMgr_SO_GstStatusCb) (eBTRMgrSOGstStatus aeBtrMgrSoGstStatus, void *apvUserData);


/* Interfaces */

/**
 * @brief This API initializes the streaming interface.
 *
 * Uses gstreamer element "appsrc" for initialization.
 *
 * @param[in]  phBTRMgrSoGstHdl         Handle to the stream out interface.
 * @param[in]  afpcBSoGstStatus         Stream Out callback function.
 * @param[in]  apvUserData              Data for the callback function.
 *
 * @return Returns the status of the operation.
 * @retval eBTRMgrSOGstSuccess  on success, appropriate  error code otherwise.
 */
eBTRMgrSOGstRet BTRMgr_SO_GstInit (tBTRMgrSoGstHdl* phBTRMgrSoGstHdl, fPtr_BTRMgr_SO_GstStatusCb afpcBSoGstStatus, void* apvUserData);

/**
 * @brief This API performs the cleanup operations.
 *
 * Cancels the threads that are running within and frees all associated memory.
 *
 * @param[in]  hBTRMgrSoGstHdl             Handle to the stream out interface.
 *
 * @return Returns the status of the operation.
 * @retval eBTRMgrSOGstSuccess  on success, appropriate  error code otherwise.
 */
eBTRMgrSOGstRet BTRMgr_SO_GstDeInit (tBTRMgrSoGstHdl hBTRMgrSoGstHdl);

/**
 * @brief This API starts the playback and listens to the events associated with it.
 *
 * @param[in]  hBTRMgrSoGstHdl            Handle to the stream out interface.
 * @param[in]  apcInFmt                   Supported formats.
 * @param[in]  ai32InRate                 Input rate.
 * @param[in]  ai32InChannels             Input channels.
 * @param[in]  ai32OutRate                Output rate.
 * @param[in]  ai32OutChannels            Channels supported.
 * @param[in]  apcOutChannelMode          Channel modes.
 * @param[in]  aui8SbcAllocMethod         Allocation methods.
 * @param[in]  aui8SbcSubbands            Sub bands.
 * @param[in]  aui8SbcBlockLength         Blocks.
 * @param[in]  aui8SbcMinBitpool          Min bit pool.
 * @param[in]  aui8SbcMaxBitpool          Max bit pool.
 * @param[in]  aui16SbcFrameLength        Frame length.
 * @param[in]  ai32BTDevFd                Input file descriptor.
 * @param[in]  ai32BTDevMTU               Block size to  read.
 *
 * @return Returns the status of the operation.
 * @retval eBTRMgrSOGstSuccess  on success, appropriate  error code otherwise.
 */
eBTRMgrSOGstRet BTRMgr_SO_GstStart (tBTRMgrSoGstHdl hBTRMgrSoGstHdl, 
                                    int ai32InBufMaxSize,
                                    const char* apcInFmt,
                                    int ai32InRate,
                                    int ai32InChannels,
                                    int ai32OutRate,
                                    int ai32OutChannels,
                                    const char* apcOutChannelMode,
                                    unsigned char aui8SbcAllocMethod,
                                    unsigned char aui8SbcSubbands,
                                    unsigned char aui8SbcBlockLength,
                                    unsigned char aui8SbcMinBitpool,
                                    unsigned char aui8SbcMaxBitpool,
                                    unsigned short aui16SbcFrameLength,
                                    int ai32BTDevFd,
                                    int ai32BTDevMTU,
                                    unsigned int    ai32BTDevDelay);

/**
 * @brief This API stops the current playback and sets the state as NULL.
 *
 * @param[in]  hBTRMgrSoGstHdl           Handle to the stream out interface.
 *
 * @return Returns the status of the operation.
 * @retval eBTRMgrSOGstSuccess  on success, appropriate  error code otherwise.
 */
eBTRMgrSOGstRet BTRMgr_SO_GstStop (tBTRMgrSoGstHdl hBTRMgrSoGstHdl);

/**
 * @brief This API pauses the current playback and listens to the events.
 *
 * Checks for the current state if it is playing sets the state to pause.
 *
 * @param[in]  hBTRMgrSoGstHdl           Handle to the stream out interface.
 *
 * @return Returns the status of the operation.
 * @retval eBTRMgrSOGstSuccess  on success, appropriate  error code otherwise.
 */
eBTRMgrSOGstRet BTRMgr_SO_GstPause (tBTRMgrSoGstHdl hBTRMgrSoGstHdl);

/**
 * @brief This API resumes the current operation and listens to the events.
 *
 * Checks for the current state if it is paused sets the state to playing.
 *
 * @param[in]  hBTRMgrSoGstHdl             Handle to the stream out interface.
 *
 * @return Returns the status of the operation.
 * @retval eBTRMgrSOGstSuccess  on success, appropriate  error code otherwise.
 */
eBTRMgrSOGstRet BTRMgr_SO_GstResume (tBTRMgrSoGstHdl hBTRMgrSoGstHdl);

/**
 * @brief This API Sets whether the input of current Buffers is Paused.
 *
 * Sets the current state, if it is in paused state, with respect to buffer input
 *
 * @param[in]  hBTRMgrSoGstHdl      Handle to the stream in interface.
 * @param[in]  ui8InputPaused       Input Buffers Paused.
 *
 * @return Returns the status of the operation.
 * @retval eBTRMgrSOGstSuccess  on success, appropriate  error code otherwise.
 */
eBTRMgrSOGstRet BTRMgr_SO_GstSetInputPaused (tBTRMgrSoGstHdl hBTRMgrSoGstHdl, unsigned char ui8InputPaused);

/**
 * @brief This API Sets the volume of current operation and listens to the events.
 *
 * Checks for the current state, if it is in paused state, playing state is set.
 *
 * @param[in]  hBTRMgrSoGstHdl      Handle to the stream in interface.
 * @param[in]  ui8Volume            Input Volume.
 *
 * @return Returns the status of the operation.
 * @retval eBTRMgrSOGstSuccess  on success, appropriate  error code otherwise.
 */
eBTRMgrSOGstRet BTRMgr_SO_GstSetVolume (tBTRMgrSoGstHdl hBTRMgrSoGstHdl, unsigned char ui8Volume);
/**
 * @brief This API Gets the volume of current operation and listens to the events.
 *
 * @param[in]  hBTRMgrSoGstHdl      Handle to the stream in interface.
 * @param[in]  ui8Volume            Output Volume.
 *
 * @return Returns the status of the operation.
 * @retval eBTRMgrSOGstSuccess  on success, appropriate  error code otherwise.
 */
eBTRMgrSOGstRet BTRMgr_SO_GstGetVolume (tBTRMgrSoGstHdl hBTRMgrSoGstHdl, unsigned char * ui8Volume);
/**
 * @brief This API Gets the delay of current stream and the number of ms in the delay queue.
 *
 * @param[in]  hBTRMgrSoGstHdl      Handle to the stream in interface.
 * @param[in]  pui16Delay            Output delay.
 * @param[in]  pui16Delay            data in ms in the gstreamer delay queue.
 *
 * @return Returns the status of the operation.
 * @retval eBTRMgrSOGstSuccess  on success, appropriate  error code otherwise.
 */
eBTRMgrSOGstRet BTMgr_SO_GstGetDelay (tBTRMgrSoGstHdl hBTRMgrSoGstHdl, unsigned int * pui16Delay, unsigned int * pui16MsInBuffer);
/**
 * @brief This API Sets the delay of current stream events.
 *
 * Checks for the current state, if it is in paused state, playing state is set.
 *
 * @param[in]  hBTRMgrSoGstHdl      Handle to the stream in interface.
 * @param[in]  delay_comp_ms            Input delay.
 *
 * @return Returns the status of the operation.
 * @retval eBTRMgrSOGstSuccess  on success, appropriate  error code otherwise.
 */
eBTRMgrSOGstRet BTMgr_SO_GstSetDelay (tBTRMgrSoGstHdl hBTRMgrSoGstHdl, unsigned int delay_comp_ms);
/**
 * @brief This API converts the delay as given by a device to the delay needed in the gstreamer pipeline
 *
 *
 * @param[in]  btDeviceDelay      Delay as given by the device in 1/10ms.
 * @param[in]  delayRet           Delay to be set on gstreamer queue element.
 *
 * @return Returns the status of the operation.
 * @retval eBTRMgrSOGstSuccess  on success, appropriate  error code otherwise.
 */
eBTRMgrSOGstRet BTRMgr_SO_GstCalculateDelayNeeded (tBTRMgrSoGstHdl hBTRMgrSoGstHdl, unsigned int   btDeviceDelay, unsigned int   *delayRet);

/**
 * @brief This API Sets the Mute of current operation and listens to the events.
 *
 * @param[in]  hBTRMgrSoGstHdl      Handle to the stream in interface.
 * @param[in]  mute                 Input mute.
 *
 * @return Returns the status of the operation.
 * @retval eBTRMgrSOGstSuccess  on success, appropriate  error code otherwise.
 */
eBTRMgrSOGstRet BTRMgr_SO_GstSetMute (tBTRMgrSoGstHdl hBTRMgrSoGstHdl, gboolean mute);
/**
 * @brief This API Gets the Mute of current operation and listens to the events.
 *
 * @param[in]  hBTRMgrSoGstHdl      Handle to the stream in interface.
 * @param[in]  mute                 Output mute.
 *
 * @return Returns the status of the operation.
 * @retval eBTRMgrSOGstSuccess  on success, appropriate  error code otherwise.
 */
eBTRMgrSOGstRet BTRMgr_SO_GstGetMute (tBTRMgrSoGstHdl hBTRMgrSoGstHdl, gboolean *mute);

/**
 * @brief This API pushes the buffer to the queue.
 *
 * @param[in]  hBTRMgrSoGstHdl         Handle to the stream out interface.
 * @param[in]  pcInBuf                 The buffer to be added to the queue.
 * @param[in]  aiInBufSize             Buffer size.
 *
 * @return Returns the status of the operation.
 * @retval eBTRMgrSOGstSuccess  on success, appropriate  error code otherwise.
 */
eBTRMgrSOGstRet BTRMgr_SO_GstSendBuffer (tBTRMgrSoGstHdl hBTRMgrSoGstHdl, char* pcInBuf, int aiInBufSize);

/**
 * @brief This API is used to push EOS(End of Stream) to the queue.
 *
 * @param[in]  hBTRMgrSoGstHdl             Handle to the stream out interface.
 *
 * @return Returns the status of the operation.
 * @retval eBTRMgrSOGstSuccess  on success, appropriate  error code otherwise.
 */
eBTRMgrSOGstRet BTRMgr_SO_GstSendEOS (tBTRMgrSoGstHdl hBTRMgrSoGstHdl);
/** @} */

#endif /* __BTR_MGR_STREAMOUT_GST_H__ */

