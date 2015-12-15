/*
 * loopback.c
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <pthread.h>
#include <stdio.h>
#include <tinyalsa/asoundlib.h>

#include "config.h"
#include "loopback.h"
#include "tinycap.h"
#include "tone-generator.h"

void *tone_play(void *config)
{
    printf("In Play thread...\n");
    struct audio_tool_config *conf = (struct audio_tool_config *) config;
    int argc = 3;
    char *argv[] = { "tone", "sine", "440" };
    conf->duration = 2;

    //ret = tone_generator_main(config, t_argc, t_argv);
    tone_generator_main(conf, argc, argv);

    return NULL;
}

void *tone_capture(void *config)
{
    printf("In Capture thread...\n");
    struct audio_tool_config *conf = (struct audio_tool_config *) config;
    int argc = 2;
    char *argv[] = { "capture", "blah.wav" };
    conf->duration = 2;

    //ret = tinycap_main(&config, argc, argv, tinycap);
    tinycap_main(conf, argc, argv, 0);

    return NULL;
}

int loopback_main(struct audio_tool_config *config)
{
    printf("Yarly! Loopback audio testin'...\n");
    pthread_t play_thread;
    pthread_t record_thread;

    // play
    if (pthread_create(&play_thread, NULL, tone_play, config)) {
        fprintf(stderr, "Errrr creating PLAY thread..\n");
        return 1;
    }
    // record
    if (pthread_create(&record_thread, NULL, tone_capture, config)) {
        fprintf(stderr, "Errrr creating RECORD thread..\n");
        return 1;
    }

    if (pthread_join(play_thread, NULL)) {
        fprintf(stderr, "Errrr joining PLAY thread..\n");
        return 2;
    }
    if (pthread_join(record_thread, NULL)) {
        fprintf(stderr, "Errrr joining RECORD thread..\n");
        return 2;
    }

    printf("\nBOOM! threads finished\n\n");

	return 0;
}

