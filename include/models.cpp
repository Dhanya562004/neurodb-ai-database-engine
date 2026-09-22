#pragma once

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <variant>
#include <iomanip>
#include <unordered_map>

using namespace std;

class NullType
{
public:
    NullType() = default;
};

class Date
{
    int year, month, day;

public:
    Date() : year(0), month(0), day(0) {}
    Date(int y, int m, int d) : year(y), month(m), day(d) {}

    int get_year() const { return year; }
    int get_month() const { return month; }
    int get_day() const { return day; }

    bool operator==(const Date &other) const
    {
        return year == other.year && month == other.month && day == other.day;
    }

    bool operator<(const Date &other) const
    {
        if (year != other.year)
            return year < other.year;
        if (month != other.month)
            return month < other.month;
        return day < other.day;
    }

    string to_string() const
    {
        ostringstream ss;
        ss << setw(4) << setfill('0') << year << '-'
           << setw(2) << setfill('0') << month << '-'
           << setw(2) << setfill('0') << day;
        return ss.str();
    }
};

using Int = int;
using Double = double;
using Char = char;
using Text = string;
using Variant = variant<NullType, Int, Double, Char, Date, Text>;

const int NOT_FOUND = -1;

class Value
{
    Variant data;

public:
    Value() : data(NullType{}) {}
    Value(Int value) : data(value) {}
    Value(Double value) : data(value) {}
    Value(Char value) : data(value) {}
    Value(Date const &value) : data(value) {}
    Value(Text value) : data(move(value)) {}
    Value(char const *value) : data(Text(value)) {}

    const Variant &raw() const { return data; }

    bool is_null() const { return holds_alternative<NullType>(data); }

    Int get_int() const
    {
        if (holds_alternative<Int>(data))
            return get<Int>(data);
        return 0;
    }

    Double get_double() const
    {
        if (holds_alternative<Double>(data))
            return get<Double>(data);
        if (holds_alternative<Int>(data))
            return (double)get<Int>(data);
        return 0.0;
    }

    Text to_string() const
    {
        return visit([](auto const &value) -> Text
                     {
            using type = decltype(value);
            
            if constexpr (is_same_v<type, const NullType&>) 
                return "NULL";
            if constexpr (is_same_v<type, const Int&>) {
                ostringstream ss;
                ss << value;
                return ss.str();
            }
            if constexpr (is_same_v<type, const Double&>) {
                ostringstream ss;
                ss << fixed << setprecision(2) << value;
                return ss.str();
            }
            if constexpr (is_same_v<type, const Char&>) 
                return Text(1, value);
            if constexpr (is_same_v<type, const Date&>) 
                return value.to_string();
            if constexpr (is_same_v<type, const Text&>) 
                return value;
            return ""; }, data);
    }

    bool operator==(const Value &other) const
    {
        if (holds_alternative<NullType>(data) && holds_alternative<NullType>(other.data))
            return true;
        if (holds_alternative<NullType>(data) || holds_alternative<NullType>(other.data))
            return false;

        if (holds_alternative<Int>(data) && holds_alternative<Int>(other.data))
            return get<Int>(data) == get<Int>(other.data);
        if (holds_alternative<Double>(data) && holds_alternative<Double>(other.data))
            return get<Double>(data) == get<Double>(other.data);

        if ((holds_alternative<Int>(data) || holds_alternative<Double>(data)) &&
            (holds_alternative<Int>(other.data) || holds_alternative<Double>(other.data)))
        {
            double val1(holds_alternative<Int>(data) ? (double)get<Int>(data) : get<Double>(data)),
                val2(holds_alternative<Int>(other.data) ? (double)get<Int>(other.data) : get<Double>(other.data));
            return val1 == val2;
        }

        if (holds_alternative<Date>(data) && holds_alternative<Date>(other.data))
            return get<Date>(data) == get<Date>(other.data);

        if ((holds_alternative<Text>(data) || holds_alternative<Char>(data)) &&
            (holds_alternative<Text>(other.data) || holds_alternative<Char>(other.data)))
        {
            string str1(holds_alternative<Text>(data) ? get<Text>(data) : string(1, get<Char>(data))),
                str2(holds_alternative<Text>(other.data) ? get<Text>(other.data) : string(1, get<Char>(other.data)));
            return str1 == str2;
        }

        return false;
    }

