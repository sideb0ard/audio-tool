#include <expat.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <tinyalsa/asoundlib.h>

//#define MIXER_XML_PATH "example_android_mixer_paths.xml.SHORT"
#define MIXER_XML_PATH "example_android_mixer_paths.xml"
#define BUF_SIZE 1024
#define INITIAL_MIXER_PATH_SIZE 8


///////////////////////////////////////////////////////
// All gloriously stolen from audio_route.c
///////////////////////////////////////////////////////

struct mixer_state {
    struct mixer_ctl *ctl;
    unsigned int num_values;
    int *old_value;
    int *new_value;
    int *reset_value;
};

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
    struct mixer *mixer;
    unsigned int num_mixer_ctls;
    struct mixer_state *mixer_state;

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

static int alloc_mixer_state(struct audio_route *ar)
{
    unsigned int i;
    // unsigned int j;
    unsigned int num_values;
    struct mixer_ctl *ctl;
    enum mixer_ctl_type type;

    ar->num_mixer_ctls = mixer_get_num_ctls(ar->mixer);
    ar->mixer_state = malloc(ar->num_mixer_ctls * sizeof(struct mixer_state));
    if (!ar->mixer_state)
        return -1;

    for (i = 0; i < ar->num_mixer_ctls; i++) {
        ctl = mixer_get_ctl(ar->mixer, i);
        num_values = mixer_ctl_get_num_values(ctl);

        ar->mixer_state[i].ctl = ctl;
        ar->mixer_state[i].num_values = num_values;

        ar->mixer_state[i].old_value = malloc(num_values * sizeof(int));
        ar->mixer_state[i].new_value = malloc(num_values * sizeof(int));
        ar->mixer_state[i].reset_value = malloc(num_values * sizeof(int));

        if (type == MIXER_CTL_TYPE_ENUM)
            ar->mixer_state[i].old_value[0] = mixer_ctl_get_value(ctl, 0);
        memcpy(ar->mixer_state[i].new_value, ar->mixer_state[i].old_value, num_values * sizeof(int));
    }
    return 0;
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
    unsigned int i;
    int path_index;
    unsigned int num_values;
    struct mixer_ctl *ctl;

    path_index = alloc_path_setting(path);

    /* initialise the new path setting */
    path->setting[path_index].name = strdup(mixer_value->name);
    if (mixer_value->id)
        path->setting[path_index].id = strdup(mixer_value->id);
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


    // TODO: something!!

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
    unsigned int ctl_index;
    struct mixer_ctl *ctl;
    int value;
    unsigned int id;
    struct mixer_value mixer_value;
    enum mixer_ctl_type type;

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
                // TODO - fix!
                ///* nested path */
                //struct mixer_path *sub_path = path_get_by_name(ar, attr_name);
                //path_add_path(ar, state->path, sub_path);
            }
        }
    }
   else if (strcmp(tag_name, "ctl") == 0) {
        if (state->level == 1) {
            // TODO: deal with init settings
        } else {
            /* nested ctl (within a path) */
            mixer_value.name = attr_name;
            mixer_value.value = attr_value;
            mixer_value.id = attr_id;

            path_add_value(ar, state->path, &mixer_value);
        }
    }
done:
    state->level++;
}

static void end_tag(void *data, const XML_Char *tag_name)
{
    struct config_parse_state *state = data;
    (void)tag_name;
    state->level--;
}

void print_routes(struct config_parse_state *state)
{
    int i;
    printf("\n==============================================\n");
    for ( i = 0; i < state->ar->num_mixer_paths; i++) {
        struct mixer_path mp = state->ar->mixer_path[i];
        printf("\nPATHe: %s\n", mp.name);
        printf("Mixer Path Size: %d\n", mp.size);
        printf("Mixer Path Length: %d\n", mp.length);
        for (int j = 0; j < mp.length; j++) {
            struct mixer_setting ms = mp.setting[j];
            printf("  MIXER SETTING NAME - %s\n", ms.name);
            //printf("  MIXER SETTING VALUE - %s\n", ms.value);
        }
    }
}

int tinyroute_main(int argc, char **argv)
{
    struct config_parse_state state;
    XML_Parser parser;
    FILE *file;
    int bytes_read;
    void *buf;
    struct audio_route *ar;   // i.e use case


    // TODO: cleanup
    // 1. allocate audio_route
    // 2. allocate state
    // 3. setup XML expat stuff
    // 4. print routes
    // 5. cleanup

    ar = calloc(1, sizeof(struct audio_route));
    if (!ar)
        goto errr;

    ar->mixer = mixer_open(0); // default card
    if (!ar->mixer) {
        printf("Oofft, nae mixer\n");
        goto errr;
    }

    ar->mixer_path = NULL;
    ar->mixer_path_size = 0;
    ar->num_mixer_paths = 0;

    if (alloc_mixer_state(ar) < 0)
        goto errr;

    file = fopen(MIXER_XML_PATH, "r");
    if (!file) {
        printf("Oooft, cannae open xml file\n");
        goto errr;
    }

    parser = XML_ParserCreate(NULL);
    if (!parser) {
        printf("Yer arse in parsely!\n");
        goto errr;
    }

    memset(&state, 0, sizeof(state));
    state.ar = ar;
    XML_SetUserData(parser, &state);
    XML_SetElementHandler(parser, start_tag, end_tag);

    for (;;) {
        buf = XML_GetBuffer(parser, BUF_SIZE);
        if (buf == NULL)
            goto errr;
        bytes_read = fread(buf, 1, BUF_SIZE, file);
        if (bytes_read < 0 )
            goto errr;
        if (XML_ParseBuffer(parser, bytes_read, bytes_read == 0) == XML_STATUS_ERROR) {
            printf("Xml file buggered\n");
            goto errr;
        }
        if (bytes_read == 0) 
            break;
    }

    XML_ParserFree(parser);
    fclose(file);

    print_routes(&state);

    return 0;

    // TODO: FREE RESOURCES

errr:
    printf("YOUCH\n");
    return -1;
}
