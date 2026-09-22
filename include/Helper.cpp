#ifndef HELPER
#define HELPER

#include <fstream>
#include <algorithm>
#include <sys/stat.h>
#include <cstring>
#ifdef _WIN32
#include <direct.h>
#include <io.h>
#include <windows.h>
#else
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#endif
#include "models.cpp"

using namespace std;

namespace FileUtils
{
    inline bool exists(const string &path)
    {
        struct stat buffer;
        return (stat(path.c_str(), &buffer) == 0);
    }

    inline bool is_directory(const string &path)
    {
        struct stat buffer;
        if (stat(path.c_str(), &buffer) != 0)
            return false;
        return (buffer.st_mode & S_IFDIR) != 0;
    }

    inline void create_dir(const string &path)
    {
        if (exists(path)) return;
#ifdef _WIN32
        _mkdir(path.c_str());
#else
        mkdir(path.c_str(), 0755);
#endif
    }

    inline vector<string> list_subdirectories(const string &dir)
    {
        vector<string> result;
#ifdef _WIN32
        string search_path = dir + "/*";
        WIN32_FIND_DATAA fd;
        HANDLE hFind = FindFirstFileA(search_path.c_str(), &fd);
        if (hFind != INVALID_HANDLE_VALUE)
        {
            do
            {
                if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
                    strcmp(fd.cFileName, ".") != 0 && strcmp(fd.cFileName, "..") != 0)
                {
                    result.push_back(fd.cFileName);
                }
            } while (FindNextFileA(hFind, &fd));
            FindClose(hFind);
        }
#else
        DIR *dp = opendir(dir.c_str());
        if (dp != nullptr)
        {
            struct dirent *entry;
            while ((entry = readdir(dp)) != nullptr)
            {
                if (entry->d_type == DT_DIR && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
                {
                    result.push_back(entry->d_name);
                }
            }
            closedir(dp);
        }
#endif
        return result;
    }
}

class Helper
{
public:
    static string trim(const string &str)
    {
        int start(0);
        while (start < str.size() && isspace(str[start]))
            ++start;

        if (start == str.size())
            return "";

        int end(str.size() - 1);
        while (end > start && isspace(str[end]))
            --end;

        return str.substr(start, end - start + 1);
    }
    static string to_lower(const string &str)
    {
        string result(str);
        for (char &c : result)
            c = tolower(c);
        return result;
    }

    static string to_upper(const string &str)
    {
        string result(str);
        for (char &c : result)
            c = toupper(c);
        return result;
    }

