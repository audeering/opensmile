/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

ffmpegSource : opens and decodes audio in media files through FFmpeg

*/


#include <ffmpeg/ffmpegSource.hpp>
#include <core/smileThread.hpp>

#define MODULE "cFFmpegSource"

#ifdef HAVE_FFMPEG
SMILECOMPONENT_STATICS(cFFmpegSource)

SMILECOMPONENT_REGCOMP(cFFmpegSource)
{
  SMILECOMPONENT_REGCOMP_INIT
  scname = COMPONENT_NAME_CFFMPEGSOURCE;
  sdescription = COMPONENT_DESCRIPTION_CFFMPEGSOURCE;

  // we inherit cDataSource configType and extend it:
  SMILECOMPONENT_INHERIT_CONFIGTYPE("cDataSource")

  SMILECOMPONENT_IFNOTREGAGAIN(
    ct->makeMandatory(ct->setField("filename","The filename of the media file to load. The file must contain an audio stream decodable by FFmpeg.","input.wav"));
    ct->setField("monoMixdown","Mix down all channels to 1 mono channel (1=on, 0=off)",1);
    ct->setField("outFieldName", "Set the name of the output field, containing the pcm data", "pcm");
    // overwrite cDataSource's default:
    ct->setField("blocksize_sec", NULL , 1.0);
  )

  SMILECOMPONENT_MAKEINFO(cFFmpegSource);
}

SMILECOMPONENT_CREATE(cFFmpegSource)

//-----

static char *avGetErrorString(int errnum)
{
  char *errbuf = (char *)malloc(AV_ERROR_MAX_STRING_SIZE);
  av_strerror(errnum, errbuf, AV_ERROR_MAX_STRING_SIZE);
  return errbuf;
}

// RAII wrapper around smileMutex to do initialization/cleanup when used as static variable
struct sSmileMutexWrapper {
  smileMutex mutex;
  sSmileMutexWrapper() { smileMutexCreate(mutex); }
  ~sSmileMutexWrapper() { smileMutexDestroy(mutex); }
};

static void avLogCallback(void *ptr, int level, const char *fmt, va_list vl)
{
  // mutex to make avLogCallback thread-safe, as required by multi-threaded FFmpeg decoders
  static sSmileMutexWrapper logMutex;

  // discard log messages above the log level
  if (level >= 0) {
    level &= 0xff;
  }
  if (level > av_log_get_level())
    return;

  smileMutexLock(logMutex.mutex);

  int printPrefix = 1;

  // get length of formatted log line
  int ret = av_log_format_line2(ptr, level, fmt, vl, NULL, 0, &printPrefix);
  if (ret < 0) {
    SMILE_WRN(3, "av_log_format_line2 failed with return code %d", ret);
    smileMutexUnlock(logMutex.mutex);
    return;
  }
  
  int lineSize = ret;
  char *line = (char *)malloc(lineSize + 1);

  // format log line
  ret = av_log_format_line2(ptr, level, fmt, vl, line, lineSize + 1, &printPrefix);
  if (ret < 0) {
    free(line);
    SMILE_WRN(3, "av_log_format_line2 failed with return code %d", ret);
    smileMutexUnlock(logMutex.mutex);
    return;
  }

  if (level <= AV_LOG_ERROR) {
    SMILE_ERR(3, line);
  } else if (level <= AV_LOG_WARNING) {
    SMILE_WRN(3, line);
  } else if (level <= AV_LOG_INFO) {
    SMILE_MSG(3, line);
  } else if (level <= AV_LOG_VERBOSE) {
    SMILE_DBG(3, line);
  }
  
  free(line);

  smileMutexUnlock(logMutex.mutex);
}

cFFmpegSource::cFFmpegSource(const char *_name) :
  cDataSource(_name),
  filename(NULL),
  monoMixdown(true),
  avFormatContext(NULL),
  avCodecContext(NULL),
  audioStreamIndex(-1),
  avFrame(NULL),
  receivingFrames(false),
  eof(false),
  numSamplesWrittenOfCurrentFrame(-1)
{
}

