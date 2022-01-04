/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/

#ifndef __CFFMPEGSOURCE_HPP
#define __CFFMPEGSOURCE_HPP

#include <core/smileCommon.hpp>
#include <core/dataSource.hpp>

#ifdef HAVE_FFMPEG

#ifdef __cplusplus
extern "C" {
#endif

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

#ifdef __cplusplus
}
#endif

#define COMPONENT_DESCRIPTION_CFFMPEGSOURCE "This component uses FFmpeg to decode audio from media files."
#define COMPONENT_NAME_CFFMPEGSOURCE "cFFmpegSource"


class cFFmpegSource : public cDataSource {
  private:
    const char *filename;
    const char *outFieldName;
    bool monoMixdown;    // if set to true, multi-channel files will be mixed down to 1 channel

    AVFormatContext *avFormatContext;
    AVCodecContext *avCodecContext;
    int audioStreamIndex;
    AVFrame *avFrame;
    bool receivingFrames;
    bool eof;    
    int numSamplesWrittenOfCurrentFrame;    // part of current frame already written out, -1 if avFrame does not contain a frame

    void openAVCodecContext(int *streamIndex, AVCodecContext **avCodecContext, AVFormatContext *avFormatContext);
    void readAndSendPacketToDecoder();
    int receiveFrame();
    void convertAndCopyFrameSamplesToMatrix(int index, int numSamples);

  protected:
    SMILECOMPONENT_STATIC_DECL_PR

    virtual void myFetchConfig() override;
    virtual eTickResult myTick(long long t) override;

    virtual int configureWriter(sDmLevelConfig &c) override;
    virtual int setupNewNames(long nEl) override;

  public:
    SMILECOMPONENT_STATIC_DECL

    cFFmpegSource(const char *_name);

    virtual ~cFFmpegSource();
};


#endif // HAVE_FFMPEG

#endif // __CFFMPEGSOURCE_HPP
