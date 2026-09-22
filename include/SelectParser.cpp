#ifndef SELECT_PARSER
#define SELECT_PARSER

#include "models.cpp"
#include "Helper.cpp"
#include <iomanip>

class SelectParser
{
    Catalog *_catalog;

    bool evaluate_condition(const Row &row, const Table *table, const string &condition)
    {
        if (condition.empty())
            return true;

        vector<string> operators = {">=", "<=", "!=", "=", ">", "<"};
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

        int col_idx = table->get_column_index(col_name_str);
        if (col_idx == -1)
            return false;

        const Column &col = table->get_column(col_idx);
        string type = col.get_type();

        Value row_value = row[col_idx];
        Value cond_value;

        if (value_str == "NULL")
            cond_value = Value();
        else if (type == "INT")
            cond_value = Value(atoi(value_str.c_str()));
        else if (type == "DOUBLE")
            cond_value = Value(atof(value_str.c_str()));
        else if (type == "DATE")
        {
            int y(0), m(0), d(0);
            sscanf(value_str.c_str(), "%d-%d-%d", &y, &m, &d);
            cond_value = Value(Date(y, m, d));
        }
        else
            cond_value = Value(value_str);

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

    void print_header(const vector<string> &col_names, const Table *table)
    {
        for (int i(0); i < col_names.size(); ++i)
        {
            cout << col_names[i];
            if (i + 1 < col_names.size())
                cout << " | ";
        }
        cout << "\n";
    }

    void print_row(const Row &row, const vector<int> &col_indices, const Table *table)
    {
        for (int i(0); i < col_indices.size(); ++i)
        {
            int idx = col_indices[i];
            cout << row[idx].to_string();
            if (i + 1 < col_indices.size())
                cout << " | ";
        }
        cout << "\n";
    }

    vector<Condition> parse_conditions(const string &cond_str)
    {
        vector<Condition> conds;
        if (cond_str.empty())
            return conds;

        string s = Helper::trim(cond_str);
        vector<string> operators = {">=", "<=", "!=", "=", ">", "<"};
        string op;
        int op_pos = -1;

        for (const auto &o : operators)
        {
            int pos = s.find(o);
            if (pos != string::npos)
            {
                op = o;
                op_pos = pos;
                break;
            }
        }

        if (op_pos == -1)
            return conds;

        string lhs(Helper::trim(s.substr(0, op_pos)));
        string rhs(Helper::trim(s.substr(op_pos + op.size())));

        if (!rhs.empty() && (rhs.front() == '\'' || rhs.front() == '"'))
            rhs = rhs.substr(1, rhs.size() - 2);

        Condition c;
        c.lhs = lhs;
        c.op = op;
        c.rhs = rhs;
        conds.push_back(c);
        return conds;
    }

    bool is_aggregate_function(const string &col_expr)
    {
        string lower_expr = Helper::to_lower(col_expr);
        return lower_expr.find("count(") != string::npos ||
               lower_expr.find("sum(") != string::npos ||
               lower_expr.find("avg(") != string::npos ||
               lower_expr.find("min(") != string::npos ||
               lower_expr.find("max(") != string::npos;
    }

    Value compute_aggregate(const string &func_expr, const vector<Row> &group_rows, const Table *table)
    {
        string lower_expr = Helper::to_lower(func_expr);
        string func_name;
        string col_expr;

        // Extract function name and column
        int paren_pos = func_expr.find('(');
        if (paren_pos != string::npos)
        {
            func_name = Helper::to_lower(Helper::trim(func_expr.substr(0, paren_pos)));
            int close_paren = func_expr.find(')', paren_pos);
            col_expr = Helper::trim(func_expr.substr(paren_pos + 1, close_paren - paren_pos - 1));
        }

        if (func_name == "count")
        {
            if (col_expr == "*")
                return Value((int)group_rows.size());

            int col_idx = table->get_column_index(col_expr);
            if (col_idx == -1)
                return Value(0);

            int count(0);
            for (const auto &row : group_rows)
            {
                if (!holds_alternative<NullType>(row[col_idx].raw()))
                    count++;
            }
            return Value(count);
        }
        else if (func_name == "sum" || func_name == "avg")
        {
            int col_idx = table->get_column_index(col_expr);
            if (col_idx == -1)
                return Value();

            double sum(0);
            int count(0);
            for (const auto &row : group_rows)
            {
                const Value &val = row[col_idx];
                if (holds_alternative<Int>(val.raw()))
                {
                    sum += get<Int>(val.raw());
                    count++;
                }
                else if (holds_alternative<Double>(val.raw()))
                {
                    sum += get<Double>(val.raw());
                    count++;
                }
            }

            if (func_name == "sum")
                return Value(sum);
            else // avg
                return count > 0 ? Value(sum / count) : Value();
        }
        else if (func_name == "min" || func_name == "max")
        {
            int col_idx = table->get_column_index(col_expr);
            if (col_idx == -1 || group_rows.empty())
                return Value();

            Value result = group_rows[0][col_idx];
            for (int i = 1; i < group_rows.size(); ++i)
            {
                const Value &val = group_rows[i][col_idx];
                if (func_name == "min")
                {
                    if (val < result)
                        result = val;
                }
                else // max
                {
                    if (val > result)
                        result = val;
                }
            }
            return result;
        }

        return Value();
    }

    string get_group_key(const Row &row, const vector<int> &group_col_indices)
    {
        string key;
        for (int i(0); i < group_col_indices.size(); ++i)
        {
            if (i > 0)
                key += "|";
            key += row[group_col_indices[i]].to_string();
        }
        return key;
    }

    bool evaluate_having(const vector<Row> &group_rows, const Table *table, const string &having_cond)
    {
        if (having_cond.empty())
            return true;

        // Parse the having condition to check if it involves aggregate functions
        vector<string> operators = {">=", "<=", "!=", "=", ">", "<"};
        string op;
        int op_pos = -1;

        for (const auto &o : operators)
        {
            int pos = having_cond.find(o);
            if (pos != string::npos)
            {
                op = o;
                op_pos = pos;
                break;
            }
        }

        if (op_pos == -1)
            return true;

        string lhs_str = Helper::trim(having_cond.substr(0, op_pos));
        string rhs_str = Helper::trim(having_cond.substr(op_pos + op.size()));

        Value lhs_value;
        if (is_aggregate_function(lhs_str))
            lhs_value = compute_aggregate(lhs_str, group_rows, table);
        else
        {
            int col_idx = table->get_column_index(lhs_str);
            if (col_idx > -1 && !group_rows.empty())
                lhs_value = group_rows[0][col_idx];
        }

        Value rhs_value;
        if (rhs_str == "NULL")
            rhs_value = Value();
        else if (is_aggregate_function(rhs_str))
            rhs_value = compute_aggregate(rhs_str, group_rows, table);
        else
        {
            if (rhs_str.find('.') != string::npos)
                rhs_value = Value(atof(rhs_str.c_str()));
            else if (isdigit(rhs_str[0]) || rhs_str[0] == '-')
                rhs_value = Value(atoi(rhs_str.c_str()));
            else
                rhs_value = Value(rhs_str);
        }

        if (op == "=")
            return lhs_value == rhs_value;
        else if (op == "!=")
            return !(lhs_value == rhs_value);
        else if (op == ">")
            return lhs_value > rhs_value;
        else if (op == "<")
            return lhs_value < rhs_value;
        else if (op == ">=")
            return lhs_value > rhs_value || lhs_value == rhs_value;
        else if (op == "<=")
            return lhs_value < rhs_value || lhs_value == rhs_value;

        return true;
    }

public:
    SelectParser(Catalog *cat) : _catalog(cat) {}

    bool parse_and_select(const string &line, AST &out_ast)
    {
        string s(Helper::trim(line));
        if (s.empty())
            return false;
        if (s.back() == ';')
            s.pop_back();

        string lower(Helper::to_lower(s));
        if (lower.find("select") != 0)
            return false;

        int pos(6);
        while (pos < s.size() && isspace(s[pos]))
            ++pos;

        int from_pos(lower.find("from", pos));
        if (from_pos == string::npos)
            return false;

        string select_part(Helper::trim(s.substr(pos, from_pos - pos)));
        if (select_part.empty())
            return false;

        vector<string> col_names;
        if (select_part == "*")
        {
            // Will be handled later when we know the table
        }
        else
        {
            col_names = Helper::split_commas_respecting_quotes(select_part);
            for (auto &col : col_names)
                col = Helper::trim(col);
        }

        pos = from_pos + 4;
        while (pos < s.size() && isspace(s[pos]))
            ++pos;

        int start(pos);
        while (pos < s.size() && !isspace(s[pos]) && s[pos] != ';')
            ++pos;

        string table_name(s.substr(start, pos - start));
        if (table_name.empty())
            return false;

        string where_condition;
        vector<string> group_by_cols;
        string having_condition;

        int where_pos(lower.find("where", pos)),
            group_by_pos(lower.find("group by", pos)),
            having_pos(lower.find("having"));

        if (where_pos != string::npos)
        {
            int where_end((group_by_pos != string::npos && group_by_pos > where_pos) ? group_by_pos : s.size());
            where_condition = Helper::trim(s.substr(where_pos + 5, where_end - where_pos - 5));
        }

        if (group_by_pos != string::npos)
        {
            int group_start(group_by_pos + 8);
            while (group_start < s.size() && isspace(s[group_start]))
                ++group_start;

            int group_end((having_pos != string::npos && having_pos > group_by_pos) ? having_pos : s.size());
            string group_by_str = Helper::trim(s.substr(group_start, group_end - group_start));

            if (!group_by_str.empty())
            {
                group_by_cols = Helper::split_commas_respecting_quotes(group_by_str);
                for (auto &col : group_by_cols)
                    col = Helper::trim(col);
            }
        }

        if (having_pos != string::npos)
        {
            int having_start(having_pos + 6);
            while (having_start < s.size() && isspace(s[having_start]))
                ++having_start;
            having_condition = Helper::trim(s.substr(having_start));
        }

        Table *table(_catalog->getTable(table_name));
        if (!table)
        {
            cout << "\nTable '" << table_name << "' not found\n";
            return false;
        }

        // Check if we have aggregates without GROUP BY
        bool has_aggregates_no_groupby = false;
        if (group_by_cols.empty() && select_part != "*")
        {
            for (const auto &col : col_names)
            {
                if (is_aggregate_function(col))
                {
                    has_aggregates_no_groupby = true;
                    break;
                }
            }
        }

        // Handle aggregates without GROUP BY (treat entire table as one group)
        if (has_aggregates_no_groupby)
        {
            vector<Row> all_rows;
            for (const auto &row : table->get_rows())
            {
                if (!where_condition.empty() && !evaluate_condition(row, table, where_condition))
                    continue;
                all_rows.push_back(row);
            }

            // Print header
            for (int i(0); i < col_names.size(); ++i)
            {
                cout << col_names[i];
                if (i + 1 < col_names.size())
                    cout << " | ";
            }
            cout << "\n";

            // Print single row with aggregate results
            for (int i(0); i < col_names.size(); ++i)
            {
                if (is_aggregate_function(col_names[i]))
                {
                    Value agg_result(compute_aggregate(col_names[i], all_rows, table));
                    cout << agg_result.to_string();
                }
                else
                {
                    // Non-aggregate column without GROUP BY - use first row value
                    int col_idx(table->get_column_index(col_names[i]));
                    if (col_idx > -1 && !all_rows.empty())
                        cout << all_rows[0][col_idx].to_string();
                }

                if (i + 1 < col_names.size())
                    cout << " | ";
            }
            cout << "\n\n1 row(s) returned\n";
            return true;
        }

        if (!group_by_cols.empty())
        {
            vector<int> group_col_indices;
            for (const auto &col : group_by_cols)
            {
                int idx(table->get_column_index(col));
                if (idx == -1)
                {
                    cout << "\nColumn '" << col << "' not found in GROUP BY\n";
                    return false;
                }
                group_col_indices.push_back(idx);
            }

            unordered_map<string, vector<Row>> groups;
            for (const auto &row : table->get_rows())
            {
                if (!where_condition.empty() && !evaluate_condition(row, table, where_condition))
                    continue;

                string key(get_group_key(row, group_col_indices));
                groups[key].push_back(row);
            }

            vector<string> display_col_names;
            bool has_aggregates(false);

            if (select_part == "*")
            {
                for (const auto &col : group_by_cols)
                    display_col_names.push_back(col);
            }
            else
            {
                for (const auto &col : col_names)
                {
                    display_col_names.push_back(col);
                    if (is_aggregate_function(col))
                        has_aggregates = true;
                }
            }

            print_header(display_col_names, table);

            int row_count(0);
            for (const auto &group_pair : groups)
            {
                const vector<Row> &group_rows = group_pair.second;

                if (!having_condition.empty() && !evaluate_having(group_rows, table, having_condition))
                    continue;

                if (select_part == "*")
                {
                    for (int i(0); i < group_col_indices.size(); ++i)
                    {
                        cout << group_rows[0][group_col_indices[i]].to_string();
                        if (i + 1 < group_col_indices.size())
                            cout << " | ";
                    }
                    cout << "\n";
                }
                else
                {
                    for (int i(0); i < col_names.size(); ++i)
                    {
                        if (is_aggregate_function(col_names[i]))
                        {
                            Value agg_result(compute_aggregate(col_names[i], group_rows, table));
                            cout << agg_result.to_string();
                        }
                        else
                        {
                            int col_idx(table->get_column_index(col_names[i]));
                            if (col_idx > -1)
                                cout << group_rows[0][col_idx].to_string();
                        }

                        if (i + 1 < col_names.size())
                            cout << " | ";
                    }
                    cout << "\n";
                }
                ++row_count;
            }

            cout << '\n'
                 << row_count
                 << " row(s) returned\n";
            return true;
        }

        vector<int> col_indices;
        vector<string> display_col_names;

        if (select_part == "*")
        {
            for (int i(0); i < table->get_column_count(); ++i)
            {
                col_indices.push_back(i);
                display_col_names.push_back(table->get_column(i).get_name());
            }
        }
        else
        {
            for (const auto &col : col_names)
            {
                int idx(table->get_column_index(col));
                if (idx == -1)
                {
                    cout << "\nColumn '" << col << "' not found\n";
                    return false;
                }
                col_indices.push_back(idx);
                display_col_names.push_back(col);
            }
        }

        print_header(display_col_names, table);

        int row_count(0);
        for (const auto &row : table->get_rows())
        {
            if (!where_condition.empty() && !evaluate_condition(row, table, where_condition))
                continue;

            print_row(row, col_indices, table);
            ++row_count;
        }

        cout << "\n"
             << row_count << " row(s) returned\n";

        return true;
    }
};

#endif
