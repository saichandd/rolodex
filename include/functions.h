void write_rolo(FILE *fp1, FILE *fp2);
void clearinit();
void clear_the_screen();
int read_rolodex(int fd);
void print_short();
void print_people(char **names);
int interactive_rolo ();

void cathelpfile (char *filepath, char *helptopic,int clear);
int rolo_menu_yes_no (char *prompt, int rtn_default, int help_allowed, char *helpfile, char *subject);
void user_eof();
void any_char_to_continue();

int rlength();
void rolo_reorder();
int nocase_compare(char *s1,int l1,char *s2,int l2);


void display_entry_for_update();

void display_entry ();
void display_field_names();

void summarize_entry_list ();
int rolo_menu_number_help_or_abort (char *prompt, int low,int high,int *ptr_ival);
int rolo_menu_data_help_or_abort (char *prompt, char *helpfile, char *subject, char **ptr_response);
void any_char_to_continue();
void save_and_exit (int rval);

void rolo_insert ();
void rolo_update_mode ();
void rolo_delete ();


int menu_match (
  int *ptr_rval,
  char **ptr_ur,
  char *prompt,
  int casetest,
  int isub,
  int r_no_match,
  int r_ambiguous,
  int n_options, ...);

void roloexit (int rval);
void rolo_search_mode (int field_index,char *field_name,char *search_string);
void rolo_add ();
int select_field_to_search_by (int *ptr_index, char **ptr_name);

void rolo_peruse_mode();
void save_to_disk ();

int entry_action ();
void display_list_of_entries();
