#include "../include/CreateParse.cpp"
#include "../include/InsertParser.cpp"
#include "../include/SelectParser.cpp"
#include "../include/UpdateParser.cpp"
#include "../include/DeleteParser.cpp"

void print_schema_json(Catalog *catalog)
{
    cout << "[\n";
    if (catalog)
    {
        auto &tables = catalog->get_tables();
        bool first_tbl = true;
        for (const auto &pair : tables)
        {
            Table *table = pair.second;
            if (!table) continue;
            if (!first_tbl) cout << ",\n";
            first_tbl = false;
            cout << "  {\n";
            cout << "    \"name\": \"" << table->get_name() << "\",\n";
            cout << "    \"columns\": [\n";
            auto &cols = table->get_columns();
            for (size_t i = 0; i < cols.size(); ++i)
            {
                cout << "      {\"name\": \"" << cols[i].get_name()
                     << "\", \"type\": \"" << cols[i].get_type()
                     << "\", \"nullable\": " << (cols[i].is_null() ? "true" : "false") << "}";
                if (i + 1 < cols.size()) cout << ",";
                cout << "\n";
            }
            cout << "    ],\n";
            cout << "    \"row_count\": " << table->get_rows().size() << "\n";
            cout << "  }";
        }
    }
    cout << "\n]\n";
}

bool execute_query(string line, CreateParser &create_parser, InsertParser &insert_parser,
                   SelectParser &select_parser, UpdateParser &update_parser, DeleteParser &delete_parser)
{
    string cmd(Helper::trim(line));
    if (cmd.empty()) return true;

    if (!cmd.empty() && cmd.back() == ';')
    {
        cmd.pop_back();
        cmd = Helper::trim(cmd);
    }

    string lower(Helper::to_lower(cmd));

    if (lower == "help" || lower == "?")
    {
        Helper::show_help();
        return true;
    }

    AST ast;
    bool success(false);

    if (Helper::starts_with_prefix(cmd, "create"))
        success = create_parser.parse_and_create(cmd, ast);
    else if (Helper::starts_with_prefix(cmd, "insert"))
        success = insert_parser.parse_and_insert(cmd, ast);
    else if (Helper::starts_with_prefix(cmd, "select"))
        success = select_parser.parse_and_select(cmd, ast);
    else if (Helper::starts_with_prefix(cmd, "update"))
        success = update_parser.parse_and_update(cmd, ast);
    else if (Helper::starts_with_prefix(cmd, "delete"))
        success = delete_parser.parse_and_delete(cmd, ast);
    else
    {
        cout << "Error: Unknown SQL command: '" << cmd << "'\n";
        return false;
    }

    if (!success)
    {
        cout << "Error: Syntax error or execution failure in query.\n";
    }
    return success;
}

int main(int argc, char *argv[])
{
    CreateParser create_parser;
    Catalog *catalog(&create_parser.catalog());
    InsertParser insert_parser(catalog);
    SelectParser select_parser(catalog);
    UpdateParser update_parser(catalog);
    DeleteParser delete_parser(catalog);

    Helper::load_existing_tables(catalog);

    if (argc > 1)
    {
        string arg1 = argv[1];
        if (arg1 == "--schema" || arg1 == "-s")
        {
            print_schema_json(catalog);
            return 0;
        }
        if ((arg1 == "--query" || arg1 == "-q") && argc > 2)
        {
            string query = argv[2];
            bool success = execute_query(query, create_parser, insert_parser, select_parser, update_parser, delete_parser);
            return success ? 0 : 1;
        }
    }

    cout << "\n================================================================\n";
    cout << "           Welcome to NeuroDB AI-Powered Engine\n";
    cout << "           Type 'help' for commands, 'exit' to quit\n";
    cout << "================================================================\n\n";

    string line;
    cout << "SQL> ";
    while (getline(cin, line))
    {
        string cmd(Helper::trim(line));
        if (cmd.empty())
        {
            cout << "SQL> ";
            continue;
        }

        string lower(Helper::to_lower(cmd));

        if (lower == "exit" || lower == "quit")
        {
            cout << "\nGoodbye!\n";
            break;
        }

        execute_query(line, create_parser, insert_parser, select_parser, update_parser, delete_parser);
        cout << "SQL> ";
    }

    return 0;
}
