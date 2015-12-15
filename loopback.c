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
#include "tone-generator.h"

void *tone_play(void *config)
{
    struct audio_tool_config *conf = (struct audio_tool_config *) config;
    int t_argc = 3;
    char *t_argv[] = { "tone", "sine", "440" };
    conf->duration = 2;

    printf("Thread DUration : %d\nCalling tone_gen...\n", conf->duration);
    tone_generator_main(config, t_argc, t_argv);

    return NULL;
}

int loopback_main(struct audio_tool_config *config)
{
    printf("Yarly! Loopback audio testin'...\n");
    pthread_t tplay_thread;

    if (pthread_create(&tplay_thread, NULL, tone_play, config)) {
        fprintf(stderr, "Errrr creating thread..\n");
        return 1;
    }

    if (pthread_join(tplay_thread, NULL)) {
        fprintf(stderr, "Errrr joining thread..\n");
        return 2;
    }

    printf("\nBOOM! thread finished\n\n");

	return 0;
}

