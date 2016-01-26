#include <expat.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "config.h"

#include <tinyalsa/asoundlib.h>

//#define MIXER_XML_PATH "example_android_mixer_paths.xml.SHORT"
#define MIXER_XML_PATH "/system/etc/mixer_paths.xml"
#define BUF_SIZE 1024
#define INITIAL_MIXER_PATH_SIZE 8
#define TOP_LEVEL_XML_SETTINGS "initial-settings"

char *mixer_paths_xml = MIXER_XML_PATH;


///////////////////////////////////////////////////////
// All gloriously stolen from audio_route.c
///////////////////////////////////////////////////////
// There is a CONFIG_PARSE_STATE which is passed around XML
// functions as a holding place for objects. This has a
// singleton AUDIO_ROUTE object which contains an array of
// MIXER_PATHS, which in turn, contains an array of MIXER_SETTINGS

struct mixer_setting {
    char *name;
    char *id;
    char *value;
};

struct mixer_value {
    char *name;
    char *id;
    char *value;
};

struct mixer_path {
    char *name;
    unsigned int size;
    unsigned int length;
    struct mixer_setting *setting;
};

struct audio_route {
    // NOT SURE IF THIS REALLY NEEDED
    struct mixer *mixer;

    unsigned int mixer_path_size;
    unsigned int num_mixer_paths;
    struct mixer_path *mixer_path;
};

struct config_parse_state {
    struct audio_route *ar;
    struct mixer_path *path;
    int level;
};


// FUNCTIONSZZ /////////////////////////

static int alloc_path_setting(struct mixer_path *path)
{
    struct mixer_setting *new_path_setting;
    int path_index;

    /* check if we need to allocate more space for path settings */
    if (path->size <= path->length) {
        if (path->size == 0)
            path->size = INITIAL_MIXER_PATH_SIZE;
        else
            path->size *= 2;

        new_path_setting = realloc(path->setting,
                                   path->size * sizeof(struct mixer_setting));
        if (new_path_setting == NULL) {
            printf("Unable to allocate more path settings");
            return -1;
        } else {
            path->setting = new_path_setting;
        }
    }

    path_index = path->length;
    path->length++;

    return path_index;
}


static int path_add_setting(struct audio_route *ar, struct mixer_path *path,
                            struct mixer_setting *setting)
{
    int path_index;

    path_index = alloc_path_setting(path);
    if (path_index < 0)
        return -1;

    if (setting->name)
        path->setting[path_index].name = strdup(setting->name);
    if (setting->id)
        path->setting[path_index].id = strdup(setting->id);
    if (setting->value)
        path->setting[path_index].value = strdup(setting->value);

    return 0;
}

static int path_add_value(struct audio_route *ar, struct mixer_path *path,
                          struct mixer_value *mixer_value)
{
    int path_index;
    path_index = alloc_path_setting(path);

    /* initialise the new path setting */
    path->setting[path_index].name = strdup(mixer_value->name);
    if (mixer_value->id) {
        path->setting[path_index].id = strdup(mixer_value->id);
	} else {
		path->setting[path_index].id = NULL;
	}
    path->setting[path_index].value = strdup(mixer_value->value);

    return 0;
}

static struct mixer_path *path_get_by_name(struct audio_route *ar,
                                           const char *name)
{
    unsigned int i;

    for (i = 0; i < ar->num_mixer_paths; i++)
        if (strcmp(ar->mixer_path[i].name, name) == 0)
            return &ar->mixer_path[i];

    return NULL;
}

static int path_add_path(struct audio_route *ar, struct mixer_path *path,
                         struct mixer_path *sub_path)
{
    unsigned int i;

    for (i = 0; i < sub_path->length; i++) {
        if (path_add_setting(ar, path, &sub_path->setting[i]) < 0)
            return -1;
    }

    return 0;
}

static struct mixer_path *path_create(struct audio_route *ar, const char *name)
{
    struct mixer_path *new_mixer_path = NULL;

    if (path_get_by_name(ar, name)) {
        printf("Path name '%s' already exists", name);
        return NULL;
    }

    /* check if we need to allocate more space for mixer paths */
    if (ar->mixer_path_size <= ar->num_mixer_paths) {
        if (ar->mixer_path_size == 0)
            ar->mixer_path_size = INITIAL_MIXER_PATH_SIZE;
        else
            ar->mixer_path_size *= 2;

        new_mixer_path = realloc(ar->mixer_path, ar->mixer_path_size *
                                 sizeof(struct mixer_path));
        if (new_mixer_path == NULL) {
            printf("Unable to allocate more paths");
            return NULL;
        } else {
            ar->mixer_path = new_mixer_path;
        }
    }

    /* initialise the new mixer path */
    ar->mixer_path[ar->num_mixer_paths].name = strdup(name);
    ar->mixer_path[ar->num_mixer_paths].size = 0;
    ar->mixer_path[ar->num_mixer_paths].length = 0;
    ar->mixer_path[ar->num_mixer_paths].setting = NULL;

    /* return the mixer path just added, then increment number of them */
    return &ar->mixer_path[ar->num_mixer_paths++];
}