    bool operator<(const Value &other) const
    {
        bool this_null(holds_alternative<NullType>(data)),
            other_null(holds_alternative<NullType>(other.data));

        if (this_null && other_null)
            return false;
        if (this_null)
            return true;
        if (other_null)
            return false;

        if ((holds_alternative<Int>(data) || holds_alternative<Double>(data)) &&
            (holds_alternative<Int>(other.data) || holds_alternative<Double>(other.data)))
        {
            double val1(holds_alternative<Int>(data) ? (double)get<Int>(data) : get<Double>(data)),
                val2(holds_alternative<Int>(other.data) ? (double)get<Int>(other.data) : get<Double>(other.data));
            return val1 < val2;
        }

        if (holds_alternative<Date>(data) && holds_alternative<Date>(other.data))
            return get<Date>(data) < get<Date>(other.data);

        if ((holds_alternative<Text>(data) || holds_alternative<Char>(data)) &&
            (holds_alternative<Text>(other.data) || holds_alternative<Char>(other.data)))
        {
            string str1(holds_alternative<Text>(data) ? get<Text>(data) : string(1, get<Char>(data))),
                str2(holds_alternative<Text>(other.data) ? get<Text>(other.data) : string(1, get<Char>(other.data)));
            return str1 < str2;
        }

        return false;
    }

    bool operator>(const Value &other) const
    {
        return other < *this;
    }
};

class Column
{
    Text name;
    Text type;
    bool is_pk_;
    bool is_null_;
    int char_length;

public:
    Column() : name(), type("TEXT"), is_pk_(false), is_null_(true), char_length(1) {}
    Column(Text n, Text t, bool pk = false, int len = 1, bool nullable = true)
        : name(move(n)), type(move(t)), is_pk_(pk), is_null_(nullable), char_length(len) {}

    const Text &get_name() const { return name; }
    const Text &get_type() const { return type; }
    bool is_pk() const { return is_pk_; }
    bool is_null() const { return is_null_; }
    int get_char_length() const { return char_length; }
    void set_name(const Text &n) { name = n; }
    void set_type(const Text &t) { type = t; }
    void set_is_pk(bool pk) { is_pk_ = pk; }
    void set_is_null(bool nullable) { is_null_ = nullable; }
    void set_char_length(int len) { char_length = len; }
};

class Row
{
    vector<Value> vals;

public:
    Row() = default;
    Row(vector<Value> values) : vals(move(values)) {}

    const vector<Value> &values() const { return vals; }
    vector<Value> &values() { return vals; }
    int size() const { return vals.size(); }
    const Value &at(int idx) const { return vals.at(idx); }
    Value &at(int idx) { return vals.at(idx); }
    const Value &operator[](int idx) const { return vals[idx]; }
    Value &operator[](int idx) { return vals[idx]; }
    void push_back(Value value) { vals.push_back(move(value)); }
};

class Table
{
    Text name;
    vector<Column> columns;
    vector<Row> rows;
    vector<int> pk_indices;
    unordered_map<Text, int> pk_map; // pk_value, row_Idx

    static const Text PK_SEP;

    static Text escape_key(const Text &str)
    {
        Text output;
        output.reserve(str.size() + 4);
        for (char ch : str)
        {
            if (ch == '\\')
                output += "\\\\";
            else if (ch == '|')
                output += "\\|";
            else
                output.push_back(ch);
        }
        return output;
    }
    Text build_pk_by_row(const Row &row) const
    {
        if (pk_indices.empty())
            throw logic_error("Table has no primary key");

        bool is_first(true);
        Text key;
        for (int idx : pk_indices)
        {
            if (!is_first)
                key += PK_SEP;

            is_first = false;
            const Value &val(row.at(idx));

            if (get_if<NullType>(&val.raw()))
                throw runtime_error("Primary Key column cannot be NULL");

            key += escape_key(val.to_string());
        }
        return key;
    }
    Text build_pk_key_from_literals(const vector<Text> &parts) const
    {
        if (pk_indices.empty())
            throw logic_error("No primary key defined for table");

        if (parts.size() != pk_indices.size())
            throw invalid_argument("PK size mismatch");

        bool first(true);
        Text key;
        for (const auto &p : parts)
        {
            if (!first)
                key += PK_SEP;

            first = false;
            key += escape_key(p);
        }
        return key;
    }

public:
    Table(const Text &tableName,
          const vector<Column> &cols,
          const vector<Text> &pkColNames = {})
        : name(tableName), columns(cols)
    {
        if (!pkColNames.empty())
        {
            for (const auto &pn : pkColNames)
            {
                int found(NOT_FOUND);
                for (int i(0); i < columns.size(); ++i)
                {
                    if (columns[i].get_name() == pn)
                    {
                        found = i;
                        break;
                    }
                }
                if (found == NOT_FOUND)
                    throw runtime_error("PRIMARY KEY column not found: " + pn);

                pk_indices.push_back(found);
            }
        }
        else
        {
            for (int i(0); i < columns.size(); ++i)
            {
                if (columns[i].is_pk())
                    pk_indices.push_back(i);
            }
        }
    }
    const string &get_name() const { return name; }
    const vector<Column> &get_columns() const { return columns; }
    vector<Column> &get_columns() { return columns; }
    const vector<Row> &get_rows() const { return rows; }
    vector<Row> &get_rows() { return rows; }
    const vector<int> &getpk_indices() const { return pk_indices; }
    int row_count() const { return rows.size(); }
    const Row &row_at(int i) const { return rows.at(i); }
    Row &row_at(int i) { return rows.at(i); }
    bool has_pk() const { return !pk_indices.empty(); }
    bool is_single_pk() const { return pk_indices.size() == 1; }

