#ifndef DELETE_PARSER
#define DELETE_PARSER

#include "models.cpp"
#include "Helper.cpp"
#include <fstream>

class DeleteParser
{
    Catalog *_catalog;

    Value parse_value(const string &val_str, const string &type)
    {
        string s(Helper::trim(val_str));

        if (s == "NULL")
            return Value();

        if (type == "INT")
            return Value(atoi(s.c_str()));

        if (type == "DOUBLE")
            return Value(atof(s.c_str()));

        if (type == "CHAR")
        {
            if (!s.empty() && (s.front() == '\'' || s.front() == '"'))
                s = s.substr(1, s.size() - 2);
            return Value(s);
        }

        if (type == "DATE")
        {
            if (!s.empty() && (s.front() == '\'' || s.front() == '"'))
                s = s.substr(1, s.size() - 2);
            int y(0), m(0), d(0);
            sscanf(s.c_str(), "%d-%d-%d", &y, &m, &d);
            return Value(Date(y, m, d));
        }

        if (!s.empty() && (s.front() == '\'' || s.front() == '"'))
            s = s.substr(1, s.size() - 2);
        return Value(s);
    }

    int find_column_index(const Table *table, const string &col_name) const
    {
        auto &cols(table->get_columns());
        for (int i(0); i < cols.size(); ++i)
        {
            if (cols[i].get_name() == col_name)
                return i;
        }
        return NOT_FOUND;
    }

    bool evaluate_condition(const Row &row, const Table *table, const string &condition)
    {
        if (condition.empty())
            return true;

        vector<string> operators = {"!=", ">=", "<=", "=", ">", "<"};
        string op;
        int op_pos = -1;

        for (const auto &o : operators)
        {
            int pos = condition.find(o);
            if (pos != string::npos)
            {
                op = o;
                op_pos = pos;
                break;
            }
        }

        if (op_pos == -1)
            return true;

        string col_name_str(Helper::trim(condition.substr(0, op_pos)));
        string value_str(Helper::trim(condition.substr(op_pos + op.size())));

        if (!value_str.empty() && (value_str.front() == '\'' || value_str.front() == '"'))
            value_str = value_str.substr(1, value_str.size() - 2);

        int col_idx = find_column_index(table, col_name_str);
        if (col_idx == -1)
            return false;

        auto &cols(table->get_columns());
        const Column &col = cols[col_idx];
        string type = col.get_type();

        Value row_value = row[col_idx];
        Value cond_value = parse_value(value_str, type);

        bool result = false;
        if (op == "=")
            result = row_value == cond_value;
        else if (op == "!=")
            result = !(row_value == cond_value);
        else if (op == ">")
            result = row_value > cond_value;
        else if (op == "<")
            result = row_value < cond_value;
        else if (op == ">=")
            result = row_value > cond_value || row_value == cond_value;
        else if (op == "<=")
            result = row_value < cond_value || row_value == cond_value;

        return result;
    }

    void rewrite_csv(Table *table)
    {
        string csv_file = Helper::csv_path(table->get_name());
        ofstream file(csv_file, ios::trunc);

        auto &cols(table->get_columns());
        for (size_t i(0); i < cols.size(); ++i)
        {
            file << cols[i].get_name();
            if (i + 1 < cols.size())
                file << ",";
        }
        file << "\n";

        auto &rows(table->get_rows());
        for (const auto &row : rows)
        {
            for (size_t i(0); i < row.size(); ++i)
            {
                file << row.at(i).to_string();
                if (i + 1 < row.size())
                    file << ",";
            }
            file << "\n";
        }
        file.close();
    }

public:
    DeleteParser(Catalog *cat) : _catalog(cat) {}

    bool parse_and_delete(const string &line, AST &out_ast)
    {
        string s(Helper::trim(line));
        if (s.empty())
            return false;
        if (s.back() == ';')
            s.pop_back();

        string lower(Helper::to_lower(s));
        if (lower.find("delete") != 0)
            return false;

        // Build AST_Delete node
        AST_Delete ast_delete;

        int pos(6);
        while (pos < s.size() && isspace(s[pos]))
            ++pos;

        if (lower.substr(pos, 4) != "from")
            return false;
        pos += 4;

        while (pos < s.size() && isspace(s[pos]))
            ++pos;

        int start(pos);
        while (pos < s.size() && !isspace(s[pos]))
            ++pos;

        string table_name(s.substr(start, pos - start));
        if (table_name.empty())
            return false;

        ast_delete.table_name = table_name;

        Table *table(_catalog->getTable(table_name));
        if (!table)
        {
            cout << "\nTable '" << table_name << "' not found\n";
            return false;
        }

        while (pos < s.size() && isspace(s[pos]))
            ++pos;

        vector<int> rows_to_delete;
        string where_clause;

        if (pos < s.size() && lower.substr(pos, 5) == "where")
        {
            pos += 5;
            while (pos < s.size() && isspace(s[pos]))
                ++pos;

            where_clause = s.substr(pos);
            
            // Parse WHERE condition for AST
            vector<string> operators = {"!=", ">=", "<=", "=", ">", "<"};
            string op;
            int op_pos = -1;

            for (const auto &o : operators)
            {
                int p = where_clause.find(o);
                if (p != string::npos)
                {
                    op = o;
                    op_pos = p;
                    break;
                }
            }

            if (op_pos == -1)
                return false;

            string where_col(Helper::trim(where_clause.substr(0, op_pos)));
            string where_val(Helper::trim(where_clause.substr(op_pos + op.size())));

            // Add Condition to AST
            Condition cond;
            cond.lhs = where_col;
            cond.op = op;
            cond.rhs = where_val;
            ast_delete.where.push_back(cond);

            // Evaluate condition for each row
            auto &rows(table->get_rows());
            for (int i(0); i < rows.size(); ++i)
            {
                if (evaluate_condition(rows[i], table, where_clause))
                    rows_to_delete.push_back(i);
            }
        }
        else
        {
            for (int i(0); i < table->get_rows().size(); ++i)
                rows_to_delete.push_back(i);
        }

        // Populate the AST output
        out_ast.kind = ASTKind::_DELETE;
        out_ast.node = ast_delete;

        if (rows_to_delete.empty())
        {
            cout << "\n0 rows deleted\n";
            return true;
        }

        sort(rows_to_delete.rbegin(), rows_to_delete.rend());

        auto &rows(table->get_rows());
        for (int idx : rows_to_delete)
        {
            if (idx >= rows.size())
                continue;

            int last(rows.size() - 1);
            if (idx != last)
                rows[idx] = move(rows[last]);
            rows.pop_back();
        }

        if (table->has_pk())
        {
            Table *new_table(new Table(table->get_name(), table->get_columns(), {}));
            vector<string> pk_names;
            auto &pk_indices(table->getpk_indices());
            auto &cols(table->get_columns());
            for (int pk_idx : pk_indices)
                pk_names.push_back(cols[pk_idx].get_name());

            new_table = new Table(table->get_name(), table->get_columns(), pk_names);

            for (const auto &row : table->get_rows())
            {
                try
                {
                    new_table->insert_row(row);
                }
                catch (const exception &e)
                {
                }
            }

            *table = *new_table;
            delete new_table;
        }

        rewrite_csv(table);

        cout << "\n" << rows_to_delete.size() << " row(s) deleted\n";
        return true;
    }
};

#endif
