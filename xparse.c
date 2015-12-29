#include <expat.h>
#include <stdio.h>
#include <string.h>

#define BUFFER_SIZE 100000
#define ARRAY_SIZE 40000000

// CONFIG_PARSE_STATE is passed bewteen functions to maintain
// a state. It contains an array of 'use_case's, which contain
// an array of 'mixer_setting's

struct mixer_setting {
    XML_Char *name;
    XML_Char *value;
    XML_Char *id;
};

struct use_case {
    XML_Char *name;
    struct mixer_setting *mixer_settings;
    int num_mixer_settings;
    int mixer_array_size;
};

struct config_parse_state {
	XML_Char last_content[BUFFER_SIZE];
    struct use_case *use_cases[10000];
    int num_use_cases;
	int level;
};

// functions to generate new 'mixer_setting' and 'use_case'

struct mixer_setting * new_mixer_setting() {
    struct mixer_setting *ms = malloc(sizeof(struct mixer_setting));
    ms->name = malloc(sizeof(XML_Char)*50);
    ms->value = malloc(sizeof(XML_Char)*50);
    ms->id = malloc(sizeof(XML_Char)*50);
    return ms;
}

struct use_case * get_use_case(struct config_parse_state *state) {
    for (int i = 0; i < state->num_use_cases; i++) {
        if (!strcmp(state->last_content, state->use_cases[i]->name)) {
            return state->use_cases[i];
        }
    }
    struct use_case *uc = malloc(sizeof(struct use_case));
    uc->name = malloc(sizeof(XML_Char)*50);
    strncpy(uc->name, state->last_content, 50);
    // 
    uc->mixer_array_size = ARRAY_SIZE;
    uc->mixer_settings = malloc(sizeof(struct mixer_setting*) * uc->mixer_array_size);
    printf("INIT MIXER ARRAY SIZE: %d\n", uc->mixer_array_size);
    // 
    state->use_cases[state->num_use_cases] = uc;
    state->num_use_cases++;
    return uc;
}

// functions used by Expat 
void start_tag(void *data, const XML_Char *tag_name, const XML_Char **attr)
{
	const XML_Char *attr_name  = NULL;
	const XML_Char *attr_id    = NULL;
	const XML_Char *attr_value = NULL;
	struct config_parse_state *state = data;

	for ( int i = 0; attr[i]; i += 2) {
		if (strcmp(attr[i], "name") == 0)
			attr_name = attr[i + 1];
		if (strcmp(attr[i], "id") == 0)
			attr_id = attr[i + 1];
		else if (strcmp(attr[i], "value") == 0)
			attr_value = attr[i + 1];
	}

    if (state->level == 0) {
		strcpy(state->last_content, "initial-settings");
	} else if (state->level ==1 && !strcmp(tag_name, "path")) {
		strcpy(state->last_content, attr_name);
    }

	if (!strcmp(tag_name, "ctl")) {

        struct use_case *uc = get_use_case(state);

        struct mixer_setting *ms = new_mixer_setting();
        if (attr_name)
            strncpy(ms->name, attr_name, 50);
        if (attr_value)
            strncpy(ms->value, attr_value, 50);
        if (attr_id)
            strncpy(ms->id, attr_id, 50);

        printf("ADDING SETTING - %d - %d\n", uc->num_mixer_settings, uc->mixer_array_size);
        uc->mixer_settings[uc->num_mixer_settings] = *ms;
        uc->num_mixer_settings++;

        //if (uc->num_mixer_settings >= uc->mixer_array_size) {
        if (uc->mixer_array_size <= uc->num_mixer_settings) {
            printf("RESIZING!\n");
            uc->mixer_array_size *= 2;
            struct mixer_setting *tmpms = NULL;
            tmpms = realloc(uc->mixer_settings, sizeof(struct mixer_setting*) * uc->mixer_array_size); 
            if (tmpms == NULL) {
                printf("Yowie, couldn't realloc\n");
            } else {
                uc->mixer_settings = tmpms;
            }
        }
	}

	state->level++;
}

void end_tag(void *data, const XML_Char *el)
{
	struct config_parse_state *state = data;
	state->level--;
}
		

int xparse_main(int argc, char **argv)
{
	struct config_parse_state state;
	XML_Parser parser;
	FILE *fp;

	fp = fopen("example_android_mixer_paths.xml", "r");
	if (fp == NULL) {
		fprintf(stderr, "Oooft, err opening xml file\n");
		return -1;
	}

	parser = XML_ParserCreate(NULL);
	if (!parser) {
		fprintf(stderr, "Boo! failed to create parser!\n");
		return -1;
	}

	memset(&state, 0, sizeof(state));

	XML_SetUserData(parser, &state);
	XML_SetElementHandler(parser, start_tag, end_tag);

	char buffer[BUFFER_SIZE + 1];
	memset(buffer, 0, BUFFER_SIZE);

	size_t items_num = 0;
	items_num = fread(&buffer, sizeof(char), BUFFER_SIZE, fp);
    if (items_num == 0) {
        printf("(okay) no items read.\n");
        return -1;
    }
	
	if (XML_Parse(parser, buffer, strlen(buffer), XML_TRUE) == XML_STATUS_ERROR) {
		printf("Errrr! %s\n", XML_ErrorString(XML_GetErrorCode(parser)));
		return -1;
	}

	fclose(fp);
	XML_ParserFree(parser);

    for ( int i = 0; i < state.num_use_cases; i++) {
        struct use_case *uc = state.use_cases[i];
        printf("\nUSE CASE:: name: %s - num_mixer_settings: %d\n", uc->name, uc->num_mixer_settings);
        for (int j = 0; j < uc->num_mixer_settings; j++) {
            printf(" J %d\n", j);
            struct mixer_setting *ms = &uc->mixer_settings[j];
            printf("  CTL:: name: %s // // val: %s\n", ms->name, ms->value);
        }
    }

	return 0;

}