    int get_column_count() const { return columns.size(); }
    int get_column_index(const string &col_name) const
    {
        for (int i(0); i < columns.size(); ++i)
        {
            if (columns[i].get_name() == col_name)
                return i;
        }
        return -1;
    }
    const Column &get_column(int idx) const { return columns.at(idx); }
    Column &get_column(int idx) { return columns.at(idx); }
    int find_row_index_by_pk_literals(const vector<Text> &pk_literals) const
    {
        if (pk_indices.empty())
            return NOT_FOUND;

        if (pk_literals.size() != pk_indices.size())
            return NOT_FOUND;

        auto it(pk_map.find(build_pk_key_from_literals(pk_literals)));

        if (it == pk_map.end())
            return NOT_FOUND;

        return it->second;
    }
    int find_row_index_by_pk_literal(const string &single_literal) const
    {
        if (!is_single_pk())
            return NOT_FOUND;

        return find_row_index_by_pk_literals({single_literal});
    }
    int find_row_index_by_pk_value(const Value &val) const
    {
        if (!is_single_pk())
            return NOT_FOUND;
        return find_row_index_by_pk_literal(val.to_string());
    }
    void insert_row(const Row &row)
    {
        if (!has_pk())
        {
            rows.push_back(row);
            return;
        }

        Text key(build_pk_by_row(row));

        if (pk_map.find(key) != pk_map.end())
            throw runtime_error("duplicate primary key: " + key);

        int idx(rows.size());
        rows.push_back(row);
        pk_map.emplace(move(key), idx);
    }
    bool delete_by_pk_literals(const vector<Text> &pk_literals)
    {
        if (!has_pk())
            return false;

        int idx(find_row_index_by_pk_literals(pk_literals));

        if (idx == NOT_FOUND)
            return false;

        Text del_key(build_pk_by_row(rows[idx]));
        pk_map.erase(del_key);

        int last(rows.size() - 1);
        if (idx != last)
        {
            rows[idx] = move(rows[last]);
            if (has_pk())
            {
                Text moved_key(build_pk_by_row(rows[idx]));
                pk_map[moved_key] = idx;
            }
        }
        rows.pop_back();
        return true;
    }
    bool delete_by_pk_literal(const Text &single_literal)
    {
        if (!is_single_pk())
            return false;
        return delete_by_pk_literals({single_literal});
    }
    void update_row_at_index(int idx, const Row &newRow)
    {
        if (idx >= rows.size())
            throw out_of_range("row index out of range");

        if (!has_pk())
        {
            rows[idx] = newRow;
            return;
        }

        Text old_key(build_pk_by_row(rows[idx])),
            new_key(build_pk_by_row(newRow));
        if (old_key == new_key)
        {
            rows[idx] = newRow;
            return;
        }

        if (pk_map.find(new_key) != pk_map.end())
            throw runtime_error("update would violate primary key uniqueness: " + new_key);

        pk_map.erase(old_key);
        rows[idx] = newRow;
        pk_map.emplace(move(new_key), idx);
    }
    const Row *get_single_row_pk_value(const Value &val) const
    {
        auto idx(find_row_index_by_pk_value(val));
        if (idx == NOT_FOUND)
            return nullptr;
        return &rows[idx];
    }
};

const Text Table::PK_SEP = "|";

class Catalog
{
private:
    unordered_map<Text, Table *> tables;

public:
    Catalog() {}
    void addTable(Table *t)
    {
        if (!t)
            throw invalid_argument("addTable: null pointer");

        tables[t->get_name()] = t;
    }
    Table *getTable(const Text &name) const
    {
        auto it(tables.find(name));
        if (it == tables.end())
            return nullptr;
        return it->second;
    }
    bool exists(const Text &name) const
    {
        return tables.find(name) != tables.end();
    }
    const unordered_map<Text, Table *> &get_tables() const
    {
        return tables;
    }
};

enum class ASTKind
{
    CREATE,
    INSERT,
    SELECT,
    UPDATE,
    _DELETE
};

class AST_Create
{
public:
    Text table_name;
    vector<Column> columns;
    vector<Text> pk_columns;
};

class AST_Insert
{
public:
    Text table_name;
    vector<Text> raw_values;
};

class Condition
{
public:
    Text lhs;
    Text op;
    Text rhs;
};

class AST_Select
{
public:
    Text table_name;
    vector<Text> select_list;
    vector<Condition> where;
    vector<Text> group_by;
    vector<Condition> having;
};

class AST_Update
{
public:
    Text table_name;
    vector<pair<Text, Text>> sets;
    vector<Condition> where;
};

class AST_Delete
{
public:
    Text table_name;
    vector<Condition> where;
};

class AST
{
public:
    ASTKind kind;
    variant<AST_Create, AST_Insert, AST_Select, AST_Update, AST_Delete> node;
};