void cFFmpegSource::myFetchConfig()
{
  cDataSource::myFetchConfig();

  filename = getStr("filename");
  SMILE_IDBG(2,"filename = '%s'",filename);
  if (filename == NULL) COMP_ERR("myFetchConfig: getStr(filename) returned NULL! missing option in config file?");

  monoMixdown = (bool)getInt("monoMixdown");
  if (monoMixdown) { SMILE_IDBG(2,"monoMixdown enabled!"); }

  outFieldName = getStr("outFieldName");
  if(outFieldName == NULL) COMP_ERR("myFetchConfig: getStr(outFieldName) returned NULL! missing option in config file?");
}

int cFFmpegSource::configureWriter(sDmLevelConfig &c)
{
  // instead of using the default log callback, we define a custom one which uses cSmileLogger
  av_log_set_level(AV_LOG_VERBOSE);
  av_log_set_callback(avLogCallback);

  // av_register_all has been deprecated starting liibavformat 58.9.100 and needs not to be called anymore
#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(58, 9, 100)
  av_register_all();
#endif

  int ret;

  if ((ret = avformat_open_input(&avFormatContext, filename, NULL, NULL)) < 0) {
    COMP_ERR("FFmpeg failed to open the file '%s'! %s",filename,avGetErrorString(ret));
  }

  if ((ret = avformat_find_stream_info(avFormatContext, NULL)) < 0) {
    COMP_ERR("could not find stream information of file '%s'! %s",filename,avGetErrorString(ret));
  }

  openAVCodecContext(&audioStreamIndex, &avCodecContext, avFormatContext);

  // log debug info about audio format
  av_dump_format(avFormatContext, 0, filename, 0);

  avFrame = av_frame_alloc();
  if (!avFrame) {
    COMP_ERR("could not allocate frame! Out of memory?");
  }

  // set config parameters
  c.T = 1.0/avCodecContext->sample_rate;
  c.N = avCodecContext->channels;
  c.noTimeMeta = true;

  return 1;
}

int cFFmpegSource::setupNewNames(long nEl) 
{
  // configure dataMemory level, names, etc.
  if (monoMixdown) {
    writer_->addField(outFieldName,1);
  } else {
    writer_->addField(outFieldName,avCodecContext->channels);
  }

  namesAreSet_ = 1;
  return 1;
}

eTickResult cFFmpegSource::myTick(long long t)
{
  if (isEOI()) {
    // Check if we successfully reached the end of the current segment,
    // or if processing got interrupted at a different point.
    // If so, give an error:
    if (!eof) {
      SMILE_IERR(1, "Processing aborted before all data was read from the input media file! There must be something wrong with your config, e.g. a dataReader blocking a dataMemory level. Look for level full error messages in the debug mode output!");
    }
    return TICK_INACTIVE;
  }

  if (eof) {
    SMILE_IWRN(6,"not reading from file, already EOF");
    return TICK_INACTIVE;
  }

  if (!writer_->checkWrite(blocksizeW_)) {
    return TICK_DEST_NO_SPACE;
  }

  int numSamplesWrittenInThisTick = 0;

  while (true) {
    if (numSamplesWrittenOfCurrentFrame != -1) {
      int samplesToWrite = MIN(avFrame->nb_samples - numSamplesWrittenOfCurrentFrame, blocksizeW_ - numSamplesWrittenInThisTick);
      convertAndCopyFrameSamplesToMatrix(numSamplesWrittenOfCurrentFrame, samplesToWrite);
      writer_->setNextMatrix(mat_);
      numSamplesWrittenInThisTick += samplesToWrite;
      numSamplesWrittenOfCurrentFrame += samplesToWrite;
      if (numSamplesWrittenOfCurrentFrame == avFrame->nb_samples) {
        av_frame_unref(avFrame);
        numSamplesWrittenOfCurrentFrame = -1;
      }
    }

    if (numSamplesWrittenInThisTick == blocksizeW_) {
      break;
    }

    int status;
    do {
      // if receivingFrames is false, we have to read a packet and send it to the decoder before we can receive more decoded data
      // if receivingFrames is true, we first need to read more frames from the decoder before feeding new packets
      if (!receivingFrames) {
        readAndSendPacketToDecoder();
      }
      status = receiveFrame();    
    } while (status == 0); // status == 0 means decoder needs more data before it can output data and has not reached EOF yet

    if (status == 1) {
      // we received a frame from the decoder
      numSamplesWrittenOfCurrentFrame = 0;
      continue;
    } else {
      // decoder has reached EOF
      eof = true;
      break;
    }
  }

  return numSamplesWrittenInThisTick > 0 ? TICK_SUCCESS : TICK_INACTIVE;
}

