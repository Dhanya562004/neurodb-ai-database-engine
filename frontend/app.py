import streamlit as st
import pandas as pd
import sys
import os
import time
import re

# ------------------ PATH SETUP ------------------
BASE_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
if BASE_DIR not in sys.path:
    sys.path.insert(0, BASE_DIR)

from python_wrapper import NeuroDBConnector, AITranslator, QueryValidator, QueryLogger

# ------------------ FALLBACK FUNCTIONS ------------------

def load_tables_fallback():
    tables = {}
    data_path = os.path.join(BASE_DIR, "data")

    if not os.path.exists(data_path):
        st.error("❌ Data folder not found. Please add CSV files to /data in GitHub.")
        return tables

    for root, _, files in os.walk(data_path):
        for file in files:
            if file.endswith(".csv"):
                try:
                    name = os.path.splitext(file)[0].lower()
                    file_path = os.path.join(root, file)
                    tables[name] = pd.read_csv(file_path)
                except Exception:
                    pass

    return tables


def get_fallback_schema(tables):
    schema = []
    for name, df in tables.items():
        schema.append({
            "name": name,
            "columns": [{"name": col, "type": str(df[col].dtype)} for col in df.columns],
            "row_count": len(df)
        })
    return schema


def execute_fallback_query(sql, tables):
    sql = sql.strip().rstrip(";")

    match = re.search(
        r"SELECT\s+\*\s+FROM\s+([a-zA-Z0-9_]+)(?:\s+WHERE\s+(.+))?",
        sql,
        re.IGNORECASE
    )

    if not match:
        return {
            "success": False,
            "dataframe": pd.DataFrame(),
            "error": "Only SELECT * FROM table supported in fallback",
            "row_count": 0,
            "execution_time_ms": 1
        }

    table_name = match.group(1).lower()
    where_clause = match.group(2)

    if table_name not in tables:
        return {
            "success": False,
            "dataframe": pd.DataFrame(),
            "error": f"Table '{table_name}' not found",
            "row_count": 0,
            "execution_time_ms": 1
        }

    df = tables[table_name].copy()

    # WHERE support
    if where_clause:
        try:
            cond = re.search(r"(\w+)\s*(=|>|<|>=|<=)\s*(.+)", where_clause)
            if cond:
                col, op, val = cond.groups()
                val = val.strip("'\"")

                if col in df.columns:
                    if pd.api.types.is_numeric_dtype(df[col]):
                        val = float(val) if "." in val else int(val)

                    if op == "=":
                        df = df[df[col] == val]
                    elif op == ">":
                        df = df[df[col] > val]
                    elif op == "<":
                        df = df[df[col] < val]
                    elif op == ">=":
                        df = df[df[col] >= val]
                    elif op == "<=":
                        df = df[df[col] <= val]
        except:
            pass

    return {
        "success": True,
        "dataframe": df,
        "row_count": len(df),
        "execution_time_ms": 1
    }

# ------------------ PAGE CONFIG ------------------

st.set_page_config(
    page_title="NeuroDB",
    page_icon="⚡",
    layout="wide"
)

# ------------------ INIT ------------------

if "connector" not in st.session_state:
    st.session_state.connector = NeuroDBConnector()

if "translator" not in st.session_state:
    st.session_state.translator = AITranslator()

if "logger" not in st.session_state:
    st.session_state.logger = QueryLogger()

connector = st.session_state.connector
translator = st.session_state.translator
logger = st.session_state.logger

# ------------------ LOAD SCHEMA ------------------

try:
    schema_data = connector.get_schema()
    if not schema_data:
        raise Exception("Empty schema")
except:
    tables = load_tables_fallback()
    schema_data = get_fallback_schema(tables)

# ------------------ SIDEBAR ------------------

st.sidebar.title("⚡ NeuroDB")
st.sidebar.subheader("📊 Schema")

if schema_data:
    for table in schema_data:
        with st.sidebar.expander(f"{table['name']} ({table['row_count']} rows)"):
            st.dataframe(pd.DataFrame(table["columns"]))
else:
    st.sidebar.warning("No tables available")

# ------------------ MAIN ------------------

st.title("NeuroDB AI Database Engine")

mode = st.radio(
    "Mode",
    ["Natural Language (AI)", "SQL Query"]
)

query = st.text_area("Enter Query")

if st.button("Execute") and query.strip():

    raw_query = query.strip()
    sql = raw_query

    # NL → SQL
    if mode == "Natural Language (AI)":
        try:
            res = translator.translate(raw_query, schema_data)
            sql = res["sql"]
            st.info(f"Generated SQL: {sql}")
        except:
            st.warning("AI translation failed")

    # Validate
    validation = QueryValidator.validate(sql)
    if validation.get("has_corrections"):
        sql = validation["corrected_query"]

    # Execute
    start = time.time()

    try:
        result = connector.execute_query(sql)

        if not result.get("success"):
            raise Exception("Engine failed")

    except:
        tables = load_tables_fallback()
        result = execute_fallback_query(sql, tables)

    end = time.time()
    result["execution_time_ms"] = round((end - start) * 1000, 2)

    # Log
    logger.log(
        raw_query=raw_query,
        executed_sql=sql,
        mode=mode,
        duration_ms=result["execution_time_ms"],
        success=result["success"],
        row_count=result["row_count"],
        error=result.get("error")
    )

    # Output
    st.subheader("Result")

    if result["success"]:
        if not result["dataframe"].empty:
            st.dataframe(result["dataframe"], use_container_width=True)
        else:
            st.success("No rows returned")
    else:
        st.error(result["error"])

# ------------------ HISTORY ------------------

st.divider()

with st.expander("Query Logs"):
    history = logger.get_history()
    if history:
        st.dataframe(pd.DataFrame(history))
    else:
        st.info("No logs yet")