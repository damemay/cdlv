#include "../cdlv.h"
#include "../cdlv_util.h"
#include "cdlv_menu.h"
#include <dirent.h>

static inline void menu_reprint(cdlv_menu* menu) {
    if(menu->current_choice < menu->file_count) {
        strcpy(menu->text, menu->path);
        strcat(menu->text, "\n" cdlv_arrow);
        for(size_t i=menu->current_choice; i<menu->file_count; ++i) {
            strcat(menu->text, menu->files[i]);
            strcat(menu->text, "\n");
        }
        strcat(menu->text, " ");
    }
}

cdlv_menu* cdlv_menu_create(cdlv_base* base, const char* path, sdl_base* sdl) {
    cdlv_menu* menu = malloc(sizeof(cdlv_menu));
    if(!menu)
        cdlv_die("Could not allocate memory for the menu");

    DIR* dir = opendir(path);
    if(!dir) {
        menu->path_exists = false;
    } else {
        menu->path_exists = true;
        cdlv_duplicate_string(&menu->path, path, strlen(path)+1);
        menu->file_count = 0;
        struct dirent* entry;
        cdlv_alloc_ptr_arr(&menu->files, cdlv_max_menu_entries, char*);
        while((entry = readdir(dir)) != NULL) {
            if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0 || !strstr(entry->d_name, ".adv"))
                continue;
            menu->files[menu->file_count] = calloc(strlen(entry->d_name)+1, sizeof(char));
            strcpy(menu->files[menu->file_count], entry->d_name);
            ++menu->file_count;
        }
    }
    closedir(dir);

    cdlv_text_create(base,
            base->config->text_font, 16, 700,
            10, 10,
            255, 255, 255, 255, sdl->renderer);

    cdlv_alloc_ptr_arr(&menu->text, cdlv_max_string_size, char);
    if(menu->path_exists) {
        menu->current_choice = 0;
        menu_reprint(menu);
    } else {
        sprintf(menu->text, "Folder %s not found!", path);
    }

    menu->text_speed = base->config->text_speed;
    base->config->text_speed = 0;
    cdlv_text_update(base, menu->text);

    return menu;
}

void cdlv_menu_render(cdlv_base* base, sdl_base* sdl) {
    SDL_RenderClear(sdl->renderer);
    cdlv_text_render(base, sdl->renderer);
}

void cdlv_menu_clean(cdlv_menu* menu) {
    for(size_t i=0; i<menu->file_count; ++i)
        free(menu->files[i]);
    free(menu->files);
    free(menu->text);
    free(menu->path);
    free(menu);
}

int check_errors(cdlv_base* base, cdlv_error err) {
    if(err == cdlv_no_err) return 0;
    char content[cdlv_max_string_size];
    switch(err) {
        case cdlv_config_err:
            cdlv_text_update(base, "ERROR: Syntax error inside config file!"); return -1;
        case cdlv_no_mem_err:
            cdlv_text_update(base, "ERROR: No available memory for allocation of new data!"); return -1;
        case cdlv_file_err:
            cdlv_text_update(base, "ERROR: Wrong file paths provided!"); return -1;
        case cdlv_script_err:
            cdlv_text_update(base, "ERROR: Syntax error inside script file!"); return -1;
        case cdlv_sdl_err:
            sprintf(content, "ERROR: SDL returned error: %s", SDL_GetError());
            cdlv_text_update(base, content); return -1;
        default:return -1;
    }
}

static inline void menu_load_script(cdlv_base** base, cdlv_menu** menu, sdl_base* sdl) {
    if((*menu)->current_choice >= 0 && (*menu)->current_choice < (*menu)->file_count) {
        char full_path[strlen((*menu)->path)+strlen((*menu)->files[(*menu)->current_choice])+3];
        sprintf(full_path, "%s/%s", (*menu)->path, (*menu)->files[(*menu)->current_choice]);
        (*base)->config->text_speed = (*menu)->text_speed;
        cdlv_config* config = (*base)->config;
        if(check_errors(*base, (*base)->error) == -1) return;
        cdlv_clean_all((*base));
        *base = NULL;
        *base = cdlv_create(config);
        cdlv_read_file((*base), full_path, &sdl->renderer);
        if(check_errors(*base, (*base)->error) == -1) return;
        SDL_SetWindowSize(sdl->window, (*base)->canvas->w, (*base)->canvas->h);
        cdlv_start((*base));
        if(check_errors(*base, (*base)->error) == -1) return;
        cdlv_menu_clean((*menu));
        *menu = NULL;
        (*base)->state = cdlv_main_run;
    }
}

void cdlv_menu_handle_keys(cdlv_base** base, cdlv_menu** menu, sdl_base* sdl) {
    if((*base)->can_interact) {
        if(sdl->event.type == SDL_KEYUP && sdl->event.key.repeat == 0) {
            switch(sdl->event.key.keysym.sym) {
                case SDLK_UP:
                    if((*menu)->current_choice > 0) {
                        --(*menu)->current_choice;
                        menu_reprint(*menu);
                        cdlv_text_update((*base), (*menu)->text);
                    }
                    break;
                case SDLK_DOWN:
                    if((*menu)->current_choice < (*menu)->file_count) {
                        ++(*menu)->current_choice;
                        menu_reprint((*menu));
                        cdlv_text_update((*base), (*menu)->text);
                    }
                    break;
                case SDLK_RETURN: menu_load_script(base, menu, sdl); break;
            }
        }
        if(sdl->event.type == SDL_CONTROLLERBUTTONUP) {
            switch(sdl->event.cbutton.button) {
                case SDL_CONTROLLER_BUTTON_DPAD_UP:
                    if((*menu)->current_choice > 0) {
                        --(*menu)->current_choice;
                        menu_reprint(*menu);
                        cdlv_text_update((*base), (*menu)->text);
                    }
                    break;
                case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
                    if((*menu)->current_choice < (*menu)->file_count) {
                        ++(*menu)->current_choice;
                        menu_reprint((*menu));
                        cdlv_text_update((*base), (*menu)->text);
                    }
                    break;
                case SDL_CONTROLLER_BUTTON_A: menu_load_script(base, menu, sdl); break;
            }
        }
    }
}