void cFFmpegSource::openAVCodecContext(int *streamIndex, AVCodecContext **avCodecContext, AVFormatContext *avFormatContext)
{
  int ret;

  // find audio stream and get decoder for it
  AVCodec *decoder;
  if ((ret = av_find_best_stream(avFormatContext, AVMEDIA_TYPE_AUDIO, -1, -1, &decoder, 0)) < 0) {
    COMP_ERR("Could not find an audio stream in the input file. %s",avGetErrorString(ret));
  } 

  *streamIndex = ret;
  AVStream *stream = avFormatContext->streams[*streamIndex];

  // set discard flag for all other streams so the demuxer may skip them
  // some demuxers ignore this flag so we still need to skip packets manually in readAndSendPacketToDecoder()
  for (int i = 0; i < avFormatContext->nb_streams; i++) {
    if (i != *streamIndex) {
      avFormatContext->streams[i]->discard = AVDISCARD_ALL;
    }
  }

  *avCodecContext = avcodec_alloc_context3(decoder);
  if (!*avCodecContext) {
    COMP_ERR("Could not allocate audio codec context");
  }

  // copy codec parameters from input stream to output codec context
  if ((ret = avcodec_parameters_to_context(*avCodecContext, stream->codecpar)) < 0) {
    COMP_ERR("Could not copy audio codec parameters to decoder context. %s",avGetErrorString(ret));
  }

  // initialize the decoder with reference counting
  AVDictionary *options = NULL;
  av_dict_set(&options, "refcounted_frames", "1", 0);

  if ((ret = avcodec_open2(*avCodecContext, decoder, &options)) < 0) {
    COMP_ERR("Could not open audio codec. %s",avGetErrorString(ret));
  }
}

void cFFmpegSource::readAndSendPacketToDecoder()
{
readFrame:
  AVPacket avPacket;
  av_init_packet(&avPacket);
  avPacket.data = NULL;
  avPacket.size = 0;

  int ret = av_read_frame(avFormatContext, &avPacket);
  if (ret >= 0) {
    // discard any packets that do not belong to the audio stream we are interested in
    if (avPacket.stream_index != audioStreamIndex) {
      av_packet_unref(&avPacket);      
      goto readFrame;      
    }
    // send packet to decoder
    if ((ret = avcodec_send_packet(avCodecContext, &avPacket)) < 0) {
      COMP_ERR("Error sending packet to decoder. %s",avGetErrorString(ret));
    }
    av_packet_unref(&avPacket);      
  } else if (ret == AVERROR_EOF) {    
    // reached end-of-input, send a flush packet to flush decoder
    if ((ret = avcodec_send_packet(avCodecContext, NULL)) < 0) {
      COMP_ERR("Error flushing decoder. %s",avGetErrorString(ret));
    }
  } else {
    COMP_ERR("Error reading frame. %s",avGetErrorString(ret));
  }
}

// Receives decoded data from the decoder
// Return value: -1 eof, 0 error, > 0 num samples read
int cFFmpegSource::receiveFrame()
{
  int ret = avcodec_receive_frame(avCodecContext, avFrame);
  if (ret == AVERROR(EAGAIN)) {
    // decoder did not produce any output and needs more input
    receivingFrames = false;
    return 0; 
  } else if (ret == AVERROR_EOF) {
    // decoder reached end-of-file and won't produce any more output
    receivingFrames = false;
    return -1; 
  } else if (ret < 0) {
    // an error occurred during decoding
    COMP_ERR("Error during decoding. %s",avGetErrorString(ret));
  } else {
    // decoding succeeded
    receivingFrames = true;
    return 1;
  }
}