static void start_tag(void *data, const XML_Char *tag_name,
                      const XML_Char **attr)
{
    const XML_Char *attr_name = NULL;
    const XML_Char *attr_id = NULL;
    const XML_Char *attr_value = NULL;
    struct config_parse_state *state = data;
    struct audio_route *ar = state->ar;
    unsigned int i;
    struct mixer_value mixer_value;

    /* Get name, id and value attributes (these may be empty) */
    for (i = 0; attr[i]; i += 2) {
        if (strcmp(attr[i], "name") == 0)
            attr_name = attr[i + 1];
        if (strcmp(attr[i], "id") == 0)
            attr_id = attr[i + 1];
        else if (strcmp(attr[i], "value") == 0)
            attr_value = attr[i + 1];
    }

    /* Look at tags */
    if (strcmp(tag_name, "path") == 0) {
        if (attr_name == NULL) {
            printf("Unnamed path!");
        } else {
            if (state->level == 1) {
                /* top level path: create and stash the path */
                state->path = path_create(ar, (char *)attr_name);
            } else {
                /* nested path */
                struct mixer_path *sub_path = path_get_by_name(ar, attr_name);
                path_add_path(ar, state->path, sub_path);
            }
        }
    }
    else if (strcmp(tag_name, "ctl") == 0) {
        mixer_value.name = (char *) attr_name;
        mixer_value.value = (char *) attr_value;
        mixer_value.id = (char *) attr_id;

        path_add_value(ar, state->path, &mixer_value);
    }

    state->level++;
}

static void end_tag(void *data, const XML_Char *tag_name)
{
    struct config_parse_state *state = data;
    (void)tag_name;
    state->level--;
}

void print_setting(struct mixer_setting *ms)
{
	printf("Name: %s // ID: %s // Value: %s\n",
			ms->name, ms->id, ms->value);
}

void print_routes(struct config_parse_state *state)
{
    int i, j;
    printf("\n==============================================\n");
    for ( i = 0; i < state->ar->num_mixer_paths; i++) {
        struct mixer_path mp = state->ar->mixer_path[i];
        printf("\nPATHe: %s\n", mp.name);
        for ( j = 0; j < mp.length; j++) {
            struct mixer_setting ms = mp.setting[j];
			print_setting(&ms);
        }
		printf("\n");
    }
}

struct audio_route * setup_audio_route(struct audio_tool_config *config)
{
    struct audio_route *ar;
    ar = calloc(1, sizeof(struct audio_route));
    if (!ar) {
		printf("Nae calloc!\n");
        goto errr;
	}

    ar->mixer = mixer_open(config->card);
    if (!ar->mixer) {
        printf("Oofft, nae mixer\n");
        goto errr;
    }

    ar->mixer_path = NULL;
    ar->mixer_path_size = 0;
    ar->num_mixer_paths = 0;

    return ar;

errr:
    printf("BURNEY! - Couldn't SETUP AUDIO ROUTE\n");
	exit(-1);
}

void parse_xml(struct config_parse_state *state)
{
    XML_Parser parser;
    FILE *file;
    int bytes_read;
    void *buf;

    file = fopen(mixer_paths_xml, "r");
    if (!file) {
        printf("Oooft, cannae open xml file\n");
        goto errr;
    }

    parser = XML_ParserCreate(NULL);
    if (!parser) {
        printf("Yer arse in parsely!\n");
        goto errr;
    }

    XML_SetUserData(parser, state);
    XML_SetElementHandler(parser, start_tag, end_tag);

    for (;;) {
        buf = XML_GetBuffer(parser, BUF_SIZE);
        if (buf == NULL) {
			printf("NULL BUFF!\n");
            goto errr;
		}
        bytes_read = fread(buf, 1, BUF_SIZE, file);
        if (bytes_read < 0 ) {
			printf("Zero bytes read!\n");
            goto errr;
		}
        if (XML_ParseBuffer(parser, bytes_read, bytes_read == 0) == XML_STATUS_ERROR) {
            printf("Xml file buggered\n");
            goto errr;
        }
        if (bytes_read == 0) 
            break;
    }

    XML_ParserFree(parser);
    fclose(file);
	return;

errr:
    printf("BURNEY! PARSE_XML is aRSE IN PARSELY\n");
	exit(-1);
}

int tinyroute_main(struct audio_tool_config *config, int argc, char **argv)
{

		// 0. check for filename on cmd line rather than default
		if (argc > 1) {
			printf("Opening Mixer Paths XML %s\n", argv[1]);
			mixer_paths_xml = argv[1];
		} else {
			printf("Opening default file %s\n", MIXER_XML_PATH);
		}

    // 1. allocate audio_route and setup state
    struct audio_route *ar = setup_audio_route(config);
    struct config_parse_state state;
    memset(&state, 0, sizeof(state));
    state.ar = ar;
		// setup top level initial settings path
		state.path = path_create(ar, TOP_LEVEL_XML_SETTINGS);

    // 2. parse XML
    parse_xml(&state);

    // 3. print results
    print_routes(&state);

    return 0;

    // TODO: FREE RESOURCES
}