    static bool starts_with_prefix(const string &str, const string &prefix)
    {
        string lower_str(to_lower(str)),
            lower_prefix(to_lower(prefix));

        if (lower_str.size() < lower_prefix.size())
            return false;

        return lower_str.compare(0, lower_prefix.size(), lower_prefix) == 0;
    }
    static pair<int, int> find_top_level_parens(const string &str)
    {
        bool in_single_quote(false),
            in_double_quote(false);
        int depth(0), start(-1), end(-1);

        for (int i(0); i < str.size(); ++i)
        {
            char c(str[i]);

            if (c == '\'' && !in_double_quote)
            {
                in_single_quote = !in_single_quote;
                continue;
            }
            if (c == '"' && !in_single_quote)
            {
                in_double_quote = !in_double_quote;
                continue;
            }

            if (in_single_quote || in_double_quote)
                continue;

            if (c == '(')
            {
                if (!depth)
                    start = i;
                ++depth;
            }
            else if (c == ')')
            {
                --depth;
                if (!depth)
                {
                    end = i;
                    break;
                }
                if (depth < 0)
                    return {-1, -1};
            }
        }

        if (start > -1 && end > -1)
            return {start, end};

        return {-1, -1};
    }
    static vector<string> split_commas_respecting_quotes(const string &str)
    {
        vector<string> result;
        string current;
        bool in_single_quote(false), in_double_quote(false);
        int paren_depth(0);

        for (int i(0); i < str.size(); ++i)
        {
            char c(str[i]);

            if (c == '\'' && !in_double_quote)
            {
                in_single_quote = !in_single_quote;
                current += c;
                continue;
            }
            if (c == '"' && !in_single_quote)
            {
                in_double_quote = !in_double_quote;
                current += c;
                continue;
            }

            if (!in_single_quote && !in_double_quote)
            {
                if (c == '(')
                {
                    ++paren_depth;
                    current += c;
                    continue;
                }
                if (c == ')')
                {
                    if (paren_depth > 0)
                        --paren_depth;
                    current += c;
                    continue;
                }
                if (c == ',' && !paren_depth)
                {
                    result.push_back(trim(current));
                    current.clear();
                    continue;
                }
            }

            current += c;
        }
        current = trim(current);
        if (!current.empty())
            result.push_back(current);

        return result;
    }
    static vector<string> split_spaces_respecting_quotes(const string &str)
    {
        vector<string> tokens;
        string current;
        bool in_single_quote(false), in_double_quote(false);

        for (int i(0); i < str.size(); ++i)
        {
            char c(str[i]);

            if (c == '\'' && !in_double_quote)
            {
                in_single_quote = !in_single_quote;
                current += c;
                continue;
            }
            if (c == '"' && !in_double_quote)
            {
                in_double_quote = !in_double_quote;
                current += c;
                continue;
            }

            if (!in_single_quote && !in_double_quote && isspace(c))
            {
                if (!current.empty())
                    tokens.push_back(current), current.clear();
            }
            else
                current += c;
        }

        if (!current.empty())
            tokens.push_back(current);

        return tokens;
    }
    static vector<string> parse_column_name_list(const string &str)
    {
        vector<string> columns;
        vector<string> parts = split_commas_respecting_quotes(str);

        for (auto &part : parts)
            columns.push_back(trim(part));

        return columns;
    }
    static string get_data_dir()
    {
        if (FileUtils::exists("data"))
            return "data";
        if (FileUtils::exists("../data"))
            return "../data";
        return "data";
    }
    static string csv_path(const string &table_name)
    {
        return get_data_dir() + "/" + table_name + "/" + table_name + ".csv";
    }
    static string meta_path(const string &table_name)
    {
        return get_data_dir() + "/" + table_name + "/" + table_name + ".meta";
    }
    static void ensure_data_dir()
    {
        string data_dir = get_data_dir();
        FileUtils::create_dir(data_dir);
    }
    static bool create_csv_header(const string &table_name, const vector<string> &columns)
    {
        ensure_data_dir();
        string table_dir = get_data_dir() + "/" + table_name;
        FileUtils::create_dir(table_dir);

        string path = csv_path(table_name);
        if (FileUtils::exists(path))
            return false;

        ofstream file(path);
        if (!file.is_open())
            return false;

        for (int i(0); i < columns.size(); ++i)
        {
            string col_name(columns[i]);
            replace(col_name.begin(), col_name.end(), ',', '_');
            file << col_name;

            if (i + 1 < columns.size())
                file << ",";
        }
        file << "\n";
        file.close();

        return true;
    }
    static bool write_meta(const string &table_name, const vector<Column> &columns,
                           const vector<string> &primary_key_cols)
    {
        ensure_data_dir();
        string table_dir = get_data_dir() + "/" + table_name;
        FileUtils::create_dir(table_dir);

        string path = meta_path(table_name);
        if (FileUtils::exists(path))
            return false;

        ofstream file(path);
        if (!file.is_open())
            return false;

        file << "columns:\n";
        for (const auto &col : columns)
        {
            file << col.get_name() << "|"
                 << col.get_type() << "|"
                 << col.get_char_length() << "|"
                 << (col.is_null() ? "1" : "0") << "\n";
        }

        file << "pk:";
        for (int i(0); i < primary_key_cols.size(); ++i)
        {
            file << primary_key_cols[i];
            if (i + 1 < primary_key_cols.size())
                file << ",";
        }
        file << "\n";
        file.close();

        return true;
    }
    static void show_help()
    {
        cout << "\n================================================================\n";
        cout << "         NeuroDB Database Engine - Command Reference\n";
        cout << "================================================================\n\n";

        cout << "--- SQL COMMANDS -----------------------------------------------\n\n";

        cout << ">> CREATE TABLE - Create a new table schema\n"
             << "  Syntax:\n"
             << "    CREATE TABLE table_name (\n"
             << "      col_name TYPE [NOT NULL] [PRIMARY KEY],\n"
             << "      ...,\n"
             << "      [PRIMARY KEY (col1, col2, ...)]\n"
             << "    );\n\n";

        cout << ">> INSERT - Add new rows to a table\n"
             << "  Syntax:\n"
             << "    INSERT INTO table_name VALUES (value1, value2, ...);\n\n";

        cout << ">> SELECT - Query and retrieve data\n"
             << "  Syntax:\n"
             << "    SELECT * | col1, col2, ... FROM table_name [WHERE condition] [GROUP BY ...] [HAVING ...];\n\n";

        cout << ">> UPDATE - Modify existing rows\n"
             << "  Syntax:\n"
             << "    UPDATE table_name SET col1=val1, ... WHERE condition;\n\n";

        cout << ">> DELETE - Remove rows from a table\n"
             << "  Syntax:\n"
             << "    DELETE FROM table_name WHERE condition;\n\n";

        cout << "================================================================\n";
        cout << "NeuroDB Engine v2.0 | Hybrid AI File-Based Database System\n";
    }

