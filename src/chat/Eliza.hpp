
// chris busch (c) copyright 1995 all rights reserved
#pragma once

#include "Database.hpp"
#include <cstring>
#include <unordered_map>

constexpr inline auto MaxDatabases = 100;
constexpr inline auto MaxNamedDatabases = 200;
constexpr inline auto MaxInputLength = 100;

namespace chat {

class Eliza {
public:
    Eliza() : compile_time_{__DATE__ " " __TIME__} { register_database_names("default", 0); }

    bool load_databases(const char *, char recurflag = 0);
    /**
     * Handle a message from a player to a talkative NPC. This works
     * by accessing the database associated with the npc_name then looking up
     * within that db an appropriate "keyword response map" based on what the player said/did,
     * then pseudo-randomly selecting a response from the responses and sending it back to the player.
     * It also expands $variables found in the response.
     */
    const char *handle_player_message(char *response_buf, const char *player_name, std::string_view message,
                                      std::string_view npc_name);

private:
    int num_databases_{};
    unsigned int num_names_{};
    Database databases_[MaxDatabases];
    std::unordered_map<std::string, int> named_databases_;
    const std::string compile_time_;

    int get_word(const char *&input, char *outword, char &outother);
    char *trim(char str[]);
    bool eval_operator(const char op, const int a, const int b);
    int strpos(std::string_view input_msg, std::string_view current_db_keyword);
    int match(std::string_view db_keywords, std::string_view input_msg, std::string_view::iterator &it,
              uint &remaining_input_pos);
    void handle_operator(std::string_view input_msg, std::string_view current_db_keyword, const char logical_operator,
                         int &progressive_match_result, int &next_match_pos, uint &remaining_input_pos);

    void register_database_names(std::string_view names, int dbnum);
    int get_database_num_by_exact_name(std::string name);
    int get_database_num_by_partial_name(std::string names);
    void expand_variables(char *response_buf, std::string_view npc_name, const std::string &response,
                          const char *player_name, char *rest);
    char *swap_term(char *in);
    void swap_pronouns_and_possessives(char s[]);
};

}
