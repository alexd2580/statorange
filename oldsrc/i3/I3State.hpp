void add_workspace(std::string const& output_name, uint8_t workspace_num, std::string const& workspace_name);

void set_command_socket(int com_socket);

// Methods for acquiring the initial state.
void get_outputs(void);
void get_workspaces(void);

// Methods for operating the windows' state.
void new_window(uint64_t id, std::string const&);
void rename_window(uint64_t id, std::string const&);
void focus_window(uint64_t id);
void close_window(uint64_t id);

// Methods for operating on the workspaces' state.
void init_workspace(uint8_t, std::string const&);
void focus_workspace(uint8_t);
void urgent_workspace(uint8_t, bool);
void empty_workspace(uint8_t);
