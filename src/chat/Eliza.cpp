
// Christopher Busch
//(c) 1993 all rights reserved.
// eliza.cpp

#include <cctype>
#include <cstdio>
#include <cstring>
#include <search.h>

#include "../string_utils.hpp"
#include "Database.hpp"
#include "Eliza.hpp"
#include "chatconstants.hpp"

using namespace chat;

/////YOU MAY NOT change the next 2 lines.
const char eliza_title[] = "chat by Christopher Busch  Copyright (c)1993";
const char eliza_version[] = "version 1.0.0";

// Gets a word out of a string.
int Eliza::get_word(const char *&input, char *outword, char &outother) {
    outword[0] = 0;
    char *outword0 = outword;
    int curchar;
    curchar = input[0];

    while (isalnum(curchar) || curchar == '_') {
        *outword = curchar;
        outword++;
        input++;
        curchar = *input;
    }
    if (*input != '\0')
        input++;
    *outword = '\0';
    outother = curchar;
    if (curchar == 0 && outword0[0] == 0)
        return 0;
    return 1;
}
/**
 * Find the number of a database using its exact name
 * as declared in the database file. Returns -1
 * if no match was found. This is used when bootstrapping
 * the chain of databases. Database links defined usng @
 * must refer to a database using its full database name.
 */
int Eliza::get_database_num_by_exact_name(std::string name) {
    auto entry = named_databases_.find(name);
    if (entry != named_databases_.end())
        return entry->second;
    return -1;
}
/**
 * Find the number of a database from a given name by splitting
 * it on spaces and looking for each word.
 * If no database is found it returns 0 which is the database containing
 * all the default keyword responses.
 */
int Eliza::get_database_num_by_partial_name(std::string names) {
    std::string::size_type pos = 0, last = 0;
    while ((pos = names.find(" ", last)) != std::string::npos) {
        std::string name(lower_case(names.substr(last, pos - last)));
        auto entry = named_databases_.find(name);
        if (entry != named_databases_.end())
            return entry->second;
        last = ++pos;
    }
    std::string name(lower_case(names.substr(last, names.size() - pos)));
    auto entry = named_databases_.find(names);
    if (entry != named_databases_.end())
        return entry->second;
    return 0;
}

/**
 * Registers all of the words in 'names' with the specified database number,
 * as well as the full name. The full name is used when resolving
 * linked databases using @ in the data file.
 */
void Eliza::register_database_names(std::string_view names, int dbnum) {
    // First store the full database name. Multiple words may be separated by space.
    std::string fullname = lower_case(std::string(names));
    named_databases_[fullname] = dbnum;
    // Next split on _ and register each word to the same db number.
    // The individual words registered are used when searching for a database
    // by words in an NPC name.
    std::string::size_type pos = 0, last = 0;
    while ((pos = names.find(" ", pos)) != std::string::npos) {
        std::string name(lower_case(names.substr(last, pos - last)));
        named_databases_[name] = dbnum;
        last = ++pos;
    }
    std::string name(lower_case(names.substr(last, names.size() - pos)));
    if (!name.empty())
        named_databases_[name] = dbnum;
}

char *Eliza::swap_term(char *in) {
    static const char pairs[][10] = {"am", "are", "I", "you", "mine", "yours", "my", "your", "me", "you", "myself",
                                     "yourself",
                                     // swapped order:
                                     "you", "I", "yours", "mine", "your", "my", "yourself", "myself", "", ""};

    for (int i = 0; pairs[i * 2][0] != '\0'; i++) {
        if (strcasecmp(in, pairs[i * 2]) == 0) {
            return (char *)pairs[i * 2 + 1];
        }
    }
    return in;
}

void Eliza::swap_pronouns_and_possessives(char s[]) {
    char buf[MaxInputLength + 20];
    buf[0] = '\0';

    char next_word[MaxInputLength + 3];
    const char *theinput = s;
    char otherch;
    char separator[] = " ";

    while (get_word(theinput, next_word, otherch)) {
        separator[0] = otherch;
        strcat(buf, swap_term(next_word));
        strcat(buf, separator);
    }
    strcpy(s, buf);
}

