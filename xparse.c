#include <expat.h>
#include <stdio.h>
#include <string.h>

#define BUFFER_SIZE 100000

struct mixer_setting {
    XML_Char *name;
    XML_Char *value;
    XML_Char *id;
};

struct use_case {
    XML_Char *name;
    struct mixer_setting *mixer_settings[20];
    int num_mixer_settings;
};

struct config_parse_state {
	XML_Char last_content[BUFFER_SIZE];
    struct use_case *use_cases[10];
    int num_use_cases;
	int level;
};

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
    state->use_cases[state->num_use_cases] = uc;
    state->num_use_cases++;
    return uc;
}
    

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
        //struct mixer_setting *ms = new_mixer_setting();
        //strncpy(ms->name, attr_name, 50);
        //strncpy(ms->value, attr_value, 50);
        //strncpy(ms->id, attr_id, 50);
        //uc->mixer_settings[uc->num_mixer_settings] = ms;
        //uc->num_mixer_settings++;
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
	printf("Boo ya!\n");

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

    //printf("INITSZZZ:: %d\n", state.num_mixer_settings);
    //for ( int i = 0; i < state.num_mixer_settings; i++) {
    //    struct mixer_setting *ms = state.initial_mixer_settings[i];
    //    printf("INIT:: name: %s id: %s val: %s\n", ms->name, ms->id, ms->value);
    //}
    printf("USE CASES:: %d\n", state.num_use_cases);
    for ( int i = 0; i < state.num_use_cases; i++) {
        struct use_case *uc = state.use_cases[i];
        printf("USE CASE:: name: %s\n", uc->name);
    }


	return 0;

}