// converts a part of the samples in avFrame to float format, optionally mixing them down to Mono, and copies them to mat_
void cFFmpegSource::convertAndCopyFrameSamplesToMatrix(int index, int numSamples)
{
  if (index + numSamples > avFrame->nb_samples)
    COMP_ERR("parameters 'index' and 'numSamples' out of range");
  if (numSamples > blocksizeW_)
    COMP_ERR("parameter 'numSamples' out of range");

  if (mat_ == NULL) {
    allocMat(monoMixdown ? 1 : avCodecContext->channels, blocksizeW_);
  }

  int nChan = avCodecContext->channels;

  if (!monoMixdown) {
    switch (avCodecContext->sample_fmt) {
      case AV_SAMPLE_FMT_U8:   // unsigned 8 bits, packed
        for (int i = index; i < index + numSamples; i++)
          for (int ch = 0; ch < nChan; ch++) {
            mat_->set(ch, i - index, (((uint8_t *)avFrame->extended_data[0])[i*nChan+ch]-128)/(FLOAT_DMEM)127.0);
          }
        break;
      case AV_SAMPLE_FMT_U8P:  // unsigned 8 bits, planar   
        for (int i = index; i < index + numSamples; i++)
          for (int ch = 0; ch < nChan; ch++) {
            mat_->set(ch, i - index, (((uint8_t *)avFrame->extended_data[ch])[i]-128)/(FLOAT_DMEM)127.0);
          }
        break;
      case AV_SAMPLE_FMT_S16:   // signed 16 bits, packed
        for (int i = index; i < index + numSamples; i++)
          for (int ch = 0; ch < nChan; ch++) {
            mat_->set(ch, i - index, ((int16_t *)avFrame->extended_data[0])[i*nChan+ch]/(FLOAT_DMEM)32767.0); 
          }
        break;
      case AV_SAMPLE_FMT_S16P:  // signed 16 bits, planar   
        for (int i = index; i < index + numSamples; i++)
          for (int ch = 0; ch < nChan; ch++) {
            mat_->set(ch, i - index, ((int16_t *)avFrame->extended_data[ch])[i]/(FLOAT_DMEM)32767.0);
          }
        break;
      case AV_SAMPLE_FMT_S32:   // signed 32 bits, packed
        for (int i = index; i < index + numSamples; i++)
          for (int ch = 0; ch < nChan; ch++) {
            mat_->set(ch, i - index, ((int32_t *)avFrame->extended_data[0])[i*nChan+ch]/(FLOAT_DMEM)2147483647.0); 
          }
        break;
      case AV_SAMPLE_FMT_S32P:  // signed 32 bits, planar   
        for (int i = index; i < index + numSamples; i++)
          for (int ch = 0; ch < nChan; ch++) {
            mat_->set(ch, i - index, ((int32_t *)avFrame->extended_data[ch])[i]/(FLOAT_DMEM)2147483647.0);
          }
        break;
      case AV_SAMPLE_FMT_FLT:   // IEEE float 32 bits, packed
        for (int i = index; i < index + numSamples; i++)
          for (int ch = 0; ch < nChan; ch++) {
            mat_->set(ch, i - index, (FLOAT_DMEM)((float *)avFrame->extended_data[0])[i*nChan+ch]);
          }
        break;
      case AV_SAMPLE_FMT_FLTP:  // IEEE float 32 bits, planar   
        for (int i = index; i < index + numSamples; i++)
          for (int ch = 0; ch < nChan; ch++) {
            mat_->set(ch, i - index, (FLOAT_DMEM)((float *)avFrame->extended_data[ch])[i]);
          }
        break;
      case AV_SAMPLE_FMT_DBL:   // IEEE float 64 bits, packed
        for (int i = index; i < index + numSamples; i++)
          for (int ch = 0; ch < nChan; ch++) {
            mat_->set(ch, i - index, (FLOAT_DMEM)((double *)avFrame->extended_data[0])[i*nChan+ch]);
          }
        break;
      case AV_SAMPLE_FMT_DBLP:  // IEEE float 64 bits, planar   
        for (int i = index; i < index + numSamples; i++)
          for (int ch = 0; ch < nChan; ch++) {
            mat_->set(ch, i - index, (FLOAT_DMEM)((double *)avFrame->extended_data[ch])[i]);
          }
        break;
      case AV_SAMPLE_FMT_S64:   // signed 64 bits, packed
        for (int i = index; i < index + numSamples; i++)
          for (int ch = 0; ch < nChan; ch++) {
            mat_->set(ch, i - index, ((int64_t *)avFrame->extended_data[0])[i*nChan+ch]/(FLOAT_DMEM)9223372036854775807.0);
          }
        break;
      case AV_SAMPLE_FMT_S64P:  // signed 64 bits, planar   
        for (int i = index; i < index + numSamples; i++)
          for (int ch = 0; ch < nChan; ch++) {
            mat_->set(ch, i - index, ((int64_t *)avFrame->extended_data[ch])[i]/(FLOAT_DMEM)9223372036854775807.0);
          }
        break;
      default:
        const char *formatName = av_get_sample_fmt_name(avCodecContext->sample_fmt);
        if (formatName != NULL) {
          COMP_ERR("Unsupported sample format returned by decoder: %s", formatName);
        } else {
          COMP_ERR("Unsupported sample format returned by decoder: %i", avCodecContext->sample_fmt);
        }
    }    
  } else {
    switch (avCodecContext->sample_fmt) {
      case AV_SAMPLE_FMT_U8:   // unsigned 8 bits, packed
        for (int i = index; i < index + numSamples; i++) {
          FLOAT_DMEM tmp = 0.0;
          for (int ch = 0; ch < nChan; ch++) {
            tmp += (((uint8_t *)avFrame->extended_data[0])[i*nChan+ch]-128)/(FLOAT_DMEM)127.0;
          }
          mat_->set(0, i - index, tmp / nChan);
        }
        break;
      case AV_SAMPLE_FMT_U8P:  // unsigned 8 bits, planar   
        for (int i = index; i < index + numSamples; i++) {
          FLOAT_DMEM tmp = 0.0;
          for (int ch = 0; ch < nChan; ch++) {
            tmp += (((uint8_t *)avFrame->extended_data[ch])[i]-128)/(FLOAT_DMEM)127.0;
          }
          mat_->set(0, i - index, tmp / nChan);
        }
        break;
      case AV_SAMPLE_FMT_S16:   // signed 16 bits, packed
        for (int i = index; i < index + numSamples; i++) {
          FLOAT_DMEM tmp = 0.0;
          for (int ch = 0; ch < nChan; ch++) {
            tmp += ((int16_t *)avFrame->extended_data[0])[i*nChan+ch]/(FLOAT_DMEM)32767.0; 
          }
          mat_->set(0, i - index, tmp / nChan);
        }
        break;
      case AV_SAMPLE_FMT_S16P:  // signed 16 bits, planar   
        for (int i = index; i < index + numSamples; i++) {
          FLOAT_DMEM tmp = 0.0;
          for (int ch = 0; ch < nChan; ch++) {
            tmp += ((int16_t *)avFrame->extended_data[ch])[i]/(FLOAT_DMEM)32767.0;
          }
          mat_->set(0, i - index, tmp / nChan);
        }
        break;
      case AV_SAMPLE_FMT_S32:   // signed 32 bits, packed
        for (int i = index; i < index + numSamples; i++) {
          FLOAT_DMEM tmp = 0.0;
          for (int ch = 0; ch < nChan; ch++) {
            tmp += ((int32_t *)avFrame->extended_data[0])[i*nChan+ch]/(FLOAT_DMEM)2147483647.0; 
          }
          mat_->set(0, i - index, tmp / nChan);
        }
        break;
      case AV_SAMPLE_FMT_S32P:  // signed 32 bits, planar   
        for (int i = index; i < index + numSamples; i++) {
          FLOAT_DMEM tmp = 0.0;
          for (int ch = 0; ch < nChan; ch++) {
            tmp += ((int32_t *)avFrame->extended_data[ch])[i]/(FLOAT_DMEM)2147483647.0;
          }
          mat_->set(0, i - index, tmp / nChan);
        }
        break;
      case AV_SAMPLE_FMT_FLT:   // IEEE float 32 bits, packed
        for (int i = index; i < index + numSamples; i++) {
          FLOAT_DMEM tmp = 0.0;
          for (int ch = 0; ch < nChan; ch++) {
            tmp += (FLOAT_DMEM)((float *)avFrame->extended_data[0])[i*nChan+ch];
          }
          mat_->set(0, i - index, tmp / nChan);
        }
        break;
      case AV_SAMPLE_FMT_FLTP:  // IEEE float 32 bits, planar   
        for (int i = index; i < index + numSamples; i++) {
          FLOAT_DMEM tmp = 0.0;
          for (int ch = 0; ch < nChan; ch++) {
            tmp += (FLOAT_DMEM)((float *)avFrame->extended_data[ch])[i];
          }
          mat_->set(0, i - index, tmp / nChan);
        }
        break;
      case AV_SAMPLE_FMT_DBL:   // IEEE float 64 bits, packed
        for (int i = index; i < index + numSamples; i++) {
          FLOAT_DMEM tmp = 0.0;
          for (int ch = 0; ch < nChan; ch++) {
            tmp += (FLOAT_DMEM)((double *)avFrame->extended_data[0])[i*nChan+ch];
          }
          mat_->set(0, i - index, tmp / nChan);
        }
        break;
      case AV_SAMPLE_FMT_DBLP:  // IEEE float 64 bits, planar   
        for (int i = index; i < index + numSamples; i++) {
          FLOAT_DMEM tmp = 0.0;
          for (int ch = 0; ch < nChan; ch++) {
            tmp += (FLOAT_DMEM)((double *)avFrame->extended_data[ch])[i];
          }
          mat_->set(0, i - index, tmp / nChan);
        }
        break;
      case AV_SAMPLE_FMT_S64:   // signed 64 bits, packed
        for (int i = index; i < index + numSamples; i++) {
          FLOAT_DMEM tmp = 0.0;
          for (int ch = 0; ch < nChan; ch++) {
            tmp += ((int64_t *)avFrame->extended_data[0])[i*nChan+ch]/(FLOAT_DMEM)9223372036854775807.0;
          }
          mat_->set(0, i - index, tmp / nChan);
        }
        break;
      case AV_SAMPLE_FMT_S64P:  // signed 64 bits, planar   
        for (int i = index; i < index + numSamples; i++) {
          FLOAT_DMEM tmp = 0.0;
          for (int ch = 0; ch < nChan; ch++) {
            tmp += ((int64_t *)avFrame->extended_data[ch])[i]/(FLOAT_DMEM)9223372036854775807.0;
          }
          mat_->set(0, i - index, tmp / nChan);
        }
        break;
      default:
        const char *formatName = av_get_sample_fmt_name(avCodecContext->sample_fmt);
        if (formatName != NULL) {
          COMP_ERR("Unsupported sample format returned by decoder: %s", formatName);
        } else {
          COMP_ERR("Unsupported sample format returned by decoder: %i", avCodecContext->sample_fmt);
        }
    }
  }

  // set size of matrix to the actual number of samples copied
  mat_->nT = numSamples;
}

cFFmpegSource::~cFFmpegSource()
{
  if (numSamplesWrittenOfCurrentFrame != -1) av_frame_unref(avFrame); // in case the current frame was still being processed
  if (avFrame != NULL) av_frame_free(&avFrame);
  if (avCodecContext != NULL) avcodec_free_context(&avCodecContext);
  if (avFormatContext != NULL) avformat_close_input(&avFormatContext);  
}

#endif // HAVE_FFMPEG