// enables $variable translation
void Eliza::expand_variables(char *response_buf, std::string_view npc_name, const std::string &response,
                             const char *player_name, char *rest) {
    trim(rest);
    std::string updated = replace_strings(response, "$t", player_name);
    updated = replace_strings(updated, "$n", npc_name);
    updated = replace_strings(updated, "$$", "$");
    updated = replace_strings(updated, "$r", rest);
    updated = replace_strings(updated, "$A", eliza_title);
    updated = replace_strings(updated, "$V", eliza_version);
    updated = replace_strings(updated, "$C", compile_time_);
    // Until response_buf can be replaced with string, limit its length to the response buffer size.
    std::string truncated = updated.substr(0, MaxChatReplyLength - 1);
    updated.copy(response_buf, truncated.size(), 0);
    response_buf[truncated.size()] = '\0';
}

const char *Eliza::handle_player_message(char *response_buf, const char *player_name, std::string_view message,
                                         std::string_view npc_name) {
    std::string npc_name_str(npc_name);
    auto dbnum = get_database_num_by_partial_name(npc_name_str);
    if (dbnum < 0)
        return "invalid database number";

    if (message.size() > MaxInputLength)
        return "You talk too much.";

    strcpy(response_buf, "I dont really know much about that, say more.");
    std::string msgbuf = reduce_spaces(message);
    int overflow = 10; // runtime check so we dont have circular database links
    do {
        for (auto &keyword_response : databases_[dbnum].keyword_responses()) {
            auto keywords = keyword_response.get_keywords();
            std::string_view::iterator it = keywords.begin();
            uint remaining_input_pos = 0;
            if (match(keywords, msgbuf, it, remaining_input_pos)) {
                auto &response = keyword_response.get_random_response();
                swap_pronouns_and_possessives(&msgbuf[remaining_input_pos]);
                expand_variables(response_buf, npc_name, response, player_name, &msgbuf[remaining_input_pos]);
                return response_buf;
            }
        }
        dbnum = databases_[dbnum].linked_database_num;
        overflow--;
    } while (dbnum >= 0 && overflow > 0);
    if (overflow <= 0) {
        return "potential circular cont (@) jump in databases";
    }
    return response_buf;
}

bool Eliza::load_databases(const char *file, char recurflag) {
    if (num_databases_ >= MaxDatabases)
        return false;

    FILE *data;
    char str[MaxInputLength];
    if ((data = fopen(file, "rt")) == nullptr) {
        return false;
    }
    int linecount = 0;

    while (fgets(str, MaxInputLength - 1, data) != nullptr) {
        linecount++;
        trim(str);
        if (strlen(str) > 0) {
            if (str[0] >= '1' && str[0] <= '9') {
                // Add a new WeightedResponse to the current KeywordResponses entry.
                databases_[num_databases_].keyword_responses().back().add_response(str[0] - '0', &(str[1]));
            } else
                switch (str[0]) {
                case '\0': break;
                case '(':
                    // Add a new KeywordResponses object to the current database with the keywords we just parsed.
                    databases_[num_databases_].keyword_responses().emplace_back(str);
                    break;
                case '#': break;
                case '"': fprintf(stderr, "%s\n", &str[1]); break;
                case '\'': fprintf(stdout, "%s\n", &str[1]); break;
                case '>': // add another database
                    if (num_databases_ < MaxDatabases - 1)
                        num_databases_++;
                    else
                        fputs("response database is full. numdbases>maxdbases", stderr);
                    register_database_names(&str[1], num_databases_);
                    break;
                case '%': // include another file inline
                    trim(str + 1);
                    if (!load_databases(str + 1, 1)) // recurflag set
                        printf("including inline '%s' failed at %d!\n", str + 1, linecount);
                    break;
                case '@': // link the current database to an earlier database to reuse its entries.
                    if (databases_[num_databases_].linked_database_num == NoDatabaseLink) {
                        trim(str + 1);
                        std::string linked_db_name = std::string(str + 1);
                        int linked_db_num = get_database_num_by_exact_name(linked_db_name);
                        if (linked_db_num < 0) {
                            printf(" @ database could not be found: %s on line %d\n", linked_db_name.c_str(),
                                   linecount);
                        }
                        databases_[num_databases_].linked_database_num = linked_db_num;
                    } else
                        printf(" @ database already linked %d at %d\n", num_databases_, linecount);
                    break;
                default: printf("extraneous line: '%s' at %d\n", str, linecount);
                }
        }
    }
    fclose(data);
    if (!recurflag) {
        num_databases_++;
    }
    return true;
}

