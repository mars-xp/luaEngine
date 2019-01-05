#ifndef ANDROIDLUA_INPUT_EVENT_H
#define ANDROIDLUA_INPUT_EVENT_H
int init_uinput_dev();
int write_click_event(int x, int y);
int write_back_event();
int write_volume_up();
int write_volume_down();

int write_home_event();
int write_home_page_event();

int write_menu_event();
int destroy();

int touchDown(int x, int y);
int touchScroll(int x, int y);
int touchUp(int x, int y);

#endif
