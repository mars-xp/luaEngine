#ifndef ANDROIDLUA_SHELL_COMMAND_H
#define ANDROIDLUA_SHELL_COMMAND_H
#define CMD_INPUT_TEXT  "am broadcast -a ADB_INPUT_TEXT --es msg "
#define CMD_RUN_APP  "am start "
#define CMD_FORCE_STOP "am force-stop"
#define USER  "--user 0"

int input_text(char *content);

int run_app(char *package_name);

int kill_app(char *package_name);

#endif
