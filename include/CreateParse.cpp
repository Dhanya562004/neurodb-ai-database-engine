#ifndef CREATE_PARSER
#define CREATE_PARSER

#include "models.cpp"
#include "Helper.cpp"

class CreateParser
{
    Catalog _catalog;

    bool parse_create_internal(const string &s, AST &out_ast)
    {
        auto words(Helper::split_spaces_respecting_quotes(s));
        if (words.size() < 3)
            return false;

        int word_idx(0);
        while (word_idx < words.size() && Helper::to_lower(words[word_idx]) != "table")
            ++word_idx;

        if (word_idx + 1 >= words.size())
            return false;

        string table_name_raw(words[word_idx + 1]);
        int paren_pos(table_name_raw.find('(')); // users(id int,...
        string table_name(paren_pos != string::npos ? table_name_raw.substr(0, paren_pos) : table_name_raw);

        auto parens(Helper::find_top_level_parens(s)); // ->()<-
        if (parens.first < 0)
            return false;

        string table_name_clean(Helper::trim(table_name));
        if (table_name_clean.empty())
            return false;

        string inside(s.substr(parens.first + 1, parens.second - parens.first - 1));
        auto parts(Helper::split_commas_respecting_quotes(inside));
        if (parts.empty())
            return false;

        AST ast;
        ast.kind = ASTKind::CREATE;
        AST_Create node;
        node.table_name = table_name_clean;

        vector<Column> columns;
        vector<Text> pk_columns;

        for (const auto &raw_part : parts) // row_part -> id int, name varchar(50)
        {
            string part(Helper::trim(raw_part));
            if (part.empty())
                continue;

            string lower(Helper::to_lower(part));

            if (lower.substr(0, 11) == "primary key") // composite pk
            {
                auto pk_parens(Helper::find_top_level_parens(part));
                if (pk_parens.first < 0)
                    return false;

                string inside_pk(part.substr(pk_parens.first + 1, pk_parens.second - pk_parens.first - 1));
                pk_columns = Helper::parse_column_name_list(inside_pk);
                continue;
            }

            auto tokens(Helper::split_spaces_respecting_quotes(part));
            if (tokens.size() < 2)
                return false;

            string col_name(tokens[0]), type_token(tokens[1]);
            bool is_primary(false);
            int char_len(1), lparen(type_token.find('(')); // varchar(50)
            string type_base(type_token);
            if (lparen != string::npos)
            {
                int rparen(type_token.find(')', lparen + 1));
                if (rparen != string::npos && rparen > lparen + 1)
                {
                    type_base = type_token.substr(0, lparen);
                    string num(type_token.substr(lparen + 1, rparen - lparen - 1));
                    char_len = stoi(num);
                    if (!char_len)
                        char_len = 1;
                }
            }
            for (char &c : type_base) // database common :)
                c = toupper(c);

            string col_type(type_base);
            if (col_type == "CHAR" || col_type == "TEXT")
            {
                col_type = "VARCHAR";
                if (type_base == "TEXT" && lparen == string::npos) // default length
                    char_len = 255;
            }

            bool is_nullable(true);

            for (int k(2); k < tokens.size(); ++k)
            {
                string tok(Helper::to_lower(tokens[k]));
                if (tok == "primary" && k + 1 < tokens.size())
                {
                    if (Helper::to_lower(tokens[k + 1]) == "key")
                        is_primary = true, ++k;
                }
                else if (tok == "not" && k + 1 < tokens.size())
                {
                    if (Helper::to_lower(tokens[k + 1]) == "null")
                        is_nullable = false, ++k;
                }
            }

            columns.emplace_back(col_name, col_type, is_primary, char_len, is_nullable);
        }

        node.columns = move(columns);
        node.pk_columns = move(pk_columns);
        ast.node = move(node);
        out_ast = move(ast);
        return true;
    }

public:
    bool parse_create_statement(const string &input, AST &out_ast)
    {
        string s(Helper::to_lower(input));
        if (s.empty())
            return false;

        if (s.back() == ';')
            s.pop_back();

        auto words(Helper::split_spaces_respecting_quotes(s));
        if (words.size() < 2)
            return false;

        if (words[0] != "create" || words[1] != "table")
            return false;

        return parse_create_internal(s, out_ast);
    }
    bool parse_and_create(const string &line, AST &out_ast)
    {
        if (!parse_create_statement(line, out_ast))
            return false;

        const AST_Create *node(get_if<AST_Create>(&out_ast.node));
        if (!node)
            return false;

        vector<string> col_names;
        for (const auto &c : node->columns)
            col_names.push_back(c.get_name());

        vector<string> pkcols = node->pk_columns;
        if (pkcols.empty()) // has values only if composite
        {
            for (const auto &c : node->columns)
            {
                if (c.is_pk())
                    pkcols.push_back(c.get_name());
            }
        }

        bool csv_created(Helper::create_csv_header(node->table_name, col_names)),
            meta_created(Helper::write_meta(node->table_name, node->columns, pkcols));

        if (!_catalog.exists(node->table_name))
        {
            Table *t(new Table(node->table_name, node->columns, pkcols));
            _catalog.addTable(t);
        }

        if (csv_created && meta_created)
            cout << "\nTable '" << node->table_name << "' created\n";
        else
            cout << "\nTable '" << node->table_name << "' already exists\n";

        return true;
    }

    Catalog &catalog() { return _catalog; }
};
#endif