    static void load_existing_tables(Catalog *catalog)
    {
        string data_dir = get_data_dir();
        if (!FileUtils::exists(data_dir) || !FileUtils::is_directory(data_dir))
            return;

        vector<string> subdirs = FileUtils::list_subdirectories(data_dir);
        for (const auto &table_name : subdirs)
        {
            string table_path = data_dir + "/" + table_name;
            string meta_file = table_path + "/" + table_name + ".meta";

            if (!FileUtils::exists(meta_file))
                continue;

            ifstream file(meta_file);
            if (!file.is_open())
                continue;

            vector<Column> columns;
            vector<string> pk_cols;
            string line;

            while (getline(file, line))
            {
                if (line == "columns:")
                    continue;

                if (line.find("pk:") == 0)
                {
                    string pk_list(line.substr(3));
                    pk_cols = Helper::split_commas_respecting_quotes(pk_list);
                    for (auto &pk : pk_cols)
                        pk = Helper::trim(pk);
                    break;
                }

                int pos1(line.find('|')),
                    pos2(line.find('|', pos1 + 1)),
                    pos3(line.find('|', pos2 + 1));
                if (pos1 != string::npos && pos2 != string::npos)
                {
                    string name(Helper::trim(line.substr(0, pos1)));
                    string type(Helper::trim(line.substr(pos1 + 1, pos2 - pos1 - 1)));
                    string len_str(Helper::trim(line.substr(pos2 + 1, pos3 != string::npos ? pos3 - pos2 - 1 : string::npos)));
                    int len(stoi(len_str));
                    if (!len)
                        len = 1;

                    bool is_nullable(true);
                    if (pos3 != string::npos)
                    {
                        string null_str(Helper::trim(line.substr(pos3 + 1)));
                        is_nullable = (null_str == "1");
                    }

                    columns.emplace_back(name, type, false, len, is_nullable);
                }
            }
            file.close();

            if (!columns.empty())
            {
                Table *t(new Table(table_name, columns, pk_cols));

                string csv_file = table_path + "/" + table_name + ".csv";
                if (FileUtils::exists(csv_file))
                {
                    ifstream csv(csv_file);
                    string header;
                    getline(csv, header);

                    string data_line;
                    while (getline(csv, data_line))
                    {
                        if (data_line.empty())
                            continue;

                        auto values(Helper::split_commas_respecting_quotes(data_line));
                        if (values.size() != columns.size())
                            continue;

                        Row row;
                        for (int i(0); i < values.size(); ++i)
                        {
                            string val(Helper::trim(values[i]));
                            string type(columns[i].get_type());

                            if (val == "NULL")
                                row.push_back(Value());
                            else if (type == "INT")
                                row.push_back(Value(stoi(val)));
                            else if (type == "DOUBLE")
                                row.push_back(Value(stod(val)));
                            else if (type == "DATE")
                            {
                                int y(0), m(0), d(0);
                                sscanf(val.c_str(), "%d-%d-%d", &y, &m, &d);
                                row.push_back(Value(Date(y, m, d)));
                            }
                            else
                                row.push_back(Value(val));
                        }

                        try
                        {
                            t->insert_row(row);
                        }
                        catch (const exception &e)
                        {
                            continue;
                        }
                    }
                    csv.close();
                }

                catalog->addTable(t);
            }
        }
    }
};

#endif