#include "../include/CreateParse.cpp"
#include "../include/InsertParser.cpp"
#include <iostream>

using namespace std;

int main()
{
    cout << "\n================================================\n";
    cout << "     Setting Up Test Database Tables       \n";
    cout << "================================================\n\n";

    CreateParser create_parser;
    Catalog *catalog(&create_parser.catalog());
    InsertParser insert_parser(catalog);
    AST ast;

    Helper::load_existing_tables(catalog);

    // Table 1: Users
    cout << "Setting up 'users' table...\n";
    if (!catalog->exists("users"))
        create_parser.parse_and_create("CREATE TABLE users (id INT PRIMARY KEY, name VARCHAR(50) NOT NULL, email TEXT, age INT);", ast);
    
    insert_parser.parse_and_insert("INSERT INTO users VALUES (1, 'Ahmed Ali', 'ahmed@example.com', 25);", ast);
    insert_parser.parse_and_insert("INSERT INTO users VALUES (2, 'Sara Mohamed', 'sara@example.com', 30);", ast);
    insert_parser.parse_and_insert("INSERT INTO users VALUES (3, 'John Smith', 'john@example.com', 28);", ast);
    insert_parser.parse_and_insert("INSERT INTO users VALUES (4, 'Emily Brown', 'emily@example.com', 35);", ast);
    insert_parser.parse_and_insert("INSERT INTO users VALUES (5, 'Mohamed Hassan', 'mohamed@example.com', 22);", ast);

    // Table 2: Products
    cout << "Setting up 'products' table...\n";
    if (!catalog->exists("products"))
        create_parser.parse_and_create("CREATE TABLE products (id INT PRIMARY KEY, name TEXT NOT NULL, price DOUBLE NOT NULL, stock INT, category TEXT);", ast);

    insert_parser.parse_and_insert("INSERT INTO products VALUES (101, 'Laptop', 999.99, 15, 'Electronics');", ast);
    insert_parser.parse_and_insert("INSERT INTO products VALUES (102, 'Mouse', 25.50, 50, 'Electronics');", ast);
    insert_parser.parse_and_insert("INSERT INTO products VALUES (103, 'Keyboard', 75.00, 30, 'Electronics');", ast);
    insert_parser.parse_and_insert("INSERT INTO products VALUES (104, 'Monitor', 299.99, 20, 'Electronics');", ast);
    insert_parser.parse_and_insert("INSERT INTO products VALUES (105, 'Desk Chair', 150.00, 10, 'Furniture');", ast);

    // Table 3: Employees
    cout << "Setting up 'employees' table...\n";
    if (!catalog->exists("employees"))
        create_parser.parse_and_create("CREATE TABLE employees (id INT PRIMARY KEY, name TEXT NOT NULL, salary DOUBLE NOT NULL, department TEXT NOT NULL, hire_date DATE);", ast);

    insert_parser.parse_and_insert("INSERT INTO employees VALUES (1001, 'Alice Johnson', 50000, 'IT', '2020-01-15');", ast);
    insert_parser.parse_and_insert("INSERT INTO employees VALUES (1002, 'Bob Wilson', 60000, 'Sales', '2019-05-20');", ast);
    insert_parser.parse_and_insert("INSERT INTO employees VALUES (1003, 'Charlie Davis', 55000, 'IT', '2021-03-10');", ast);
    insert_parser.parse_and_insert("INSERT INTO employees VALUES (1004, 'Diana Martinez', 65000, 'HR', '2018-11-05');", ast);

    // Table 4: Students
    cout << "Setting up 'students' table...\n";
    if (!catalog->exists("students"))
        create_parser.parse_and_create("CREATE TABLE students (student_id INT PRIMARY KEY, name TEXT NOT NULL, major TEXT, gpa DOUBLE, marks INT);", ast);

    insert_parser.parse_and_insert("INSERT INTO students VALUES (1, 'Alice Smith', 'Computer Science', 3.9, 95);", ast);
    insert_parser.parse_and_insert("INSERT INTO students VALUES (2, 'Bob Jones', 'Data Science', 3.4, 82);", ast);
    insert_parser.parse_and_insert("INSERT INTO students VALUES (3, 'Charlie Brown', 'Electrical Engineering', 3.7, 88);", ast);
    insert_parser.parse_and_insert("INSERT INTO students VALUES (4, 'Diana Prince', 'Computer Science', 3.2, 75);", ast);
    insert_parser.parse_and_insert("INSERT INTO students VALUES (5, 'Evan Wright', 'Mathematics', 3.8, 91);", ast);

    cout << "\n✓ Test Database Setup Complete!\n";
    return 0;
}
