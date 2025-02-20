/* misc.c -- functions that do not fit in any other file */

/*
 * This file is part of CliFM
 * 
 * Copyright (C) 2016-2021, L. Abramovich <johndoe.arch@outlook.com>
 * All rights reserved.

 * CliFM is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * CliFM is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
*/

#include "helpers.h"

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#if defined(__FreeBSD__)
#include <sys/mount.h>
#include <sys/sysctl.h>
#endif
#include <time.h>
#include <unistd.h>
#include <readline/readline.h>

#include "aux.h"
#include "bookmarks.h"
#include "checks.h"
#include "exec.h"
#include "history.h"
#include "init.h"
#include "jump.h"
#include "listing.h"
#include "navigation.h"
#include "readline.h"
#include "strings.h"

void
set_term_title(const char *str)
{
	char *tmp = (char *)NULL;
	tmp = home_tilde(str);

	printf("\033]2;%s - %s\007", PROGRAM_NAME, tmp ? tmp : str);
	fflush(stdout);

	if (tmp)
		free(tmp);
}

int
filter_function(const char *arg)
{
	if (!arg) {
		printf(_("Current filter: %c%s\n"), filter_rev ? '!' : 0,
				filter ? filter : "none");
		return EXIT_SUCCESS;
	}

	if (*arg == '-' && strcmp(arg, "--help") == 0) {
		puts(_("Usage: ft, filter [unset] [REGEX]"));
		return EXIT_SUCCESS;
	}

	if (*arg == 'u' && strcmp(arg, "unset") == 0) {
		if (filter) {
			free(filter);
			filter = (char *)NULL;
			regfree(&regex_exp);
			puts(_("Filter unset"));
			filter_rev = 0;
		}

		else
			puts(_("No filter set"));

		return EXIT_SUCCESS;
	}

	if (filter)
		free(filter);

	regfree(&regex_exp);

	if (*arg == '!') {
		filter_rev = 1;
		arg++;
	} else {
		filter_rev = 0;
	}

	filter = savestring(arg, strlen(arg));

	if (regcomp(&regex_exp, filter, REG_NOSUB | REG_EXTENDED) != EXIT_SUCCESS) {
		fprintf(stderr, _("%s: '%s': Invalid regular expression\n"),
		    PROGRAM_NAME, filter);
		free(filter);
		filter = (char *)NULL;
		regfree(&regex_exp);
	}

	else
		puts(_("New filter successfully set"));

	return EXIT_SUCCESS;
}

/* Print either all tips (if ALL == 1) or just a random one (ALL == 0) */
void
print_tips(int all)
{
	const char *TIPS[] = {
	    "Try the autocd and auto-open functions: run 'FILE' instead "
	    "of 'open FILE' or 'cd FILE'",
	    "Add a new entry to the mimelist file with 'mm edit' or F6",
	    "Do not forget to take a look at the manpage",
	    "Need more speed? Try the light mode (Alt-y)",
	    "The Selection Box is shared among different instances of CliFM",
	    "Select files here and there with the 's' command",
	    "Use wildcards and regular expressions with the 's' command: "
	    "'s *.c' or 's .*\\.c$'",
	    "ELN's and the 'sel' keyword work for shell commands as well: "
	    "'file 1 sel'",
	    "Press TAB to automatically expand an ELN: 's 2' -> TAB -> "
	    "'s FILENAME'",
	    "Easily copy everything in CWD into another directory: 's * "
	    "&& c sel ELN/DIR'",
	    "Use ranges (ELN-ELN) to easily move multiple files: 'm 3-12 "
	    "ELN/DIR'",
	    "Trash files with a simple 't ELN'",
	    "Get mime information for a file: 'mm info ELN'",
	    "If too many files are listed, try enabling the pager ('pg on')",
	    "Once in the pager, go backwards pressing the keyboard shortcut "
	    "provided by your terminal emulator",
	    "Once in the pager, press 'q' to stop it",
	    "Press 'Alt-l' to switch to long view mode",
	    "Search for files using the slash command: '/*.png'",
	    "The search function allows regular expressions: '/^c'",
	    "Add a new bookmark by just entering 'bm ELN/FILE'",
	    "Use c, l, m, md, and r instead of cp, ln, mv, mkdir, and rm",
	    "Access a remote file system using the 'net' command",
	    "Manage default associated applications with the 'mime' command",
	    "Go back and forth in the directory history with 'Alt-j' and 'Alt-k' "
	    "or Shift-Left and Shift-Right",
	    "Open a new instance of CliFM with the 'x' command: 'x ELN/DIR'",
	    "Send a command directly to the system shell with ';CMD'",
	    "Run the last executed command by just running '!!'",
	    "Import aliases from file using 'alias import FILE'",
	    "List available aliases by running 'alias'",
	    "Create aliases to easily run your preferred commands",
	    "Open and edit the configuration file with 'edit'",
	    "Find a description for each CliFM command by running 'cmd'",
	    "Print the currently used color codes list by entering 'cc'",
	    "Press 'Alt-i' or 'Alt-.' to toggle hidden files on/off",
	    "List mountpoints by pressing 'Alt-m'",
	    "Disallow the use of shell commands with the -x option: 'clifm -x'",
	    "Go to the root directory by just pressing 'Alt-r'",
	    "Go to the home directory by just pressing 'Alt-e'",
	    "Press 'F8' to open and edit current color scheme",
	    "Press 'F9' to open and edit the keybindings file",
	    "Press 'F10' to open and edit the configuration file",
	    "Press 'F11' to open and edit the bookmarks file",
	    "Customize the starting path using the -p option: 'clifm -p PATH'",
	    "Use the 'o' command to open files and directories: 'o 12'",
	    "Bypass the resource opener specifying an application: 'o 12 "
	    "leafpad'",
	    "Open a file and send it to the background running 'o 24 &'",
	    "Create a custom prompt editing the configuration file",
	    "Customize color codes using the configuration file",
	    "Open the bookmarks manager by just pressing 'Alt-b'",
	    "Chain commands using ; and &&: 's 2 7-10; r sel'",
	    "Add emojis to the prompt by copying them to the Prompt line "
	    "in the configuration file",
	    "Create a new profile running 'pf add PROFILE' or 'clifm -P "
	    "PROFILE'",
	    "Switch profiles using 'pf set PROFILE'",
	    "Delete a profile using 'pf del PROFILE'",
	    "Copy selected files into CWD by just running 'v sel' or "
	    "pressing Ctrl-Alt-v",
	    "Use 'p ELN' to print file properties for ELN",
	    "Deselect all selected files by pressing 'Alt-d'",
	    "Select all files in CWD by pressing 'Alt-a'",
	    "Jump to the Selection Box by pressing 'Alt-s'",
	    "Restore trashed files using the 'u' command",
	    "Empty the trash bin running 't clear'",
	    "Press Alt-f to toggle list-folders-first on/off",
	    "Use the 'fc' command to disable the files counter",
	    "Take a look at the splash screen with the 'splash' command",
	    "Have some fun trying the 'bonus' command",
	    "Launch the default system shell in CWD using ':' or ';'",
	    "Use 'Alt-z' and 'Alt-x' to switch sorting methods",
	    "Reverse sorting order using the 'rev' option: 'st rev'",
	    "Compress and decompress files using the 'ac' and 'ad' "
	    "commands respectivelly",
	    "Rename multiple files at once with the bulk rename function: "
	    "'br *.txt'",
	    "Need no more tips? Disable this feature in the configuration "
	    "file",
	    "Need root privileges? Launch a new instance of CLifM as root "
	    "running the 'X' command",
	    "Create custom commands and features using the 'actions' command",
	    "Create a fresh configuration file by running 'edit gen'",
	    "Use 'ln edit' (or 'le') to edit symbolic links",
	    "Change default keyboard shortcuts by editing the keybindings file (F9)",
	    "Keep in sight previous and next visited directories enabling the "
	    "DirhistMap option in the configuration file",
	    "Leave no traces at all running in stealth mode (-S)",
	    "Pin a file via the 'pin' command and then use it with the "
	    "period keyword (,). Ex: 'pin DIR' and then 'cd ,'",
	    "Switch between color schemes using the 'cs' command",
	    "Use the 'j' command to quickly navigate through visited "
	    "directories",
	    "Switch workspaces by pressing Alt-[1-4]",
	    "Use the 'ws' command to list available workspaces",
	    "Take a look at available plugins using the 'actions' command",
	    "Space is not needed: enter 'p12' instead of 'p 12'",
	    "When searching or selecting files, use the exclamation mark "
	    "to reverse the meaning of a pattern",
	    "Enable the TrashAsRm option to prevent accidental deletions",
	    "Don't like ELN's? Disable them using the -e option",
	    "Use the 'n' command to create multiple files and/or directories",
	    "Customize your prompt by adding prompt commands",
	    "Need git integration? Consult the manpage",
	    NULL};

	size_t tipsn = (sizeof(TIPS) / sizeof(TIPS[0])) - 1;

	if (all) {
		size_t i;
		for (i = 0; i < tipsn; i++)
			printf("%sTIP %zu%s: %s\n", bold, i, df_c, TIPS[i]);

		return;
	}

	srand((unsigned int)time(NULL));
	printf("%sTIP%s: %s\n", bold, df_c, TIPS[rand() % tipsn]);
}