char *Eliza::trim(char str[]) {
    int i, a, ln;
    for (ln = strlen(str); (ln > 0) && (str[ln - 1] <= ' '); ln--)
        ;
    str[ln] = 0;
    for (i = 0; (i < ln) && (str[i] <= ' '); i++)
        ;
    if (i > 0)
        for (ln -= i, a = 0; a <= ln; a++)
            str[a] = str[a + i];
    return str;
}

bool Eliza::eval_operator(const char op, const int a, const int b) {
    switch (op) {
    case '\0':
    case '&': return a && b;
    case '|': return a || b;
    case '~': return a && !b;
    default: return false;
    }
}

int Eliza::strpos(std::string_view input_msg, std::string_view current_db_keyword) {
    auto input_msg_len = input_msg.size();
    auto keyword_len = current_db_keyword.size();
    size_t a;
    int run;
    int space = 1;
    if (current_db_keyword[0] == '=') { // = exact equality operator
        return strcasecmp(&current_db_keyword[1], input_msg.data()) ? 0 : input_msg_len;
    }
    if (current_db_keyword[0] == '^') { // match start operator
        return strncasecmp(&current_db_keyword[1], input_msg.data(), keyword_len - 1) ? 0 : input_msg_len;
    }
    if (keyword_len > input_msg_len)
        return 0;

    run = input_msg_len - keyword_len;
    for (int i = 0; i <= run; i++) {
        if (space) {
            for (a = 0; a < keyword_len && (tolower(input_msg[i + a]) == current_db_keyword[a]); a++)
                ;
            if (a == keyword_len)
                return (i + a);
        }
        space = input_msg[i] == ' ';
    }

    return 0;
}

int Eliza::match(std::string_view db_keywords, std::string_view input_msg, std::string_view::iterator &it,
                 uint &remaining_input_pos) {
    // Records the match result through a sequence of logical expressions (e.g. (foo|bar|baz)
    int progressive_match_result = 0;
    // Records the index into input_msg that the current_db_keyword was matched at, if it was matched,
    // or zero if it didn't match.
    int next_match_pos = 0;
    char logical_operator = 0;
    // db_keywords is split on ( ) & | and ~
    // and current_db_keyword var stores a copy of the current token from it.
    std::string current_db_keyword{};
    for (; it != db_keywords.end(); it++) {
        switch (*it) {
        case '(':
            next_match_pos = match(db_keywords, input_msg, ++it, remaining_input_pos);
            if (logical_operator == '\0')
                progressive_match_result = next_match_pos;
            else
                progressive_match_result = eval_operator(logical_operator, progressive_match_result, next_match_pos);
            break;
        case '&': {
            logical_operator = '&';
            current_db_keyword = reduce_spaces(current_db_keyword);
            handle_operator(input_msg, current_db_keyword, logical_operator, progressive_match_result, next_match_pos,
                            remaining_input_pos);
            current_db_keyword.clear();
            break;
        }
        case '|': {
            logical_operator = '|';
            current_db_keyword = reduce_spaces(current_db_keyword);
            handle_operator(input_msg, current_db_keyword, logical_operator, progressive_match_result, next_match_pos,
                            remaining_input_pos);
            current_db_keyword.clear();
            break;
        }
        case '~': {
            logical_operator = '~';
            handle_operator(input_msg, current_db_keyword, logical_operator, progressive_match_result, next_match_pos,
                            remaining_input_pos);
            current_db_keyword.clear();
            break;
        }
        case ')':
            current_db_keyword = reduce_spaces(current_db_keyword);
            if (!current_db_keyword.empty()) {
                // If we encountered no logic in the current db_keywords at this level of recursion
                // and hit a closing paren, set this to 1 as it will be anded with the result of strpos()
                if (logical_operator == 0)
                    progressive_match_result = 1;
                next_match_pos = strpos(input_msg, current_db_keyword);
                if (next_match_pos > 0)
                    remaining_input_pos = next_match_pos;
                return eval_operator(logical_operator, progressive_match_result, next_match_pos);
            } else
                return progressive_match_result;

            break;
        default: current_db_keyword.push_back(*it);
        }
    }
    return progressive_match_result;
}

void Eliza::handle_operator(std::string_view input_msg, std::string_view current_db_keyword,
                            const char logical_operator, int &progressive_match_result, int &next_match_pos,
                            uint &remaining_input_pos) {
    if (!current_db_keyword.empty()) {
        next_match_pos = strpos(input_msg, current_db_keyword);
        if (next_match_pos > 0)
            remaining_input_pos = next_match_pos;
        progressive_match_result = eval_operator(logical_operator, progressive_match_result, next_match_pos);
    }
}
