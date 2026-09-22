#ifndef UPDATE_PARSER
#define UPDATE_PARSER

#include "models.cpp"
#include "Helper.cpp"
#include <fstream>

class UpdateParser
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

        if (type == "CHAR" || type == "VARCHAR" || type == "TEXT")
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
        int op_pos(-1);

        for (const auto &o : operators)
        {
            int pos(condition.find(o));
            if (pos != string::npos)
            {
                op = o;
                op_pos = pos;
                break;
            }
        }

        if (op_pos == -1)
            return true;

        string col_name_str(Helper::trim(condition.substr(0, op_pos))),
            value_str(Helper::trim(condition.substr(op_pos + op.size())));

        if (!value_str.empty() && (value_str.front() == '\'' || value_str.front() == '"'))
            value_str = value_str.substr(1, value_str.size() - 2);

        int col_idx(find_column_index(table, col_name_str));
        if (col_idx == -1)
            return false;

        auto &cols(table->get_columns());
        const Column &col = cols[col_idx];
        string type(col.get_type());

        Value row_value(row[col_idx]),
            cond_value(parse_value(value_str, type));

        bool result(false);
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
    UpdateParser(Catalog *cat) : _catalog(cat) {}

    bool parse_and_update(const string &line, AST &out_ast)
    {
        string s(Helper::trim(line));
        if (s.empty())
            return false;
        if (s.back() == ';')
            s.pop_back();

        string lower(Helper::to_lower(s));
        if (lower.find("update") != 0)
            return false;

        // Build AST_Update node
        AST_Update ast_update;

        int pos(6);
        while (pos < s.size() && isspace(s[pos]))
            ++pos;

        int start(pos);
        while (pos < s.size() && !isspace(s[pos]))
            ++pos;

        string table_name(s.substr(start, pos - start));
        if (table_name.empty())
            return false;

        ast_update.table_name = table_name;

        Table *table(_catalog->getTable(table_name));
        if (!table)
        {
            cout << "\nTable '" << table_name << "' not found\n";
            return false;
        }

        while (pos < s.size() && isspace(s[pos]))
            ++pos;

        if (lower.substr(pos, 3) != "set")
            return false;
        pos += 3;

        while (pos < s.size() && isspace(s[pos]))
            ++pos;

        int set_start(pos);
        int where_pos(lower.find("where", pos));
        int set_end(where_pos != string::npos ? where_pos : s.size());

        string set_clause(s.substr(set_start, set_end - set_start));
        auto set_parts(Helper::split_commas_respecting_quotes(set_clause));

        struct UpdateInfo
        {
            int col_idx;
            Value placeholder;
            string expression;
            string op;
        };
        vector<UpdateInfo> updates;
        auto &cols(table->get_columns());

        for (const auto &part : set_parts)
        {
            string trimmed(Helper::trim(part)), compound_op("");
            int op_pos(-1);
            vector<string> compound_ops = {"+=", "-=", "*=", "/="};

            for (const auto &cop : compound_ops)
            {
                int pos = trimmed.find(cop);
                if (pos != string::npos)
                {
                    compound_op = cop;
                    op_pos = pos;
                    break;
                }
            }

            string col_name;
            string val_str;
            bool is_compound = !compound_op.empty();

            if (is_compound)
            {
                col_name = Helper::trim(trimmed.substr(0, op_pos));
                val_str = Helper::trim(trimmed.substr(op_pos + 2));
            }
            else
            {
                int eq_pos(trimmed.find('='));
                if (eq_pos == string::npos)
                    return false;

                col_name = Helper::trim(trimmed.substr(0, eq_pos));
                val_str = Helper::trim(trimmed.substr(eq_pos + 1));
            }

            int col_idx(find_column_index(table, col_name));
            if (col_idx == NOT_FOUND)
            {
                cout << "\nColumn '" << col_name << "' not found\n";
                return false;
            }

            if (is_compound)
                ast_update.sets.push_back({col_name, col_name + " " + compound_op.substr(0, 1) + " " + val_str});
            else
                ast_update.sets.push_back({col_name, val_str});

            updates.push_back({col_idx, Value(), val_str, is_compound ? compound_op.substr(0, 1) : ""});
        }

        vector<int> rows_to_update;
        string where_clause;

        if (where_pos != string::npos)
        {
            pos = where_pos + 5;
            while (pos < s.size() && isspace(s[pos]))
                ++pos;

            where_clause = s.substr(pos);

            vector<string> operators = {"!=", ">=", "<=", "=", ">", "<"};
            string op;
            int op_pos(-1);

            for (const auto &o : operators)
            {
                int p(where_clause.find(o));
                if (p != string::npos)
                {
                    op = o;
                    op_pos = p;
                    break;
                }
            }

            if (op_pos == -1)
                return false;

            string where_col(Helper::trim(where_clause.substr(0, op_pos))),
                where_val(Helper::trim(where_clause.substr(op_pos + op.size())));

            Condition cond;
            cond.lhs = where_col;
            cond.op = op;
            cond.rhs = where_val;
            ast_update.where.push_back(cond);

            auto &rows(table->get_rows());
            for (int i(0); i < rows.size(); ++i)
            {
                if (evaluate_condition(rows[i], table, where_clause))
                    rows_to_update.push_back(i);
            }
        }
        else
        {
            for (int i(0); i < table->get_rows().size(); ++i)
                rows_to_update.push_back(i);
        }

        out_ast.kind = ASTKind::UPDATE;
        out_ast.node = ast_update;

        if (rows_to_update.empty())
        {
            cout << "\n0 rows updated\n";
            return true;
        }

        for (int row_idx : rows_to_update)
        {
            Row new_row(table->row_at(row_idx));

            for (const auto &upd : updates)
            {
                int col_idx(upd.col_idx);
                string expr(upd.expression),
                    op(upd.op);

                Value new_val;

                if (!op.empty())
                {
                    Value current(new_row.at(col_idx)),
                        operand(parse_value(expr, cols[col_idx].get_type()));

                    if (op == "+")
                    {
                        if (holds_alternative<Int>(current.raw()) && holds_alternative<Int>(operand.raw()))
                            new_val = Value(current.get_int() + operand.get_int());
                        else
                            new_val = Value(current.get_double() + operand.get_double());
                    }
                    else if (op == "-")
                    {
                        if (holds_alternative<Int>(current.raw()) && holds_alternative<Int>(operand.raw()))
                            new_val = Value(current.get_int() - operand.get_int());
                        else
                            new_val = Value(current.get_double() - operand.get_double());
                    }
                    else if (op == "*")
                    {
                        if (holds_alternative<Int>(current.raw()) && holds_alternative<Int>(operand.raw()))
                            new_val = Value(current.get_int() * operand.get_int());
                        else
                            new_val = Value(current.get_double() * operand.get_double());
                    }
                    else if (op == "/")
                    {
                        if (holds_alternative<Int>(current.raw()) && holds_alternative<Int>(operand.raw()))
                            new_val = Value(current.get_int() / operand.get_int());
                        else
                            new_val = Value(current.get_double() / operand.get_double());
                    }
                }
                else if (expr.find('+') != string::npos || expr.find('-') != string::npos ||
                         expr.find('*') != string::npos || expr.find('/') != string::npos)
                {
                    char arith_op('=');
                    int arith_pos(-1);
                    vector<char> arith_ops = {'+', '-', '*', '/'};

                    for (char c : arith_ops)
                    {
                        int pos(expr.find(c));
                        if (pos != string::npos)
                        {
                            arith_op = c;
                            arith_pos = pos;
                            break;
                        }
                    }

                    if (arith_pos != -1)
                    {
                        string left_expr(Helper::trim(expr.substr(0, arith_pos))),
                            right_expr(Helper::trim(expr.substr(arith_pos + 1)));

                        Value left_val;
                        if (left_expr == cols[col_idx].get_name())
                            left_val = new_row.at(col_idx);
                        else
                            left_val = parse_value(left_expr, cols[col_idx].get_type());

                        Value right_val(parse_value(right_expr, cols[col_idx].get_type()));

                        if (arith_op == '+')
                        {
                            if (holds_alternative<Int>(left_val.raw()) && holds_alternative<Int>(right_val.raw()))
                                new_val = Value(left_val.get_int() + right_val.get_int());
                            else
                                new_val = Value(left_val.get_double() + right_val.get_double());
                        }
                        else if (arith_op == '-')
                        {
                            if (holds_alternative<Int>(left_val.raw()) && holds_alternative<Int>(right_val.raw()))
                                new_val = Value(left_val.get_int() - right_val.get_int());
                            else
                                new_val = Value(left_val.get_double() - right_val.get_double());
                        }
                        else if (arith_op == '*')
                        {
                            if (holds_alternative<Int>(left_val.raw()) && holds_alternative<Int>(right_val.raw()))
                                new_val = Value(left_val.get_int() * right_val.get_int());
                            else
                                new_val = Value(left_val.get_double() * right_val.get_double());
                        }
                        else if (arith_op == '/')
                        {
                            if (holds_alternative<Int>(left_val.raw()) && holds_alternative<Int>(right_val.raw()))
                                new_val = Value(left_val.get_int() / right_val.get_int());
                            else
                                new_val = Value(left_val.get_double() / right_val.get_double());
                        }
                    }
                }
                else
                    new_val = parse_value(expr, cols[col_idx].get_type());

                new_row.at(col_idx) = new_val;
            }

            try
            {
                table->update_row_at_index(row_idx, new_row);
            }
            catch (const exception &e)
            {
                cout << "\nError updating row: " << e.what() << "\n";
                return false;
            }
        }

        rewrite_csv(table);

        cout << "\n"
             << rows_to_update.size() << " row(s) updated\n";
        return true;
    }
};

#endif
