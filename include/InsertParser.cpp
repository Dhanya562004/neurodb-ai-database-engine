#ifndef INSERT_PARSER
#define INSERT_PARSER

#include "models.cpp"
#include "Helper.cpp"
#include <fstream>

class InsertParser
{
    Catalog *_catalog;

    Value parse_value(const string &val_str, const Column &col)
    {
        string s(Helper::trim(val_str));
        const string &type(col.get_type());

        if (s == "NULL")
            return Value();

        if (type == "INT")
            return Value(stoi(s));

        if (type == "DOUBLE")
            return Value(stod(s));

        if (type == "CHAR" || type == "VARCHAR" || type == "TEXT")
        {
            if (s.empty() || (s.front() != '\'' && s.front() != '"'))
                throw invalid_argument("String values must be enclosed in single or double quotes");

            s = s.substr(1, s.size() - 2);

            if (s.empty())
                return Value();

            int max_length(col.get_char_length());
            if (s.length() > max_length)
                throw invalid_argument("String length (" + to_string(s.length()) + ") exceeds maximum allowed length (" + to_string(max_length) + ") for column '" + col.get_name() + "'");

            if (type == "CHAR" && s.length() < max_length)
                s.resize(max_length, ' ');

            return Value(s);
        }

        if (type == "DATE")
        {
            if (s.empty() || (s.front() != '\'' && s.front() != '"'))
                throw invalid_argument("Date values must be enclosed in single or double quotes");

            s = s.substr(1, s.size() - 2);
            int y(0), m(0), d(0);
            sscanf(s.c_str(), "%d-%d-%d", &y, &m, &d);

            return Value(Date(y, m, d));
        }

        if (s.empty() || (s.front() != '\'' && s.front() != '"'))
            throw invalid_argument("String values must be enclosed in single or double quotes");

        s = s.substr(1, s.size() - 2);
        if (s.empty())
            return Value();

        return Value(s);
    }

public:
    InsertParser(Catalog *cat) : _catalog(cat) {}

    bool parse_and_insert(const string &line, AST &out_ast)
    {
        string s(Helper::trim(line));
        if (s.empty())
            return false;

        if (s.back() == ';')
            s.pop_back();

        string lower(Helper::to_lower(s));
        if (lower.find("insert") != 0)
            return false;

        int pos(6);
        while (pos < s.size() && isspace(s[pos])) // go to into keyword
            ++pos;

        if (lower.substr(pos, 4) != "into")
            return false;
        pos += 4;

        while (pos < s.size() && isspace(s[pos])) // go to table_name
            ++pos;

        int start(pos);
        while (pos < s.size() && !isspace(s[pos])) // extract tabel_name
            ++pos;

        string table_name(s.substr(start, pos - start));
        if (table_name.empty())
            return false;

        while (pos < s.size() && isspace(s[pos])) // go to values keyword
            ++pos;

        if (lower.substr(pos, 6) != "values")
            return false;
        pos += 6;

        while (pos < s.size() && isspace(s[pos]))
            ++pos;

        auto parens(Helper::find_top_level_parens(s.substr(pos)));
        if (parens.first < 0)
            return false;

        string inside(s.substr(pos + parens.first + 1, parens.second - parens.first - 1));
        auto values(Helper::split_commas_respecting_quotes(inside)); // 12 ali 15000

        AST_Insert insert_node;
        insert_node.table_name = table_name;
        for (const auto &v : values)
            insert_node.raw_values.push_back(Helper::trim(v));

        out_ast.kind = ASTKind::INSERT;
        out_ast.node = insert_node;

        Table *table(_catalog->getTable(table_name));
        if (!table)
        {
            cout << "\nTable '" << table_name << "' not found\n";
            return false;
        }

        auto &cols(table->get_columns());
        if (values.size() > cols.size())
        {
            cout << "\nToo many values: expected " << cols.size() << ", got " << values.size() << "\n";
            return false;
        }

        Row row;
        for (int i(0); i < cols.size(); ++i)
        {
            if (i < values.size())
            {
                try
                {
                    Value parsed_value(parse_value(values[i], cols[i]));

                    if (holds_alternative<NullType>(parsed_value.raw()) && !cols[i].is_null())
                    {
                        cout << "\nColumn '" << cols[i].get_name() << "' cannot be NULL\n";
                        return false;
                    }

                    row.push_back(parsed_value);
                }
                catch (const invalid_argument &e)
                {
                    cout << "\nSyntax error: " << e.what() << "\n";
                    return false;
                }
            }
            else
            {
                if (cols[i].is_null())
                    row.push_back(Value());
                else
                {
                    cout << "\nColumn '" << cols[i].get_name() << "' cannot be NULL\n";
                    return false;
                }
            }
        }

        try
        {
            table->insert_row(row);
        }
        catch (const exception &e)
        {
            cout << "\nError: " << e.what() << "\n";
            return false;
        }

        ofstream file(Helper::csv_path(table_name), ios::app);
        for (int i(0); i < row.size(); ++i)
        {
            file << row.at(i).to_string();
            if (i + 1 < row.size())
                file << ",";
        }
        file << "\n";
        file.close();

        cout << "\n1 row inserted\n";
        return true;
    }
};

#endif