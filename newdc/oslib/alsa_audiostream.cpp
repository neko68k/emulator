
#include "audiostream_rif.h"
#include "oslib/oslib.h"

#include "cfg/cfg.h"

#if 0 && HOST_OS==OS_LINUX && !defined(TARGET_NACL32)

#if !defined(ANDROID)
#include <alsa/asoundlib.h>
#include <pthread.h>

snd_pcm_t *handle;

u8 Tempbuffer[8192*4];
void* AudioThread(void*)
{
	sched_param		sched;int policy;
	
	pthread_getschedparam(pthread_self(),&policy,&sched);
	sched.sched_priority++;//policy=SCHED_RR;
	pthread_setschedparam(pthread_self(),policy,&sched);	

	for(;;)
	{
		UpdateBuff(Tempbuffer);
		int rc = snd_pcm_writei(handle, Tempbuffer, settings.aica.BufferSize);
		if (rc == -EPIPE) {
		  /* EPIPE means underrun */
		  fprintf(stderr, "underrun occurred\n");
		  snd_pcm_prepare(handle);
		} else if (rc < 0) {
		  fprintf(stderr,
				  "error from writei: %s\n",
				  snd_strerror(rc));
		}  else if (rc != (int)settings.aica.BufferSize) {
		  fprintf(stderr,
				  "short write, write %d frames\n", rc);
		}
	}
}

cThread aud_thread(AudioThread,0);

void os_InitAudio()
{

	if (cfgLoadInt("audio","disable",0))
		return;

	cfgSaveInt("audio","disable",0);

	long loops;
  int size;
  
  snd_pcm_hw_params_t *params;
  unsigned int val;
  int dir;
  snd_pcm_uframes_t frames;

	/* Open PCM device for playback. */
  int rc = snd_pcm_open(&handle, "default",
                    SND_PCM_STREAM_PLAYBACK, 0);

  if (rc<0)
	rc = snd_pcm_open(&handle, "plughw:0,0,0",
                    SND_PCM_STREAM_PLAYBACK, 0);

  if (rc<0)
	rc = snd_pcm_open(&handle, "plughw:0,0",
                    SND_PCM_STREAM_PLAYBACK, 0);

  if (rc < 0) {
    fprintf(stderr,
            "unable to open pcm device: %s\n",
            snd_strerror(rc));
    return;
  }

  /* Allocate a hardware parameters object. */
  
	  snd_pcm_hw_params_alloca(&params);
  /* Fill it in with default values. */
  rc=snd_pcm_hw_params_any(handle, params);
    if (rc < 0) {
    fprintf(stderr,
            "Error:snd_pcm_hw_params_any %s\n",
            snd_strerror(rc));
    return;
  }
  /* Set the desired hardware parameters. */

  /* Interleaved mode */
  rc=snd_pcm_hw_params_set_access(handle, params,
                      SND_PCM_ACCESS_RW_INTERLEAVED);
    if (rc < 0) {
    fprintf(stderr,
            "Error:snd_pcm_hw_params_set_access %s\n",
            snd_strerror(rc));
    return;
  }
  /* Signed 16-bit little-endian format */
  rc=snd_pcm_hw_params_set_format(handle, params,
                              SND_PCM_FORMAT_S16_LE);
    if (rc < 0) {
    fprintf(stderr,
            "Error:snd_pcm_hw_params_set_format %s\n",
            snd_strerror(rc));
    return;
  }
  /* Two channels (stereo) */
  rc=snd_pcm_hw_params_set_channels(handle, params, 2);
    if (rc < 0) {
    fprintf(stderr,
            "Error:snd_pcm_hw_params_set_channels %s\n",
            snd_strerror(rc));
    return;
  }
  /* 44100 bits/second sampling rate (CD quality) */
  val = 44100;
  rc=snd_pcm_hw_params_set_rate_near(handle, params,
                                  &val, &dir);
    if (rc < 0) {
    fprintf(stderr,
            "Error:snd_pcm_hw_params_set_rate_near %s\n",
            snd_strerror(rc));
    return;
  }
  /* Set period size to settings.aica.BufferSize frames. */
  frames = settings.aica.BufferSize;
  rc=snd_pcm_hw_params_set_period_size_near(handle,
                              params, &frames, &dir);
    if (rc < 0) {
    fprintf(stderr,
            "Error:snd_pcm_hw_params_set_buffer_size_near %s\n",
            snd_strerror(rc));
    return;
  }
  frames*=4;
  rc=snd_pcm_hw_params_set_buffer_size_near(handle,
                              params, &frames);
    if (rc < 0) {
    fprintf(stderr,
            "Error:snd_pcm_hw_params_set_buffer_size_near %s\n",
            snd_strerror(rc));
    return;
  }

  /* Write the parameters to the driver */
  rc = snd_pcm_hw_params(handle, params);
  if (rc < 0) {
    fprintf(stderr,
            "unable to set hw parameters: %s\n",
            snd_strerror(rc));
    return;
  }

  aud_thread.Start();
}

void os_TermAudio()
{
	snd_pcm_drain(handle);
	snd_pcm_close(handle);
}
#else

#endif
#endif