/* Open DIR in a new instance of the program (using TERM, set in the config
 * file, as terminal emulator) */
int
new_instance(char *dir, int sudo)
{
	if (!term) {
		fprintf(stderr, _("%s: Default terminal not set. Use the "
				"configuration file to set one\n"), PROGRAM_NAME);
		return EXIT_FAILURE;
	}

	if (!(flags & GUI)) {
		fprintf(stderr, _("%s: Function only available for graphical "
				"environments\n"), PROGRAM_NAME);
		return EXIT_FAILURE;
	}

	/* Get absolute path of executable name of itself */
#if defined(__linux__)
	char *self = realpath("/proc/self/exe", NULL);

	if (!self) {
#elif defined(__FreeBSD__)
	const int mib[4] = { CTL_KERN, KERN_PROC, KERN_PROC_PATHNAME, -1 };
	char *self = malloc(PATH_MAX);
	size_t len = PATH_MAX;

	if (!self || sysctl(mib, 4, self, &len, NULL, 0) == -1) {
#endif
		fprintf(stderr, "%s: %s\n", PROGRAM_NAME, strerror(errno));
		return EXIT_FAILURE;
	}

	if (!dir) {
		free(self);
		return EXIT_FAILURE;
	}

	char *deq_dir = dequote_str(dir, 0);

	if (!deq_dir) {
		fprintf(stderr, _("%s: %s: Error dequoting file name\n"),
		    PROGRAM_NAME, dir);
		free(self);
		return EXIT_FAILURE;
	}

	struct stat file_attrib;

	if (stat(deq_dir, &file_attrib) == -1) {
		fprintf(stderr, "%s: %s: %s\n", PROGRAM_NAME, deq_dir,
		    strerror(errno));
		free(self);
		free(deq_dir);
		return EXIT_FAILURE;
	}

	if ((file_attrib.st_mode & S_IFMT) != S_IFDIR) {
		fprintf(stderr, _("%s: %s: Not a directory\n"), PROGRAM_NAME, deq_dir);
		free(self);
		free(deq_dir);
		return EXIT_FAILURE;
	}

	char *path_dir = (char *)NULL;

	if (*deq_dir != '/') {
		path_dir = (char *)xnmalloc(strlen(ws[cur_ws].path)
							+ strlen(deq_dir) + 2, sizeof(char));
		sprintf(path_dir, "%s/%s", ws[cur_ws].path, deq_dir);
		free(deq_dir);
	} else
		path_dir = deq_dir;

	/*  char *cmd = (char *)xnmalloc(strlen(term) + strlen(self)
								 + strlen(path_dir) + 13, sizeof(char));
	sprintf(cmd, "%s %s -p \"%s\" &", term, self, path_dir);

	int ret = launch_execle(cmd);
	free(cmd); */

	char **tmp_term = (char **)NULL, **tmp_cmd = (char **)NULL;

	if (strcntchr(term, ' ') != -1) {

		tmp_term = get_substr(term, ' ');

		if (tmp_term) {
			size_t i;

			for (i = 0; tmp_term[i]; i++)
				;

			size_t num = i;
			tmp_cmd = (char **)xrealloc(tmp_cmd, (i + (sudo ? 5 : 4))
													* sizeof(char *));
			for (i = 0; tmp_term[i]; i++) {
				tmp_cmd[i] = savestring(tmp_term[i], strlen(tmp_term[i]));
				free(tmp_term[i]);
			}
			free(tmp_term);

			i = num - 1;

			int plus = 1;

			if (sudo) {
				tmp_cmd[i + plus] = (char *)xnmalloc(strlen(self) + 1,
				    sizeof(char));
				strcpy(tmp_cmd[i + plus], "sudo");
				plus++;
			}

			tmp_cmd[i + plus] = (char *)xnmalloc(strlen(self) + 1, sizeof(char));
			strcpy(tmp_cmd[i + plus++], self);
			tmp_cmd[i + plus] = (char *)xnmalloc(3, sizeof(char));
			strcpy(tmp_cmd[i + plus++], "-p\0");
			tmp_cmd[i + plus] = (char *)xnmalloc(strlen(path_dir) + 1, sizeof(char));
			strcpy(tmp_cmd[i + plus++], path_dir);
			tmp_cmd[i + plus] = (char *)NULL;
		}
	}

	int ret = -1;

	if (tmp_cmd) {
		ret = launch_execve(tmp_cmd, BACKGROUND, E_NOFLAG);

		for (size_t i = 0; tmp_cmd[i]; i++)
			free(tmp_cmd[i]);
		free(tmp_cmd);
	}

	else {
		fprintf(stderr, _("%s: No option specified for '%s'\n"
				"Trying '%s -e %s -p %s'\n"), PROGRAM_NAME, term,
				term, self, ws[cur_ws].path);

		if (sudo) {
			char *cmd[] = {term, "-e", "sudo", self, "-p", path_dir, NULL};
			ret = launch_execve(cmd, BACKGROUND, E_NOFLAG);
		}

		else {
			char *cmd[] = {term, "-e", self, "-p", path_dir, NULL};
			ret = launch_execve(cmd, BACKGROUND, E_NOFLAG);
		}
	}

//	if (*deq_dir != '/')
		free(path_dir);

//	free(deq_dir);
	free(self);

	if (ret != EXIT_SUCCESS)
		fprintf(stderr, _("%s: Error lauching new instance\n"), PROGRAM_NAME);

	return ret;
}

int
alias_import(char *file)
{
	if (xargs.stealth_mode == 1) {
		printf("%s: The alias function is disabled in stealth mode\n",
				PROGRAM_NAME);
		return EXIT_SUCCESS;
	}

	if (!file)
		return EXIT_FAILURE;

	char rfile[PATH_MAX] = "";
	rfile[0] = '\0';

	/*  if (*file == '~' && *(file + 1) == '/') { */

	if (*file == '~') {
		char *file_exp = tilde_expand(file);

		if (!file_exp) {
			fprintf(stderr, "%s: %s: %s\n", PROGRAM_NAME, file, strerror(errno));
			return EXIT_FAILURE;
		}

		realpath(file_exp, rfile);
		free(file_exp);
	}

	else
		realpath(file, rfile);

	if (rfile[0] == '\0') {
		fprintf(stderr, "%s: %s: %s\n", PROGRAM_NAME, file, strerror(errno));
		return EXIT_FAILURE;
	}

	if (access(rfile, F_OK | R_OK) != 0) {
		fprintf(stderr, "%s: %s: %s\n", PROGRAM_NAME, rfile, strerror(errno));
		return EXIT_FAILURE;
	}

	/* Open the file to import aliases from */
	FILE *fp = fopen(rfile, "r");

	if (!fp) {
		fprintf(stderr, "%s: '%s': %s\n", PROGRAM_NAME, rfile, strerror(errno));
		return EXIT_FAILURE;
	}

	/* Open CLiFM config file as well */
	FILE *config_fp = fopen(CONFIG_FILE, "a");

	if (!config_fp) {
		fprintf(stderr, "%s: %s: %s\n", PROGRAM_NAME, CONFIG_FILE,
		    strerror(errno));
		fclose(fp);
		return EXIT_FAILURE;
	}

	size_t line_size = 0, i;
	char *line = (char *)NULL;
	ssize_t line_len = 0;
	size_t alias_found = 0, alias_imported = 0;
	int first = 1;

	while ((line_len = getline(&line, &line_size, fp)) > 0) {

		if (strncmp(line, "alias ", 6) == 0) {
			alias_found++;

			/* If alias name conflicts with some internal command,
			 * skip it */
			char *alias_name = strbtw(line, ' ', '=');

			if (!alias_name)
				continue;

			if (is_internal_c(alias_name)) {
				fprintf(stderr, _("%s: Alias conflicts with "
						"internal command\n"), alias_name);
				free(alias_name);
				continue;
			}

			char *p = line + 6; /* p points now to the beginning of the
			alias name (because "alias " == 6) */

			/* Only accept single quoted aliases commands */
			char *tmp = strchr(p, '=');

			if (!tmp)
				continue;

			if (*(++tmp) != '\'') {
				free(alias_name);
				continue;
			}

			/* If alias already exists, skip it too */
			int exists = 0;

			for (i = 0; i < aliases_n; i++) {
				int alias_len = strcntchr(aliases[i], '=');

				if (alias_len != -1 && strncmp(aliases[i], p,
											(size_t)alias_len + 1) == 0) {
					exists = 1;
					break;
				}
			}

			if (!exists) {
				if (first) {
					first = 0;
					fputs("\n\n", config_fp);
				}

				alias_imported++;

				/* Write the new alias into CLiFM config file */
				fputs(line, config_fp);
			} else
				fprintf(stderr, _("%s: Alias already exists\n"),
				    alias_name);

			free(alias_name);
		}
	}

	free(line);

	fclose(fp);
	fclose(config_fp);

	/* No alias was found in FILE */
	if (alias_found == 0) {
		fprintf(stderr, _("%s: %s: No alias found\n"), PROGRAM_NAME,
		    rfile);
		return EXIT_FAILURE;
	}

	/* Aliases were found in FILE, but none was imported (either because
	 * they conflicted with internal commands or the alias already
	 * existed) */
	else if (alias_imported == 0) {
		fprintf(stderr, _("%s: No alias imported\n"), PROGRAM_NAME);
		return EXIT_FAILURE;
	}

	/* If some alias was found and imported, print the corresponding
	 * message and update the aliases array */
	if (alias_imported > 1)
		printf(_("%s: %zu aliases were successfully imported\n"),
		    PROGRAM_NAME, alias_imported);

	else
		printf(_("%s: 1 alias was successfully imported\n"), PROGRAM_NAME);

	/* Add new aliases to the internal list of aliases */
	get_aliases();

	/* Add new aliases to the commands list for TAB completion */
	if (bin_commands) {

		for (i = 0; bin_commands[i]; i++)
			free(bin_commands[i]);

		free(bin_commands);
		bin_commands = (char **)NULL;
	}

	get_path_programs();

	return EXIT_SUCCESS;
}

/* Store last visited directory for the restore last path and the
 * cd on quit functions */
void
save_last_path(void)
{
	if (!config_ok || !CONFIG_DIR)
		return;

	char *last_dir = (char *)xnmalloc(strlen(CONFIG_DIR) + 7, sizeof(char));
	sprintf(last_dir, "%s/.last", CONFIG_DIR);

	FILE *last_fp;

	last_fp = fopen(last_dir, "w");

	if (!last_fp) {
		fprintf(stderr, _("%s: Error saving last visited "
				  "directory\n"),
		    PROGRAM_NAME);
		free(last_dir);
		return;
	}

	size_t i;
	for (i = 0; i < MAX_WS; i++) {
		if (ws[i].path) {
			/* Mark current workspace with an asterisk. It will
			 * be read at startup by get_last_path */
			if ((size_t)cur_ws == i)
				fprintf(last_fp, "*%zu:%s\n", i, ws[i].path);
			else
				fprintf(last_fp, "%zu:%s\n", i, ws[i].path);
		}
	}

	fclose(last_fp);

	char *last_dir_tmp = xnmalloc(strlen(CONFIG_DIR_GRAL) + 7, sizeof(char *));
	sprintf(last_dir_tmp, "%s/.last", CONFIG_DIR_GRAL);

	if (cd_on_quit) {
		char *cmd[] = {"cp", "-p", last_dir, last_dir_tmp,
		    NULL};

		launch_execve(cmd, FOREGROUND, E_NOFLAG);
	}

	/* If not cd on quit, remove the file */
	else {
		char *cmd[] = {"rm", "-f", last_dir_tmp, NULL};

		launch_execve(cmd, FOREGROUND, E_NOFLAG);
	}

	free(last_dir_tmp);
	free(last_dir);

	return;
}

char *
parse_usrvar_value(const char *str, const char c)
{
	if (c == '\0' || !str)
		return (char *)NULL;

	/* Get whatever comes after c */
	char *tmp = (char *)NULL;
	tmp = strchr(str, c);

	/* If no C or there's nothing after C */
	if (!tmp || !*(++tmp))
		return (char *)NULL;

	/* Remove leading quotes */
	if (*tmp == '"' || *tmp == '\'')
		tmp++;

	/* Remove trailing spaces, tabs, new line chars, and quotes */
	size_t tmp_len = strlen(tmp),
		   i;

	for (i = tmp_len - 1; tmp[i] && i > 0; i--) {

		if (tmp[i] != ' ' && tmp[i] != '\t' && tmp[i] != '"' && tmp[i] != '\''
		&& tmp[i] != '\n')
			break;

		else
			tmp[i] = '\0';
	}

	if (!*tmp)
		return (char *)NULL;

	/* Copy the result string into a buffer and return it */
	char *buf = (char *)NULL;
	buf = savestring(tmp, strlen(tmp));
	tmp = (char *)NULL;

	if (buf)
		return buf;

	return (char *)NULL;
}

int
create_usr_var(char *str)
{
	char *name = strbfr(str, '=');
	char *value = parse_usrvar_value(str, '=');

	if (!name) {

		if (value)
			free(value);

		fprintf(stderr, _("%s: Error getting variable name\n"),
		    PROGRAM_NAME);

		return EXIT_FAILURE;
	}

	if (!value) {
		free(name);

		fprintf(stderr, _("%s: Error getting variable value\n"),
		    PROGRAM_NAME);

		return EXIT_FAILURE;
	}

	usr_var = xrealloc(usr_var, (size_t)(usrvar_n + 1) * sizeof(struct usrvar_t));
	usr_var[usrvar_n].name = savestring(name, strlen(name));
	usr_var[usrvar_n++].value = savestring(value, strlen(value));

	free(name);
	free(value);

	return EXIT_SUCCESS;
}

/* Custom POSIX implementation of GNU asprintf() modified to log program
 * messages. MSG_TYPE is one of: 'e', 'w', 'n', or zero (meaning this
 * latter that no message mark (E, W, or N) will be added to the prompt).
 * PROMPT tells whether to print the message immediately before the
 * prompt or rather in place. Based on littlstar's xasprintf
 * implementation:
 * https://github.com/littlstar/asprintf.c/blob/master/asprintf.c*/
int
_err(int msg_type, int prompt, const char *format, ...)
{
	va_list arglist, tmp_list;
	int size = 0;

	va_start(arglist, format);
	va_copy(tmp_list, arglist);
	size = vsnprintf((char *)NULL, 0, format, tmp_list);
	va_end(tmp_list);

	if (size < 0) {
		va_end(arglist);
		return EXIT_FAILURE;
	}

	char *buf = (char *)xcalloc((size_t)size + 1, sizeof(char));

	vsprintf(buf, format, arglist);
	va_end(arglist);

	/* If the new message is the same as the last message, skip it */
	if (msgs_n && strcmp(messages[msgs_n - 1], buf) == 0) {
		free(buf);
		return EXIT_SUCCESS;
	}

	if (buf) {
		if (msg_type) {
			switch (msg_type) {
			case 'e':
				pmsg = error;
				break;
			case 'w':
				pmsg = warning;
				break;
			case 'n':
				pmsg = notice;
				break;
			default:
				pmsg = nomsg;
			}
		}

		log_msg(buf, (prompt) ? PRINT_PROMPT : NOPRINT_PROMPT);
		free(buf);

		return EXIT_SUCCESS;
	}

	return EXIT_FAILURE;
}

/* Set STR as the program current shell */
int
set_shell(char *str)
{
	if (!str || !*str)
		return EXIT_FAILURE;

	/* IF no slash in STR, check PATH env variable for a file named STR
	 * and get its full path*/
	char *full_path = (char *)NULL;

	if (strcntchr(str, '/') == -1)
		full_path = get_cmd_path(str);

	char *tmp = (char *)NULL;

	if (full_path)
		tmp = full_path;

	else
		tmp = str;

	if (access(tmp, X_OK) == -1) {
		fprintf(stderr, "%s: %s: %s\n", PROGRAM_NAME, tmp, strerror(errno));
		return EXIT_FAILURE;
	}

	if (user.shell)
		free(user.shell);

	user.shell = savestring(tmp, strlen(tmp));
	printf(_("Successfully set '%s' as %s default shell\n"), user.shell,
	    PROGRAM_NAME);

	if (full_path)
		free(full_path);

	return EXIT_SUCCESS;
}

/* List available mountpoints and chdir into one of them */
int
list_mountpoints(void)
{
#if defined(__linux__)
	FILE *mp_fp = fopen("/proc/mounts", "r");

	if (!mp_fp) {
		fprintf(stderr, "%s: mp: fopen: /proc/mounts: %s\n",
				PROGRAM_NAME, strerror(errno));
		return EXIT_FAILURE;
	}
#endif

	printf(_("%sMountpoints%s\n\n"), bold, df_c);

	char **mountpoints = (char **)NULL;
	size_t mp_n = 0;
	int i, exit_status = EXIT_SUCCESS;

#if defined(__linux__)
	size_t line_size = 0;
	char *line = (char *)NULL;
	ssize_t line_len = 0;

	while ((line_len = getline(&line, &line_size, mp_fp)) > 0) {

		/* Do not list all mountpoints, but only those corresponding
		 * to a block device (/dev) */
		if (strncmp(line, "/dev/", 5) == 0) {
			char *str = (char *)NULL;
			size_t counter = 0;

			/* use strtok() to split LINE into tokens using space as
			 * IFS */
			str = strtok(line, " ");
			size_t dev_len = strlen(str);

			char *device = savestring(str, dev_len);
			/* Print only the first two fileds of each /proc/mounts
			 * line */
			while (str && counter < 2) {

				if (counter == 1) { /* 1 == second field */
					printf("%s%zu%s %s%s%s (%s)\n", el_c, mp_n + 1,
					    df_c, (access(str, R_OK | X_OK) == 0) ? di_c : nd_c, str, df_c,
					    device);
					/* Store the second field (mountpoint) into an
					 * array */
					mountpoints = (char **)xrealloc(mountpoints,
					    (mp_n + 1) * sizeof(char *));
					mountpoints[mp_n++] = savestring(str, strlen(str));
				}

				str = strtok(NULL, " ,");
				counter++;
			}

			free(device);
		}
	}

	free(line);
	line = (char *)NULL;
	fclose(mp_fp);

#elif defined(__FreeBSD__)
	struct statfs *fslist;
	mp_n = getmntinfo(&fslist, MNT_NOWAIT);
#endif

	/* This should never happen: There should always be a mountpoint,
	 * at least "/" */
	if (mp_n == 0) {
		fputs(_("mp: There are no available mountpoints\n"), stdout);
		return EXIT_SUCCESS;
	}
#if defined(__FreeBSD__)
	int j;
	for (i = j = 0; i < mp_n; i++) {
		/* Do not list all mountpoints, but only those corresponding
		 * to a block device (/dev) */
		if (strncmp(fslist[i].f_mntfromname, "/dev/", 5) == 0) {
			printf("%s%u%s %s%s%s (%s)\n", el_c, j + 1, df_c,
			    (access(fslist[i].f_mntonname, R_OK | X_OK)
			    == 0) ? di_c : nd_c, fslist[i].f_mntonname,
			    df_c, fslist[i].f_mntfromname);
			/* Store the mountpoint into an array */
			mountpoints = (char **)xrealloc(mountpoints,
			    (j + 1) * sizeof(char *));
			mountpoints[j++] = savestring(fslist[i].f_mntonname,
			    strlen(fslist[i].f_mntonname));
		}
	}
	/* Update filesystem counter as it would be used to free() the
	 * mountpoints entries later (below) */
	mp_n = j;
#endif

	putchar('\n');
	/* Ask the user and chdir into the selected mountpoint */
	char *input = (char *)NULL;

	while (!input)
		input = rl_no_hist(_("Choose a mountpoint ('q' to quit): "));

	if (!(*input == 'q' && *(input + 1) == '\0')) {
		int atoi_num = atoi(input);

		if (atoi_num > 0 && atoi_num <= (int)mp_n) {

			if (xchdir(mountpoints[atoi_num - 1], SET_TITLE) == EXIT_SUCCESS) {

				free(ws[cur_ws].path);
				ws[cur_ws].path = savestring(mountpoints[atoi_num - 1],
				    strlen(mountpoints[atoi_num - 1]));

				if (cd_lists_on_the_fly) {
					free_dirlist();

					if (list_dir() != EXIT_SUCCESS)
						exit_status = EXIT_FAILURE;
				}

				add_to_dirhist(ws[cur_ws].path);
				add_to_jumpdb(ws[cur_ws].path);
			}

			else {
				fprintf(stderr, "%s: %s: %s\n", PROGRAM_NAME,
				    mountpoints[atoi_num - 1], strerror(errno));
				exit_status = EXIT_FAILURE;
			}
		} else {
			fprintf(stderr, "%s: %s: %s\n", PROGRAM_NAME,
			    mountpoints[atoi_num - 1], strerror(errno));
			exit_status = EXIT_FAILURE;
		}
	}

	/* Free stuff and exit */
	free(input);

	i = (int)mp_n;
	while (--i >= 0)
		free(mountpoints[i]);

	free(mountpoints);

	return exit_status;
}

/* Store pinned directory for the next session */
void
save_pinned_dir(void)
{
	if (pinned_dir && config_ok) {

		char *pin_file = (char *)xnmalloc(strlen(CONFIG_DIR) + 7,
		    sizeof(char));
		sprintf(pin_file, "%s/.pin", CONFIG_DIR);

		FILE *fp = fopen(pin_file, "w");

		if (!fp)
			fprintf(stderr, _("%s: Error storing pinned "
					"directory\n"), PROGRAM_NAME);

		else {
			fprintf(fp, "%s", pinned_dir);
			fclose(fp);
		}

		free(pin_file);
	}

	return;
}

/* This function is called by atexit() to clear whatever is there at exit
 * time and avoid thus memory leaks */
void
free_stuff(void)
{
	int i = 0;

	if (STDIN_TMP_DIR) {
		char *rm_cmd[] = {"rm", "-rd", "--", STDIN_TMP_DIR, NULL};
		launch_execve(rm_cmd, FOREGROUND, E_NOFLAG);
		free(STDIN_TMP_DIR);
	}

	if (color_schemes) {
		for (i = 0; color_schemes[i]; i++)
			free(color_schemes[i]);

		free(color_schemes);
	}
	if (xargs.stealth_mode != 1) {
		save_pinned_dir();
		save_jumpdb();
	}

	if (jump_db) {

		i = (int)jump_n;
		while (--i >= 0)
			free(jump_db[i].path);

		free(jump_db);
	}

	if (pinned_dir)
		free(pinned_dir);

	if (filter) {
		regfree(&regex_exp);
		free(filter);
	}

	free_bookmarks();

	if (eln_as_file_n)
		free(eln_as_file);

	save_dirhist();

	if (restore_last_path || cd_on_quit)
		save_last_path();

	if (ext_colors_n)
		free(ext_colors_len);

	if (opener)
		free(opener);

	if (encoded_prompt)
		free(encoded_prompt);

	if (profile_names) {
		for (i = 0; profile_names[i]; i++)
			free(profile_names[i]);
		free(profile_names);
	}

	if (alt_profile)
		free(alt_profile);

	free_dirlist();

	if (sel_n > 0) {
		i = (int)sel_n;
		while (--i >= 0)
			free(sel_elements[i]);
		free(sel_elements);
	}

	if (bin_commands) {
		i = (int)path_progsn;
		while (--i >= 0)
			free(bin_commands[i]);
		free(bin_commands);
	}

	if (paths) {
		i = (int)path_n;
		while (--i >= 0)
			free(paths[i]);

		free(paths);
	}

	if (history) {
		i = (int)current_hist_n;
		while (--i >= 0)
			free(history[i]);

		free(history);
	}

	if (argv_bk) {
		i = argc_bk;
		while (--i >= 0)
			free(argv_bk[i]);

		free(argv_bk);
	}

	if (dirhist_total_index) {
		i = (int)dirhist_total_index;
		while (--i >= 0)
			free(old_pwd[i]);
		free(old_pwd);
	}

	i = (int)kbinds_n;
	while (--i >= 0) {
		free(kbinds[i].function);
		free(kbinds[i].key);
	}
	free(kbinds);

	i = (int)usrvar_n;
	while (--i >= 0) {
		free(usr_var[i].name);
		free(usr_var[i].value);
	}
	free(usr_var);

	i = (int)actions_n;
	while (--i >= 0) {
		free(usr_actions[i].name);
		free(usr_actions[i].value);
	}
	free(usr_actions);

	i = (int)aliases_n;
	while (--i >= 0)
		free(aliases[i]);
	free(aliases);

	i = (int)prompt_cmds_n;
	while (--i >= 0)
		free(prompt_cmds[i]);
	free(prompt_cmds);

	if (flags & FILE_CMD_OK)
		free(file_cmd_path);

	if (msgs_n) {
		i = (int)msgs_n;
		while (--i >= 0)
			free(messages[i]);
		free(messages);
	}

	if (ext_colors_n) {
		i = (int)ext_colors_n;
		while (--i >= 0)
			free(ext_colors[i]);
		free(ext_colors);
	}

	free(user.name);
	free(user.home);
	free(user.shell);

	i = MAX_WS;
	while (--i >= 0)
		if (ws[i].path)
			free(ws[i].path);
	free(ws);

	free(DATA_DIR);
	free(CONFIG_DIR_GRAL);
	free(CONFIG_DIR);
	free(TRASH_DIR);
	free(TRASH_FILES_DIR);
	free(TRASH_INFO_DIR);
	free(TMP_DIR);
	free(BM_FILE);
	free(LOG_FILE);
	free(HIST_FILE);
	free(DIRHIST_FILE);
	free(CONFIG_FILE);
	free(PROFILE_FILE);
	free(MSG_LOG_FILE);
	free(SEL_FILE);
	free(MIME_FILE);
	free(PLUGINS_DIR);
	free(ACTIONS_FILE);
	free(KBINDS_FILE);
	free(COLORS_DIR);

	/* Restore the color of the running terminal */
	fputs("\x1b[0;39;49m", stdout);
}

void
set_signals_to_ignore(void)
{
	/*  signal(SIGINT, signal_handler); C-c */
	signal(SIGINT, SIG_IGN);  /* C-c */
	signal(SIGQUIT, SIG_IGN); /* C-\ */
	signal(SIGTSTP, SIG_IGN); /* C-z */
	/*  signal(SIGTERM, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN); */
}

void
handle_stdin()
{
	/* If files are passed via stdin, we need to disable restore
	 * last path in order to correctly understand relative paths */
	restore_last_path = 0;

	/* Max input size: 512 * (512 * 1024)
	 * 512 chunks of 524288 bytes (512KiB) each
	 * == (65535 * PATH_MAX)
	 * == 262MiB of data ((65535 * PATH_MAX) / 1024) */

	size_t chunk = 512 * 1024,
		   chunks_n = 1,
		   total_len = 0,
		   max_chunks = 512;

	ssize_t input_len = 0;

	/* Initial buffer allocation == 1 chunk */
	char *buf = (char *)xnmalloc(chunk, sizeof(char));

	while (chunks_n < max_chunks) {
		input_len = read(STDIN_FILENO, buf + total_len, chunk);

		/* Error */
		if (input_len < 0) {
			free(buf);
			return;
		}

		/* Nothing else to be read */
		if (input_len == 0)
			break;

		total_len += input_len;
		chunks_n++;

		/* Append a new chunk of memory to the buffer */
		buf = (char *)xrealloc(buf, (chunks_n + 1) * chunk);
	}

	if (total_len == 0)
		goto FREE_N_EXIT;

	/* Null terminate the input buffer */
	buf[total_len] = '\0';

	/* Create tmp dir to store links to files */
	char *rand_ext = gen_rand_str(6);
	if (!rand_ext)
		goto FREE_N_EXIT;

	if (TMP_DIR) {
		STDIN_TMP_DIR = (char *)xnmalloc(strlen(TMP_DIR) + 14, sizeof(char));
		sprintf(STDIN_TMP_DIR, "%s/clifm.%s", TMP_DIR, rand_ext);
	}

	else {
		STDIN_TMP_DIR = (char *)xnmalloc(18, sizeof(char));
		sprintf(STDIN_TMP_DIR, "/tmp/clifm.%s", rand_ext);
	}

	free(rand_ext);

	char *cmd[] = {"mkdir", "-p", STDIN_TMP_DIR, NULL};
	if (launch_execve(cmd, FOREGROUND, E_NOFLAG) != EXIT_SUCCESS)
		goto FREE_N_EXIT;

	/* Get CWD: we need it to preppend it to relative paths */
	char *cwd = (char *)NULL;
	cwd = getcwd(NULL, 0);
	if (!cwd)
		goto FREE_N_EXIT;

	/* Get substrings from buf */
	char *p = buf, *q = buf;

	while (*p) {

		if (!*p || *p == '\n') {
			*p = '\0';

			/* Create symlinks (in tmp dir) to each valid file in
			 * the buffer */

			/* If file does not exist */
			struct stat attr;
			if (lstat(q, &attr) == -1)
				continue;

			/* Construct source and destiny files */
			char *tmp_file = strrchr(q, '/');

			if (!tmp_file || !*(++tmp_file))
				tmp_file = q;

			char source[PATH_MAX + 1];

			if (*q != '/' || !q[1])
				snprintf(source, PATH_MAX, "%s/%s", cwd, q);

			else
				strncpy(source, q, PATH_MAX);

			char dest[PATH_MAX + 1];
			sprintf(dest, "%s/%s", STDIN_TMP_DIR, tmp_file);

			if (symlink(source, dest) == -1)
				_err('w', PRINT_PROMPT, "ln: '%s': %s\n", q, strerror(errno));

			q = p + 1;
		}

		p++;
	}

	/* chdir to tmp dir and update path var */
	if (xchdir(STDIN_TMP_DIR, SET_TITLE) == -1) {
		fprintf(stderr, "%s: %s: %s\n", PROGRAM_NAME, STDIN_TMP_DIR,
		    strerror(errno));

		char *rm_cmd[] = {"rm", "-drf", STDIN_TMP_DIR, NULL};
		launch_execve(rm_cmd, FOREGROUND, E_NOFLAG);

		free(cwd);
		goto FREE_N_EXIT;
	}

	free(cwd);

	if (ws[cur_ws].path)
		free(ws[cur_ws].path);

	ws[cur_ws].path = savestring(STDIN_TMP_DIR, strlen(STDIN_TMP_DIR));

	goto FREE_N_EXIT;

FREE_N_EXIT:
	free(buf);

	/* Go back to tty */
	dup2(STDOUT_FILENO, STDIN_FILENO);

	if (cd_lists_on_the_fly) {
		free_dirlist();
		list_dir();
		add_to_dirhist(ws[cur_ws].path);
	}

	return;
}

int
pin_directory(char *dir)
{
	if (!dir || !*dir)
		return EXIT_FAILURE;

	struct stat attr;

	if (lstat(dir, &attr) == -1) {
		fprintf(stderr, "%s: %s: %s\n", PROGRAM_NAME, dir, strerror(errno));
		return EXIT_FAILURE;
	}

	if (pinned_dir) {
		free(pinned_dir);
		pinned_dir = (char *)NULL;
	}

	/* If absolute path */
	if (*dir == '/')
		pinned_dir = savestring(dir, strlen(dir));

	else { /* If relative path */

		if (strcmp(ws[cur_ws].path, "/") == 0) {
			pinned_dir = (char *)xnmalloc(strlen(dir) + 2, sizeof(char));
			sprintf(pinned_dir, "/%s", dir);
		}

		else {
			pinned_dir = (char *)xnmalloc(strlen(dir)
								+ strlen(ws[cur_ws].path) + 2, sizeof(char));
			sprintf(pinned_dir, "%s/%s", ws[cur_ws].path, dir);
		}
	}

	printf(_("%s: Succesfully pinned '%s'\n"), PROGRAM_NAME, dir);

	return EXIT_SUCCESS;
}

int
unpin_dir(void)
{
	if (!pinned_dir) {
		printf(_("%s: No pinned file\n"), PROGRAM_NAME);
		return EXIT_SUCCESS;
	}

	if (CONFIG_DIR && xargs.stealth_mode != 1) {

		int cmd_error = 0;
		char *pin_file = (char *)xnmalloc(strlen(CONFIG_DIR) + 7, sizeof(char));
		sprintf(pin_file, "%s/.pin", CONFIG_DIR);

		if (unlink(pin_file) == -1) {
			fprintf(stderr, "%s: %s: %s\n", PROGRAM_NAME, pin_file,
			    strerror(errno));
			cmd_error = 1;
		}

		free(pin_file);

		if (cmd_error)
			return EXIT_FAILURE;
	}

	printf(_("Succesfully unpinned %s\n"), pinned_dir);

	free(pinned_dir);
	pinned_dir = (char *)NULL;

	return EXIT_SUCCESS;
}

int
hidden_function(char **comm)
{
	int exit_status = EXIT_SUCCESS;

	if (strcmp(comm[1], "status") == 0)
		printf(_("%s: Hidden files %s\n"), PROGRAM_NAME,
		    (show_hidden) ? _("enabled") : _("disabled"));

	else if (strcmp(comm[1], "off") == 0) {
		if (show_hidden == 1) {
			show_hidden = 0;

			if (cd_lists_on_the_fly) {
				free_dirlist();
				exit_status = list_dir();
			}
		}
	}

	else if (strcmp(comm[1], "on") == 0) {
		if (show_hidden == 0) {
			show_hidden = 1;

			if (cd_lists_on_the_fly) {
				free_dirlist();
				exit_status = list_dir();
			}
		}
	}

	else
		fputs(_("Usage: hidden, hf [on, off, status]\n"), stderr);

	return exit_status;
}

/* Instead of recreating here the commands description, just jump to the
 * corresponding section in the manpage */
int
list_commands(void)
{
	char *cmd[] = {"man", "-P", "less -p ^[0-9]+\\.[[:space:]]COMMANDS",
					PNL, NULL};
	if (launch_execve(cmd, FOREGROUND, E_NOFLAG) != EXIT_SUCCESS)
		return EXIT_FAILURE;

	return EXIT_SUCCESS;
}

void
help_function(void)
{
	printf(_("%s %s (%s), by %s\n"), PROGRAM_NAME, VERSION, DATE, AUTHOR);

	printf(_("\nUSAGE: %s %s\n\
\n -a, --no-hidden\t\t do not show hidden files (default)\
\n -A, --show-hidden\t\t show hidden files\
\n -b, --bookmarks-file=FILE\t specify an alternative bookmarks file\
\n -c, --config-file=FILE\t\t specify an alternative configuration file\
\n -D, --config-dir=DIR\t\t specify an alternative configuration directory\
\n -e, --no-eln\t\t\t do not print ELN (entry list number) at \
\n              the left of each file name \
\n -f, --no-folders-first\t\t do not list folders first\
\n -F, --folders-first\t\t list folders first (default)\
\n -g, --pager\t\t\t enable the pager\
\n -G, --no-pager\t\t\t disable the pager (default)\
\n -h, --help\t\t\t show this help and exit\
\n -i, --no-case-sensitive\t no case-sensitive files listing (default)\
\n -I, --case-sensitive\t\t case-sensitive files listing\
\n -k, --keybindings-file=FILE\t specify an alternative keybindings file\
\n -l, --no-long-view\t\t disable long view mode (default)\
\n -L, --long-view\t\t enable long view mode\
\n -m, --dihist-map\t\t enable the directory history map\
\n -o, --no-list-on-the-fly\t 'cd' works as the shell 'cd' command\
\n -O, --list-on-the-fly\t\t 'cd' lists files on the fly (default)\
\n -p, --path=PATH\t\t (Deprecated: use positional parameters instead)\
\n              use PATH as %s starting path\
\n -P, --profile=PROFILE\t\t use (or create) PROFILE as profile\
\n -s, --splash \t\t\t enable the splash screen\
\n -S, --stealth-mode \t\t leave no trace on the host system.\
\n              Nothing is read from any file nor any file \
\n              is created: all settings are set to the \
\n              default value. However, most settings can \
\n              be controlled via command line options\
\n -u, --no-unicode \t\t disable unicode\
\n -U, --unicode \t\t\t enable unicode to correctly list file names \
\n              containing accents, tildes, umlauts, \
\n              non-latin letters, etc. This option is \
\n              enabled by default\
\n -v, --version\t\t\t show version details and exit\
\n -w, --workspace=NUM\t\t start in workspace NUM\
\n -x, --no-ext-cmds\t\t\t Disallow the use of external commands\
\n -y, --light-mode\t\t enable the light mode\
\n -z, --sort=METHOD\t\t sort files by METHOD, where METHOD \
\n              could be: 0 = none, 1 = name, 2 = size, \
\n              3 = atime, 4 = btime, 5 = ctime, \
\n              6 = mtime, 7 = version, 8 = extension, \
\n              9 = inode, 10 = owner, 11 = group"),
	    PNL, GRAL_USAGE, PROGRAM_NAME);

	printf("\
\n     --case-sens-dirjump\t do not ignore case when consulting the \
\n              jump database (via the 'j' command)\
\n     --case-sens-path-comp\t enable case sensitive path completion\
\n     --cd-on-quit\t\t write last visited path to \
\n              $XDG_CONFIG_HOME/clifm/.last to be accessed\
\n              later by a shell funtion. See the manpage\
\n     --color-scheme=NAME\t use color scheme NAME\
\n     --cwd-in-title\t\t print current directory in terminal \
\n              window title\
\n     --disk-usage\t\t show disk usage (free/total) for the\
\n              filesystem to which the current directory \
\n              belongs\
\n     --enable-logs\t\t enable program logs\
\n     --expand-bookmarks\t\t expand bookmark names into the \
\n              corresponding bookmark paths. TAB \
\n              completion for bookmark names is also \
\n              available\
\n     --icons\t\t\t enable icons\
\n     --icons-use-file-color\t icons color follows file color\
\n     --list-and-quit\t\t list files and quit. It may be used\
\n              in conjunction with -p\
\n     --max-dirhist\t\t maximum number of visited directories to \
\n              remember\
\n     --max-files=NUM\t\t list only up to NUM files\
\n     --max-path=NUM\t\t set the maximun number of characters \
\n              after which the current directory in the \
\n              prompt line will be abreviated to the \
\n              directory base name (if \\z is used in \
\n              the prompt\
\n     --no-dir-jumper\t\t disable the directory jumper function\
\n     --no-cd-auto\t\t by default, %s changes to directories \
\n\t\t\t\tby just specifying the corresponding ELN \
\n              (e.g. '12' instead of 'cd 12'). This \
\n              option forces the use of 'cd'\
\n     --no-classify\t\tDo not append file type indicators\
\n     --no-clear-screen\t\t do not clear the screen when listing \
\n              directories\
\n     --no-colors\t\t disable file type colors for files listing \
\n     --no-columns\t\t disable columned files listing\
\n     --no-files-counter\t\t disable the files counter for \
\n              directories. This option is especially \
\n              useful to speed up the listing process; \
\n              counting files in directories is expensive\
\n     --no-open-auto\t\t same as no-cd-auto, but for files\
\n     --no-tips\t\t\t disable startup tips\
\n     --no-welcome-message\t disable the welcome message\
\n     --only-dirs\t\t list only directories and symbolic links\
\n              to directories\
\n     --open=FILE\t run as a stand-alone resource opener: open\
\n              FILE and exit\
\n     --opener=APPLICATION\t resource opener to use instead of 'lira',\
\n              %s built-in opener\
\n     --print-sel\t\t keep the list of selected files in sight\
\n     --restore-last-path\t save last visited directory to be \
\n              restored in the next session\
\n     --rl-vi-mode\t\t set readline to vi editing mode (defaults \
\n              to emacs editing mode)\
\n     --share-selbox\t\t make the Selection Box common to \
\n              different profiles\
\n     --sort-reverse\t\t sort in reverse order, for example: z-a \
\n              instead of a-z, which is the default order)\
\n     --trash-as-rm\t\t the 'r' command executes 'trash' instead of \
				'rm' to prevent accidental deletions\n",
	    PROGRAM_NAME, PROGRAM_NAME);

	printf(_("\nBUILT-IN COMMANDS:\n\nThe following is just a brief list of "
			"available commands and possible parameters.\n\nFor a complete "
			"description of each of these commands run '%scmd%s' (or press "
			"%sF2%s) or consult the %smanpage%s (%sF1%s).\n\nYou can also try "
			"the '%sih%s' action to run the interactive help plugin (it "
			"depends on FZF). Just enter 'ih', that's it.\n\nIt is also "
			"recommended to consult the project's %swiki%s "
			"(https://github.com/leo-arch/clifm/wiki)\n\n"),
			bold, df_c, bold, df_c, bold, df_c, bold,
			df_c, bold, df_c, bold, df_c);

	puts(_("ELN/FILE/DIR (auto-open and autocd functions)\n\
 /PATTERN [DIR] [-filetype] [-x] (quick search)\n\
 ;[CMD], :[CMD] (run CMD via the system shell)\n\
 ac, ad ELN/FILE ... (archiving functions)\n\
 acd, autocd [on, off, status]\n\
 actions [edit]\n\
 alias [import FILE]\n\
 ao, auto-open [on, off, status]\n\
 b, back [h, hist] [clear] [!ELN]\n\
 bl ELN/FILE ... (batch links)\n\
 bm, bookmarks [a, add PATH] [d, del] [edit] [SHORTCUT or NAME]\n\
 br, bulk ELN/FILE ...\n\
 c, l [e, edit], m, md, r (copy, link, move, makedir, and remove)\n\
 cc, colors\n\
 cd [ELN/DIR]\n\
 cl, columns [on, off]\n\
 cmd, commands\n\
 cs, colorscheme [edit] [COLORSCHEME]\n\
 d, dup SOURCE [DEST]\n\
 ds, desel [*, a, all]\n\
 edit [APPLICATION]\n\
 exp, export [ELN/FILE ...]\n\
 ext [on, off, status]\n\
 f, forth [h, hist] [clear] [!ELN]\n\
 fc, filescounter [on, off, status]\n\
 ff, folders-first [on, off, status]\n\
 fs\n\
 ft, filter [unset] [REGEX]\n\
 hf, hidden [on, off, status]\n\
 history [clear] [-n]\n\
 icons [on, off]\n\
 j, jc, jp, jl [STRING ...] jo [NUM], je (directory jumper function)\n\
 kb, keybinds [edit] [reset]\n\
 lm [on, off] (lightmode)\n\
 log [clear]\n\
 mf NUM (List up to NUM files)\n\
 mm, mime [info ELN/FILE] [edit] [import] (resource opener)\n\
 mp, mountpoints\n\
 msg, messages [clear]\n\
 n, new FILE DIR/ ...n\n\
 net [smb, ftp, sftp]://ADDRESS [OPTIONS]\n\
 o, open [ELN/FILE] [APPLICATION]\n\
 opener [default] [APPLICATION]\n\
 p, pr, pp, prop [ELN/FILE ... n]\n\
 path, cwd\n\
 pf, prof, profile [ls, list] [set, add, del PROFILE]\n\
 pg, pager [on, off, status]\n\
 pin [FILE/DIR]\n\
 q, quit, exit\n\
 Q\n\
 rf, refresh\n\
 rl, reload\n\
 s, sel ELN/FILE... [[!]PATTERN] [-FILETYPE] [:PATH]\n\
 sb, selbox\n\
 shell [SHELL]\n\
 splash\n\
 st, sort [METHOD] [rev]\n\
 t, tr, trash [ELN/FILE ... n] [ls, list] [clear] [del, rm]\n\
 te [FILE(s)]\n\
 tips\n\
 u, undel, untrash [*, a, all]\n\
 uc, unicode [on, off, status]\n\
 unpin\n\
 v, vv, paste sel [DESTINY]\n\
 ver, version\n\
 ws [NUM, +, -] (workspaces)\n\
 x, X [ELN/DIR] (new instance)\n"));

	printf(_("DEFAULT KEYBOARD SHORTCUTS:\n\n"
		 " M-c: Clear the current command line buffer\n\
 M-f: Toggle list-folders-first on/off\n\
 C-r: Refresh the screen\n\
 M-l: Toggle long view mode on/off\n\
 M-m: List mountpoints\n\
 M-t: Clear messages\n\
 M-h: Show directory history\n\
 M-i, M-.: Toggle hidden files on/off\n\
 M-s: Open the Selection Box\n\
 M-a: Select all files in the current working directory\n\
 M-d: Deselect all selected files\n\
 M-r: Change to the root directory\n\
 M-e, Home: Change to the home directory\n\
 M-u, S-Up: Change to the parent directory\n\
 M-j, S-Left: Change to previous visited directory\n\
 M-k, S-Right: Change to next visited directory\n\
 M-o: Lock terminal\n\
 M-p: Change to pinned directory\n\
 M-1: Switch to workspace 1\n\
 M-2: Switch to workspace 2\n\
 M-3: Switch to workspace 3\n\
 M-4: Switch to workspace 4\n\
 C-M-j: Change to first visited directory\n\
 C-M-k: Change to last visited directory\n\
 C-M-o: Switch to previous profile\n\
 C-M-p: Switch to next profile\n\
 C-M-a: Archive selected files\n\
 C-M-e: Export selected files\n\
 C-M-r: Rename selected files\n\
 C-M-d: Remove selected files\n\
 C-M-t: Trash selected files\n\
 C-M-u: Restore trashed files\n\
 C-M-b: Bookmark last selected file or directory\n\
 C-M-g: Open/change-into last selected file/directory\n\
 C-M-n: Move selected files into the current working directory\n\
 C-M-v: Copy selected files into the current working directory\n\
 M-y: Toggle light mode on/off\n\
 M-z: Switch to previous sorting method\n\
 M-x: Switch to next sorting method\n\
 C-x: Launch a new instance\n\
 F1: Manual page\n\
 F2: Commands help\n\
 F3: Keybindings help\n\
 F6: Open the MIME list file\n\
 F7: Open the jump database file\n\
 F8: Open the current color scheme file\n\
 F9: Open the keybindings file\n\
 F10: Open the configuration file\n\
 F11: Open the bookmarks file\n\
 F12: Quit\n\n"
		 "NOTE: C stands for Ctrl, S for Shift, and M for Meta (Alt key in "
		 "most keyboards)\n\n"));

	puts(_("Run the 'colors' or 'cc' command to see the list "
	       "of currently used color codes.\n"));

	puts(_("The configuration and profile files allow you to customize "
	       "colors, define some prompt commands and aliases, and more. "
	       "For a full description consult the manpage."));
}

void
free_software(void)
{
	puts(_("Excerpt from 'What is Free Software?', by Richard Stallman. \
Source: https://www.gnu.org/philosophy/free-sw.html\n \
\n\"'Free software' means software that respects users' freedom and \
community. Roughly, it means that the users have the freedom to run, \
copy, distribute, study, change and improve the software. Thus, 'free \
software' is a matter of liberty, not price. To understand the concept, \
you should think of 'free' as in 'free speech', not as in 'free beer'. \
We sometimes call it 'libre software', borrowing the French or Spanish \
word for 'free' as in freedom, to show we do not mean the software is \
gratis.\n\
\nWe campaign for these freedoms because everyone deserves them. With \
these freedoms, the users (both individually and collectively) control \
the program and what it does for them. When users don't control the \
program, we call it a 'nonfree' or proprietary program. The nonfree \
program controls the users, and the developer controls the program; \
this makes the program an instrument of unjust power. \n\
\nA program is free software if the program's users have the four \
essential freedoms:\n\n\
- The freedom to run the program as you wish, for any purpose \
(freedom 0).\n\
- The freedom to study how the program works, and change it so it does \
your computing as you wish (freedom 1). Access to the source code is a \
precondition for this.\n\
- The freedom to redistribute copies so you can help your neighbor \
(freedom 2).\n\
- The freedom to distribute copies of your modified versions to others \
(freedom 3). By doing this you can give the whole community a chance to \
benefit from your changes. Access to the source code is a precondition \
for this. \n\
\nA program is free software if it gives users adequately all of these \
freedoms. Otherwise, it is nonfree. While we can distinguish various \
nonfree distribution schemes in terms of how far they fall short of \
being free, we consider them all equally unethical (...)\""));
}

void
version_function(void)
{
	printf(_("%s %s (%s), by %s\nContact: %s\nWebsite: "
		 "%s\nLicense: %s\n"),
	    PROGRAM_NAME, VERSION, DATE,
	    AUTHOR, CONTACT, WEBSITE, LICENSE);
}

void
splash(void)
{
	printf("\n%s"
"     .okkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkd. \n"
"    'kkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkc\n"
"    xkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkk\n"
"    xkkkkkc::::::::::::::::::dkkkkkkc:::::kkkkkk\n"
"    xkkkkk'..................okkkkkk'.....kkkkkk\n"
"    xkkkkk'..................okkkkkk'.....kkkkkk\n"
"    xkkkkk'.....okkkkkk,.....okkkkkk'.....kkkkkk\n"
"    xkkkkk'.....dkkkkkk;.....okkkkkk'.....kkkkkk\n"
"    xkkkkk'.....dkkkkkk;.....okkkkkk'.....kkkkkk\n"
"    xkkkkk'.....dkkkkkk;.....okkkkkk'.....kkkkkk\n"
"    xkkkkk'.....dkkkkkk;.....okkkkkk'.....kkkkkk\n"
"    xkkkkk'.....dkkkkkk;.....okkkkkk'.....kkkkkk\n"
"    xkkkkk'.....dkkkkkk;.....okkkkkk'.....kkkkkk\n"
"    xkkkkk'.....dkkkkkk;.....okkkkkk'.....kkkkkk\n"
"    xkkkkk'.....dkkkkkk;.....okkkkkk'.....kkkkkk\n"
"    xkkkkk'.....coooooo'.....:llllll......kkkkkk\n"
"    xkkkkk'...............................kkkkkk\n"
"    xkkkkk'...............................kkkkkk\n"
"    xkkkkklccccccccccccccccccccccccccccccckkkkkk\n"
"    lkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkx\n"
"     ;kkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkc \n"
"        :c::::::::::::::::::::::::::::::::::.",
	d_cyan);

	printf(_("\n\n%s\t\t       CliFM\n\t%sThe anti-eye-candy/KISS file manager\n%s"),
	    white, "\x1b[0m", df_c);

	if (splash_screen) {
		printf(_("\n\t\t\tPress any key to continue... "));
		xgetchar();
		putchar('\n');
	} else
		putchar('\n');
}

void
bonus_function(void)
{
	char *phrases[] = {
	    "\"Vamos Boca Juniors Carajo!\" (La mitad + 1)",
	    "\"Hey! Look behind you! A three-headed monkey! (G. Threepweed)",
	    "\"Free as in free speech, not as in free beer\" (R. M. S)",
	    "\"Nothing great has been made in the world without passion\" (G. W. F. Hegel)",
	    "\"Simplicity is the ultimate sophistication\" (Leo Da Vinci)",
	    "\"Yo vendí semillas de alambre de púa, al contado, y me lo agradecieron\" (Marquitos, 9 Reinas)",
	    "\"I'm so happy, because today I've found my friends, they're in my head\" (K. D. Cobain)",
	    "\"The best code is written with the delete key (Someone, somewhere, sometime)",
	    "\"I'm selling these fine leather jackets (Indy)",
	    "\"I pray to God to make me free of God\" (Meister Eckhart)",
	    "¡Truco y quiero retruco mierda!",
	    "The only truth is that there is no truth",
	    "\"This is a lie\" (The liar paradox)",
	    "\"There are two ways to write error-free programs; only the third one works\" (Alan J. Perlis)",
	    "The man who sold the world was later sold by the big G",
	    "A programmer is always one year older than herself",
	    "A smartphone is anything but smart",
	    "And he did it: he killed the one who killed him",
	    ">++('>",
	    ":(){:|:&};:",
	    "Keep it simple, stupid",
	    "If ain't broken, brake it",
	    "An Archer knows her target like the back of her hands",
	    "\"I only know that I know nothing\" (Socrates)",
	    "(Learned) Ignorance is the true outcome of wisdom (Nicholas "
	    "of Cusa)",
	    "True intelligence is about questions, not about answers",
	    "Humanity is just an arrow released towards God",
	    "Buzz is right: infinity is our only and ultimate goal",
	    "That stain will never ever be erased (La 12)",
	    "\"A work of art is never finished, but adandoned\" (J. L. Guerrero)",
	    "At the beginning, software was hardware; but today hardware is "
	    "being absorbed by software",
	    NULL};

	size_t num = (sizeof(phrases) / sizeof(phrases[0])) - 1;

	srand((unsigned int)time(NULL));
	puts(phrases[rand() % num]);
}
